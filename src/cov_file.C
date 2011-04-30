/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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
#include "covio.H"
#include "estring.H"
#include "string_var.H"
#include "tok.H"
#include "hashtable.H"
#include "filename.h"
#include "demangle.h"
#include "cpp_parser.H"
#include "cpp_parser.H"
#include "cov_suppression.H"

CVSID("$Id: cov_file.C,v 1.83 2010-05-09 05:37:15 gnb Exp $");

static gboolean filename_is_common(const char *filename);

hashtable_t<const char, cov_file_t> *cov_file_t::files_;
list_t<cov_file_t> cov_file_t::files_list_;
list_t<char> cov_file_t::search_path_;
char *cov_file_t::common_path_;
int cov_file_t::common_len_;
void *cov_file_t::files_model_;

#define _NEW_VERSION(major, minor, release) \
     	(((gnb_u32_t)('0'+(major))<<24)| \
	 ((gnb_u32_t)('0'+(minor)/10)<<16)| \
	 ((gnb_u32_t)('0'+(minor)%10)<<8)| \
	 ((gnb_u32_t)(release)))
#define BBG_VERSION_GCC45  	_NEW_VERSION(4,5,'*')
#define BBG_VERSION_GCC44  	_NEW_VERSION(4,4,'*')
#define BBG_VERSION_GCC43   	_NEW_VERSION(4,3,'*')
#define BBG_VERSION_GCC41_UBU	_NEW_VERSION(4,1,'p')	/* Ubuntu Edgy */
#define BBG_VERSION_GCC41   	_NEW_VERSION(4,1,'*')
#define BBG_VERSION_GCC40_UBU  	_NEW_VERSION(4,0,'U')	/* Ubuntu Dapper Drake */
#define BBG_VERSION_GCC40_RH   	_NEW_VERSION(4,0,'R')
#define BBG_VERSION_GCC40_APL  	_NEW_VERSION(4,0,'A')	/* Apple MacOS X*/
#define BBG_VERSION_GCC40    	_NEW_VERSION(4,0,'*')
#define BBG_VERSION_GCC34    	_NEW_VERSION(3,4,'*')
/*
 * RedHat's FC3 compiler appears to be based on a CVS sample partway
 * between gcc 3.4.0 and gcc 4.0.  It has funcids in the .gcno file
 * but the .gcda file doesn't use a 0 tag as a terminator.
 */
#define BBG_VERSION_GCC34_UBU	_NEW_VERSION(3,4,'U')
#define BBG_VERSION_GCC34_RH	_NEW_VERSION(3,4,'R')
#define BBG_VERSION_GCC34_MDK  	_NEW_VERSION(3,4,'M')	/* Mandrake crud */
#define BBG_VERSION_GCC33   	_NEW_VERSION(3,3,'p')
#define BBG_VERSION_GCC33_SUSE	_NEW_VERSION(3,3,'S')   /* SUSE crud */
#define BBG_VERSION_GCC33_MDK	_NEW_VERSION(3,3,'M')   /* Mandrake crud */
#define BBG_VERSION_OLD     	0
#define BBG_VERSION_OLDPLUS     1

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_file_t::cov_file_t(const char *name, const char *relpath)
 :  name_(name),
    relpath_(relpath)
{
    /*
     * It is the caller's responsibility to create cov_file_t objects
     * with an absolute filename which is not already known.
     */
    assert(name[0] == '/');
    assert(find(name_) == 0);

    functions_ = new ptrarray_t<cov_function_t>();
    functions_by_name_ = new hashtable_t<const char, cov_function_t>;
    functions_by_id_ = new hashtable_t<gnb_u64_t, cov_function_t>;
    lines_ = new ptrarray_t<cov_line_t>();
    null_line_ = new cov_line_t();

    files_->insert(name_, this);
    if ((common_ = filename_is_common(name_)))
	add_name(name_);
}

cov_file_t::~cov_file_t()
{
    unsigned int i;

    files_list_.remove(this);
    files_->remove(name_);

    for (i = 0 ; i < functions_->length() ; i++)
    	delete functions_->nth(i);
    delete functions_;

    delete functions_by_name_;
    delete functions_by_id_;

    for (i = 0 ; i < lines_->length() ; i++)
    	delete lines_->nth(i);
    delete lines_;
    delete null_line_;

    if (common_)
	dirty_common_path();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::init()
{
    files_ = new hashtable_t<const char, cov_file_t>;
    common_path_ = 0;
    common_len_ = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
filename_matches_directory_prefix(const char *filename, const char *dir)
{
    int dirlen = strlen(dir);

    return (!strncmp(dir, filename, dirlen) &&
	    (filename[dirlen] == '\0' || filename[dirlen] == '/'));
}

static gboolean
filename_is_common(const char *filename)
{
    static const char * const uncommon_dirs[] = 
    {
	"/usr/include",
	"/usr/lib",
	0
    };
    const char * const * dirp;

    for (dirp = uncommon_dirs ; *dirp != 0 ; dirp++)
    {
    	if (filename_matches_directory_prefix(filename, *dirp))
	    return FALSE;
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0
gboolean
cov_file_t::is_self_suppressed() const
{
    /* TOOO: implement suppression by filename, directory, or library here */
}

void
cov_file_t::suppress()
{
    unsigned int i;

    suppressed_ = TRUE;
    for (i = 0 ; i < functions_->length() ; i++)
    	functions_->nth(i)->suppress();
}
#endif

void
cov_file_t::finalise()
{
    unsigned int i;

    if (finalised_)
	return;
    finalised_ = TRUE;

#if 0
    /* TODO: push file-level suppression downwards to functions */
    if (is_self_suppressed())
    	suppress();
#endif

    for (i = 0 ; i < functions_->length() ; i++)
    	functions_->nth(i)->finalise();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
compare_files(const cov_file_t *fa, const cov_file_t *fb)
{
    return strcmp(fa->minimal_name(), fb->minimal_name());
}

void
cov_file_t::post_read_1(
    const char *name,
    cov_file_t *f,
    gpointer userdata)
{
    files_list_.prepend(f);
    f->finalise();
}

void
cov_file_t::post_read()
{
    files_list_.remove_all();
    files_->foreach(post_read_1, 0);
    files_list_.sort(compare_files);
}

list_iterator_t<cov_file_t>
cov_file_t::first()
{
    return files_list_.first();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

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
    if (common_len_ < 0)
	return;	/* dirty, need to recalculate later */
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
    if (f->common_)
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
    if (!common_)
    	return name_;
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
	return g_strconcat(common_path_, name, (char *)0);
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

cov_line_t *
cov_file_t::get_nth_line(unsigned int lineno)
{
    cov_line_t *ln;
    
    if (lineno > lines_->length() ||
    	(ln = lines_->nth(lineno-1)) == 0)
	lines_->set(lineno-1, ln = new cov_line_t());
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
    	f = new cov_file_t(filename, filename);
	assert(f != 0);
    }
    assert(f->name_[0] == '/');    
    assert(lineno > 0);
    
    ln = f->get_nth_line(lineno);
    
    /*
     * This incredibly obscure corner case keeps tripping the regression
     * tests.  The 2nd last block in a function normally represents the
     * function epilogue; in some functions which have an empty epilogue
     * the gcc 2.96 compiler spits out a line entry for the epilogue block
     * which is a duplicate of the real last line.  This is mostly harmless
     * except in functions where the last line is not always executed, e.g.
     * foo()
     * {
     *    ...
     *    if (...)
     *       yadda();
     * }
     * Here the last line spuriously contains 2 blocks, one of which counts
     * the number of times yadda() was called and the other counts the number
     * of times it wasn't; the result reported is the maximum of the two.
     * This tweak suppresses the line entry for the epilogue block if
     * another block already has the line.
     */
    if (ln->blocks_ != 0 && b->is_epilogue())
    {
    	if (debug_enabled(D_BB))
	{
    	    string_var desc = b->describe();
    	    duprintf3("Block %s skipping duplicate epilogue line %s:%lu\n",
		      desc.data(), filename, lineno);
	}
    	return;
    }

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

cov::status_t
cov_file_t::calc_stats(cov_stats_t *stats) const
{
    unsigned int fnidx;
    cov_stats_t mine;
    
    assert(finalised_);
    
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
	nth_function(fnidx)->calc_stats(&mine);
    
    stats->accumulate(&mine);
    return mine.status_by_blocks();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::zero_arc_counts()
{
    unsigned int fnidx;

    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
	nth_function(fnidx)->zero_arc_counts();
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
	    fprintf(stderr, "ERROR: could not solve flow graph for %s\n",
	    	    nth_function(fnidx)->name());
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
cov_file_t::read_bb_file(covio_t *io)
{
    gnb_u32_t tag;
    estring funcname;
    estring filename;
    cov_function_t *fn = 0;
    int funcidx;
    int bidx = 0;
    int line;
    int nlines;
    
    dprintf1(D_FILES, "Reading .bb file \"%s\"\n", io->filename());

    io->set_format(covio_t::FORMAT_OLD);


    funcidx = 0;
    line = 0;
    nlines = 0;
    while (io->read_u32(tag))
    {
    	switch (tag)
	{
	case BB_FILENAME:
	    if (!io->read_bbstring(filename, tag))
	    	return FALSE;
	    filename = make_absolute(filename);
	    dprintf1(D_BB, "BB filename = \"%s\"\n", filename.data());
	    break;
	    
	case BB_FUNCTION:
	    if (!io->read_bbstring(funcname, tag))
	    	return FALSE;
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

/*
 * Skip some pointless crap seen in Fedora Core 1:  a per-function
 * header with the function name, which duplicates the info in
 * the .bb file which is also present.  This rubbish is seen in
 * both the .bbg and .da files.  Returns TRUE if header correct,
 * FALSE if broken and EOF on EOF.
 */

int
cov_file_t::skip_oldplus_func_header(covio_t *io, const char *prefix)
{
    gnb_u32_t crud;
    estring funcname;

    if (!io->read_u32(crud))
    	return EOF;
    if (crud != BBG_SEPARATOR)
	bbg_failed1("expecting separator, got %u", crud);

    if (!io->read_string(funcname))
	bbg_failed0("short file");
    dprintf2(D_BBG, "%sskipping function name: \"%s\"\n", prefix, funcname.data());

    if (!io->read_u32(crud))
	bbg_failed0("short file");
    if (crud != BBG_SEPARATOR)
	bbg_failed1("expecting separator, got %u", crud);

    if (!io->read_u32(crud))
	bbg_failed0("short file");
    dprintf2(D_BBG, "%sskipping function flags(?): 0x%08x\n", prefix, crud);

    return TRUE;
}


gboolean
cov_file_t::read_old_bbg_function(covio_t *io)
{
    gnb_u32_t nblocks, totnarcs, narcs;
    gnb_u32_t bidx, aidx;
    gnb_u32_t dest, flags;
    gnb_u32_t sep;
    cov_arc_t *a;
    cov_function_t *fn;
    
    dprintf0(D_BBG, "BBG reading function\n");
    
    if (format_version_ == BBG_VERSION_OLDPLUS)
    {
    	switch (skip_oldplus_func_header(io, "BBG   "))
	{
	case FALSE: return FALSE;
	case EOF: return TRUE;
	}
    }
    
    if (!io->read_u32(nblocks))
    {
    	if (format_version_ == BBG_VERSION_OLDPLUS)
    	    bbg_failed0("short file");
    	return TRUE;	/* end of file */
    }

    if (!io->read_u32(totnarcs))
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
	if (!io->read_u32(narcs))
    	    bbg_failed0("short file");

	if (narcs > BBG_MAX_ARCS)
    	    bbg_failed2("narcs=%u > %u", narcs, BBG_MAX_ARCS);

	for (aidx = 0 ; aidx < narcs ; aidx++)
	{
	    io->read_u32(dest);
	    if (!io->read_u32(flags))
	    	bbg_failed0("short file");

    	    dprintf7(D_BBG, "BBG     arc %u: %u->%u flags %x(%s,%s,%s)\n",
	    	    	    aidx,
			    bidx, dest, flags,
			    (flags & BBG_ON_TREE ? "on_tree" : ""),
			    (flags & BBG_FAKE ? "fake" : ""),
			    (flags & BBG_FALL_THROUGH ? "fall_through" : ""));
	    if (dest >= nblocks)
    	    	bbg_failed2("dest=%u > nblocks=%u", dest, nblocks);
			    
	    a = new cov_arc_t();
	    a->on_tree_ = !!(flags & BBG_ON_TREE);
	    a->fall_through_ = !!(flags & BBG_FALL_THROUGH);
    	    a->call_ = (nblocks >= 2 && dest == nblocks-1 && !a->fall_through_);
	    a->attach(fn->nth_block(bidx), fn->nth_block(dest));
	    /*
	     * We used to be able to detect function calls with the fake_ flag
	     * from the .bbg file, but that flag went away in braindead versions
	     * of gcc 2.96 so we have to fall back to a more subtle test.  The
	     * test relies on the last block in a function being an unreal block
	     * whose purpose is to be the target of all call arcs in the function.
	     * This works with most of the data generated by gcc 2.96, but sometimes
	     * it screws that up too.  Note that the commandline "gcov" utility
	     * that comes with gcc 2.96 cannot get any call stats at all.
	     */
    	    if (a->call_)
	    {
	    	num_expected_fake_++;
    		if (!(flags & BBG_FAKE))
		{
	    	    num_missing_fake_++;
    	    	    dprintf0(D_BBG, "BBG     missing fake flag\n");
		}
	    }
	}
    }

    io->read_u32(sep);
    if (sep != BBG_SEPARATOR)
    	bbg_failed2("sep=0x%08x != 0x%08x", sep, BBG_SEPARATOR);
	
    return TRUE;
}

gboolean
cov_file_t::read_old_bbg_file_common(covio_t *io)
{
    io->set_format(covio_t::FORMAT_OLD);
    io->seek(0L);

    while (!io->eof())
    {
    	if (!read_old_bbg_function(io))
	{
	    /* TODO */
	    fprintf(stderr, "%s: file is corrupted or in a bad file format.\n",
	    	    io->filename());
	    return FALSE;
	}
    }

    return TRUE;
}

gboolean
cov_file_t::read_old_bbg_file(covio_t *io)
{
    format_version_ = BBG_VERSION_OLD;
    return read_old_bbg_file_common(io);
}

gboolean
cov_file_t::read_oldplus_bbg_file(covio_t *io)
{
    format_version_ = BBG_VERSION_OLDPLUS;
    return read_old_bbg_file_common(io);
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

/*
 * If `relpath' matches the tail part of `abspath',
 * return the number of characters from `abspath'
 * which *preceed* the match.  Otherwise, return -1.
 */
static int
path_is_suffix(const char *abspath, const char *relpath)
{
    unsigned int rellen = strlen(relpath);
    unsigned int abslen = strlen(abspath);
    if (rellen < abslen &&
	abspath[abslen-rellen-1] == '/' &&
	!strcmp(abspath+abslen-rellen, relpath))
	return abslen-rellen-1;
    return -1;
}

void
cov_file_t::infer_compilation_directory(const char *path)
{
    int clen;

    dprintf1(D_BBG, "infer_compilation_directory(\"%s\")\n", path);

    if (path[0] == '/' &&
        (clen = path_is_suffix(path, relpath_)) > 0)
    {
	/*
	 * `path' is an absolute path whose tail is
	 * identical to `relpath_', so we can infer that
	 * the compiledir is the remainder of `path'.
	 */
	compiledir_ = g_strndup(path, clen);
	dprintf1(D_BBG, "compiledir_=\"%s\"\n", compiledir_.data());
	return;
    }

    if (path[0] != '/' &&
        (clen = path_is_suffix(name_, path)) > 0)
    {
	/*
	 * `path' is a relative path whose last element is
	 * identical to the last element of `relpath_', so
	 * we infer (possibly ambiguously) that path_ is the
	 * relative path seen by the compiler at compiletime
	 * and thus the compiledir is the absolute path `name_'
	 * with `path' removed from its tail.
	 */
	compiledir_ = g_strndup(name_, clen);
	dprintf1(D_BBG, "compiledir_=\"%s\"\n", compiledir_.data());
	return;
    }

    /* shit, now we're for it */
    fprintf(stderr, "Warning: could not calculate compiledir for %s from location %s!!!\n",
	    name_.data(), path);
}

char *
cov_file_t::make_absolute(const char *filename) const
{
    if (compiledir_ != (const char *)0)
	return file_make_absolute_to_dir(filename, compiledir_);
    return file_make_absolute_to_file(filename, name_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
decode_new_version(gnb_u32_t ver, unsigned int *major,
		   unsigned int *minor, unsigned char *rel)
{
    unsigned char b;

    b = (ver>>24) & 0xff;
    if (!isdigit(b))
	return FALSE;
    *major = b - '0';

    b = (ver>>16) & 0xff;
    if (!isdigit(b))
	return FALSE;
    *minor = b - '0';

    b = (ver>>8) & 0xff;
    if (!isdigit(b))
	return FALSE;
    *minor = (*minor * 10) + (b - '0');

    b = (ver) & 0xff;
    if (!isalnum(b) && b != '*')
	return FALSE;
    *rel = b;

    return TRUE;
}

gboolean
cov_file_t::read_gcc3_bbg_file(covio_t *io,
			       gnb_u32_t expect_version,
			       covio_t::format_t ioformat)
{
    gnb_u32_t tag, length;
    cov_function_t *fn = 0;
    estring filename, funcname;
    gnb_u32_t tmp;
    unsigned int nblocks = 0;
    gnb_u32_t bidx, last_bidx = 0;
    unsigned int nlines = 0;
    cov_arc_t *a;
    gnb_u32_t dest, flags;
    gnb_u32_t line, last_line = 0;
    unsigned int len_unit = 1;
    gnb_u64_t funcid = 0;

    io->set_format(ioformat);

    if (!io->read_u32(format_version_))
    	bbg_failed0("short file");
    switch (format_version_)
    {
    case BBG_VERSION_GCC33_SUSE:
    case BBG_VERSION_GCC33_MDK:
    	if (expect_version == BBG_VERSION_GCC33)
	    expect_version = format_version_;
	/* fall through */
    case BBG_VERSION_GCC33:
    	break;
    case BBG_VERSION_GCC40:
    case BBG_VERSION_GCC40_RH:
    case BBG_VERSION_GCC40_UBU:
    case BBG_VERSION_GCC40_APL:
    case BBG_VERSION_GCC41:
    case BBG_VERSION_GCC41_UBU:
    case BBG_VERSION_GCC43:
    case BBG_VERSION_GCC44:
    case BBG_VERSION_GCC45:
	features_ |= FF_DA0TAG;
	/* fall through */
    case BBG_VERSION_GCC34_UBU:
    case BBG_VERSION_GCC34_RH:
    case BBG_VERSION_GCC34_MDK:
	features_ |= FF_FUNCIDS;
	/* fall through */
    case BBG_VERSION_GCC34:
	features_ |= FF_TIMESTAMP;
	if (expect_version == BBG_VERSION_GCC34)
	    expect_version = format_version_;
    	break;
    default:
	unsigned int major, minor;
	unsigned char rel;
	if (!decode_new_version(format_version_, &major, &minor, &rel))
	    fprintf(stderr, "%s: undecodable compiler version %08x, "
			    "probably corrupted file\n",
			    io->filename(), format_version_);
	else
	    fprintf(stderr, "%s: unsupported compiler version %u.%u(%c), "
			    "contact author for support\n",
			    io->filename(), major, minor, rel);
	return FALSE;
    }
    if (format_version_ != expect_version)
    	bbg_failed1("unexpected version=0x%08x", format_version_);

    if ((features_ & FF_TIMESTAMP))
    {
	io->read_u32(tmp);	/* ignore the timestamp */
    	/* TODO: should really do something useful with this */
	len_unit = 4;	/* records lengths are in 4-byte units now */
    }

    while (io->read_u32(tag))
    {
	if (!io->read_u32(length))
    	    bbg_failed0("short file");
	length *= len_unit;
	
    	dprintf3(D_BBG, "tag=0x%08x (%s) length=%u\n",
	    	tag, gcov_tag_as_string(tag), length);
    	switch (tag)
	{
	case GCOV_TAG_FUNCTION:
	    if ((features_ & FF_FUNCIDS))
	    {
		estring filename;

		if (!io->read_u64(funcid) ||
		    !io->read_string(funcname) ||
		    !io->read_string(filename) ||
		    !io->read_u32(tmp)/* this seems to be a line number */)
    		    bbg_failed0("short file");
		if (compiledir_ == (const char *)0)
		    infer_compilation_directory(filename);
	    }
	    else
	    {
		if (!io->read_string(funcname) ||
		    !io->read_u32(tmp)/* ignore the checksum */)
    		    bbg_failed0("short file");
	    }
	    funcname = demangle(funcname);
	    funcname = normalise_mangled(funcname);
    	    fn = add_function();
	    fn->set_name(funcname);
	    if (funcid != 0)
	    	fn->set_id(funcid);
    	    dprintf1(D_BBG, "added function \"%s\"\n", funcname.data());
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
	    io->skip(length);
	    break;

	case GCOV_TAG_ARCS:
	    if (!io->read_u32(bidx))
	    	bbg_failed0("short file");
	    for (length -= 4 ; length > 0 ; length -= 8)
	    {
		if (!io->read_u32(dest) ||
		    !io->read_u32(flags))
	    	    bbg_failed0("short file");

    		dprintf6(D_BBG, "BBG     arc %u->%u flags %x(%s,%s,%s)\n",
			    bidx, dest, flags,
			    (flags & BBG_ON_TREE ? "on_tree" : ""),
			    (flags & BBG_FAKE ? "fake" : ""),
			    (flags & BBG_FALL_THROUGH ? "fall_through" : ""));
		if (dest >= nblocks)
    	    	    bbg_failed2("dest=%u > nblocks=%u", dest, nblocks);

		a = new cov_arc_t();
		a->fall_through_ = !!(flags & BBG_FALL_THROUGH);
		/*
		 * The FAKE flag is used both for calls and exception handling,
		 * so we can't rely on it exclusively to determine calls.
		 *
		 * Note that the FAKE flag appears to be reliably present again
		 * from at least gcc 3.4.  However FALL_THROUGH becomes unreliable
		 * at gcc 4.1.1, which will emit an arc (N-2 -> N-1, !FALL_THROUGH)
		 * for a "return" statement which the last line in a function.
		 */
    		a->call_ = (dest == nblocks-1 && (flags & BBG_FAKE));
		a->on_tree_ = !!(flags & BBG_ON_TREE);
		a->attach(fn->nth_block(bidx), fn->nth_block(dest));
	    }
	    break;

	case GCOV_TAG_LINES:
	    if (!io->read_u32(bidx))
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
    	    while (io->read_u32(line))
	    {
		if (line == 0)
		{
		    estring s;
		    if (!io->read_string(s))
		    	bbg_failed0("short file");
		    if (s.length() == 0)
		    {
			/* end of LINES block */
			break;
		    }

		    if (compiledir_ == (const char *)0)
			infer_compilation_directory(s);

    	    	    filename = make_absolute(s);
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
	    	    io->filename(), tag);
	    io->skip(length);
	    break;
	}
    }    

    return TRUE;
}

gboolean
cov_file_t::read_gcc33_bbg_file(covio_t *io)
{
    return read_gcc3_bbg_file(io, BBG_VERSION_GCC33, covio_t::FORMAT_GCC33);
}

gboolean
cov_file_t::read_gcc34l_bbg_file(covio_t *io)
{
    return read_gcc3_bbg_file(io, BBG_VERSION_GCC34, covio_t::FORMAT_GCC34L);
}

gboolean
cov_file_t::read_gcc34b_bbg_file(covio_t *io)
{
    return read_gcc3_bbg_file(io, BBG_VERSION_GCC34, covio_t::FORMAT_GCC34B);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::read_bbg_file(covio_t *io)
{
    dprintf1(D_FILES, "Reading .bbg file \"%s\"\n", io->filename());

    return (this->*(format_->read_bbg_))(io);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const cov_file_t::format_rec_t cov_file_t::formats[] =
{
    {
    	"gcno", 4,
	&cov_file_t::read_gcc34b_bbg_file,
	&cov_file_t::read_gcc34b_da_file,
	"gcc 3.4 or 4.0 .gcno (big-endian) format"
    },
    {
    	"oncg", 4,
	&cov_file_t::read_gcc34l_bbg_file,
	&cov_file_t::read_gcc34l_da_file,
	"gcc 3.4 or 4.0 .gcno (little-endian) format"
    },
    {
    	"gbbg", 4,
	&cov_file_t::read_gcc33_bbg_file,
	&cov_file_t::read_gcc33_da_file,
	"gcc 3.3 .bbg format"
    },
    {
    	"\x01\x00\x00\x80", 4,
	&cov_file_t::read_oldplus_bbg_file,
	&cov_file_t::read_oldplus_da_file,
	"old .bbg format plus function names (e.g. Fedora Core 1)"
    },
    {
    	0, 0,
	&cov_file_t::read_old_bbg_file,
	&cov_file_t::read_old_da_file,
	"old .bbg format"
    },
};

#define MAX_MAGIC_LEN	    	    4


gboolean
cov_file_t::discover_format(covio_t *io)
{
    char magic[MAX_MAGIC_LEN];
    const format_rec_t *fmt;

    dprintf1(D_FILES, "Detecting format of .bbg file \"%s\"\n", io->filename());

    if (io->read(magic, MAX_MAGIC_LEN) != MAX_MAGIC_LEN)
    {
    	/* TODO */
    	fprintf(stderr, "%s: short file while reading magic number\n",
	    	io->filename());
	return FALSE;
    }

    for (fmt = formats ; fmt->magic_ != 0 ; fmt++)
    {
    	if (!memcmp(magic, fmt->magic_, fmt->magic_len_))
	    break;
    }
    dprintf1(D_FILES, "Detected %s\n", fmt->description_);
    format_ = fmt;
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::read_old_da_file(covio_t *io)
{
    gnb_u64_t nents;
    gnb_u64_t ent;
    unsigned int fnidx;
    unsigned int bidx;
    list_iterator_t<cov_arc_t> aiter;
    
    io->set_format(covio_t::FORMAT_OLD);
    io->read_u64(nents);
    
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
    		if (!io->read_u64(ent))
		{
		    fprintf(stderr, "%s: short file\n", io->filename());
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
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * The da_failed*() macros are for debugging problems with .da files.
 */
#define da_failed0(fmt) \
    { \
	dprintf1(D_DA, "da:%d, " fmt "\n", __LINE__); \
    	return FALSE; \
    }
#define da_failed1(fmt, a1) \
    { \
	dprintf2(D_DA, "da:%d, " fmt "\n", __LINE__, a1); \
    	return FALSE; \
    }
#define da_failed2(fmt, a1, a2) \
    { \
	dprintf3(D_DA, "da:%d, " fmt "\n", __LINE__, a1, a2); \
    	return FALSE; \
    }

#define DA_OLDPLUS_MAGIC    0x8000007b

gboolean
cov_file_t::read_oldplus_da_file(covio_t *io)
{
    gnb_u32_t crud;
    gnb_u32_t file_narcs;
    gnb_u64_t ent;
    unsigned int fnidx;
    unsigned int bidx;
    unsigned int actual_narcs;
    list_iterator_t<cov_arc_t> aiter;
    
    io->set_format(covio_t::FORMAT_OLD);
    
    /*
     * I haven't yet looked in the FC1 gcc source to figure out what
     * it's writing in the .da header...this is reverse engineered.
     */
    if (!io->read_u32(crud))
    	da_failed0("short file");
    if (crud != DA_OLDPLUS_MAGIC)
    	da_failed2("bad magic, expecting 0x%08x got 0x%08x\n",
	    	   DA_OLDPLUS_MAGIC, crud);
    
    if (!io->read_u32(crud))
    	da_failed0("short file");
    if (crud != num_functions())
    	da_failed2("bad num functions, expecting %d got %d\n",
	    	   num_functions(), crud);

    if (!io->read_u32(crud) || !io->skip(crud))
    	da_failed0("short file");

    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    {
    	cov_function_t *fn = nth_function(fnidx);
	
    	if (!skip_oldplus_func_header(io, "DA "))
	    return FALSE;

	if (!io->read_u32(file_narcs))
    	    da_failed0("short file");
	actual_narcs = 0;

	for (bidx = 0 ; bidx < fn->num_blocks() ; bidx++)
	{
    	    cov_block_t *b = fn->nth_block(bidx);
	
	    for (aiter = b->out_arc_iterator() ; aiter != (cov_arc_t *)0 ; ++aiter)
	    {
	    	cov_arc_t *a = *aiter;
		
		if (a->on_tree_)
		    continue;

    	    	if (++actual_narcs > file_narcs)
		    da_failed2("bad num arcs, expecting %d got >= %d\n",
		    	    	file_narcs, actual_narcs);

    		if (!io->read_u64(ent))
		    da_failed0("short file");

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
	
	if (actual_narcs != file_narcs)
	    da_failed2("bad num arcs, expecting %d got %d\n",
		    	file_narcs, actual_narcs);
    }    
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define _DA_MAGIC(a,b,c,d) \
    	(((gnb_u32_t)a<<24)| \
    	 ((gnb_u32_t)b<<16)| \
	 ((gnb_u32_t)c<<8)| \
	 ((gnb_u32_t)d))

#define DA_GCC34_MAGIC	    _DA_MAGIC('g','c','d','a')	/* also 4.0 */
#define DA_GCC33_MAGIC	    _DA_MAGIC('g','c','o','v')

gboolean
cov_file_t::read_gcc3_da_file(covio_t *io,
			      gnb_u32_t expect_magic,
			      covio_t::format_t ioformat)
{
    gnb_u32_t magic, version;
    gnb_u32_t tag, length;
    cov_function_t *fn = 0;
    gnb_u64_t count;
    gnb_u32_t tmp;
    unsigned int bidx;
    list_iterator_t<cov_arc_t> aiter;
    unsigned int len_unit = 1;

    io->set_format(ioformat);

    if (!io->read_u32(magic) ||
        !io->read_u32(version))
    	da_failed0("short file");

    if (magic != expect_magic)
    	da_failed2("bad magic=0x%08x != 0x%08x",
	    	    magic, expect_magic);
    
    if (version != format_version_)
    	da_failed2("bad version=0x%08x != 0x%08x",
	    	    version, format_version_);

    if ((features_ & FF_TIMESTAMP))
    {
    	if (!io->read_u32(tmp))    	/* ignore timestamp */
    	    da_failed0("short file");
	len_unit = 4;	/* record lengths are in 4-byte units */
    }

    while (io->read_u32(tag))
    {
	if (tag == 0 && ((features_ & FF_DA0TAG)))
	    break;  /* end of file */

	if (!io->read_u32(length))
    	    da_failed0("short file");
	length *= len_unit;
	
    	dprintf3(D_DA, "tag=0x%08x (%s) length=%u\n",
	    	tag, gcov_tag_as_string(tag), length);
    	switch (tag)
	{
	case GCOV_TAG_FUNCTION:
	    if ((features_ & FF_FUNCIDS))
	    {
		gnb_u64_t funcid;
		if (!io->read_u64(funcid))
		    fn = 0;
		else if ((fn = functions_by_id_->lookup(&funcid)) == 0)
	    	    da_failed1("unexpected function id %llu", (unsigned long long)funcid);
	    }
	    else
	    {
		estring funcname;
		if (!io->read_string(funcname))
		    da_failed0("short file");
		if (format_version_ == BBG_VERSION_GCC33_SUSE)
		    funcname = demangle(funcname);
		funcname = normalise_mangled(funcname);
    		fn = find_function(funcname);
		if (fn == 0)
	    	    da_failed1("unexpected function name \"%s\"", funcname.data());
		io->read_u32(tmp);	/* ignore the checksum */
	    }
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

		    if (!io->read_u64(count))
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
	    	    io->filename(), tag);
	    /* fall through */
    	case GCOV_TAG_OBJECT_SUMMARY:
    	case GCOV_TAG_PROGRAM_SUMMARY:
	    io->skip(length);
	    break;
	}
    }    

    return TRUE;
}

gboolean
cov_file_t::read_gcc33_da_file(covio_t *io)
{
    return read_gcc3_da_file(io, DA_GCC33_MAGIC, covio_t::FORMAT_GCC33);
}

gboolean
cov_file_t::read_gcc34b_da_file(covio_t *io)
{
    return read_gcc3_da_file(io, DA_GCC34_MAGIC, covio_t::FORMAT_GCC34B);
}

gboolean
cov_file_t::read_gcc34l_da_file(covio_t *io)
{
    return read_gcc3_da_file(io, DA_GCC34_MAGIC, covio_t::FORMAT_GCC34L);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::read_da_file(covio_t *io)
{
    dprintf1(D_FILES, "Reading runtime data file \"%s\"\n", io->filename());

    return (this->*(format_->read_da_))(io);
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
    cov_block_t *pure_candidate = 0;

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
	    b->add_call(callname_dem, loc);
	    return TRUE;
	}
	if (debug_enabled(D_CGRAPH))
	{
    	    string_var desc = b->describe();
	    duprintf1("    skipping block %s\n", desc.data());
	}
	if (pure_candidate == 0)
	    pure_candidate = b;
    }
    
    /*
     * Maybe it's a pure call from one of the blocks on the line
     * that we skipped.  There's no way to know for sure from the
     * information in the object file, so guess that the first
     * block we skipped is the one.  If you want block-accurate
     * callgraph data, use the 12bp file!
     */
    if (pure_candidate != 0)
    {
    	pure_candidate->add_call(callname_dem, loc);
	return TRUE;
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
cov_file_t::scan_o_file_calls(covio_t *io)
{
    cov_bfd_t *cbfd;
    cov_call_scanner_t *cs;
    gboolean ret = FALSE;
    
    dprintf1(D_FILES, "Reading .o file \"%s\"\n", io->filename());
    
    if ((cbfd = new(cov_bfd_t)) == 0 ||
    	!cbfd->open(io->filename(), io->take()) ||
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
	{
	    cov_location_t loc = cdata.location;
    	    loc.filename = make_absolute(loc.filename);
	    o_file_add_call(&loc, cdata.callname);
	    g_free(loc.filename);
	}
	delete cs;
	ret = (r == 0); /* 0=>successfully finished scan */
    }
    delete cbfd;
    return ret;
}


gboolean
cov_file_t::read_o_file(covio_t *io)
{
    unsigned int fnidx;

    if (!scan_o_file_calls(io))
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

class cov_file_src_parser_t : public cpp_parser_t
{
private:
    cov_file_t *file_;
    unsigned int is_suppressed_;    /* mask of active suppression types */
    hashtable_t<const char, cov_suppression_t> *active_ranges_;

    static void
    check_one_ifdef(const char *key, cov_suppression_t *s, void *closure)
    {
	cov_file_src_parser_t *self = (cov_file_src_parser_t *)closure;

	if (self->depends(key))
	    self->is_suppressed_ |= (1<<cov_suppression_t::IFDEF);
    }

    void
    depends_changed()
    {
    	if (debug_enabled(D_CPP|D_VERBOSE))
	    dump();

	is_suppressed_ &= ~(1<<cov_suppression_t::IFDEF);
	cov_suppression_t::foreach(cov_suppression_t::IFDEF,
	    	    	    	   check_one_ifdef, this);
    	dprintf1(D_CPP, "depends_changed suppressed=%u\n", is_suppressed_);
    }

    void
    post_line()
    {
	cov_line_t *ln;
	
	if (is_suppressed_ &&
	    lineno() <= file_->lines_->length() &&
	    (ln = file_->lines_->nth(lineno()-1)) != 0)
	{
    	    dprintf1(D_CPP|D_VERBOSE, "post_line: suppressing %u\n", is_suppressed_);
	    ln->suppress();
	}
    	/* line suppression is one-shot */
	is_suppressed_ &= ~(1<<cov_suppression_t::COMMENT_LINE);
    }

    void
    handle_comment(const char *text)
    {
	cov_suppression_t *s;

    	dprintf1(D_CPP, "handle_comment: \"%s\"\n", text);

	s = cov_suppression_t::find(text, cov_suppression_t::COMMENT_LINE);
	if (s != 0)
	{
	    /* comment suppresses this line only */
	    is_suppressed_ |= (1<<cov_suppression_t::COMMENT_LINE);
    	    dprintf0(D_CPP, "handle_comment: suppressing line\n");
	}

	s = cov_suppression_t::find(text, cov_suppression_t::COMMENT_RANGE);
	if (s != 0)
	{
	    /* comment starts a new active range */
    	    dprintf2(D_CPP, "handle_comment: starting range %s-%s\n",
	    	    	    s->word_.data(), s->word2_.data());
	    if (active_ranges_ == 0)
	    {
		active_ranges_ = new hashtable_t<const char, cov_suppression_t>;
		active_ranges_->insert(s->word2_, s);
	    }
	    else if (active_ranges_->lookup(s->word2_) == 0)
	    {
		active_ranges_->insert(s->word2_, s);
	    }
	    is_suppressed_ |= (1<<cov_suppression_t::COMMENT_RANGE);
	}

	if (active_ranges_ != 0 && (s = active_ranges_->lookup(text)) != 0)
	{
	    /* comment ends an active range */
	    active_ranges_->remove(text);
    	    dprintf2(D_CPP, "handle_comment: ending range %s-%s\n",
	    	    	    s->word_.data(), s->word2_.data());
	    if (active_ranges_->size() == 0)
	    {
		/*
		 * Removed last active range; stop suppressing
		 * by range, but suppress this line so the range
		 * is inclusive of the closing magical comment line
		 */
		is_suppressed_ &= ~(1<<cov_suppression_t::COMMENT_RANGE);
		is_suppressed_ |= (1<<cov_suppression_t::COMMENT_LINE);
    		dprintf0(D_CPP, "handle_comment: last range\n");
	    }
	}
    }


public:
    cov_file_src_parser_t(cov_file_t *f)
     :  cpp_parser_t(f->name()),
     	file_(f),
	is_suppressed_(0),
	active_ranges_(0)
    {
    }
    ~cov_file_src_parser_t()
    {
	delete active_ranges_;
    }
};

gboolean
cov_file_t::read_src_file()
{
    cov_file_src_parser_t parser(this);
    if (!parser.parse())
    	return FALSE;
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::search_path_append(const char *dir)
{
    search_path_.append(g_strdup(dir));
}

covio_t *
cov_file_t::try_file(const char *dir, const char *ext) const
{
    string_var ofilename, dfilename;
    
    if (dir == 0)
    	ofilename = name();
    else
    	ofilename = g_strconcat(dir, "/", file_basename_c(name()), (char *)0);

    if (ext[0] == '+')
    	dfilename = g_strconcat(ofilename, ext+1, (char *)0);
    else
	dfilename = file_change_extension(ofilename, 0, ext);
    
    dprintf1(D_FILES|D_VERBOSE, "    try %s\n", dfilename.data());
    
    if (file_is_regular(dfilename) < 0)
    	return 0;
	
    covio_t *io = new covio_t(dfilename);
    if (!io->open_read())
    {
	int e = errno;
    	perror(dfilename);
    	delete io;
	errno = e;
    	return 0;
    }

    return io;
}

covio_t *
cov_file_t::find_file(const char *ext, gboolean quiet) const
{
    list_iterator_t<char> iter;
    covio_t *io;
    
    dprintf2(D_FILES|D_VERBOSE,
    	    "Searching for %s file matching %s\n",
    	    ext, file_basename_c(name()));

    /*
     * First try the same directory as the source file.
     */
    if ((io = try_file(0, ext)) != 0)
    	return io;
	
    /*
     * Now look in the search path.
     */
    for (iter = search_path_.first() ; iter != (char *)0 ; ++iter)
    {
	if ((io = try_file(*iter, ext)) != 0)
    	    return io;
    }
    
    if (!quiet)
    {
	int e = errno;
    	file_missing(ext, 0);
	errno = e;
    }
    
    return 0;
}

void
cov_file_t::file_missing(const char *ext, const char *ext2) const
{
    list_iterator_t<char> iter;
    string_var dir = file_dirname(name());
    string_var which = (ext2 == 0 ? g_strdup("") :
    	    	    	    g_strdup_printf(" or %s", ext2));

    fprintf(stderr, "Couldn't find %s%s file for %s in path:\n",
	    	ext, which.data(), file_basename_c(name()));
    fprintf(stderr, "   %s\n", dir.data());
    for (iter = search_path_.first() ; iter != (char *)0 ; ++iter)
	fprintf(stderr, "   %s\n", *iter);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::read(gboolean quiet)
{
    string_var filename;
    const char *da_ext = ".da";
    covio_var io;

    if ((io = find_file(".bbg", TRUE)) == 0)
    {
    	/* The .bbg file was gratuitously renamed .gcno in gcc 3.4 */
    	if ((io = find_file(".gcno", TRUE)) == 0)
	{
	    if (!quiet)
	    	file_missing(".bbg", ".gcno");
	    return FALSE;
	}
	/* The .da file was renamed too */
	da_ext = ".gcda";
    }

    if (!discover_format(io))
	return FALSE;
    
    if (!read_bbg_file(io))
	return FALSE;

    /* 
     * In the new formats, the information from the .bb file has been
     * merged into the .bbg file, so only read the .bb for the old format.
     */
    if (format_version_ <= BBG_VERSION_OLDPLUS)
    {
	if ((io = find_file(".bb", quiet)) == 0 ||
	    !read_bb_file(io))
	    return FALSE;
    }

    /* TODO: read multiple .da files from the search path & accumulate */
    if ((io = find_file(da_ext, quiet)) == 0)
    {
	if (errno != ENOENT)
	    return FALSE;
	zero_arc_counts();
    }
    else if (!read_da_file(io))
	return FALSE;

    if (cov_suppression_t::count() != 0 && !read_src_file())
    {
	static const char warnmsg[] = 
	"could not scan source file for cpp conditionals or comments, "
	"reports may be inaccurate.\n";

    	/* TODO: save and report in alert to user */
    	fprintf(stderr, "%s: WARNING: %s", name(), warnmsg);
    }

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
    	if ((io = find_file(".o", quiet)) == 0 ||
	    !read_o_file(io))
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

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
