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

#include "cov.H"
#include "covio.h"
#include "estring.H"
#include "filename.h"
#include "estring.H"
#include <dirent.h>


#include <bfd.h>
#include <elf.h>

CVSID("$Id: cov.C,v 1.1 2002-12-29 13:14:16 gnb Exp $");

/* TODO: ? reorg this */
GHashTable *cov_callnodes;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_location_t::make_key() const
{
    return g_strdup_printf("%s:%lu", filename, lineno);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_get_count_by_location(
    const cov_location_t *loc,
    count_t *countp,
    gboolean *existsp)
{
    const GList *blocks = cov_block_t::find_by_location(loc);
    
    *existsp = (blocks != 0);
    *countp = cov_block_t::total(blocks);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_is_source_filename(const char *filename)
{
    const char *ext;
    static const char *recognised_exts[] = { ".c", ".C", 0 };
    int i;
    
    if ((ext = file_extension_c(filename)) == 0)
    	return FALSE;
    for (i = 0 ; recognised_exts[i] != 0 ; i++)
    {
    	if (!strcmp(ext, recognised_exts[i]))
	    return TRUE;
    }
    return FALSE;
}

#define FN_BB	0
#define FN_BBG	1
#define FN_DA	2
#define FN_O	3
#define FN_MAX	4

gboolean
cov_read_source_file_2(const char *fname, gboolean quiet)
{
    cov_file_t *f;
    gboolean res;
    int i;
    char *filename, *filenames[FN_MAX];
    static const char *extensions[FN_MAX] = { ".bb", ".bbg", ".da", ".o" };


    filename = file_make_absolute(fname);
#if DEBUG    
    fprintf(stderr, "Handling source file %s\n", filename);
#endif

    if ((f = cov_file_t::find(filename)) != 0)
    {
    	if (!quiet)
    	    fprintf(stderr, "Internal error: handling %s twice\n", filename);
    	g_free(filename);
    	return FALSE;
    }

    memset(filenames, 0, sizeof(filenames));
    
    for (i = 0 ; i < FN_MAX ; i++)
    {
	filenames[i] = file_change_extension(filename, 0, extensions[i]);
	if (file_is_regular(filenames[i]) < 0)
	{
    	    if (!quiet)
    		perror(filenames[i]);
	    for (i = 0 ; i < FN_MAX ; i++)
	    	strdelete(filenames[i]);
    	    g_free(filename);
	    return FALSE;
	}
    }
        
    f = new cov_file_t(filename);

    res = TRUE;
    if (!f->read_bbg_file(filenames[FN_BBG]) ||
	!f->read_bb_file(filenames[FN_BB]) ||
	!f->read_da_file(filenames[FN_DA]) ||
	!f->read_o_file(filenames[FN_O]) ||
    	!f->solve())
	res = FALSE;

    for (i = 0 ; i < FN_MAX ; i++)
	strdelete(filenames[i]);
    g_free(filename);
    return res;
}

gboolean
cov_read_source_file(const char *filename)
{
    return cov_read_source_file_2(filename, /*quiet*/FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: really, really need a proper iteration interface for files */
static void
cov_file_calc_stats_tramp(cov_file_t *f, void *userdata)
{
    f->calc_stats((cov_stats_t *)userdata);
}

void
cov_overall_calc_stats(cov_stats_t *stats)
{
    cov_file_t::foreach(cov_file_calc_stats_tramp, stats);
}


void
cov_range_calc_stats(
    const cov_location_t *startp,
    const cov_location_t *endp,
    cov_stats_t *stats)
{
    cov_file_t *f;
    cov_location_t start = *startp, end = *endp;
    cov_block_t *b;
    unsigned fnidx, bidx;
    unsigned long lastline;
    const GList *startblocks, *endblocks;

    /*
     * Check inputs
     */
    if ((f = cov_file_t::find(start.filename)) == 0)
    	return;     /* nonexistant filename */
    if (strcmp(start.filename, end.filename))
    	return;     /* invalid range */
    if (start.lineno > end.lineno)
    	return;     	/* invalid range */
    if (start.lineno == 0 || end.lineno == 0)
    	return;     	/* invalid range */
    lastline = f->get_last_location()->lineno;
    if (start.lineno > lastline)
    	return;     	/* range is outside file */
    if (end.lineno > lastline)
    	end.lineno = lastline;     /* clamp range to file */
    
    /*
     * Get blocklists for start and end.
     */
    do
    {
    	startblocks = cov_block_t::find_by_location(&start);
    } while (startblocks == 0 && ++start.lineno <= end.lineno);
    
    if (startblocks == 0)
    	return;     	/* no executable lines in the given range */

    do
    {
    	endblocks = cov_block_t::find_by_location(&end);
    } while (endblocks == 0 && --end.lineno > start.lineno-1);
    
    assert(endblocks != 0);
    assert(start.lineno <= end.lineno);
    

    /*
     * Iterate over the blocks between start and end,
     * gathering stats as we go.  Note that this can
     * span functions.
     */    
    b = (cov_block_t *)startblocks->data;
    bidx = b->bindex();
    fnidx = b->function()->findex();
    
    do
    {
	b = f->nth_function(fnidx)->nth_block(bidx);
    	b->calc_stats(stats);
	if (++bidx == f->nth_function(fnidx)->num_blocks())
	{
	    bidx = 0;
	    ++fnidx;
	}
    } while (b != (cov_block_t *)endblocks->data);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    void (*func)(cov_callnode_t *, void *);
    void *userdata;
} cov_callnode_foreach_rec_t;

static void
cov_callnode_foreach_tramp(gpointer key, gpointer value, gpointer userdata)
{
    cov_callnode_t *cn = (cov_callnode_t *)value;
    cov_callnode_foreach_rec_t *rec = (cov_callnode_foreach_rec_t *)userdata;
    
    (*rec->func)(cn, rec->userdata);
}


void
cov_callnode_foreach(
    void (*func)(cov_callnode_t*, void *userdata),
    void *userdata)
{
    cov_callnode_foreach_rec_t rec;
    
    rec.func = func;
    rec.userdata = userdata;
    g_hash_table_foreach(cov_callnodes, cov_callnode_foreach_tramp, &rec);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callnode_t *
cov_callnode_find(const char *name)
{
    return (cov_callnode_t *)g_hash_table_lookup(cov_callnodes, name);
}

static cov_callnode_t *
cov_callnode_new(const char *name)
{
    cov_callnode_t *cn;
    
    cn = new(cov_callnode_t);
    strassign(cn->name, name);
    
    g_hash_table_insert(cov_callnodes, cn->name, cn);
    
    return cn;
}

#if 0
static gboolean
cov_callnode_delete_one(gpointer key, gpointer value, gpointer userdata)
{
    cov_callnode_t *cn = (cov_callnode_t *)value;
    
    listdelete(cn->out_arcs, cov_callarc_t, g_free);
    listclear(cn->in_arcs);
    strdelete(cn->name);
    g_free(cn);
    
    return TRUE;    /* please remove me */
}

static void
cov_callgraph_clear(void)
{
    g_hash_table_foreach_remove(cov_callnodes, cov_callnode_delete_one, 0);
}
#endif

static cov_callarc_t *
cov_callnode_find_arc_to(
    cov_callnode_t *from,
    cov_callnode_t *to)
{
    GList *iter;
    
    for (iter = from->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *ca = (cov_callarc_t *)iter->data;
	
	if (ca->to == to)
	    return ca;
    }
    
    return 0;
}

static cov_callarc_t *
cov_callnode_add_arc_to(
    cov_callnode_t *from,
    cov_callnode_t *to)
{
    cov_callarc_t *ca;
    
    ca = new(cov_callarc_t);
    
    ca->from = from;
    from->out_arcs = g_list_append(from->out_arcs, ca);

    ca->to = to;
    to->in_arcs = g_list_append(to->in_arcs, ca);
    
    return ca;
}

static void
cov_callarc_add_count(cov_callarc_t *ca, count_t count)
{
    ca->count += count;
    ca->to->count += count;
}

static void
cov_add_callnodes(cov_file_t *f, void *userdata)
{
    unsigned int fnidx;
    cov_callnode_t *cn;
    
    for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
    {
    	cov_function_t *fn = f->nth_function(fnidx);
	
    	if (fn->is_suppressed())
	    continue;

	if ((cn = cov_callnode_find(fn->name())) == 0)
	{
	    cn = cov_callnode_new(fn->name());
	    cn->function = fn;
	}
    }
}

static void
cov_add_callarcs(cov_file_t *f, void *userdata)
{
    unsigned int fnidx;
    unsigned int bidx;
    list_iterator_t<cov_arc_t> aiter;
    cov_callnode_t *from;
    cov_callnode_t *to;
    cov_callarc_t *ca;
    
    for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
    {
    	cov_function_t *fn = f->nth_function(fnidx);
	
	if (fn->is_suppressed())
	    continue;

	from = cov_callnode_find(fn->name());
	assert(from != 0);
	
	for (bidx = 0 ; bidx < fn->num_blocks()-2 ; bidx++)
	{
    	    cov_block_t *b = fn->nth_block(bidx);
	
	    for (aiter = b->out_arc_iterator() ; aiter != (cov_arc_t *)0 ; ++aiter)
	    {
	    	cov_arc_t *a = *aiter;

    	    	if (!a->is_call() || a->name() == 0)
		    continue;
		    		
		if ((to = cov_callnode_find(a->name())) == 0)
		    to = cov_callnode_new(a->name());
		    
		if ((ca = cov_callnode_find_arc_to(from, to)) == 0)
		    ca = cov_callnode_add_arc_to(from, to);
		    
		cov_callarc_add_count(ca, a->from()->count());
	    }
	}
    }
}


/*
 * For N blocks, 0..N-1,
 * block 0: in arcs are calls to the function => 1 arc 1 fake
 *          out arcs are fallthroughs to the function body => 1 arc 0 fake
 * block N-1: in arcs are total calls from this function => same
 *                number of arcs as calls, all fake.
 *            out arcs are returns from the function, 1 arc, 1 fake.
 */
void
cov_check_fakeness(cov_file_t *f, void *userdata)
{
    unsigned int fnidx;
    unsigned int bidx;
    
    fprintf(stderr, "check_fakeness: file %s\n", f->minimal_name());
    for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
    {
    	cov_function_t *fn = f->nth_function(fnidx);
	unsigned int ncalls = 0;
	
	if (fn->is_suppressed())
	    continue;

	fprintf(stderr, "    func %s [%d blocks]\n", fn->name(), fn->num_blocks());
	for (bidx = 0 ; bidx < fn->num_blocks() ; bidx++)
	{
    	    cov_block_t *b = fn->nth_block(bidx);
	
	    fprintf(stderr, "        block %u: in=%d/%d out=%d/%d\n",
	    	    bidx,
		    cov_arc_t::nfake(b->in_arcs_),
		    b->in_arcs_.length(),
		    cov_arc_t::nfake(b->out_arcs_),
		    b->out_arcs_.length());
    	    if (bidx == 0)
	    {
    		assert(cov_arc_t::nfake(b->out_arcs_) == 0);
    		assert(b->out_arcs_.length() == 1);
    		assert(cov_arc_t::nfake(b->in_arcs_) == 1);
    		assert(b->out_arcs_.length() == 1);
	    }
	    else if (bidx == fn->num_blocks()-1)
	    {
    		assert(cov_arc_t::nfake(b->out_arcs_) == 1);
    		assert(b->out_arcs_.length() == 1);
    		assert(cov_arc_t::nfake(b->in_arcs_) == ncalls);
    		assert(b->in_arcs_.length() == ncalls);
	    }
	    else
	    {
		assert(cov_arc_t::nfake(b->out_arcs_) >= 0);
    		assert(cov_arc_t::nfake(b->out_arcs_) <= 1);
		assert(cov_arc_t::nfake(b->out_arcs_) == 0 || b->out_arcs_.length() <= 2);
    		assert(cov_arc_t::nfake(b->in_arcs_) == 0);
		ncalls += cov_arc_t::nfake(b->out_arcs_);
	    }
	}
    }
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
cov_bfd_error_handler(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void
cov_init(void)
{
    bfd_init();
    bfd_set_error_handler(cov_bfd_error_handler);

    cov_callnodes = g_hash_table_new(g_str_hash, g_str_equal);
    cov_file_t::init();
    cov_block_t::init();
}

void
cov_pre_read(void)
{
    /* TODO: clear out all the previous data structures */
}

void
cov_post_read(void)
{
    /* Build the callgraph */
    cov_file_t::foreach(cov_add_callnodes, 0);
    cov_file_t::foreach(cov_add_callarcs, 0);
    cov_file_t::foreach(cov_check_fakeness, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Large chunks of code ripped from binutils/rddbg.c and stabs.c
 */
 
typedef struct
{
    uint32_t strx;
#define N_SO 0x64
    uint8_t type;
    uint8_t other;
    uint16_t desc;
    uint32_t value;
} cov_stab32_t;

gboolean
cov_read_object_file(const char *exefilename)
{
    bfd *abfd;
    asection *stab_sec, *string_sec;
    bfd_size_type string_size;
    bfd_size_type stroff, next_stroff;
    unsigned int num_stabs;
    cov_stab32_t *stabs, *st;
    char *strings;
    char *dir = "", *file;
    int successes = 0;
        
#if DEBUG
    fprintf(stderr, "Reading object or exe file \"%s\"\n", exefilename);
#endif
    
    if ((abfd = bfd_openr(exefilename, 0)) == 0)
    {
    	/* TODO */
    	bfd_perror(exefilename);
	return FALSE;
    }
    if (!bfd_check_format(abfd, bfd_object))
    {
    	/* TODO */
    	bfd_perror(exefilename);
	bfd_close(abfd);
	return FALSE;
    }


    if ((stab_sec = bfd_get_section_by_name(abfd, ".stab")) == 0)
    {
    	fprintf(stderr, "%s: no .stab section\n", exefilename);
	bfd_close(abfd);
	return FALSE;
    }

    if ((string_sec = bfd_get_section_by_name(abfd, ".stabstr")) == 0)
    {
    	fprintf(stderr, "%s: no .stabstr section\n", exefilename);
	bfd_close(abfd);
	return FALSE;
    }
    
    num_stabs = bfd_section_size(abfd, stab_sec);
    stabs = (cov_stab32_t *)gnb_xmalloc(num_stabs);
    if (!bfd_get_section_contents(abfd, stab_sec, stabs, 0, num_stabs))
    {
    	/* TODO */
    	bfd_perror(exefilename);
	bfd_close(abfd);
	return FALSE;
    }
    num_stabs /= sizeof(cov_stab32_t);

    string_size = bfd_section_size(abfd, string_sec);
    strings = (char *)gnb_xmalloc(string_size);
    if (!bfd_get_section_contents(abfd, string_sec, strings, 0, string_size))
    {
    	/* TODO */
    	bfd_perror(exefilename);
	bfd_close(abfd);
	g_free(stabs);
	return FALSE;
    }

    assert(sizeof(cov_stab32_t) == 12);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    assert(bfd_little_endian(abfd));
#else
    assert(bfd_big_endian(abfd));
#endif

    stroff = 0;
    next_stroff = 0;
    for (st = stabs ; st < stabs + num_stabs ; st++)
    {
    	if (st->type == 0)
	{
	    /*
	     * Special type 0 stabs indicate the offset to the
	     * next string table.
	     */
	    stroff = next_stroff;
	    next_stroff += st->value;
	}
	else if (st->type == N_SO)
	{
	    char *s;
	    
	    if (stroff + st->strx > string_size)
	    {
		fprintf(stderr, "%s: stab entry %d is corrupt, strx = 0x%x, type = %d\n",
		    exefilename,
		    (st - stabs), st->strx, st->type);
		continue;
	    }
	    
    	    s = strings + stroff + st->strx;
	    
#if 0
	    printf("%u|%02x|%02x|%04x|%08x|%s|\n",
	    	(st - stabs),
		st->type,
		st->other,
		st->desc,
		st->value,
		s);
#endif
	    if (s[0] == '\0')
	    	continue;
	    if (s[strlen(s)-1] == '/')
	    {
	    	dir = s;
	    }
	    else
	    {
	    	file = g_strconcat(dir, s, 0);
		if (cov_is_source_filename(file) &&
		    file_is_regular(file) == 0 &&
		    cov_read_source_file_2(file, /*quiet*/TRUE))
		    successes++;
		g_free(file);
	    }
	}
    }

    g_free(stabs); 
    g_free(strings);
    bfd_close(abfd);
    if (successes == 0)
    	fprintf(stderr, "found no coveraged source files in executable \"%s\"\n",
	    	exefilename);
    return (successes > 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_read_directory(const char *dirname)
{
    DIR *dir;
    struct dirent *de;
    int dirlen;
    int successes = 0;
    
    estring child = file_make_absolute(dirname);

    if ((dir = opendir(child.data())) == 0)
    {
    	perror(child.data());
    	return FALSE;
    }
    
    if (child.last() != '/')
	child.append_char('/');
    dirlen = child.length();
    
    while ((de = readdir(dir)) != 0)
    {
    	if (!strcmp(de->d_name, ".") || 
	    !strcmp(de->d_name, ".."))
	    continue;
	    
	child.truncate_to(dirlen);
	child.append_string(de->d_name);
	
    	if (file_is_regular(child.data()) == 0 &&
	    cov_is_source_filename(child.data()))
	    successes += cov_read_source_file(child.data());
    }
    
    closedir(dir);
    if (successes == 0)
    	fprintf(stderr, "found no coveraged source files in directory \"%s\"\n",
	    	dirname);
    return (successes > 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/