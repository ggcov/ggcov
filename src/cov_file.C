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
#include "cov_specific.H"
#include "covio.h"
#include "estring.H"
#include "string_var.H"
#include "filename.h"
#include "demangle.h"

CVSID("$Id: cov_file.C,v 1.27 2003-11-04 00:40:25 gnb Exp $");


hashtable_t<const char*, cov_file_t> *cov_file_t::files_;
list_t<cov_file_t> cov_file_t::files_list_;
list_t<char> cov_file_t::search_path_;
char *cov_file_t::common_path_;
int cov_file_t::common_len_;
void *cov_file_t::files_model_;

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

    functions_ = new ptrarray_t<cov_function_t>();
    functions_by_name_ = new hashtable_t<const char*, cov_function_t>;
    lines_ = new ptrarray_t<cov_line_t>();
    null_line_ = new cov_line_t();

    files_->insert(name_, this);
}

cov_file_t::~cov_file_t()
{
    unsigned int i;

    if (finalised_)
    {
        files_list_.remove(this);
    }
    files_->remove(name_);

    for (i = 0 ; i < functions_->length() ; i++)
    	delete functions_->nth(i);
    delete functions_;

    delete functions_by_name_;

    for (i = 0 ; i < lines_->length() ; i++)
    	delete lines_->nth(i);
    delete lines_;
    delete null_line_;

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
    files_list_.remove_all();
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
    dprintf2(D_FILES, "cov_file_t::add_name: name=\"%s\" => common=\"%s\"\n",
    	    	name, common_path_);
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
    	dprintf0(D_FILES, "cov_file_t::check_common_path: recalculating common path\n");
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

void *
cov_file_t::files_model()
{
    /* currently MVC models just need to be a unique address */
    if (files_model_ == 0)
    	files_model_ = (void *)&files_model_;
    return files_model_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_line_t *
cov_file_t::nth_line(unsigned int n) const
{
    cov_line_t *ln;
    
    if (n > lines_->length() ||
    	(ln = lines_->nth(n-1)) == 0)
	ln = null_line_;
    return ln;
}

void
cov_file_t::add_location(
    cov_block_t *b,
    const char *filename,
    unsigned long lineno)
{
    cov_file_t *f;
    cov_line_t *ln;
    
    if (!strcmp(filename, name_))
    {
    	/*
	 * The common case is that we add locations in the file
	 * we're currently read()ing, which has not yet been
	 * inserted so that find() can find it.
	 */
    	f = this;
    }
    else if ((f = find(filename)) == 0)
    {
    	f = new cov_file_t(filename);
	assert(f != 0);
	/*
	 * Need to finalise the new file immediately
	 * otherwise it's not available to find().
	 */
	f->finalise();
    }
    assert(f->name_[0] == '/');    
    assert(lineno > 0);
    
    if (lineno > f->lines_->length() ||
    	(ln = f->lines_->nth(lineno-1)) == 0)
	f->lines_->set(lineno-1, ln = new cov_line_t());
    
    if (debug_enabled(D_BB))
    {
	string_var desc = b->describe();
	duprintf3("Block %s adding location %s:%lu\n",
    		  desc.data(), filename, lineno);
	if (ln->blocks_ != 0)
    	    duprintf3("%s:%lu: this line belongs to %d blocks\n",
	    	      filename, lineno, g_list_length(ln->blocks_)+1);
    }

    ln->blocks_ = g_list_append(ln->blocks_, b);
    b->add_location(f->name_, lineno);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t *
cov_file_t::add_function()
{
    cov_function_t *fn;
    
    fn = new cov_function_t();
    
    fn->idx_ = functions_->append(fn);
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
    
    dprintf1(D_FILES, "Reading .bb file \"%s\"\n", bbfilename);

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
	    filename = file_make_absolute_to(filename, name_);
	    dprintf1(D_BB, "BB filename = \"%s\"\n", filename.data());
	    break;
	    
	case BB_FUNCTION:
	    funcname = covio_read_bbstring(fp, tag);
	    funcname = normalise_mangled(funcname);
	    dprintf1(D_BB, "BB function = \"%s\"\n", funcname.data());
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
		add_location(fn->nth_block(bidx), filename, line);
	    }
	    bidx++;
	    nlines = 0;
	    break;
	    
	default:
	    dprintf2(D_BB, "BB line = %d (block %d)\n", (int)tag, bidx);
	    assert(fn != 0);

    	    line = tag;
	    add_location(fn->nth_block(bidx), filename, line);
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
#define bbg_failed0(fmt) \
    { \
	dprintf1(D_BBG, "BBG:%d, " fmt "\n", __LINE__); \
    	return FALSE; \
    }
#define bbg_failed1(fmt, a1) \
    { \
	dprintf2(D_BBG, "BBG:%d, " fmt "\n", __LINE__, a1); \
    	return FALSE; \
    }
#define bbg_failed2(fmt, a1, a2) \
    { \
	dprintf3(D_BBG, "BBG:%d, " fmt "\n", __LINE__, a1, a2); \
    	return FALSE; \
    }


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
    
    dprintf0(D_BBG, "BBG reading function\n");
    
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
    	dprintf1(D_BBG, "BBG   block %d\n", bidx);
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

    	    dprintf7(D_BBG, "BBG     arc %u: %u->%u flags %x(%s,%s,%s)\n",
	    	    	    aidx,
			    bidx, dest, flags,
			    (flags & BBG_ON_TREE ? "on_tree" : ""),
			    (flags & BBG_FAKE ? "fake" : ""),
			    (flags & BBG_FALL_THROUGH ? "fall_through" : ""));
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
		{
	    	    num_missing_fake_++;
    	    	    dprintf0(D_BBG, "BBG     missing fake flag\n");
		}
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
    dprintf0(D_FILES, "Detected old .bbg format\n");
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
    unsigned int bidx, last_bidx = 0;
    unsigned int nlines = 0;
    cov_arc_t *a;
    gnb_u32_t dest, flags;
    gnb_u32_t line, last_line = 0;

    dprintf0(D_FILES, "Detected new .bbg format\n");

    if (!covio_read_bu32(fp, &format_version_))
    	bbg_failed0("short file");
    if (format_version_ != BBG_VERSION_3_3p)
    	bbg_failed2("bad version=0x%08x != 0x%08x",
	    	    format_version_, BBG_VERSION_3_3p);

    while (covio_read_bu32(fp, &tag))
    {
	if (!covio_read_bu32(fp, &length))
    	    bbg_failed0("short file");
	
    	dprintf3(D_BBG, "tag=0x%08x (%s) length=%u\n",
	    	tag, gcov_tag_as_string(tag), length);
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

    		dprintf6(D_BBG, "BBG     arc %u->%u flags %x(%s,%s,%s)\n",
			    bidx, dest, flags,
			    (flags & BBG_ON_TREE ? "on_tree" : ""),
			    (flags & BBG_FAKE ? "fake" : ""),
			    (flags & BBG_FALL_THROUGH ? "fall_through" : ""));
		if (dest >= nblocks)
    	    	    bbg_failed2("dest=%u > nblocks=%u", dest, nblocks);
		    
		a = new cov_arc_t(fn->nth_block(bidx), fn->nth_block(dest));
#ifdef HAVE_BBG_FAKE_FLAG
		a->fake_ = !!(flags & BBG_FAKE);
#endif
		a->fall_through_ = !!(flags & BBG_FALL_THROUGH);
		a->on_tree_ = (flags & BBG_ON_TREE);

    		if (nblocks >= 2 &&
		    dest == nblocks-1 &&
		    !(bidx == dest-1 && (flags & BBG_FALL_THROUGH)))
		{
	    	    num_expected_fake_++;
    		    if (!(flags & BBG_FAKE))
		    {
	    		num_missing_fake_++;
    	    		dprintf0(D_BBG, "BBG     missing fake flag\n");
		    }
		}
	    }
	    break;

	case GCOV_TAG_LINES:
	    if (!covio_read_bu32(fp, &bidx))
	    	bbg_failed0("short file");
	    if (bidx >= nblocks)
    	    	bbg_failed2("bidx=%u > nblocks=%u", bidx, nblocks);
	    if (bidx > 0 && bidx < nblocks-1)
	    {
	    	/* may need to interpolate some block->line assignments */
		for (last_bidx++ ; last_bidx < bidx ; last_bidx++)
		{
    		    dprintf0(D_BBG, "BBG     interpolating line:\n");
    	    	    add_location(fn->nth_block(last_bidx), filename, last_line);
		}
	    }
	    nlines = 0;
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

    	    	    filename = file_make_absolute_to(s, name_);
		    g_free(s);
		}
		else
		{
		    add_location(fn->nth_block(bidx), filename, line);
		    nlines++;
		    last_line = line;
		}
	    }
	    last_bidx = bidx;
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
    
    dprintf1(D_FILES, "Reading .bbg file \"%s\"\n", bbgfilename);
    
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
    
    dprintf0(D_FILES, "Detected old .da format\n");
    
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

    	    	if (debug_enabled(D_DA))
		{
    	    	    string_var fromdesc = a->from()->describe();
    	    	    string_var todesc = a->to()->describe();
    	    	    duprintf3("DA arc {from=%s to=%s} count=%llu\n",
		    	      fromdesc.data(),
		    	      todesc.data(),
			      ent);
    	    	}

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
#define da_failed0(fmt) \
    { \
	dprintf1(D_DA, "da:%d, " fmt "\n", __LINE__); \
	fclose(fp); \
    	return FALSE; \
    }
#define da_failed1(fmt, a1) \
    { \
	dprintf2(D_DA, "da:%d, " fmt "\n", __LINE__, a1); \
	fclose(fp); \
    	return FALSE; \
    }
#define da_failed2(fmt, a1, a2) \
    { \
	dprintf3(D_DA, "da:%d, " fmt "\n", __LINE__, a1, a2); \
	fclose(fp); \
    	return FALSE; \
    }

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


    dprintf0(D_FILES, "Detected new .da format\n");
    
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
	
    	dprintf3(D_DA, "tag=0x%08x (%s) length=%u\n",
	    	tag, gcov_tag_as_string(tag), length);
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
    	    	    if (debug_enabled(D_DA))
		    {
    	    		string_var fromdesc = a->from()->describe();
    	    		string_var todesc = a->to()->describe();
    	    		duprintf3("DA arc {from=%s to=%s} count=%llu\n",
		    		  fromdesc.data(),
		    		  todesc.data(),
				  count);
    	    	    }
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
    dprintf1(D_FILES, "Reading .da file \"%s\"\n", dafilename);
    
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

gboolean
cov_file_t::o_file_add_call(
    const cov_location_t *loc,
    const char *callname_dem)
{
    cov_line_t *ln;
    const GList *iter;

    if ((ln = cov_line_t::find(loc)) == 0)
    {
	fprintf(stderr, "No blocks for call to %s at %s:%ld\n",
		    callname_dem, loc->filename, loc->lineno);
	return FALSE;
    }

    for (iter = ln->blocks() ; iter != 0 ; iter = iter->next)
    {
	cov_block_t *b = (cov_block_t *)iter->data;

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
	    if (debug_enabled(D_CGRAPH))
	    {
    		string_var desc = b->describe();
    	        duprintf1("    block %s\n", desc.data());
	    }
	    b->add_call(callname_dem);
	    return TRUE;
	}
	if (debug_enabled(D_CGRAPH))
	{
    	    string_var desc = b->describe();
	    duprintf1("    skipping block %s\n", desc.data());
	}
    }
    /*
     * Something is badly wrong if we get here: at least one of
     * the blocks on the line should have needed a call and none
     * did.  Either the .o file is out of sync with the .bb or
     * .bbg files, or we've encountered the braindead gcc 2.96.
     */
    fprintf(stderr, "Could not assign block for call to %s at %s:%ld\n",
		callname_dem, loc->filename, loc->lineno);
    return FALSE;
}

/*
 * Use the BFD library to scan relocation records in the .o file.
 */
gboolean
cov_file_t::scan_o_file_calls(const char *ofilename)
{
    cov_bfd_t *cbfd;
    cov_call_scanner_t *cs;
    gboolean ret = FALSE;
    
    dprintf1(D_FILES, "Reading .o file \"%s\"\n", ofilename);
    
    if ((cbfd = new(cov_bfd_t)) == 0 ||
    	!cbfd->open(ofilename) ||
	cbfd->num_symbols() == 0)
    {
    	delete cbfd;
	return FALSE;
    }

    cov_factory_t<cov_call_scanner_t> factory;
    do
    {
    	dprintf1(D_FILES, "Trying scanner %s\n", factory.name());
    	if ((cs = factory.create()) != 0 && cs->attach(cbfd))
	    break;
	delete cs;
	cs = 0;
    }
    while (factory.next());

    if (cs != 0)
    {
    	int r;
	cov_call_scanner_t::calldata_t cdata;

    	while ((r = cs->next(&cdata)) == 1)
	    o_file_add_call(&cdata.location, cdata.callname);
	delete cs;
	ret = (r == 0); /* 0=>successfully finished scan */
    }
    delete cbfd;
    return ret;
}


gboolean
cov_file_t::read_o_file(const char *ofilename)
{
    unsigned int fnidx;

    if (!scan_o_file_calls(ofilename))
	return TRUE;	    /* this info is optional */
    
    /*
     * Calls can fail to be reconciled for perfectly harmless
     * reasons (e.g. the code uses function pointers) so don't
     * fail if reconciliation fails.
     * TODO: record the failure in the cov_function_t.
     */
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    	nth_function(fnidx)->reconcile_calls();
    
    return TRUE;
}

#endif /* HAVE_LIBBFD */

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
    
    dprintf1(D_FILES|D_VERBOSE, "    try %s\n", dfilename.data());

    return (file_is_regular(dfilename) < 0 ? 0 : dfilename.take());
}

char *
cov_file_t::find_file(const char *ext, gboolean quiet) const
{
    list_iterator_t<char> iter;
    char *file;
    
    dprintf2(D_FILES|D_VERBOSE,
    	    "Searching for %s file matching %s\n",
    	    ext, file_basename_c(name()));

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
#ifdef HAVE_LIBBFD
    if (gcc296_braindeath())
    {
	static const char warnmsg[] = 
	"data files written by a broken gcc 2.96; the Calls "
	"statistics and the contents of the Calls, Call Butterfly and "
	"Call Graph windows will be incomplete.  Skipping object "
	"file.\n"
	;
    	/* TODO: save and report in alert to user */
	/* TODO: update the message to point out that line stats are fine */
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
#endif

    if (!solve())
    	return FALSE;
	
    finalise();
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
