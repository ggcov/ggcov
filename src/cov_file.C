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
#include "string_var.H"
#include "filename.h"
#include "demangle.h"

#ifdef HAVE_LIBBFD
#include <bfd.h>
#include <elf.h>
#endif

CVSID("$Id: cov_file.C,v 1.16 2003-06-28 10:43:53 gnb Exp $");


hashtable_t<const char*, cov_file_t> *cov_file_t::files_;
list_t<cov_file_t> cov_file_t::files_list_;
list_t<char> cov_file_t::search_path_;
char *cov_file_t::common_path_;
int cov_file_t::common_len_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


cov_file_t::cov_file_t(const char *name)
 :  name_(name)
{
    /*
     * It is the caller's responsibility to create cov_file_t objects
     * with an absolute filename which is not already known.
     */
    assert(name[0] == '/');
    assert(find(name_) == 0);

    functions_ = g_ptr_array_new();
    functions_by_name_ = new hashtable_t<const char*, cov_function_t>;
}

cov_file_t::~cov_file_t()
{
    unsigned int i;

    if (finalised_)
    {
        files_list_.remove(this);
        files_->remove(name_);
    }

    for (i = 0 ; i < num_functions() ; i++)
    	delete nth_function(i);
    g_ptr_array_free(functions_, /*delete_seg*/TRUE);

    delete functions_by_name_;

    if (finalised_)
    {
	dirty_common_path();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::init()
{
    files_ = new hashtable_t<const char*, cov_file_t>;
    common_path_ = 0;
    common_len_ = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::finalise()
{
    files_->insert(name_, this);

    add_name(name_);

    finalised_ = TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
compare_files(const cov_file_t *fa, const cov_file_t *fb)
{
    return strcmp(fa->minimal_name(), fb->minimal_name());
}

static void
cov_file_add(const char *name, cov_file_t *f, gpointer userdata)
{
    list_t<cov_file_t> *listp = (list_t<cov_file_t> *)userdata;
    
    listp->prepend(f);
}

void
cov_file_t::post_read()
{
    files_->foreach(cov_file_add, &files_list_);
    files_list_.sort(compare_files);
}

list_iterator_t<cov_file_t>
cov_file_t::first()
{
    return files_list_.first();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

	/* TODO: handle names relative to $PWD ? */

cov_file_t *
cov_file_t::find(const char *name)
{
    assert(files_ != 0);
    string_var fullname = unminimise_name(name);
    return files_->lookup(fullname);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::add_name(const char *name)
{
    assert(name[0] == '/');
    if (common_path_ == 0)
    {
    	/* first filename: initialise the common path to the directory */
	char *p;
    	common_path_ = g_strdup(name);
	if ((p = strrchr(common_path_, '/')) != 0)
	    p[1] = '\0';
    }
    else
    {
    	/* subsequent filenames: shrink common path as necessary */
	char *cs, *ce, *ns, *ne;
	cs = common_path_+1;
	ns = (char *)name+1;
	for (;;)
	{
	    if ((ne = strchr(ns, '/')) == 0)
	    	break;
	    if ((ce = strchr(cs, '/')) == 0)
	    	break;
	    if ((ce - cs) != (ne - ns))
	    	break;
	    if (memcmp(cs, ns, (ne - ns)))
	    	break;
	    cs = ce+1;
	    ns = ne+1;
	}
	*cs = '\0';
    }
    common_len_ = strlen(common_path_);
#if DEBUG > 1
    fprintf(stderr, "cov_file_t::add_name: name=\"%s\" => common=\"%s\"\n",
    	    	name, common_path_);
#endif
}

void
cov_file_t::dirty_common_path()
{
    if (common_path_ != 0)
    {
	g_free(common_path_);
	common_path_ = 0;
	common_len_ = -1;   /* indicates dirty */
    }
}

void
cov_file_t::add_name_tramp(const char *name, cov_file_t *f, gpointer userdata)
{
    add_name(name);
}

void
cov_file_t::check_common_path()
{
    if (common_len_ < 0)
    {
#if DEBUG > 1
    	fprintf(stderr, "cov_file_t::check_common_path: recalculating common path\n");
#endif
    	common_len_ = 0;
	files_->foreach(add_name_tramp, 0);
    }
}

const char *
cov_file_t::minimal_name() const
{
    check_common_path();
    return name_.data() + common_len_;
}

char *
cov_file_t::minimise_name(const char *name)
{
    check_common_path();
    if (!strncmp(name, common_path_, common_len_))
    {
    	return g_strdup(name + common_len_);
    }
    else
    {
	assert(name[0] == '/');
	return g_strdup(name);
    }
}

char *
cov_file_t::unminimise_name(const char *name)
{
    if (name[0] == '/')
    {
    	/* absolute name */
    	return g_strdup(name);
    }
    else
    {
    	/* partial, presumably minimal, name */
    	check_common_path();
	return g_strconcat(common_path_, name, 0);
    }
}

const char *
cov_file_t::common_path()
{
    check_common_path();
    return common_path_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const cov_location_t *
cov_file_t::get_last_location() const
{
    return last_location_;
}

void
cov_file_t::add_location(const cov_location_t *loc)
{
    if (!strcmp(name_, loc->filename) &&
    	(last_location_ == 0 || last_location_->lineno < loc->lineno))
	last_location_ = loc;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t *
cov_file_t::add_function()
{
    cov_function_t *fn;
    
    fn = new cov_function_t();
    
    fn->idx_ = functions_->len;
    g_ptr_array_add(functions_, fn);    
    fn->file_ = this;
    
    return fn;
}

cov_function_t *
cov_file_t::find_function(const char *fnname) const
{
    return functions_by_name_->lookup(fnname);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::calc_stats(cov_stats_t *stats) const
{
    unsigned int fnidx;
    
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    {
	nth_function(fnidx)->calc_stats(stats);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::solve()
{
    unsigned int fnidx;
    
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    {
    	if (!nth_function(fnidx)->solve())
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

gboolean
cov_file_t::read_bb_file(const char *bbfilename)
{
    FILE *fp;
    gnb_u32_t tag;
    string_var funcname;
    string_var filename;
    cov_function_t *fn = 0;
    int funcidx;
    int bidx = 0;
    int line;
    int nlines;
    
#if DEBUG > 1
    fprintf(stderr, "Reading .bb file \"%s\"\n", bbfilename);
#endif

    if ((fp = fopen(bbfilename, "r")) == 0)
    {
    	perror(bbfilename);
	return FALSE;
    }


    funcidx = 0;
    line = 0;
    nlines = 0;
    while (covio_read_lu32(fp, &tag))
    {
    	switch (tag)
	{
	case BB_FILENAME:
	    filename = covio_read_bbstring(fp, tag);
	    if (strchr(filename, '/') == 0 &&
	    	!strcmp(filename, file_basename_c(name_)))
		filename = name_.data();
#if DEBUG > 1
	    fprintf(stderr, "BB filename = \"%s\"\n", filename.data());
#endif
	    break;
	    
	case BB_FUNCTION:
	    funcname = covio_read_bbstring(fp, tag);
	    funcname = normalise_mangled(funcname);
#if DEBUG > 1
	    fprintf(stderr, "BB function = \"%s\"\n", funcname.data());
#endif
	    fn = nth_function(funcidx);
	    funcidx++;
	    bidx = 0;
	    line = 0;
	    nlines = 0;
	    fn->set_name(funcname);
	    break;
	
	case BB_ENDOFLIST:
	    if (line != 0 && nlines == 0)
	    {
		assert(fn != 0);
		assert(bidx != 0);
		assert(filename != (const char*)0);
		fn->nth_block(bidx)->add_location(filename, line);
	    }
	    bidx++;
	    nlines = 0;
	    break;
	    
	default:
#if DEBUG > 1
	    fprintf(stderr, "BB line = %d (block %d)\n", (int)tag, bidx);
#endif
	    assert(fn != 0);

    	    line = tag;
	    fn->nth_block(bidx)->add_location(filename, line);
	    nlines++;
	    break;
	}
    }
    
    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BBG_SEPARATOR	0x80000001

/* arc flags */
#define BBG_ON_TREE	    	0x1
#define BBG_FAKE	    	0x2
#define BBG_FALL_THROUGH	0x4

/*
 * Put an arbitrary upper limit on complexity of functions to
 * prevent bogus data files (or the gcc 3.2 format which we don't
 * yet parse) from causing us to eat all swap in a tight loop.
 */
#define BBG_MAX_BLOCKS	(64 * 1024)
#define BBG_MAX_ARCS	(64 * 1024)

/*
 * The bbg_failed*() macros are for debugging problems with .bbg files.
 */
#if DEBUG > 1
/* I can't wait for C99 variadic macros to become common */
#define bbg_failed0(fmt) \
    { \
	fprintf(stderr, "BBG:%d, " fmt "\n", __LINE__); \
    	return FALSE; \
    }
#define bbg_failed1(fmt, a1) \
    { \
	fprintf(stderr, "BBG:%d, " fmt "\n", __LINE__, a1); \
    	return FALSE; \
    }
#define bbg_failed2(fmt, a1, a2) \
    { \
	fprintf(stderr, "BBG:%d, " fmt "\n", __LINE__, a1, a2); \
    	return FALSE; \
    }
#else
#define bbg_failed0(fmt)    	    	return FALSE;
#define bbg_failed1(fmt, a1)    	return FALSE;
#define bbg_failed2(fmt, a1, a2)    	return FALSE;
#endif


gboolean
cov_file_t::read_old_bbg_function(FILE *fp)
{
    gnb_u32_t nblocks, totnarcs, narcs;
    gnb_u32_t bidx, aidx;
    gnb_u32_t dest, flags;
    gnb_u32_t sep;
    cov_block_t *b;
    cov_arc_t *a;
    cov_function_t *fn;
    
#if DEBUG > 1
    fprintf(stderr, "BBG reading function\n");
#endif
    
    if (!covio_read_lu32(fp, &nblocks))
    	return TRUE;	/* end of file */

    if (!covio_read_lu32(fp, &totnarcs))
    	bbg_failed0("short file");
    
    if (nblocks > BBG_MAX_BLOCKS)
    	bbg_failed2("nblocks=%u > %u", nblocks, BBG_MAX_BLOCKS);
    if (totnarcs > BBG_MAX_ARCS)
    	bbg_failed2("totnarcs=%u > %u", totnarcs, BBG_MAX_ARCS);
    
    fn = add_function();
    for (bidx = 0 ; bidx < nblocks ; bidx++)
    	fn->add_block();
	
    for (bidx = 0 ; bidx < nblocks ; bidx++)
    {
#if DEBUG > 1
    	fprintf(stderr, "BBG   block %d\n", bidx);
#endif
	b = fn->nth_block(bidx);
	if (!covio_read_lu32(fp, &narcs))
    	    bbg_failed0("short file");

	if (narcs > BBG_MAX_ARCS)
    	    bbg_failed2("narcs=%u > %u", narcs, BBG_MAX_ARCS);

	for (aidx = 0 ; aidx < narcs ; aidx++)
	{
	    covio_read_lu32(fp, &dest);
	    if (!covio_read_lu32(fp, &flags))
	    	bbg_failed0("short file");

#if DEBUG > 1
    	    fprintf(stderr, "BBG     arc %u: %u->%u flags %x(%s,%s,%s)\n",
	    	    	    aidx,
			    bidx, dest, flags,
			    (flags & BBG_ON_TREE ? "on_tree" : ""),
			    (flags & BBG_FAKE ? "fake" : ""),
			    (flags & BBG_FALL_THROUGH ? "fall_through" : ""));
#endif
	    if (dest >= nblocks)
    	    	bbg_failed2("dest=%u > nblocks=%u", dest, nblocks);
			    
	    a = new cov_arc_t(fn->nth_block(bidx), fn->nth_block(dest));
	    a->on_tree_ = (flags & BBG_ON_TREE);
#ifdef HAVE_BBG_FAKE_FLAG
	    a->fake_ = !!(flags & BBG_FAKE);
#endif
    	    if (nblocks >= 2 && dest == nblocks-1)
	    {
	    	num_expected_fake_++;
    		if (!(flags & BBG_FAKE))
	    	    num_missing_fake_++;
	    }
	    a->fall_through_ = !!(flags & BBG_FALL_THROUGH);
	}
    }

    covio_read_lu32(fp, &sep);
    if (sep != BBG_SEPARATOR)
    	bbg_failed2("sep=0x%08x != 0x%08x", sep, BBG_SEPARATOR);
	
    return TRUE;
}

gboolean
cov_file_t::read_old_bbg_file(const char *bbgfilename, FILE *fp)
{
#if DEBUG > 1
    fprintf(stderr, "Reading old format .bbg file \"%s\"\n", bbgfilename);
#endif
    format_version_ = 0;

    /*
     * Rewind to the start of the file, because the FILE* passed in has
     * been seeked past the magic number, which we need to read again
     * as a function block count.
     */
    fseek(fp, 0L, SEEK_SET);
    
    while (!feof(fp))
    {
    	if (!read_old_bbg_function(fp))
	{
	    /* TODO */
	    fprintf(stderr, "%s: file is corrupted or in a bad file format.\n",
	    	    bbgfilename);
	    fclose(fp);
	    return FALSE;
	}
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * GCOV_TAG_* defines copied from gcc/gcc/gcov-io.h (cvs 20030615)
 */
/* The record tags.  Values [1..3f] are for tags which may be in either
   file.  Values [41..9f] for those in the bbg file and [a1..ff] for
   the data file.  */

#define GCOV_TAG_FUNCTION	 ((gnb_u32_t)0x01000000)
#define GCOV_TAG_BLOCKS		 ((gnb_u32_t)0x01410000)
#define GCOV_TAG_ARCS		 ((gnb_u32_t)0x01430000)
#define GCOV_TAG_LINES		 ((gnb_u32_t)0x01450000)
#define GCOV_TAG_COUNTER_BASE 	 ((gnb_u32_t)0x01a10000)
#define GCOV_TAG_OBJECT_SUMMARY  ((gnb_u32_t)0xa1000000)
#define GCOV_TAG_PROGRAM_SUMMARY ((gnb_u32_t)0xa3000000)

#if DEBUG > 1
static const struct 
{
    const char *name;
    gnb_u32_t value;
}
gcov_tags[] = 
{
{"GCOV_TAG_FUNCTION",		GCOV_TAG_FUNCTION},
{"GCOV_TAG_BLOCKS",		GCOV_TAG_BLOCKS},
{"GCOV_TAG_ARCS",		GCOV_TAG_ARCS},
{"GCOV_TAG_LINES",		GCOV_TAG_LINES},
{"GCOV_TAG_COUNTER_BASE",	GCOV_TAG_COUNTER_BASE},
{"GCOV_TAG_OBJECT_SUMMARY",	GCOV_TAG_OBJECT_SUMMARY},
{"GCOV_TAG_PROGRAM_SUMMARY",	GCOV_TAG_PROGRAM_SUMMARY},
{0, 0}
};

static const char *
gcov_tag_as_string(gnb_u32_t tag)
{
    int i;
    
    for (i = 0 ; gcov_tags[i].name != 0 ; i++)
    {
    	if (gcov_tags[i].value == tag)
	    return gcov_tags[i].name;
    }
    return "unknown";
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BBG_VERSION_3_3p \
     	(((gnb_u32_t)'3'<<24)| \
	 ((gnb_u32_t)'0'<<16)| \
	 ((gnb_u32_t)'3'<<8)| \
	 ((gnb_u32_t)'p'))

gboolean
cov_file_t::read_new_bbg_file(const char *bbgfilename, FILE *fp)
{
    gnb_u32_t tag, length;
    cov_function_t *fn = 0;
    string_var filename, funcname;
    gnb_u32_t tmp;
    unsigned int nblocks = 0;
    unsigned int bidx;
    cov_arc_t *a;
    gnb_u32_t dest, flags;
    gnb_u32_t line;

#if DEBUG > 1
    fprintf(stderr, "Reading new format .bbg file \"%s\"\n", bbgfilename);
#endif

    if (!covio_read_bu32(fp, &format_version_))
    	bbg_failed0("short file");
    if (format_version_ != BBG_VERSION_3_3p)
    	bbg_failed2("bad version=0x%08x != 0x%08x",
	    	    format_version_, BBG_VERSION_3_3p);

    while (covio_read_bu32(fp, &tag))
    {
	if (!covio_read_bu32(fp, &length))
    	    bbg_failed0("short file");
	
#if DEBUG
    	fprintf(stderr, "tag=0x%08x (%s) length=%u\n",
	    	tag, gcov_tag_as_string(tag), length);
#endif
    	switch (tag)
	{
	case GCOV_TAG_FUNCTION:
	    funcname = covio_read_string(fp);
	    funcname = normalise_mangled(funcname);
    	    fn = add_function();
	    fn->set_name(funcname);
	    covio_read_bu32(fp, &tmp);	/* ignore the checksum */
	    nblocks = 0;
	    break;

	case GCOV_TAG_BLOCKS:
	    if (fn == 0)
	    	bbg_failed0("no FUNCTION tag seen");
	    if (nblocks > 0)
	    	bbg_failed0("duplicate BLOCKS tag");
	    nblocks = length/4;
	    for (bidx = 0 ; bidx < nblocks ; bidx++)
    		fn->add_block();
	    /* skip the per-block flags */
	    for ( ; length ; length--)
		fgetc(fp);
	    break;

	case GCOV_TAG_ARCS:
	    if (!covio_read_bu32(fp, &bidx))
	    	bbg_failed0("short file");
	    for (length -= 4 ; length > 0 ; length -= 8)
	    {
		if (!covio_read_bu32(fp, &dest) ||
		    !covio_read_bu32(fp, &flags))
	    	    bbg_failed0("short file");

#if DEBUG > 1
    		fprintf(stderr, "BBG     arc %u->%u flags %x(%s,%s,%s)\n",
			    bidx, dest, flags,
			    (flags & BBG_ON_TREE ? "on_tree" : ""),
			    (flags & BBG_FAKE ? "fake" : ""),
			    (flags & BBG_FALL_THROUGH ? "fall_through" : ""));
#endif
		if (dest >= nblocks)
    	    	    bbg_failed2("dest=%u > nblocks=%u", dest, nblocks);
		    
	    	if (dest == nblocks -1 &&
		    bidx == nblocks - 2 &&
		    (flags & (BBG_FAKE|BBG_FALL_THROUGH)) == BBG_FALL_THROUGH)
		{
    		    /* HACK HACK HACK */
		    fprintf(stderr, "Hacking %d->%d to %d->%d\n",
		    	bidx, dest, nblocks-1, 0);
		    a = new cov_arc_t(fn->nth_block(nblocks-1), fn->nth_block(0));
#ifdef HAVE_BBG_FAKE_FLAG
		    a->fake_ = TRUE;
#endif
		    a->fall_through_ = FALSE;
		}
		else
		{
		    a = new cov_arc_t(fn->nth_block(bidx), fn->nth_block(dest));
#ifdef HAVE_BBG_FAKE_FLAG
		    a->fake_ = !!(flags & BBG_FAKE);
#endif
		    a->fall_through_ = !!(flags & BBG_FALL_THROUGH);
		}
		a->on_tree_ = (flags & BBG_ON_TREE);

    		if (nblocks >= 2 && dest == nblocks-1)
		{
	    	    num_expected_fake_++;
    		    if (!(flags & BBG_FAKE))
	    		num_missing_fake_++;
		}
	    }
	    break;

	case GCOV_TAG_LINES:
	    if (!covio_read_bu32(fp, &bidx))
	    	bbg_failed0("short file");
    	    while (covio_read_bu32(fp, &line))
	    {
		if (line == 0)
		{
		    char *s = covio_read_string(fp);
		    if (s == 0)
		    	bbg_failed0("short file");
		    if (*s == '\0')
		    {
			/* end of LINES block */
			g_free(s);
			break;
		    }

		    filename = s;
		    if (strchr(filename, '/') == 0 &&
	    		!strcmp(filename, file_basename_c(name_)))
			filename = name_.data();
		}
		else
		{
		    fn->nth_block(bidx)->add_location(filename, line);
		}
	    }
	    break;

	default:
	    fprintf(stderr, "%s: skipping unknown tag 0x%08x\n",
	    	    bbgfilename, tag);
	    for ( ; length ; length--)
		fgetc(fp);
	    break;
	}
    }    

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BBG_NEW_MAGIC	    	"gbbg"
#define BBG_NEW_MAGIC_LEN	4

gboolean
cov_file_t::read_bbg_file(const char *bbgfilename)
{
    FILE *fp;
    char magic[BBG_NEW_MAGIC_LEN];
    gboolean ret;
    
#if DEBUG > 1
    fprintf(stderr, "Reading .bbg file \"%s\"\n", bbgfilename);
#endif
    
    if ((fp = fopen(bbgfilename, "r")) == 0)
    {
    	perror(bbgfilename);
	return FALSE;
    }
    
    if (fread(magic, 1, BBG_NEW_MAGIC_LEN, fp) != BBG_NEW_MAGIC_LEN)
    {
    	/* TODO */
    	fprintf(stderr, "%s: short file while reading magic number\n",
	    	bbgfilename);
	return FALSE;
    }
    
    if (!memcmp(magic, BBG_NEW_MAGIC, BBG_NEW_MAGIC_LEN))
	ret = read_new_bbg_file(bbgfilename, fp);
    else
	ret = read_old_bbg_file(bbgfilename, fp);
    
    fclose(fp);
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::read_old_da_file(const char *dafilename)
{
    FILE *fp;
    gnb_u64_t nents;
    gnb_u64_t ent;
    unsigned int fnidx;
    unsigned int bidx;
    list_iterator_t<cov_arc_t> aiter;
    
#if DEBUG > 1
    fprintf(stderr, "Reading old format .da file \"%s\"\n", dafilename);
#endif
    
    if ((fp = fopen(dafilename, "r")) == 0)
    {
    	perror(dafilename);
	return FALSE;
    }

    covio_read_lu64(fp, &nents);
    
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    {
    	cov_function_t *fn = nth_function(fnidx);

	for (bidx = 0 ; bidx < fn->num_blocks() ; bidx++)
	{
    	    cov_block_t *b = fn->nth_block(bidx);
	
	    for (aiter = b->out_arc_iterator() ; aiter != (cov_arc_t *)0 ; ++aiter)
	    {
	    	cov_arc_t *a = *aiter;
		
		if (a->on_tree_)
		    continue;

    	    	/* TODO: check that nents is correct */
    		if (!covio_read_lu64(fp, &ent))
		{
		    fprintf(stderr, "%s: short file\n", dafilename);
		    return FALSE;
		}

#if DEBUG > 1
    	    	estring fromdesc = a->from()->describe();
    	    	estring todesc = a->to()->describe();
    	    	fprintf(stderr, "DA arc {from=%s to=%s} count=%llu\n",
		    	    fromdesc.data(),
		    	    todesc.data(),
			    ent);
#endif

    	    	a->set_count(ent);
	    }
	}
    }    
    
    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define DA_NEW_MAGIC	\
     	(((gnb_u32_t)'g'<<24)| \
	 ((gnb_u32_t)'c'<<16)| \
	 ((gnb_u32_t)'o'<<8)| \
	 ((gnb_u32_t)'v'))

/*
 * The da_failed*() macros are for debugging problems with .da files.
 */
#if DEBUG > 1
/* I can't wait for C99 variadic macros to become common */
#define da_failed0(fmt) \
    { \
	fprintf(stderr, "da:%d, " fmt "\n", __LINE__); \
	fclose(fp); \
    	return FALSE; \
    }
#define da_failed1(fmt, a1) \
    { \
	fprintf(stderr, "da:%d, " fmt "\n", __LINE__, a1); \
	fclose(fp); \
    	return FALSE; \
    }
#define da_failed2(fmt, a1, a2) \
    { \
	fprintf(stderr, "da:%d, " fmt "\n", __LINE__, a1, a2); \
	fclose(fp); \
    	return FALSE; \
    }
#else
#define da_failed0(fmt) \
    { \
	fclose(fp); \
    	return FALSE; \
    }
#define da_failed1(fmt, a1) \
    { \
	fclose(fp); \
    	return FALSE; \
    }
#define da_failed2(fmt, a1, a2) \
    { \
	fclose(fp); \
    	return FALSE; \
    }
#endif

gboolean
cov_file_t::read_new_da_file(const char *dafilename)
{
    FILE *fp;
    gnb_u32_t magic, version;
    gnb_u32_t tag, length;
    string_var funcname;
    cov_function_t *fn = 0;
    gnb_u64_t count;
    gnb_u32_t tmp;
    unsigned int bidx;
    list_iterator_t<cov_arc_t> aiter;


#if DEBUG > 1
    fprintf(stderr, "Reading new format .da file \"%s\"\n", dafilename);
#endif
    
    if ((fp = fopen(dafilename, "r")) == 0)
    {
    	perror(dafilename);
	return FALSE;
    }

    if (!covio_read_bu32(fp, &magic) ||
        !covio_read_bu32(fp, &version))
    	da_failed0("short file");

    if (magic != DA_NEW_MAGIC)
    	da_failed2("bad magic=0x%08x != 0x%08x",
	    	    magic, DA_NEW_MAGIC);
    
    if (version != format_version_)
    	da_failed2("bad version=0x%08x != 0x%08x",
	    	    version, format_version_);

    while (covio_read_bu32(fp, &tag))
    {
	if (!covio_read_bu32(fp, &length))
    	    da_failed0("short file");
	
#if DEBUG
    	fprintf(stderr, "tag=0x%08x (%s) length=%u\n",
	    	tag, gcov_tag_as_string(tag), length);
#endif
    	switch (tag)
	{
	case GCOV_TAG_FUNCTION:
	    funcname = covio_read_string(fp);
	    funcname = normalise_mangled(funcname);
    	    fn = find_function(funcname);
	    if (fn == 0)
	    	da_failed1("unexpected function name \"%s\"", funcname.data());
	    covio_read_bu32(fp, &tmp);	/* ignore the checksum */
	    break;

    	case GCOV_TAG_COUNTER_BASE:
	    if (fn == 0)
	    	da_failed0("missing FUNCTION or duplicate COUNTER_BASE tags");
	    for (bidx = 0 ; bidx < fn->num_blocks() ; bidx++)
	    {
    		cov_block_t *b = fn->nth_block(bidx);

		for (aiter = b->out_arc_iterator() ; aiter != (cov_arc_t *)0 ; ++aiter)
		{
	    	    cov_arc_t *a = *aiter;

		    if (a->on_tree_)
			continue;

		    if (!covio_read_bu64(fp, &count))
		    	da_failed0("short file");
#if DEBUG > 1
    	    	    string_var fromdesc = a->from()->describe();
    	    	    string_var todesc = a->to()->describe();
    	    	    fprintf(stderr, "DA arc {from=%s to=%s} count=%llu\n",
		    		fromdesc.data(),
		    		todesc.data(),
				count);
#endif
    	    	    a->set_count(count);
		}
	    }
	    fn = 0;
	    break;

	default:
	    fprintf(stderr, "%s: skipping unknown tag 0x%08x\n",
	    	    dafilename, tag);
	    /* fall through */
    	case GCOV_TAG_OBJECT_SUMMARY:
    	case GCOV_TAG_PROGRAM_SUMMARY:
	    for ( ; length ; length--)
		fgetc(fp);
	    break;
	}
    }    

    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::read_da_file(const char *dafilename)
{
#if DEBUG > 1
    fprintf(stderr, "Reading .da file \"%s\"\n", dafilename);
#endif
    
    switch (format_version_)
    {
    case BBG_VERSION_3_3p:
    	return read_new_da_file(dafilename);
    case 0:
    	return read_old_da_file(dafilename);
    default:
    	fprintf(stderr, "%s: unknown format version 0x%08x\n",
	    	dafilename, format_version_);
	return FALSE;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifdef HAVE_LIBBFD

struct cov_read_state_t
{
    bfd *abfd;
    asection *section;
    asymbol **symbols;
    long nsymbols;
};

gboolean
cov_o_file_add_call(
    cov_read_state_t *rs,
    unsigned long address,
    const char *callname)
{
    const char *filename = 0;
    const char *function = 0;
    unsigned int lineno = 0;
    cov_location_t loc;
    const GList *blocks;
    string_var callname_dem = demangle(callname);

    if (!bfd_find_nearest_line(rs->abfd, rs->section, rs->symbols, address,
		    	       &filename, &function, &lineno))
	return FALSE;
#if DEBUG > 1
    {
    	string_var function_dem = demangle(function);
	fprintf(stderr, "%s:%d: %s calls %s\n",
		file_basename_c(filename), lineno,
		function_dem.data(),
		callname_dem.data());
    }
#endif

    loc.filename = (char *)filename;
    loc.lineno = lineno;
    blocks = cov_block_t::find_by_location(&loc);
    
    if (blocks == 0)
    {
	fprintf(stderr, "No blocks for call to %s at %s:%ld\n",
		    callname_dem.data(), loc.filename, loc.lineno);
	return FALSE;
    }
    else
    {
    	const GList *iter;
    	for (iter = blocks ; iter != 0 ; iter = iter->next)
	{
	    cov_block_t *b = (cov_block_t *)iter->data;
#if DEBUG > 1
    	    estring desc = b->describe();
#endif

    	    /*
	     * Multiple blocks on the same line, the first doesn't
	     * do the call: skip until we find the one that does.
	     * Also, multiple blocks with calls in the same statement
	     * (line numbers are only recorded at the start of the
	     * statement); both the blocks and relocs are ordered,
	     * so assume it's 1:1 and skip blocks which already have
	     * a call name recorded.  This breaks if the statement
	     * mixes calls to static and extern functions.
	     */
    	    if (b->needs_call())
	    {
#if DEBUG > 1
    	        fprintf(stderr, "    block %s\n", desc.data());
#endif
		b->add_call(callname_dem);
		return TRUE;
	    }
#if DEBUG > 1
	    fprintf(stderr, "    skipping block %s\n", desc.data());
#endif

	}
	/*
	 * Something is badly wrong if we get here: at least one of
	 * the blocks on the line should have needed a call and none
	 * did.  Either the .o file is out of sync with the .bb or
	 * .bbg files, or we've encountered the braindead gcc 2.96.
	 */
	fprintf(stderr, "Could not assign block for call to %s at %s:%ld\n",
		    callname_dem.data(), loc.filename, loc.lineno);
	return FALSE;
    }
}

#ifdef __i386__

#define read_lu32(p)	\
    ( (p)[0] |      	\
     ((p)[1] <<  8) |	\
     ((p)[2] << 16) |	\
     ((p)[3] << 24))

static void
cov_o_file_scan_static_calls(
    cov_read_state_t *rs,
    unsigned long startaddr,
    unsigned long endaddr)
{
    unsigned char *buf, *p;
    unsigned long len = endaddr - startaddr;
    unsigned long callfrom, callto;
    int i;
    
    if (len < 1)
    	return;
    buf = g_new(unsigned char, len);
    
    if (!bfd_get_section_contents(rs->abfd, rs->section, buf, startaddr, len))
    {
    	g_free(buf);
    	return;
    }
    
    /*
     * TODO: presumably it is more efficient to scan through the relocs
     * looking for PCREL32 to static functions and double-check that the
     * preceding byte is the CALL instruction.
     */

    /* CALL instruction is 5 bytes long so don't bother scanning last 5 bytes */
    for (p = buf ; p < buf+len-4 ; p++)
    {
    	if (*p != 0xe8)
	    continue;	    /* not a CALL instruction */
	callfrom = startaddr + (p - buf);
	callto = callfrom + read_lu32(p+1) + 5;
	
	/*
	 * Scan symbols to see if this is a PCREL32
	 * reference to a static function entry point
	 */
	for (i = 0 ; rs->symbols[i] != 0 ; i++)
	{
	    if (rs->symbols[i]->section == rs->section &&
	    	rs->symbols[i]->value == callto &&
	    	(rs->symbols[i]->flags & (BSF_LOCAL|BSF_GLOBAL|BSF_FUNCTION)) == 
		    	    	         (BSF_LOCAL|           BSF_FUNCTION))
	    {
#if DEBUG > 1
    	    	fprintf(stderr, "Scanned static call\n");
#endif
		if (!cov_o_file_add_call(rs, callfrom, rs->symbols[i]->name))
		{
		    /* something is very wrong */
		    g_free(buf);
		    return;
		}
		p += 4;
    	    	break;
	    }
	}
    }
    
    g_free(buf);
}
#else /* !__i386 */

static void
cov_o_file_scan_static_calls(
    cov_read_state_t *rs,
    unsigned long startaddr,
    unsigned long endaddr)
{
}

#endif /* !__i386 */
#endif /* HAVE_LIBBFD */

/*
 * Use the BFD library to scan relocation records in the .o file.
 */
gboolean
cov_file_t::read_o_file_relocs(const char *ofilename)
{
#ifdef HAVE_LIBBFD
    cov_read_state_t rs;
    asymbol *sym;
    asection *sec;
    long i;
    unsigned codesectype = (SEC_ALLOC|SEC_HAS_CONTENTS|SEC_RELOC|SEC_CODE|SEC_READONLY);
    
#if DEBUG > 1
    fprintf(stderr, "Reading .o file \"%s\"\n", ofilename);
#endif
    
    if ((rs.abfd = bfd_openr(ofilename, 0)) == 0)
    {
    	/* TODO */
    	bfd_perror(ofilename);
	return FALSE;
    }
    if (!bfd_check_format(rs.abfd, bfd_object))
    {
    	/* TODO */
    	bfd_perror(ofilename);
	bfd_close(rs.abfd);
	return FALSE;
    }

#if DEBUG > 1
    fprintf(stderr, "%s: reading symbols...\n", ofilename);
#endif
    rs.nsymbols = bfd_get_symtab_upper_bound(rs.abfd);
    rs.symbols = g_new(asymbol*, rs.nsymbols);
    rs.nsymbols = bfd_canonicalize_symtab(rs.abfd, rs.symbols);
    if (rs.nsymbols < 0)
    {
	bfd_perror(ofilename);
	bfd_close(rs.abfd);
	g_free(rs.symbols);
	return FALSE;
    }

#if DEBUG > 5
    for (i = 0 ; i < rs.nsymbols ; i++)
    {
    	sym = rs.symbols[i];
	string_var dem = demangle(sym->name);
	
	if (!strcmp(dem, sym->name))
	    fprintf(stderr, "%s\n", sym->name);
	else
	    fprintf(stderr, "%s (%s)\n", sym->name, dem.data());
    }
#endif

    for (sec = rs.abfd->sections ; sec != 0 ; sec = sec->next)
    {
	unsigned long lastaddr = 0UL;
	arelent **relocs, *rel;
	long nrelocs;

#if DEBUG > 1
	fprintf(stderr, "%s[%d %s]: ", ofilename, sec->index, sec->name);
#endif

	if ((sec->flags & codesectype) != codesectype)
	{
#if DEBUG > 1
	    fprintf(stderr, "skipping\n");
#endif
    	    continue;
	}

#if DEBUG > 1
	fprintf(stderr, "reading relocs...\n");
#endif

    	nrelocs = bfd_get_reloc_upper_bound(rs.abfd, sec);
	relocs = g_new(arelent*, nrelocs);
    	nrelocs = bfd_canonicalize_reloc(rs.abfd, sec, relocs, rs.symbols);
	if (nrelocs < 0)
	{
	    bfd_perror(ofilename);
	    g_free(relocs);
	    continue;
	}
    
    	for (i = 0 ; i < nrelocs ; i++)
	{
	    
	    rel = relocs[i];
	    sym = *rel->sym_ptr_ptr;
	    
#if DEBUG > 1
    	    {
		char *type;
		string_var name_dem = demangle(sym->name);

		if ((sym->flags & BSF_FUNCTION))
	    	    type = "FUN";
		else if ((sym->flags & BSF_OBJECT))
	    	    type = "DAT";
		else if (sym->flags & BSF_SECTION_SYM)
	    	    type = "SEC";
		else if ((sym->flags & (BSF_LOCAL|BSF_GLOBAL)) == 0)
	    	    type = "UNK";
		else
	    	    type = "---";
		fprintf(stderr, "%5ld %08lx %08lx %08lx %d(%s) %s %s\n",
	    		i, rel->address, rel->addend,
			(unsigned long)sym->flags,
			rel->howto->type,
			rel->howto->name,
			type, name_dem.data());
    	    }
#endif

#ifdef __i386__
    	    /*
	     * Experiment shows that functions calls result in an R_386_PC32
	     * reloc and external data references in an R_386_32 reloc.
	     * Haven't yet seen any others -- so give a warning if we do.
	     */
	    if (rel->howto->type == R_386_32)
	    	continue;
	    else if (rel->howto->type != R_386_PC32)
	    {
	    	fprintf(stderr, "%s: Warning unexpected 386 reloc howto type %d\n",
		    	    	    ofilename, rel->howto->type);
	    	continue;
	    }
#endif

    	    /* __bb_init_func is code inserted by gcc to instrument blocks */
    	    if (!strcmp(sym->name, "__bb_init_func"))
	    	continue;
    	    if ((sym->flags & BSF_FUNCTION) ||
		(sym->flags & (BSF_LOCAL|BSF_GLOBAL|BSF_SECTION_SYM|BSF_OBJECT)) == 0)
	    {

    	    	/*
		 * Scan the instructions between the previous reloc and
		 * this instruction for calls to static functions.  Very
		 * platform specific!
		 */
		rs.section = sec;
		cov_o_file_scan_static_calls(&rs, lastaddr, rel->address);
		lastaddr = rel->address + bfd_get_reloc_size(rel->howto);
		
		if (!cov_o_file_add_call(&rs, rel->address, sym->name))
		{
		    /* something is very wrong */
		    g_free(relocs);
		    g_free(rs.symbols);
		    bfd_close(rs.abfd);
		    return FALSE;
		}
	    }

	}
    	g_free(relocs);

    	if (lastaddr < sec->_raw_size)
	{
	    rs.section = sec;
	    cov_o_file_scan_static_calls(&rs, lastaddr, sec->_raw_size);
	}
    }

    g_free(rs.symbols);
    
    bfd_close(rs.abfd);
#endif /* HAVE_LIBBFD */
    return TRUE;
}


gboolean
cov_file_t::read_o_file(const char *ofilename)
{
#ifdef HAVE_LIBBFD
    unsigned int fnidx;

    if (!read_o_file_relocs(ofilename))
	return TRUE;	    /* this info is optional */
    
    /*
     * Calls can fail to be reconciled for perfectly harmless
     * reasons (e.g. the code uses function pointers) so don't
     * fail if reconciliation fails.
     * TODO: record the failure in the cov_function_t.
     */
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    	nth_function(fnidx)->reconcile_calls();
    
#endif /* HAVE_LIBBFD */
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::search_path_append(const char *dir)
{
    search_path_.append(g_strdup(dir));
}

char *
cov_file_t::try_file(const char *dir, const char *ext) const
{
    string_var ofilename, dfilename;
    
    if (dir == 0)
    	ofilename = name();
    else
    	ofilename = g_strconcat(dir, "/", file_basename_c(name()), (char *)0);
    
    dfilename = file_change_extension(ofilename, 0, ext);

    return (file_is_regular(dfilename) < 0 ? 0 : dfilename.take());
}

char *
cov_file_t::find_file(const char *ext, gboolean quiet) const
{
    list_iterator_t<char> iter;
    char *file;
    
    /*
     * First try the same directory as the source file.
     */
    if ((file = try_file(0, ext)) != 0)
    	return file;
	
    /*
     * Now look in the search path.
     */
    for (iter = search_path_.first() ; iter != (char *)0 ; ++iter)
    {
	if ((file = try_file(*iter, ext)) != 0)
    	    return file;
    }
    
    if (!quiet)
    {
    	string_var dir = file_dirname(name());

    	fprintf(stderr, "Couldn't find %s file for %s in path:\n",
	    	    ext, file_basename_c(name()));
	fprintf(stderr, "   %s\n", dir.data());
	for (iter = search_path_.first() ; iter != (char *)0 ; ++iter)
	    fprintf(stderr, "   %s\n", *iter);
    }
    
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::read(gboolean quiet)
{
    string_var filename;

    if ((filename = find_file(".bbg", quiet)) == 0 ||
	 !read_bbg_file(filename))
	return FALSE;

    /* 
     * In the new formats, the information from the .bb file has been
     * merged into the .bbg file, so only read the .bb for the old format.
     */
    if (format_version_ == 0)
    {
	if ((filename = find_file(".bb", quiet)) == 0 ||
	    !read_bb_file(filename))
	    return FALSE;
    }

    /* TODO: read multiple .da files from the search path & accumulate */
    if ((filename = find_file(".da", quiet)) == 0 ||
	 !read_da_file(filename))
	return FALSE;

    /*
     * If the data files were written by broken versions of gcc 2.96
     * the callgraph will be irretrievably broken and there's no point
     * at all trying to read the object file.
     */
    if (gcc296_braindeath())
    {
	static const char warnmsg[] = 
	"data files written by a broken gcc 2.96; the Calls "
	"statistics and the contents of the Calls, Call Butterfly and "
	"Call Graph windows will be incomplete.  Skipping object "
	"file.\n"
	;
    	/* TODO: save and report in alert to user */
    	fprintf(stderr, "%s: WARNING: %s", name(), warnmsg);
    }
    else
    {
	/*
	 * The data we get from object files is optional, a user can
	 * still get lots of value from ggcov with the remainder of the
	 * files.  So if we can't find the object or can't read it,
	 * complain and keep going.
	 */
    	if ((filename = find_file(".o", quiet)) == 0 ||
	    !read_o_file(filename))
	{
	    static const char warnmsg[] = 
	    "could not find or read matching object file; the contents "
	    "of the Calls, Call Butterfly and Call Graph windows may "
	    "be inaccurate or incomplete.\n"
	    ;
    	    /* TODO: save and report in alert to user */
    	    fprintf(stderr, "%s: WARNING: %s", name(), warnmsg);
	}
    }

    if (!solve())
    	return FALSE;
	
    finalise();
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
