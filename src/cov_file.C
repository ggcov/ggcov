/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2015 Greg Banks <gnb@users.sourceforge.net>
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

#include "cov_priv.H"
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
#include "logging.H"

hashtable_t<const char, cov_file_t> *cov_file_t::files_;
list_t<cov_file_t> cov_file_t::files_list_;
list_t<char> cov_file_t::search_path_;
string_var cov_file_t::gcda_prefix_;
char *cov_file_t::common_path_;
int cov_file_t::common_len_;
void *cov_file_t::files_model_;
static logging::logger_t &files_log = logging::find_logger("files");
static logging::logger_t &bb_log = logging::find_logger("bb");
static logging::logger_t &bbg_log = logging::find_logger("bbg");
static logging::logger_t &da_log = logging::find_logger("da");
static logging::logger_t &cpp_log = logging::find_logger("cpp");
static logging::logger_t &cgraph_log = logging::find_logger("cgraph");

#define _NEW_VERSION(major, minor, release) \
	(((uint32_t)('0'+(major))<<24)| \
	 ((uint32_t)('0'+(minor)/10)<<16)| \
	 ((uint32_t)('0'+(minor)%10)<<8)| \
	 ((uint32_t)(release)))
/* yet another version numbering scheme, this one dating from
 * svn+ssh://gcc.gnu.org/svn/gcc/trunk@238702 committed 20160725
 * first released in gcc 7.1 */
#define _NEWER_VERSION(major, minor, release) \
	(((uint32_t)('A'+((major)/10))<<24)| \
	 ((uint32_t)('0'+((major)%10))<<16)| \
	 ((uint32_t)('0'+(minor))<<8)| \
	 ((uint32_t)(release)))
#define BBG_VERSION_GCC34       _NEW_VERSION(3,4,'*')
#define BBG_VERSION_GCC33       _NEW_VERSION(3,3,'p')

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
    functions_by_id_ = new hashtable_t<uint64_t, cov_function_t>;
    lines_ = new ptrarray_t<cov_line_t>();
    null_line_ = new cov_line_t();

    files_->insert(name_, this);

    suppress(cov_suppressions.find(name_, cov_suppression_t::FILENAME));
    if (!suppression_)
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

    if (!suppression_)
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

void
cov_file_t::suppress(const cov_suppression_t *s)
{
    if (s && !suppression_)
    {
	cgraph_log.debug("suppressing file: %s\n", s->describe());
	suppression_ = s;

	/* in most cases we'll be suppressed before any lines or
	 * functions are added, but this is here for completeness */
	for (ptrarray_iterator_t<cov_function_t> fnitr = functions_->first() ; *fnitr ; ++fnitr)
	    (*fnitr)->suppress(s);
	for (ptrarray_iterator_t<cov_line_t> liter = lines_->first() ; *liter ; ++liter)
	    (*liter)->suppress(s);
    }
}

void
cov_file_t::finalise()
{
    if (finalised_)
	return;
    finalised_ = TRUE;

    for (ptrarray_iterator_t<cov_line_t> liter = lines_->first() ; *liter ; ++liter)
	(*liter)->finalise();
    for (ptrarray_iterator_t<cov_function_t> fnitr = functions_->first() ; *fnitr ; ++fnitr)
	(*fnitr)->finalise();

    if (!suppression_)
    {
	/* suppress the file if all it's
	 * functions are suppressed */
	cov_suppression_combiner_t c(cov_suppressions);
	for (ptrarray_iterator_t<cov_function_t> fnitr = functions_->first() ; *fnitr ; ++fnitr)
	    c.add((*fnitr)->suppression_);
	suppress(c.result());
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
compare_files(const cov_file_t *fa, const cov_file_t *fb)
{
    return strcmp(fa->minimal_name(), fb->minimal_name());
}

void
cov_file_t::post_read()
{
    files_list_.remove_all();

    for (hashtable_iter_t<const char, cov_file_t> itr = files_->first() ; *itr ; ++itr)
    {
	cov_file_t *f = *itr;
	files_list_.prepend(f);
	f->finalise();
    }
    files_list_.sort(compare_files);
}

list_iterator_t<cov_file_t>
cov_file_t::first()
{
    return files_list_.first();
}

unsigned int
cov_file_t::length()
{
    return files_->size();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_file_t *
cov_file_t::find(const char *name)
{
    assert(files_ != 0);
    return files_->lookup(unminimise_name(name));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::add_name(const char *name)
{
    assert(name[0] == '/');
    if (common_len_ < 0)
	return; /* dirty, need to recalculate later */
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
    files_log.debug("cov_file_t::add_name: name=\"%s\" => common=\"%s\"\n",
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
cov_file_t::check_common_path()
{
    if (common_len_ < 0)
    {
	files_log.debug("cov_file_t::check_common_path: recalculating common path\n");
	common_len_ = 0;
	for (hashtable_iter_t<const char, cov_file_t> itr = files_->first() ; *itr ; ++itr)
	    if (!(*itr)->suppression_)
		add_name(itr.key());
    }
}

const char *
cov_file_t::minimal_name() const
{
    if (suppression_)
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

const char *
cov_file_t::unminimise_name(const char *name)
{
    if (name[0] == '/')
    {
	/* absolute name */
	return name;
    }
    else
    {
	static estring full;
	/* partial, presumably minimal, name */
	check_common_path();
	full.truncate();
	full.append_string(common_path_);
	full.append_string(name);
	return full.data();
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
    {
	ln = new cov_line_t();
	ln->suppress(suppression_);
	lines_->set(lineno-1, ln);
    }
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
    if (ln->has_blocks() && b->is_epilogue())
    {
	bb_log.debug("Block %s skipping duplicate epilogue line %s:%lu\n",
		  b->describe(), filename, lineno);
	return;
    }

    if (bb_log.is_enabled(logging::DEBUG))
    {
	bb_log.debug("Block %s adding location %s:%lu\n",
		     b->describe(), filename, lineno);
	if (ln->has_blocks())
	    bb_log.debug("%s:%lu: this line belongs to %d blocks\n",
		         filename, lineno, ln->num_blocks()+1);
    }

    ln->add_block(b);
    b->add_location(f->name_, lineno);
    f->has_locations_ = TRUE;
}

cov_line_t *
cov_file_t::find_line(const cov_location_t *loc)
{
    cov_file_t *f;

    return ((f = cov_file_t::find(loc->filename)) == 0 ||
	    loc->lineno < 1 ||
	    loc->lineno > f->num_lines()
	    ? 0
	    : f->nth_line(loc->lineno));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t *
cov_file_t::add_function()
{
    cov_function_t *fn;

    fn = new cov_function_t();

    fn->idx_ = functions_->append(fn);
    fn->file_ = this;
    fn->suppress(suppression_);

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
    cov_stats_t mine;
    cov::status_t st;

    assert(finalised_);

    if (suppression_)
	st = cov::SUPPRESSED;
    else
    {
	for (ptrarray_iterator_t<cov_function_t> fnitr = functions_->first() ; *fnitr ; ++fnitr)
	    (*fnitr)->calc_stats(&mine);
	stats->accumulate(&mine);
	st = mine.status_by_blocks();
    }
    return st;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::zero_arc_counts()
{
    for (ptrarray_iterator_t<cov_function_t> fnitr = functions_->first() ; *fnitr ; ++fnitr)
	(*fnitr)->zero_arc_counts();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::solve()
{
    for (ptrarray_iterator_t<cov_function_t> fnitr = functions_->first() ; *fnitr ; ++fnitr)
    {
	cov_function_t *fn = *fnitr;
	if (!fn->solve())
	{
	    files_log.error("could not solve flow graph for %s\n", fn->name());
	    return FALSE;
	}
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BB_FILENAME     0x80000001
#define BB_FUNCTION     0x80000002
#define BB_ENDOFLIST    0x00000000

gboolean
cov_file_t::read_bb_file(covio_t *io)
{
    uint32_t tag;
    estring funcname;
    estring filename;
    cov_function_t *fn = 0;
    int funcidx;
    int bidx = 0;
    int line;
    int nlines;

    files_log.debug("Reading .bb file \"%s\"\n", io->filename());

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
	    bb_log.debug("BB filename = \"%s\"\n", filename.data());
	    break;

	case BB_FUNCTION:
	    if (!io->read_bbstring(funcname, tag))
		return FALSE;
	    funcname = normalise_mangled(funcname);
	    bb_log.debug("BB function = \"%s\"\n", funcname.data());
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
	    bb_log.debug("BB line = %d (block %d)\n", (int)tag, bidx);
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

#define BBG_SEPARATOR   0x80000001

/* arc flags */
#define BBG_ON_TREE             0x1
#define BBG_FAKE                0x2
#define BBG_FALL_THROUGH        0x4

/*
 * Put an arbitrary upper limit on complexity of functions to
 * prevent bogus data files (or the gcc 3.2 format which we don't
 * yet parse) from causing us to eat all swap in a tight loop.
 */
#define BBG_MAX_BLOCKS  (64 * 1024)
#define BBG_MAX_ARCS    (64 * 1024)

/*
 * The bbg_failed*() macros are for debugging problems with .bbg files.
 */
#define bbg_failed0(fmt) \
    { \
	bbg_log.debug("BBG:%d, failed: " fmt "\n", __LINE__); \
	return FALSE; \
    }
#define bbg_failed1(fmt, a1) \
    { \
	bbg_log.debug("BBG:%d, failed: " fmt "\n", __LINE__, a1); \
	return FALSE; \
    }
#define bbg_failed2(fmt, a1, a2) \
    { \
	bbg_log.debug("BBG:%d, failed: " fmt "\n", __LINE__, a1, a2); \
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
    uint32_t crud;
    estring funcname;

    if (!io->read_u32(crud))
	return EOF;
    if (crud != BBG_SEPARATOR)
	bbg_failed1("expecting separator, got %u", crud);

    if (!io->read_string(funcname))
	bbg_failed0("short file");
    bbg_log.debug("%sskipping function name: \"%s\"\n", prefix, funcname.data());

    if (!io->read_u32(crud))
	bbg_failed0("short file");
    if (crud != BBG_SEPARATOR)
	bbg_failed1("expecting separator, got %u", crud);

    if (!io->read_u32(crud))
	bbg_failed0("short file");
    bbg_log.debug("%sskipping function flags(?): 0x%08x\n", prefix, crud);

    return TRUE;
}


gboolean
cov_file_t::read_old_bbg_function(covio_t *io)
{
    uint32_t nblocks, totnarcs, narcs;
    uint32_t bidx, aidx;
    uint32_t dest, flags;
    uint32_t sep;
    cov_arc_t *a;
    cov_function_t *fn;

    bbg_log.debug("BBG reading function\n");

    if ((features_ & FF_OLDPLUS))
    {
	switch (skip_oldplus_func_header(io, "BBG   "))
	{
	case FALSE: return FALSE;
	case EOF: return TRUE;
	}
    }

    if (!io->read_u32(nblocks))
    {
	if ((features_ & FF_OLDPLUS))
	    bbg_failed0("short file");
	return TRUE;    /* end of file */
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
	bbg_log.debug("BBG   block %d\n", bidx);
	if (!io->read_u32(narcs))
	    bbg_failed0("short file");

	if (narcs > BBG_MAX_ARCS)
	    bbg_failed2("narcs=%u > %u", narcs, BBG_MAX_ARCS);

	for (aidx = 0 ; aidx < narcs ; aidx++)
	{
	    io->read_u32(dest);
	    if (!io->read_u32(flags))
		bbg_failed0("short file");

	    bbg_log.debug("BBG     arc %u: %u->%u flags %x(%s,%s,%s)\n",
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
		    bbg_log.debug("BBG     missing fake flag\n");
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
	    files_log.error("%s: file is corrupted or in a bad file format.\n",
		    io->filename());
	    return FALSE;
	}
    }

    return TRUE;
}

gboolean
cov_file_t::read_old_bbg_file(covio_t *io)
{
    features_ |= FF_BBFILE;
    return read_old_bbg_file_common(io);
}

gboolean
cov_file_t::read_oldplus_bbg_file(covio_t *io)
{
    features_ |= FF_BBFILE|FF_OLDPLUS;
    return read_old_bbg_file_common(io);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * GCOV_TAG_* defines copied from gcc/gcc/gcov-io.h (cvs 20030615)
 */
/* The record tags.  Values [1..3f] are for tags which may be in either
   file.  Values [41..9f] for those in the bbg file and [a1..ff] for
   the data file.  */

#define GCOV_TAG_FUNCTION        ((uint32_t)0x01000000)
#define GCOV_TAG_BLOCKS          ((uint32_t)0x01410000)
#define GCOV_TAG_ARCS            ((uint32_t)0x01430000)
#define GCOV_TAG_LINES           ((uint32_t)0x01450000)
#define GCOV_TAG_COUNTER_BASE    ((uint32_t)0x01a10000)
#define GCOV_TAG_OBJECT_SUMMARY  ((uint32_t)0xa1000000)
#define GCOV_TAG_PROGRAM_SUMMARY ((uint32_t)0xa3000000)

static const struct
{
    const char *name;
    uint32_t value;
}
gcov_tags[] =
{
{"GCOV_TAG_FUNCTION",           GCOV_TAG_FUNCTION},
{"GCOV_TAG_BLOCKS",             GCOV_TAG_BLOCKS},
{"GCOV_TAG_ARCS",               GCOV_TAG_ARCS},
{"GCOV_TAG_LINES",              GCOV_TAG_LINES},
{"GCOV_TAG_COUNTER_BASE",       GCOV_TAG_COUNTER_BASE},
{"GCOV_TAG_OBJECT_SUMMARY",     GCOV_TAG_OBJECT_SUMMARY},
{"GCOV_TAG_PROGRAM_SUMMARY",    GCOV_TAG_PROGRAM_SUMMARY},
{0, 0}
};

static const char *
gcov_tag_as_string(uint32_t tag)
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

    bbg_log.debug("infer_compilation_directory(\"%s\")\n", path);

    /*
     * If used without the -o option, flex generates #lines like
     *
     *    #line 3 "<stdout>"
     *
     * which results in a filename of "<stdout>" in the .gcno file.
     * We're not going to get anywhere useful with this, so there's
     * little point trying and complaining.
     */
    if (!strcmp(path, "<stdout>"))
	return;

    string_var normpath = file_normalise(path);
    path = normpath.data();

    if (path[0] == '/' &&
	(clen = path_is_suffix(path, relpath_)) > 0)
    {
	/*
	 * `path' is an absolute path whose tail is
	 * identical to `relpath_', so we can infer that
	 * the compiledir is the remainder of `path'.
	 */
	compiledir_ = g_strndup(path, clen);
	bbg_log.debug("compiledir_=\"%s\"\n", compiledir_.data());
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
	bbg_log.debug("compiledir_=\"%s\"\n", compiledir_.data());
	return;
    }

    /* This might be a problem...but probably not */
    bbg_log.debug("Could not calculate compiledir for %s from location %s\n",
	    name_.data(), path);
}

const char *
cov_file_t::make_absolute(const char *filename) const
{
    if (compiledir_ != (const char *)0)
	return file_make_absolute_to_dir(filename, compiledir_);
    if (*filename != '/')
	bbg_log.warning("no compiledir when converting "
			"path \"%s\" to absolute, trying plan B\n",
			filename);
    return file_make_absolute_to_file(filename, name_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
decode_new_version(uint32_t ver, unsigned int *major,
		   unsigned int *minor, unsigned char *rel)
{
    unsigned char b;

    b = (ver>>24) & 0xff;
    if (isdigit(b))
    {
	/* from gcc 3.3 and up*/
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
    else if (b >= 'A' && b <= ('A'+9))
    {
	/* from gcc 7.1 and up */
	*major = b - 'A';

	b = (ver>>16) & 0xff;
	if (!isdigit(b))
	    return FALSE;
	*major = (*major * 10) + (b - '0');

	b = (ver>>8) & 0xff;
	if (!isdigit(b))
	    return FALSE;
	*minor = b - '0';

	b = (ver) & 0xff;
	if (!isalnum(b) && b != '*')
	    return FALSE;
	*rel = b;

	return TRUE;
    }
    else
    {
	return FALSE;
    }
}

gboolean
cov_file_t::read_gcc3_bbg_file(covio_t *io,
			       uint32_t expect_version,
			       covio_t::format_t ioformat)
{
    uint32_t tag, length;
    cov_function_t *fn = 0;
    const char *filename = NULL;
    estring funcname;
    uint32_t tmp;
    uint32_t nblocks = 0;
    uint32_t bidx, last_bidx = (exit_block_is_1() ? 1 : 0);
    unsigned int nlines = 0;
    cov_arc_t *a;
    uint32_t dest, flags;
    uint32_t line, last_line = 0;
    unsigned int len_unit = 1;
    uint64_t funcid = 0;

    io->set_format(ioformat);

    if (!io->read_u32(format_version_))
	bbg_failed0("short file");
    switch (format_version_)
    {
    case _NEW_VERSION(3,3,'S'):   /* SUSE crud */
	features_ |= FF_DAMANGLED;
	/* fall through */
    case _NEW_VERSION(3,3,'M'):   /* Mandrake crud */
    case BBG_VERSION_GCC33:
	if (expect_version != BBG_VERSION_GCC33)
	    bbg_failed1("unexpected version=0x%08x", format_version_);
	break;
    case _NEWER_VERSION(8,3,'R'):   /* gcc 8.3.x in Fedora 29 */
    case _NEWER_VERSION(8,3,'*'):
    case _NEWER_VERSION(8,2,'*'):
    case _NEWER_VERSION(8,1,'*'):
	features_ |= FF_UNEXECUTED_BLOCKS | FF_FNCOLUMN | FF_SIMPLE_BLOCKS;
	/* fall through */
    case _NEWER_VERSION(7,4,'*'):
    case _NEWER_VERSION(7,3,'R'):  /* gcc 7.3.x in Fedora 27 */
    case _NEWER_VERSION(7,3,'*'):  /* gcc 7.3.x, stock and Ubuntu bionic & cosmic */
    case _NEWER_VERSION(7,1,'*'):
    case _NEW_VERSION(7,0,'*'):
    case _NEW_VERSION(6,5,'*'):
    case _NEW_VERSION(6,4,'*'):
    case _NEW_VERSION(6,3,'*'):
    case _NEW_VERSION(6,2,'*'):
    case _NEW_VERSION(6,1,'*'):
    case _NEW_VERSION(6,0,'*'):
    case _NEW_VERSION(5,4,'*'):
    case _NEW_VERSION(5,3,'*'):
    case _NEW_VERSION(5,2,'*'):
    case _NEW_VERSION(5,1,'*'):
    case _NEW_VERSION(5,0,'*'):
    case _NEW_VERSION(4,9,'*'):
    case _NEW_VERSION(4,8,'R'): /* Fedora 19 */
    case _NEW_VERSION(4,8,'*'):
	features_ |= FF_EXITBLOCK1;
	/* fall through */
    case _NEW_VERSION(4,7,'R'): /* RedHat Fedora 18 */
    case _NEW_VERSION(4,7,'*'):
	features_ |= FF_FNCHECKSUM2;
	/* fall through */
    case _NEW_VERSION(4,0,'*'):
    case _NEW_VERSION(4,0,'R'):
    case _NEW_VERSION(4,0,'U'): /* Ubuntu Dapper Drake */
    case _NEW_VERSION(4,0,'A'): /* Apple MacOS X*/
    case _NEW_VERSION(4,1,'*'):
    case _NEW_VERSION(4,1,'p'): /* Ubuntu Edgy */
    case _NEW_VERSION(4,3,'*'):
    case _NEW_VERSION(4,4,'*'):
    case _NEW_VERSION(4,5,'*'):
    case _NEW_VERSION(4,6,'p'): /* pre-release in Debian Wheezy */
    case _NEW_VERSION(4,6,'*'):
    case _NEW_VERSION(4,6,'U'): /* Ubuntu Oneiric */
	features_ |= FF_DA0TAG;
	/* fall through */
    case _NEW_VERSION(3,4,'U'): /* Ubuntu */
    case _NEW_VERSION(3,4,'R'): /* RedHat */
    case _NEW_VERSION(3,4,'M'): /* Mandrake crud */
	features_ |= FF_FUNCIDS;
	/* fall through */
    case BBG_VERSION_GCC34:
	features_ |= FF_TIMESTAMP;
	if (expect_version != BBG_VERSION_GCC34)
	    bbg_failed1("unexpected version=0x%08x", format_version_);
	break;
    default:
	unsigned int major, minor;
	unsigned char rel;
	if (!decode_new_version(format_version_, &major, &minor, &rel))
	    files_log.error("%s: undecodable compiler version %08x, "
			    "probably corrupted file\n",
			    io->filename(), format_version_);
	else
	    files_log.error("%s: unsupported compiler version %u.%u(%c), "
			    "contact author for support\n",
			    io->filename(), major, minor, rel);
	return FALSE;
    }

    if ((features_ & FF_TIMESTAMP))
    {
	if (!io->read_u32(tmp))      /* ignore the timestamp */
	    bbg_failed0("short file");
	bbg_log.debug("timestamp=0x%08x\n", tmp);
	/* TODO: should really do something useful with this */
	len_unit = 4;   /* records lengths are in 4-byte units now */
    }

    if ((features_ & FF_UNEXECUTED_BLOCKS))
    {
	if (!io->read_u32(tmp))         /* ignore supports_has_unexecuted_blocks */
	    bbg_failed0("short file");
	bbg_log.debug("supports_has_unexecuted_blocks=%d\n", tmp);
    }

    off_t expected_record_start = io->tell();
    while (io->read_u32(tag))
    {
        if (io->tell() - 4 != expected_record_start)
        {
	    bbg_log.error("%s: unexpected offset at start of record, "
                          "expecting 0x%x got 0x%x.  Skipping file\n",
                          io->filename(),
                          expected_record_start,
                          io->tell() - 4);
            return FALSE;
        }
	if (tag == 0 && ((features_ & FF_DA0TAG)))
	    break;  /* end of file */

	if (!io->read_u32(length))
	    bbg_failed0("short file");
	length *= len_unit;

        expected_record_start = io->tell() + length;

	bbg_log.debug("tag=0x%08x (%s) length=%u offset=0x%x\n",
		tag, gcov_tag_as_string(tag), length,
		(unsigned long)(io->tell()-8));
	switch (tag)
	{
	case GCOV_TAG_FUNCTION:
	    if ((features_ & FF_FNCOLUMN))
	    {
		estring filename;

		if (!io->read_u64(funcid) ||        // ident, lineno_checksum
		    !io->read_u32(tmp) ||           // cfg_checksum
		    !io->read_string(funcname) ||   // name
		    !io->read_u32(tmp) ||	    // artificial
		    !io->read_string(filename) ||   // source
		    !io->read_u32(tmp) ||           // start_line
		    !io->read_u32(tmp) ||           // start_column
		    !io->read_u32(tmp))             // end_line
		    bbg_failed0("short file");
		if (compiledir_ == (const char *)0)
		    infer_compilation_directory(filename);
	    }
	    else if ((features_ & FF_FNCHECKSUM2))
	    {
		estring filename;

		if (!io->read_u64(funcid) ||        // ident, lineno_checksum
		    !io->read_u32(tmp) ||           // cfg_checksum
		    !io->read_string(funcname) ||   // name
		    !io->read_string(filename) ||   // source
		    !io->read_u32(tmp))             // lineno
		    bbg_failed0("short file");
		if (compiledir_ == (const char *)0)
		    infer_compilation_directory(filename);
	    }
	    else if ((features_ & FF_FUNCIDS))
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
	    bbg_log.debug("added function \"%s\"\n", funcname.data());
	    nblocks = 0;
	    break;

	case GCOV_TAG_BLOCKS:
	    if (fn == 0)
		bbg_failed0("no FUNCTION tag seen");
	    if (nblocks > 0)
		bbg_failed0("duplicate BLOCKS tag");
	    if ((features_ & FF_SIMPLE_BLOCKS))
	    {
		if (!io->read_u32(nblocks))
		    bbg_failed0("short file");
	    }
	    else
	    {
		nblocks = length/4;
		/* skip the per-block flags */
		io->skip(length);
	    }
	    for (bidx = 0 ; bidx < nblocks ; bidx++)
		fn->add_block();
	    break;

	case GCOV_TAG_ARCS:
	    if (!io->read_u32(bidx))
		bbg_failed0("short file");
	    for (length -= 4 ; length > 0 ; length -= 8)
	    {
		if (!io->read_u32(dest) ||
		    !io->read_u32(flags))
		    bbg_failed0("short file");

		bbg_log.debug("BBG     arc %u->%u flags %x(%s,%s,%s)\n",
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
		a->call_ = (dest == fn->exit_block() && (flags & BBG_FAKE));
		a->on_tree_ = !!(flags & BBG_ON_TREE);
		a->attach(fn->nth_block(bidx), fn->nth_block(dest));
	    }
	    break;

	case GCOV_TAG_LINES:
	    if (!io->read_u32(bidx))
		bbg_failed0("short file");
	    if (bidx >= nblocks)
		bbg_failed2("bidx=%u > nblocks=%u", bidx, nblocks);
	    if (filename && last_line &&
		bidx >= fn->first_real_block() && bidx <= fn->last_real_block())
	    {
		/* may need to interpolate some block->line assignments */
		for (last_bidx++ ; last_bidx < bidx ; last_bidx++)
		{
		    bbg_log.debug("BBG     interpolating line:\n");
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
	    files_log.info("%s: skipping unknown tag 0x%08x offset 0x%08lx length 0x%08x\n",
		    io->filename(), tag, (unsigned long)(io->tell()-8), length);
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
    files_log.debug("Reading .bbg file \"%s\"\n", io->filename());

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

#define MAX_MAGIC_LEN               4


gboolean
cov_file_t::discover_format(covio_t *io)
{
    char magic[MAX_MAGIC_LEN];
    const format_rec_t *fmt;

    files_log.debug("Detecting format of .bbg file \"%s\"\n", io->filename());

    if (io->read(magic, MAX_MAGIC_LEN) != MAX_MAGIC_LEN)
    {
	files_log.error("%s: short file while reading magic number\n", io->filename());
	return FALSE;
    }

    for (fmt = formats ; fmt->magic_ != 0 ; fmt++)
    {
	if (!memcmp(magic, fmt->magic_, fmt->magic_len_))
	    break;
    }
    files_log.debug("Detected %s\n", fmt->description_);
    format_ = fmt;
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::read_old_da_file(covio_t *io)
{
    uint64_t nents;
    uint64_t ent;

    io->set_format(covio_t::FORMAT_OLD);
    io->read_u64(nents);

    for (ptrarray_iterator_t<cov_function_t> fnitr = functions_->first() ; *fnitr ; ++fnitr)
    {
	for (ptrarray_iterator_t<cov_block_t> bitr = (*fnitr)->blocks().first() ; *bitr ; ++bitr)
	{
	    cov_block_t *b = *bitr;

	    for (list_iterator_t<cov_arc_t> aiter = b->first_arc() ; *aiter ; ++aiter)
	    {
		cov_arc_t *a = *aiter;

		if (a->on_tree_)
		    continue;

		/* TODO: check that nents is correct */
		if (!io->read_u64(ent))
		{
		    da_log.error("%s: short file\n", io->filename());
		    return FALSE;
		}

		if (da_log.is_enabled(logging::DEBUG))
		{
		    string_var fromdesc = a->from()->describe();
		    string_var todesc = a->to()->describe();
		    da_log.debug("DA arc {from=%s to=%s} count=%llu\n",
			         fromdesc.data(), todesc.data(),
			         (unsigned long long)ent);
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
	da_log.debug("da:%d, " fmt "\n", __LINE__); \
	return FALSE; \
    }
#define da_failed1(fmt, a1) \
    { \
	da_log.debug("da:%d, " fmt "\n", __LINE__, a1); \
	return FALSE; \
    }
#define da_failed2(fmt, a1, a2) \
    { \
	da_log.debug("da:%d, " fmt "\n", __LINE__, a1, a2); \
	return FALSE; \
    }

#define DA_OLDPLUS_MAGIC    0x8000007b

gboolean
cov_file_t::read_oldplus_da_file(covio_t *io)
{
    uint32_t crud;
    uint32_t file_narcs;
    uint64_t ent;
    unsigned int actual_narcs;

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

    for (ptrarray_iterator_t<cov_function_t> fnitr = functions_->first() ; *fnitr ; ++fnitr)
    {
	cov_function_t *fn = *fnitr;

	if (!skip_oldplus_func_header(io, "DA "))
	    return FALSE;

	if (!io->read_u32(file_narcs))
	    da_failed0("short file");
	actual_narcs = 0;

	for (ptrarray_iterator_t<cov_block_t> bitr = fn->blocks().first() ; *bitr ; ++bitr)
	{
	    cov_block_t *b = *bitr;

	    for (list_iterator_t<cov_arc_t> aiter = b->first_arc() ; *aiter ; ++aiter)
	    {
		cov_arc_t *a = *aiter;

		if (a->on_tree_)
		    continue;

		if (++actual_narcs > file_narcs)
		    da_failed2("bad num arcs, expecting %d got >= %d\n",
				file_narcs, actual_narcs);

		if (!io->read_u64(ent))
		    da_failed0("short file");

		if (da_log.is_enabled(logging::DEBUG))
		{
		    string_var fromdesc = a->from()->describe();
		    string_var todesc = a->to()->describe();
		    da_log.debug("DA arc {from=%s to=%s} count=%llu\n",
			         fromdesc.data(), todesc.data(),
			         (unsigned long long)ent);
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
	(((uint32_t)a<<24)| \
	 ((uint32_t)b<<16)| \
	 ((uint32_t)c<<8)| \
	 ((uint32_t)d))

#define DA_GCC34_MAGIC      _DA_MAGIC('g','c','d','a')  /* also 4.0 */
#define DA_GCC33_MAGIC      _DA_MAGIC('g','c','o','v')

gboolean
cov_file_t::read_gcc3_da_file(covio_t *io,
			      uint32_t expect_magic,
			      covio_t::format_t ioformat)
{
    uint32_t magic, version;
    uint32_t tag, length;
    cov_function_t *fn = 0;
    uint64_t count;
    uint32_t tmp;
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
	if (!io->read_u32(tmp))         /* ignore timestamp */
	    da_failed0("short file");
	len_unit = 4;   /* record lengths are in 4-byte units */
    }

    while (io->read_u32(tag))
    {
	if (tag == 0 && ((features_ & FF_DA0TAG)))
	    break;  /* end of file */

	if (!io->read_u32(length))
	    da_failed0("short file");
	length *= len_unit;

	da_log.debug("tag=0x%08x (%s) length=%u\n",
		     tag, gcov_tag_as_string(tag), length);
	switch (tag)
	{
	case GCOV_TAG_FUNCTION:
	    if ((features_ & FF_FNCHECKSUM2))
	    {
		uint64_t funcid;
		if (!io->read_u64(funcid) ||        // ident, lineno_checksum
		    !io->read_u32(tmp))             // cfg_checksum
		    fn = 0;
		else if ((fn = functions_by_id_->lookup(&funcid)) == 0)
		    da_failed1("unexpected function id %llu", (unsigned long long)funcid);
	    }
	    else if ((features_ & FF_FUNCIDS))
	    {
		uint64_t funcid;
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
		if ((features_ & FF_DAMANGLED))
		    funcname = demangle(funcname);
		funcname = normalise_mangled(funcname);
		fn = find_function(funcname);
		if (fn == 0)
		    da_failed1("unexpected function name \"%s\"", funcname.data());
		io->read_u32(tmp);      /* ignore the checksum */
	    }
	    break;

	case GCOV_TAG_COUNTER_BASE:
	    if (fn == 0)
		da_failed0("missing FUNCTION or duplicate COUNTER_BASE tags");
	    for (ptrarray_iterator_t<cov_block_t> bitr = fn->blocks().first() ; *bitr ; ++bitr)
	    {
		cov_block_t *b = *bitr;

		for (list_iterator_t<cov_arc_t> aiter = b->first_arc() ; *aiter ; ++aiter)
		{
		    cov_arc_t *a = *aiter;

		    if (a->on_tree_)
			continue;

		    if (!io->read_u64(count))
			da_failed0("short file");
		    if (da_log.is_enabled(logging::DEBUG))
		    {
			string_var fromdesc = a->from()->describe();
			string_var todesc = a->to()->describe();
			da_log.debug("DA arc {from=%s to=%s} count=%llu\n",
				     fromdesc.data(), todesc.data(),
				     (unsigned long long)count);
		    }
		    a->set_count(count);
		}
	    }
	    fn = 0;
	    break;

	default:
	    da_log.error("%s: skipping unknown tag 0x%08x offset 0x%08lx length 0x%08x\n",
		         io->filename(), tag, (unsigned long)(io->tell()-8), length);
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
    files_log.debug("Reading runtime data file \"%s\"\n", io->filename());

    return (this->*(format_->read_da_))(io);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if defined(HAVE_LIBBFD) && defined(CALLTREE_ENABLED)

gboolean
cov_file_t::o_file_add_call(
    cov_location_t loc,
    const char *callname_dem)
{
    cov_line_t *ln;
    cov_block_t *pure_candidate = 0;

    loc.filename = (char *)make_absolute(loc.filename);

    /*
     * Sometimes, for some reason particularly with .y files, gcc
     * and bfd conspire to report incorrect filenames.  For example,
     * if a file named baz/quux.y is built from a directory named
     * /foo/bar, bfd reports the filename as /foo/bar/quux.c instead
     * of the correct /foo/bar/baz/quux.c.  Here we heuristically
     * work around that bug.
     */
    if (file_exists(loc.filename) < 0)
    {
	string_var candidate = file_make_absolute_to_file(file_basename_c(loc.filename), name_);
	if (file_exists(candidate) == 0)
	{
	    cgraph_log.debug("o_file_add_call: heuristically replacing \"%s\" with \"%s\"\n",
			     loc.filename, (const char *)candidate);
	    loc.filename = candidate.take();
	}
    }

    if ((ln = find_line(&loc)) == 0)
    {
	cgraph_log.error("No blocks for call to %s at %s:%ld\n",
			 callname_dem, loc.filename, loc.lineno);
	return FALSE;
    }

    for (list_iterator_t<cov_block_t> itr = ln->blocks().first() ; *itr ; ++itr)
    {
	cov_block_t *b = *itr;

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
	    cgraph_log.debug("    block %s\n", b->describe());
	    b->add_call(callname_dem, &loc);
	    return TRUE;
	}
	cgraph_log.debug("    skipping block %s\n", b->describe());
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
	pure_candidate->add_call(callname_dem, &loc);
	return TRUE;
    }

    /*
     * Something is badly wrong if we get here: at least one of
     * the blocks on the line should have needed a call and none
     * did.  Either the .o file is out of sync with the .bb or
     * .bbg files, or we've encountered the braindead gcc 2.96.
     */
    cgraph_log.error("Could not assign block for call to %s at %s:%ld\n",
		     callname_dem, loc.filename, loc.lineno);
    return FALSE;
}

/*
 * Use the BFD library to scan relocation records in the .o file.
 */
gboolean
cov_file_t::scan_o_file_calls(cov_bfd_t *cbfd)
{
    cov_call_scanner_t *cs;
    gboolean ret = FALSE;

    files_log.debug("Scanning .o file \"%s\" for calls\n", cbfd->filename());

    cov_factory_t<cov_call_scanner_t> factory;
    do
    {
	files_log.debug("Trying scanner %s\n", factory.name());
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
	    o_file_add_call(cdata.location, cdata.callname);
	delete cs;
	ret = (r == 0); /* 0=>successfully finished scan */
    }
    return ret;
}

/*
 * Use the BFD library to extract function linkage information
 * from the object file's symbol table.  This will be useful
 * for resolving symbol name clashes in the callgraph.
 */
void
cov_file_t::scan_o_file_linkage(cov_bfd_t *cbfd)
{
    files_log.debug("Scanning .o file \"%s\" for linkage\n", cbfd->filename());

    int n = cbfd->num_symbols();
    for (int i = 0 ; i < n ; i++)
    {
	const asymbol *sym = cbfd->nth_symbol(i);
	if (!(sym->flags & BSF_FUNCTION))
	    continue;
	cov_function_t *fn = find_function(sym->name);
	if (!fn)
	    continue;

	switch (sym->flags & (BSF_LOCAL|BSF_GLOBAL))
	{
	case BSF_LOCAL:
	    files_log.debug2("Function %s is LOCAL\n", sym->name);
	    fn->set_linkage(cov_function_t::LOCAL);
	    break;
	case BSF_GLOBAL:
	    files_log.debug2("Function %s is GLOBAL\n", sym->name);
	    fn->set_linkage(cov_function_t::GLOBAL);
	    break;
	/* case 0: undefined, can't tell from here */
	/* default: should not happen */
	}
    }
}


gboolean
cov_file_t::read_o_file(covio_t *io)
{
    files_log.debug("Reading .o file \"%s\"\n", io->filename());

    cov_bfd_t *cbfd = new(cov_bfd_t);
    if (!cbfd->open(io->filename(), io->take()) || cbfd->num_symbols() == 0)
    {
	delete cbfd;
	return FALSE;
    }

    if (!scan_o_file_calls(cbfd))
    {
	delete cbfd;
	return TRUE;        /* this info is optional */
    }

    /*
     * Calls can fail to be reconciled for perfectly harmless
     * reasons (e.g. the code uses function pointers) so don't
     * fail if reconciliation fails.
     * TODO: record the failure in the cov_function_t.
     */
    for (ptrarray_iterator_t<cov_function_t> fnitr = functions_->first() ; *fnitr ; ++fnitr)
	(*fnitr)->reconcile_calls();

    scan_o_file_linkage(cbfd);

    delete cbfd;
    return TRUE;
}

#endif /* HAVE_LIBBFD && CALLTREE_ENABLED */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class cov_file_src_parser_t : public cpp_parser_t
{
private:
    cov_file_t *file_;
    const cov_suppression_t *suppressions_[cov_suppression_t::NUM_TYPES];
    hashtable_t<const char, const cov_suppression_t> *active_ranges_;

    void
    depends_changed()
    {
	if (cpp_log.is_enabled(logging::DEBUG2))
	    dump();

	suppressions_[cov_suppression_t::IFDEF] = 0;
	for (list_iterator_t<const cov_suppression_t> itr =
	     cov_suppressions.first(cov_suppression_t::IFDEF) ; *itr ; ++itr)
	{
	    const cov_suppression_t *s = *itr;
	    if (depends(s->word()))
	    {
		cpp_log.debug("depends_changed suppressions_[IFDEF]=%s\n", s->describe());
		suppressions_[s->type()] = s;
		break;
	    }
	}
    }

    const cov_suppression_t *
    suppression() const
    {
	int i;

	for (i = 0 ; i < cov_suppression_t::NUM_TYPES ; i++)
	    if (suppressions_[i])
		return suppressions_[i];
	return 0;
    }

    void
    post_line()
    {
	cov_line_t *ln;
	const cov_suppression_t *s = suppression();

	if (s &&
	    lineno() <= file_->lines_->length() &&
	    (ln = file_->lines_->nth(lineno()-1)) != 0)
	{
	    cpp_log.debug("post_line: suppressing: %s\n", s->describe());
	    ln->suppress(s);
	}
	/* line suppression is one-shot */
	suppressions_[cov_suppression_t::COMMENT_LINE] = 0;
    }

    void
    handle_comment(const char *text)
    {
	const cov_suppression_t *s;

	cpp_log.debug("handle_comment: \"%s\"\n", text);

	s = cov_suppressions.find(text, cov_suppression_t::COMMENT_LINE);
	if (s)
	{
	    /* comment suppresses this line only */
	    suppressions_[s->type()] = s;
	    cpp_log.debug("handle_comment: suppressing line\n");
	}

	s = cov_suppressions.find(text, cov_suppression_t::COMMENT_RANGE);
	if (s)
	{
	    /* comment starts a new active range */
	    cpp_log.debug("handle_comment: starting range %s-%s\n",
			    s->word(), s->word2());
	    if (!active_ranges_->lookup(s->word2()))
		active_ranges_->insert(s->word2(), s);
	    suppressions_[s->type()] = s;
	}

	s = active_ranges_->lookup(text);
	if (s)
	{
	    /* comment ends an active range */
	    active_ranges_->remove(text);
	    cpp_log.debug("handle_comment: ending range %s-%s\n",
			    s->word(), s->word2());
	    if (active_ranges_->size() == 0)
	    {
		/*
		 * Removed last active range; stop suppressing
		 * by range, but suppress this line so the range
		 * is inclusive of the closing magical comment line
		 */
		suppressions_[cov_suppression_t::COMMENT_RANGE] = 0;
		suppressions_[cov_suppression_t::COMMENT_LINE] = s;
		cpp_log.debug("handle_comment: last range\n");
	    }
	}
    }


public:
    cov_file_src_parser_t(cov_file_t *f)
     :  cpp_parser_t(f->name()),
	file_(f)
    {
	memset(suppressions_, 0, sizeof(suppressions_));
	active_ranges_ = new hashtable_t<const char, const cov_suppression_t>;
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

void
cov_file_t::set_gcda_prefix(const char *dir)
{
    gcda_prefix_ = dir;
}


//
// Attempts to open a file given a filename 'fn' and a filename
// extension 'ext'.  The extension replaces the existing extension on
// the filename.  Returns a covio_t object opened for reading, or on
// error returns NULL and sets errno.
//
covio_t *
cov_file_t::try_file(const char *fn, const char *ext) const
{
    string_var ofilename = fn;

    assert(ext[0] != '+');  /* this used to be a feature */
    string_var dfilename = file_change_extension(ofilename, 0, ext);

    files_log.debug2("    try %s\n", dfilename.data());

    if (file_is_regular(dfilename) < 0)
    {
	int e = errno;
	if (e == EISDIR)
	    files_log.error("%s: not a regular file\n", dfilename.data());
	else if (e != ENOENT)
	    perror(dfilename);
	errno = e;
	return 0;
    }

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
cov_file_t::find_file(const char *ext, gboolean quiet,
		      const char *prefix) const
{
    covio_t *io;

    files_log.debug2("Searching for %s file matching %s\n",
		ext, file_basename_c(name()));

    if (prefix)
    {
	/*
	 * First try the prefix.
	 */
	string_var fn = g_strconcat(prefix, name(), (char *)0);
	if ((io = try_file(fn, ext)) != 0 || errno != ENOENT)
	    return io;
    }

    /*
     * Then try the same directory as the source file.
     */
    if ((io = try_file(name(), ext)) != 0 || errno != ENOENT)
	return io;

    /*
     * Then try the .libs/ directory in the same directory
     * as the source file - libtool built objects sometimes
     * dump their .gcda files there
     */
    string_var dirname = file_dirname(name());
    string_var ltlibfn = g_strconcat(dirname, "/.libs/", file_basename_c(name()), (char *)0);
    if ((io = try_file(ltlibfn, ext)) != 0 || errno != ENOENT)
	return io;

    /*
     * Now look in the search path.
     */
    for (list_iterator_t<char> iter = search_path_.first() ; *iter ; ++iter)
    {
	string_var fn = g_strconcat(*iter, "/", file_basename_c(name()), (char *)0);
	if ((io = try_file(fn, ext)) != 0 || errno != ENOENT)
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
    string_var dir = file_dirname(name());
    string_var which = (ext2 == 0 ? g_strdup("") :
			    g_strdup_printf(" or %s", ext2));

    files_log.error("Couldn't find %s%s file for %s in path:\n",
		ext, which.data(), file_basename_c(name()));
    files_log.error("   %s\n", dir.data());
    for (list_iterator_t<char> iter = search_path_.first() ; *iter ; ++iter)
	files_log.error("   %s\n", *iter);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
have_line_suppressions(void)
{
    if (cov_suppressions.count(cov_suppression_t::IFDEF))
	return TRUE;
    if (cov_suppressions.count(cov_suppression_t::COMMENT_LINE))
	return TRUE;
    if (cov_suppressions.count(cov_suppression_t::COMMENT_RANGE))
	return TRUE;
    return FALSE;
}

gboolean
cov_file_t::read(gboolean quiet)
{
    string_var filename;
    const char *da_ext = ".da";
    covio_var io;

    if ((io = find_file(".bbg", TRUE, 0)) == 0)
    {
	/* The .bbg file was gratuitously renamed .gcno in gcc 3.4 */
	if ((io = find_file(".gcno", TRUE, 0)) == 0)
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
    if ((features_ & FF_BBFILE))
    {
	if ((io = find_file(".bb", quiet, 0)) == 0 ||
	    !read_bb_file(io))
	    return FALSE;
    }

    /*
     * We can successfully read a .gcno, all of whose locations
     * refer to *other* files.  This happens for example when
     * we've done a directory walk and discovered both of foo.c
     * and foo.y where foo.c is generated from foo.y.  In this
     * case we want to silently ignore foo.c.
     */
    if (!has_locations_)
    {
	files_log.debug("Ignoring file %s because it has no locations\n", name_.data());
	return FALSE;
    }

    /* TODO: read multiple .da files from the search path & accumulate */
    if ((io = find_file(da_ext, quiet, gcda_prefix_)) == 0)
    {
	if (errno != ENOENT)
	    return FALSE;
	zero_arc_counts();
    }
    else if (!read_da_file(io))
	return FALSE;

    if (have_line_suppressions() && !read_src_file())
    {
	static int count = 0;
	static const char warnmsg[] =
	"could not scan source file for cpp conditionals or comments, "
	"reports may be inaccurate.\n";

	if (!count++)
	    files_log.warning("%s: %s", name(), warnmsg);
    }

    /*
     * If the data files were written by broken versions of gcc 2.96
     * the callgraph will be irretrievably broken and there's no point
     * at all trying to read the object file.
     */
#if defined(HAVE_LIBBFD) && defined(CALLTREE_ENABLED)
    if (gcc296_braindeath())
    {
	static int count = 0;
	static const char warnmsg[] =
	"data files written by a broken gcc 2.96; the Calls "
	"statistics and the contents of the Calls, Call Butterfly and "
	"Call Graph windows will be incomplete.  Skipping object "
	"file.\n"
	;
	/* TODO: update the message to point out that line stats are fine */
	if (!count++)
	    files_log.warning("%s: %s", name(), warnmsg);
    }
    else
    {
	/*
	 * The data we get from object files is optional, a user can
	 * still get lots of value from ggcov with the remainder of the
	 * files.  So if we can't find the object or can't read it,
	 * complain and keep going.
	 */
	io = find_file(".o", quiet, 0);
	if (!io)
	    io = find_file(".os", TRUE, 0);
	if (!io || !read_o_file(io))
	{
	    static int count = 0;
	    static const char warnmsg[] =
	    "could not find or read matching object file; the contents "
	    "of some windows may be inaccurate or incomplete, but ggcov "
	    "is still usable.\n"
	    ;
	    /* TODO: save and report in alert to user */
	    if (!count++)
		files_log.warning("%s: %s", name(), warnmsg);
	}
    }
#endif  /* HAVE_LIBBFD && CALLTREE_ENABLED */

    if (!solve())
	return FALSE;

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_file_t::line_iterator_t::line_iterator_t(const cov_file_t *file, unsigned int lineno)
 :  file_(file)
{
    loc_.filename = (char *)file_->name();
    loc_.lineno = lineno;
}

cov_file_t::line_iterator_t::~line_iterator_t()
{
}

cov_file_t::line_iterator_t cov_file_t::lines_begin() const
{
    line_iterator_t itr(this, 1U);
    return itr;
}

/* returns an iterator pointing 1 past the last line */
cov_file_t::line_iterator_t cov_file_t::lines_end() const
{
    line_iterator_t itr(this, num_lines()+1);
    return itr;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_file_annotator_t::cov_file_annotator_t(cov_file_t *file)
 :  file_(file),
    fp_(0),
    lineno_(0UL)
{
    if ((fp_ = fopen(file_->name(), "r")) == 0)
    {
	perror(file_->name());
	return;
    }
}

cov_file_annotator_t::~cov_file_annotator_t()
{
    if (fp_)
	fclose(fp_);
}

bool
cov_file_annotator_t::next()
{
    if (!fp_)
	return false;

    // read the next line
    if (fgets(buf_, sizeof(buf_), fp_) == 0)
    {
	// no more lines
	fclose(fp_);
	fp_ = 0;
	return false;
    }
    ++lineno_;
    ln_ = file_->nth_line(lineno_);
    return true;
}

const char *
cov_file_annotator_t::count_as_string() const
{
    if (status() == cov::UNINSTRUMENTED ||
	status() == cov::SUPPRESSED)
    {
	return "-";
    }
    else if (!count())
    {
	return "#####";
    }
    else
    {
	static char buf[32];
	snprintf(buf, sizeof(buf), "%llu", (unsigned long long)count());
	return buf;
    }
}

const char *
cov_file_annotator_t::blocks_as_string() const
{
    static char buf[512];
    format_blocks(buf, sizeof(buf)-1);
    return buf;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
