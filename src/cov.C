/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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
#include "string_var.H"
#include <dirent.h>


#ifdef HAVE_LIBBFD
#include <bfd.h>
#include <elf.h>
#endif

CVSID("$Id: cov.C,v 1.8 2003-05-11 02:32:30 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_location_t::make_key() const
{
    return g_strdup_printf("%s:%lu", filename, lineno);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_status_t
cov_get_count_by_location(const cov_location_t *loc, count_t *countp)
{
    const GList *blocks = cov_block_t::find_by_location(loc);
    count_t minc = COV_COUNT_MAX, maxc = 0;
    int len = 0;
    
    for ( ; blocks != 0 ; blocks = blocks->next)
    {
    	cov_block_t *b = (cov_block_t *)blocks->data;
	
	if (b->count() > maxc)
	    maxc = b->count();
	if (b->count() < minc)
	    minc = b->count();
	len++;
    }
    
    if (countp != 0)
	*countp = maxc;

    if (len == 0)
    	return CS_UNINSTRUMENTED;
    if (maxc == 0)
    	return CS_UNCOVERED;
    return (minc == 0 ? CS_PARTCOVERED : CS_COVERED);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_add_search_directory(const char *dir)
{
    cov_file_t::search_path_append(dir);
}

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

gboolean
cov_read_source_file_2(const char *fname, gboolean quiet)
{
    cov_file_t *f;
    string_var filename;

    filename = file_make_absolute(fname);
#if DEBUG    
    fprintf(stderr, "Handling source file %s\n", filename.data());
#endif

    if ((f = cov_file_t::find(filename)) != 0)
    {
    	if (!quiet)
    	    fprintf(stderr, "Internal error: handling %s twice\n", filename.data());
    	return FALSE;
    }

    f = new cov_file_t(filename);

    if (!f->read(quiet))
    {
	delete f;
	return FALSE;
    }
    
    return TRUE;
}

gboolean
cov_read_source_file(const char *filename)
{
    return cov_read_source_file_2(filename, /*quiet*/FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_overall_calc_stats(cov_stats_t *stats)
{
    list_iterator_t<cov_file_t> iter;
    
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    	(*iter)->calc_stats(stats);
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

#if DEBUG > 1
/*
 * For N blocks, 0..N-1,
 * block 0: in arcs are calls to the function => 1 arc 1 fake
 *          out arcs are fallthroughs to the function body => 1 arc 0 fake
 * block N-1: in arcs are total calls from this function => same
 *                number of arcs as calls, all fake.
 *            out arcs are returns from the function, 1 arc, 1 fake.
 */
void
cov_check_fakeness(cov_file_t *f)
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

#endif /* DEBUG>1 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifdef HAVE_LIBBFD

static void
cov_bfd_error_handler(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}
#endif /* HAVE_LIBBFD */

void
cov_init(void)
{
#ifdef HAVE_LIBBFD
    bfd_init();
    bfd_set_error_handler(cov_bfd_error_handler);
#endif /* HAVE_LIBBFD */

    cov_callnode_t::init();
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
    list_iterator_t<cov_file_t> iter;
    
    /* construct the list of filenames */
    cov_file_t::post_read();

    /* Build the callgraph */
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    	cov_add_callnodes(*iter);
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    	cov_add_callarcs(*iter);

#if DEBUG > 1
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    	cov_check_fakeness(*iter);
#endif
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
#ifdef HAVE_LIBBFD
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
#else /* !HAVE_LIBBFD */
    return TRUE;
#endif /* !HAVE_LIBBFD */
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
