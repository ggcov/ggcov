/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "cov.h"
#include "covio.h"
#include "filename.h"
#include "estring.h"

CVSID("$Id: cov.c,v 1.1 2001-11-23 03:47:48 gnb Exp $");

GHashTable *cov_files;
/* TODO: ? reorg this */
GHashTable *cov_blocks_by_location; 	/* GList of blocks keyed on "file:line" */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t *
cov_function_new(void)
{
    cov_function_t *fn;
    
    fn = new(cov_function_t);
    
    fn->blocks = g_ptr_array_new();
    
    return fn;
}

/* TODO: ability to delete these structures !!  */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_function_set_name(cov_function_t *fn, const char *funcname)
{
    assert(fn->name == 0);
    strassign(fn->name, funcname);
    g_hash_table_insert(fn->file->functions_by_name, fn->name, fn);
}

cov_block_t *
cov_function_add_block(cov_function_t *fn)
{
    cov_block_t *b;
    
    b = new(cov_block_t);

    b->idx = fn->blocks->len;
    g_ptr_array_add(fn->blocks, b);
    b->function = fn;
    
    return b;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_file_t *
cov_file_new(const char *name)
{
    cov_file_t *f;
    
    f = new(cov_file_t);
    strassign(f->name, name);
    f->functions = g_ptr_array_new();
    f->functions_by_name = g_hash_table_new(g_str_hash, g_str_equal);
    
    if (cov_files == 0)
        cov_files = g_hash_table_new(g_str_hash, g_str_equal);

    g_hash_table_insert(cov_files, f->name, f);
    
    return f;
}

#if 0
void
cov_file_delete(cov_file_t *f)
{
    g_hash_table_remove(cov_files, f->name);

    strdelete(f->name);
    g_ptr_array_free(f->functions, /*free_seg*/TRUE);
    g_ptr_array_free(f->blocks, /*free_seg*/TRUE);
    g_ptr_array_free(f->arcs, /*free_seg*/TRUE);
    g_free(f);
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_file_t *
cov_file_find(const char *name)
{
    return (cov_files == 0 ? 0 : g_hash_table_lookup(cov_files, name));
}


typedef struct
{
    void (*func)(cov_file_t *, void *);
    void *userdata;
} cov_file_foreach_rec_t;

static void
cov_file_foreach_tramp(gpointer key, gpointer value, gpointer userdata)
{
    cov_file_t *f = (cov_file_t *)value;
    cov_file_foreach_rec_t *rec = (cov_file_foreach_rec_t *)userdata;
    
    (*rec->func)(f, rec->userdata);
}


void
cov_file_foreach(
    void (*func)(cov_file_t*, void *userdata),
    void *userdata)
{
    cov_file_foreach_rec_t rec;
    
    rec.func = func;
    rec.userdata = userdata;
    g_hash_table_foreach(cov_files, cov_file_foreach_tramp, &rec);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t *
cov_file_add_function(cov_file_t *f)
{
    cov_function_t *fn;
    
    fn = cov_function_new();
    
    g_ptr_array_add(f->functions, fn);    
    fn->file = f;
    
    return fn;
}

cov_function_t *
cov_file_find_function(cov_file_t *f, const char *fnname)
{
    return g_hash_table_lookup(f->functions_by_name, fnname);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_arc_t *
cov_block_add_arc_to(cov_block_t *from, cov_block_t *to)
{
    cov_arc_t *a;
    
    a = new(cov_arc_t);
    
    a->from = from;
    from->out_arcs = g_list_append(from->out_arcs, a);
    from->out_ninvalid++;

    a->to = to;
    to->in_arcs = g_list_append(to->in_arcs, a);
    to->in_ninvalid++;
    
    return a;
}

static void
cov_block_set_count(cov_block_t *b, count_t count)
{
    assert(!b->count_valid);
    b->count_valid = TRUE;
    b->count = count;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *
cov_location_make_key(const char *filename, unsigned lineno)
{
    return g_strdup_printf("%s:%u", filename, lineno);
}

void
cov_block_add_location(cov_block_t *b, const char *filename, unsigned lineno)
{
    cov_location_t *loc;
    char *key;
    GList *list;

#if DEBUG    
    fprintf(stderr, "Block %s:%d adding location %s:%d\n",
    	    	b->function->name, b->idx,
		filename, lineno);
#endif
    
    loc = new(cov_location_t);
    strassign(loc->filename, filename); /* TODO: hashtable to reduce storage */
    loc->lineno = lineno;
    
    b->locations = g_list_append(b->locations, loc);


    if (cov_blocks_by_location == 0)
    	cov_blocks_by_location = g_hash_table_new(g_str_hash, g_str_equal);
    
    key = cov_location_make_key(filename, lineno);
    list = g_hash_table_lookup(cov_blocks_by_location, key);
    
    if (list == 0)
    	g_hash_table_insert(cov_blocks_by_location, key, g_list_append(0, b));
    else
    {
    	g_list_append(list, b);
	g_free(key);
#if DEBUG    
    	fprintf(stderr, "%s:%d: this line belongs to %d blocks\n",
	    	    	    loc->filename, loc->lineno, g_list_length(list));
#endif
    }
}

const GList *
cov_blocks_find_by_location(const char *filename, unsigned lineno)
{
    GList *list;
    char *key = cov_location_make_key(filename, lineno);

    list = g_hash_table_lookup(cov_blocks_by_location, key);
    g_free(key);
    return list;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
cov_arc_set_count(cov_arc_t *a, count_t count)
{
    assert(!a->count_valid);
    a->count_valid = TRUE;
    a->count = count;
    a->from->out_ninvalid--;
    a->to->in_ninvalid--;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static count_t
arc_list_total(GList *list)
{
    count_t total = 0;
    
    for ( ; list != 0 ; list = list->next)
    {
    	cov_arc_t *a = (cov_arc_t *)list->data;
	
	/* Some of the counts will be invalid, but they are zero,
	   so adding it in also doesn't hurt.  */
	total += a->count;
    }
    return total;
}

static cov_arc_t *
arc_list_find_invalid(GList *list)
{
    for ( ; list != 0 ; list = list->next)
    {
    	cov_arc_t *a = (cov_arc_t *)list->data;
	
	if (!a->count_valid)
	    return a;
    }
    return 0;
}

/*
 * Solves the program flow graph for a function -- calculates
 * all the bblock and arc execution counts from the minimal
 * set encoded in the .da file.  This code is lifted almost
 * directly from gcov.c, including comments, with only the data
 * structures changed to protect the innocent.  And formatting
 * of course ;-)
 */
 
static gboolean
cov_function_solve(cov_function_t *fn)
{
    int passes, changes;
    int i;
    cov_arc_t *a;
    cov_block_t *b;
    int num_blocks;
    GList *iter;

    num_blocks = fn->blocks->len;

    /* For every block in the file,
       - if every exit/entrance arc has a known count, then set the block count
       - if the block count is known, and every exit/entrance arc but one has
	 a known execution count, then set the count of the remaining arc

       As arc counts are set, decrement the succ/pred count, but don't delete
       the arc, that way we can easily tell when all arcs are known, or only
       one arc is unknown.  */

    /* The order that the basic blocks are iterated through is important.
       Since the code that finds spanning trees starts with block 0, low numbered
       arcs are put on the spanning tree in preference to high numbered arcs.
       Hence, most instrumented arcs are at the end.  Graph solving works much
       faster if we propagate numbers from the end to the start.

       This takes an average of slightly more than 3 passes.  */

    changes = 1;
    passes = 0;
    while (changes)
    {
	passes++;
	changes = 0;

	for (i = num_blocks - 1; i >= 0; i--)
	{
	    b = g_ptr_array_index(fn->blocks, i);
	    
	    if (!b->count_valid)
	    {
		if (b->out_ninvalid == 0)
		{
		    cov_block_set_count(b, arc_list_total(b->out_arcs));
		    changes++;
		}
		else if (b->in_ninvalid == 0)
		{
		    cov_block_set_count(b, arc_list_total(b->in_arcs));
		    changes++;
		}
	    }
	    
	    if (b->count_valid)
	    {
		if (b->out_ninvalid == 1)
		{
		    /* Search for the invalid arc, and set its count.  */
		    if ((a = arc_list_find_invalid(b->out_arcs)) == 0)
			return FALSE;	/* ERROR */
		    /* Calculate count for remaining arc by conservation.  */
		    /* One of the counts will be invalid, but it is zero,
		       so adding it in also doesn't hurt.  */
		    cov_arc_set_count(a, b->count - arc_list_total(b->out_arcs));
		    changes++;
		}
		if (b->in_ninvalid == 1)
		{
		    /* Search for the invalid arc, and set its count.  */
		    if ((a = arc_list_find_invalid(b->in_arcs)) == 0)
			return FALSE;	/* ERROR */
		    /* Calculate count for remaining arc by conservation.  */
		    /* One of the counts will be invalid, but it is zero,
		       so adding it in also doesn't hurt.  */
		    cov_arc_set_count(a, b->count - arc_list_total(b->in_arcs));
		    changes++;
		}
	    }
	}
    }

    /* If the graph has been correctly solved, every block will have a
       succ and pred count of zero.  */
    for (i = 0; i < num_blocks; i++)
    {
    	b = g_ptr_array_index(fn->blocks, i);
	if (b->out_ninvalid || b->in_ninvalid)
	    return FALSE;
    }

#if DEBUG    
    fprintf(stderr, "Solved flow graph for %s in %d passes\n",
    	    	    	fn->name, passes);
#endif
    	
    return TRUE;
}


gboolean
cov_file_solve(cov_file_t *f)
{
    int fnidx;
    
    for (fnidx = 0 ; fnidx < f->functions->len ; fnidx++)
    {
    	if (!cov_function_solve(
	    	    g_ptr_array_index(f->functions, fnidx)))
	{
	    fprintf(stderr, "ERROR: could not solve flow graph\n");
	    return FALSE;
	}
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BB_FILENAME 	0x80000001
#define BB_FUNCTION 	0x80000002
#define BB_ENDOFLIST	0x00000000

static gboolean
cov_read_bb_file(cov_file_t *f, const char *bbfilename)
{
    FILE *fp;
    covio_u32_t tag;
    char *funcname = 0;
    char *filename = 0;
    cov_function_t *fn = 0;
    int funcidx;
    int bidx;
    
#if DEBUG    
    fprintf(stderr, "Reading .bb file \"%s\"\n", bbfilename);
#endif

    if ((fp = fopen(bbfilename, "r")) == 0)
    {
    	perror(bbfilename);
	return FALSE;
    }


    funcidx = 0;
    while (covio_read_u32(fp, &tag))
    {
    	switch (tag)
	{
	case BB_FILENAME:
	    if (filename != 0)
	    	g_free(filename);
	    filename = covio_read_bbstring(fp, tag);
#if DEBUG    
	    fprintf(stderr, "BB filename = \"%s\"\n", filename);
#endif
	    break;
	    
	case BB_FUNCTION:
	    funcname = covio_read_bbstring(fp, tag);
#if DEBUG    
	    fprintf(stderr, "BB function = \"%s\"\n", funcname);
#endif
	    fn = g_ptr_array_index(f->functions, funcidx);
	    funcidx++;
	    bidx = 0;
	    cov_function_set_name(fn, funcname);
	    g_free(funcname);
	    break;
	
	case BB_ENDOFLIST:
	    bidx++;
	    break;
	    
	default:
#if DEBUG    
	    fprintf(stderr, "BB line = %d\n", (int)tag);
#endif
	    assert(fn != 0);

	    cov_block_add_location(
	    	    g_ptr_array_index(fn->blocks, bidx),
		    filename, tag);
	    break;
	}
    }
    
    strdelete(filename);
    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BBG_SEPERATOR	0x80000001

static gboolean
cov_read_bbg_function(cov_file_t *f, FILE *fp)
{
    covio_u32_t nblocks, totnarcs, narcs;
    covio_u32_t bidx, aidx;
    covio_u32_t dest, flags;
    covio_u32_t sep;
    cov_block_t *b;
    cov_arc_t *a;
    cov_function_t *fn;
    
#if DEBUG    
    fprintf(stderr, "BBG reading function\n");
#endif
    
    if (!covio_read_u32(fp, &nblocks))
    	return FALSE;
    covio_read_u32(fp, &totnarcs);
    
    fn = cov_file_add_function(f);
    for (bidx = 0 ; bidx < nblocks ; bidx++)
    	cov_function_add_block(fn);
	
    for (bidx = 0 ; bidx < nblocks ; bidx++)
    {
#if DEBUG    
    	fprintf(stderr, "BBG   block %d\n", bidx);
#endif
	b = g_ptr_array_index(fn->blocks, bidx);
	covio_read_u32(fp, &narcs);

	for (aidx = 0 ; aidx < narcs ; aidx++)
	{
	    covio_read_u32(fp, &dest);
	    covio_read_u32(fp, &flags);

#if DEBUG    
    	    fprintf(stderr, "BBG     arc %d: %d->%d flags %s,%s,%s\n",
	    	    	    aidx,
			    bidx, dest,
			    (flags & 1 ? "on_tree" : ""),
			    (flags & 1 ? "fake" : ""),
			    (flags & 1 ? "fall_through" : ""));
#endif
			    
	    a = cov_block_add_arc_to(
	    	    g_ptr_array_index(fn->blocks, bidx),
	    	    g_ptr_array_index(fn->blocks, dest));
	    a->on_tree = (flags & 0x1);
	    a->fake = !!(flags & 0x2);
	    a->fall_through = !!(flags & 0x4);
	}
    }

    covio_read_u32(fp, &sep);
    if (sep != BBG_SEPERATOR)
    	return FALSE;
	
    return TRUE;
}


static gboolean
cov_read_bbg_file(cov_file_t *f, const char *bbgfilename)
{
    FILE *fp;
    
#if DEBUG    
    fprintf(stderr, "Reading .bbg file \"%s\"\n", bbgfilename);
#endif
    
    if ((fp = fopen(bbgfilename, "r")) == 0)
    {
    	perror(bbgfilename);
	return FALSE;
    }
    
    while (!feof(fp))
    	cov_read_bbg_function(f, fp);

    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
cov_read_da_file(cov_file_t *f, const char *dafilename)
{
    FILE *fp;
    covio_u64_t nents;
    covio_u64_t ent;
    int fnidx;
    int bidx;
    GList *aiter;
    
#if DEBUG    
    fprintf(stderr, "Reading .da file \"%s\"\n", dafilename);
#endif
    
    if ((fp = fopen(dafilename, "r")) == 0)
    {
    	perror(dafilename);
	return FALSE;
    }

    covio_read_u64(fp, &nents);
    
    for (fnidx = 0 ; fnidx < f->functions->len ; fnidx++)
    {
    	cov_function_t *fn = g_ptr_array_index(f->functions, fnidx);
	
	for (bidx = 0 ; bidx < fn->blocks->len ; bidx++)
	{
    	    cov_block_t *b = g_ptr_array_index(fn->blocks, bidx);
	
	    for (aiter = b->out_arcs ; aiter != 0 ; aiter = aiter->next)
	    {
	    	cov_arc_t *a = (cov_arc_t *)aiter->data;
		
		if (a->on_tree)
		    continue;

    	    	/* TODO: check that nents is correct */
    		if (!covio_read_u64(fp, &ent))
		{
		    fprintf(stderr, "%s: short file\n", dafilename);
		    return FALSE;
		}

#if DEBUG
    	    	fprintf(stderr, "DA arc {from=%s:%d to=%s:%d} count=%llu\n",
		    	    a->from->function->name, a->from->idx,
		    	    a->to->function->name, a->to->idx,
			    ent);
#endif

    	    	cov_arc_set_count(a, ent);
	    }
	}
    }    
    
    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_handle_c_file(const char *cfilename)
{
    char *bbfilename;
    char *bbgfilename;
    char *dafilename;
    cov_file_t *f;
    gboolean res;

#if DEBUG    
    fprintf(stderr, "Handling C source file %s\n", cfilename);
#endif
        
    bbfilename = file_change_extension(cfilename, ".c", ".bb");
    if (file_is_regular(bbfilename) < 0)
    {
    	perror(bbfilename);
	return FALSE;
    }

    bbgfilename = file_change_extension(cfilename, ".c", ".bbg");
    if (file_is_regular(bbgfilename) < 0)
    {
    	perror(bbgfilename);
	g_free(bbfilename);
	return FALSE;
    }

    dafilename = file_change_extension(cfilename, ".c", ".da");
    if (file_is_regular(dafilename) < 0)
    {
    	perror(dafilename);
	g_free(bbfilename);
	g_free(bbgfilename);
	return FALSE;
    }

    if ((f = cov_file_find(cfilename)) != 0)
    {
    	fprintf(stderr, "Internal error: handling %s twice\n", cfilename);
	g_free(bbfilename);
	g_free(bbgfilename);
	g_free(dafilename);
    	return FALSE;
    }
    f = cov_file_new(cfilename);

    res = TRUE;
    if (!cov_read_bbg_file(f, bbgfilename) ||
	!cov_read_bb_file(f, bbfilename) ||
	!cov_read_da_file(f, dafilename) ||
    	!cov_file_solve(f))
	res = FALSE;

    g_free(bbfilename);
    g_free(bbgfilename);
    g_free(dafilename);
    return res;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
