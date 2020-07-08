/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
#include "cov_suppression.H"
#include "estring.H"
#include "filename.h"
#include "estring.H"
#include "string_var.H"
#include "mvc.h"
#include "tok.H"
#include <dirent.h>
#include "logging.H"

static gboolean cov_read_one_object_file(const char *exefilename, int depth);
static void cov_calculate_duplicate_counts(void);
extern char *argv0;
cov_suppression_set_t cov_suppressions;
cov_callgraph_t cov_callgraph;
static logging::logger_t &_log = logging::find_logger("files");
static logging::logger_t &dump_log = logging::find_logger("dump");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: there's gotta be a better place to put this */
const char *
cov_location_t::describe() const
{
    static estring buf;
    buf.truncate();
    buf.append_printf("%s:%lu", filename, lineno);
    return buf;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_add_search_directory(const char *dir)
{
    _log.debug("Adding search directory \"%s\"\n", dir);
    cov_file_t::search_path_append(dir);
}

gboolean
cov_is_source_filename(const char *filename)
{
    const char *ext;
    static const char * const recognised_exts[] =
    {
	".c", ".cc", ".cxx", ".cpp", ".C", ".c++",
	".y", ".l",
	0
    };
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
    const char *filename;

    filename = file_make_absolute(fname);
    _log.debug("Handling source file %s\n", filename);

    if ((f = cov_file_t::find(filename)) != 0)
    {
	// already seen this file, no drama
	return TRUE;
    }

    f = new cov_file_t(filename, fname);

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

#ifdef HAVE_LIBBFD

#if HAVE_BFD_ERROR_HANDLER_VPRINTFLIKE
static void
cov_bfd_error_handler(const char *fmt, va_list args)
{
    _log.vmessage(logging::ERROR, fmt, args);
}
#else
static void
cov_bfd_error_handler(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    _log.vmessage(logging::ERROR, fmt, args);
    va_end(args);
}
#endif /* HAVE_BFD_ERROR_HANDLER_VPRINTFLIKE */

#endif /* HAVE_LIBBFD */

void
cov_init(void)
{
#ifdef HAVE_LIBBFD
    bfd_init();
    bfd_set_error_handler(cov_bfd_error_handler);
#endif /* HAVE_LIBBFD */

    new cov_callgraph_t();      // becomes singleton instance
    cov_file_t::init();
    cov_suppressions.init_builtins();
}

void
cov_pre_read(void)
{
}

void
cov_post_read(void)
{
    list_iterator_t<cov_file_t> iter;

    /* construct the list of filenames */
    cov_file_t::post_read();

    /* Build the callgraph */
    /* TODO: only do this to newly read files */
    for (iter = cov_file_t::first() ; *iter ; ++iter)
	cov_callgraph.add_nodes(*iter);
    for (iter = cov_file_t::first() ; *iter ; ++iter)
	cov_callgraph.add_arcs(*iter);

    cov_calculate_duplicate_counts();

    /* emit an MVC notification */
    mvc_changed(cov_file_t::files_model(), 1);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_read_object_file(const char *exefilename)
{
    return cov_read_one_object_file(exefilename, 0);
}

static gboolean
cov_read_shlibs(cov_bfd_t *b, int depth)
{
    cov_factory_t<cov_shlib_scanner_t> factory;
    cov_shlib_scanner_t *ss;
    string_var file;
    int successes = 0;

    _log.debug("Scanning \"%s\" for shared libraries\n", b->filename());

    do
    {
	_log.debug("Trying scanner %s\n", factory.name());
	if ((ss = factory.create()) != 0 && ss->attach(b))
	    break;
	delete ss;
	ss = 0;
    }
    while (factory.next());

    if (ss == 0)
	return FALSE;   /* no scanner can open this file */

    /*
     * TODO: instead of using the first scanner that succeeds open()
     *       use the first one that returns any results.
     */
    while ((file = ss->next()) != 0)
    {
	_log.debug("Trying filename %s\n", file.data());
	if (cov_read_one_object_file(file, depth))
	    successes++;
    }

    delete ss;
    return (successes > 0);
}

static gboolean
cov_read_one_object_file(const char *exefilename, int depth)
{
    cov_bfd_t *b;
    cov_filename_scanner_t *fs;
    string_var dir;
    string_var file;
    int successes = 0;

    _log.debug("Scanning object or exe file \"%s\"\n", exefilename);

    if ((b = new cov_bfd_t()) == 0)
	return FALSE;
    if (!b->open(exefilename))
    {
	delete b;
	return FALSE;
    }

    cov_factory_t<cov_filename_scanner_t> factory;
    do
    {
	_log.debug("Trying scanner %s\n", factory.name());
	if ((fs = factory.create()) != 0 && fs->attach(b))
	    break;
	delete fs;
	fs = 0;
    }
    while (factory.next());

    if (fs == 0)
    {
	delete b;
	return FALSE;   /* no scanner can open this file */
    }

    dir = file_dirname(exefilename);
    cov_add_search_directory(dir);

    /*
     * TODO: instead of using the first scanner that succeeds open()
     *       use the first one that returns any results.
     */
    while ((file = fs->next()) != 0)
    {
	_log.debug("Trying filename %s\n", file.data());
	if (cov_is_source_filename(file) &&
	    file_is_regular(file) == 0 &&
	    cov_read_source_file_2(file, /*quiet*/TRUE))
	    successes++;
    }

    delete fs;

    successes += cov_read_shlibs(b, depth+1);

    if (depth == 0 && successes == 0)
	_log.error("found no coveraged source files in executable \"%s\"\n",
		   exefilename);
    delete b;
    return (successes > 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static boolean
directory_is_ignored(const char *dirname, const char *child)
{
    if (strcmp(dirname, "."))
	return FALSE;
    if (!strcmp(child, "debian") ||
	!strcmp(child, ".git"))
	return TRUE;
    return FALSE;
}

static unsigned int
cov_read_directory_2(
    const char *dirname,
    gboolean recursive,
    gboolean quiet)
{
    DIR *dir;
    struct dirent *de;
    int dirlen;
    unsigned int successes = 0;

    estring child = dirname;
    _log.debug("Scanning directory \"%s\"\n", child.data());

    if ((dir = opendir(dirname)) == 0)
    {
	perror(child.data());
	return 0;
    }

    while (child.last() == '/')
	child.truncate_to(child.length()-1);
    if (!strcmp(child, "."))
	child.truncate();
    else
	child.append_char('/');
    dirlen = child.length();

    while ((de = readdir(dir)) != 0)
    {
	if (!strcmp(de->d_name, ".") ||
	    !strcmp(de->d_name, ".."))
	    continue;

	child.truncate_to(dirlen);
	child.append_string(de->d_name);

	if (file_is_regular(child) == 0 &&
	    cov_is_source_filename(child))
	    successes += cov_read_source_file_2(child, /*quiet*/TRUE);
	else if (recursive &&
		 file_is_directory(child) == 0 &&
		 !directory_is_ignored(dirname, child))
	    successes += cov_read_directory_2(child, recursive, /*quiet*/TRUE);
    }

    closedir(dir);
    if (successes == 0 && !quiet)
    {
	if (recursive)
	    _log.error("found no coveraged source files in or under directory \"%s\"\n", dirname);
	else
	    _log.error("found no coveraged source files in directory \"%s\"\n", dirname);
    }
    return successes;
}

unsigned int
cov_read_directory(const char *dirname, gboolean recursive)
{
    return cov_read_directory_2(dirname, recursive, /*quiet*/FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
cov_calculate_duplicate_counts(void)
{
    hashtable_t<const char, void> *dupcount_by_name = new hashtable_t<const char, void>();

    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	for (ptrarray_iterator_t<cov_function_t> fnitr = (*iter)->functions().first() ; *fnitr ; ++fnitr)
	{
	    cov_function_t *fn = *fnitr;
	    void *value = NULL;
	    if (dupcount_by_name->lookup_extended(fn->name(), (const char *)NULL, &value))
		dupcount_by_name->insert(fn->name(), (void *)(((unsigned long)value)+1));
	    else
		dupcount_by_name->insert(fn->name(), (void *)1UL);
	}
    }

    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	for (ptrarray_iterator_t<cov_function_t> fnitr = (*iter)->functions().first() ; *fnitr ; ++fnitr)
	{
	    cov_function_t *fn = *fnitr;
	    fn->set_dup_count((unsigned long)dupcount_by_name->lookup(fn->name()));
	}
    }

    delete dupcount_by_name;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Read coverage data discovered by following the given list
 * of filenames.  Each file can be a C source file, a directory
 * recursively containing other files, an object file, or an
 * executable.  If no files are specified, the current working
 * directory is read (recursively if the -r commandline option
 * was used).
 *
 * Also applies the various global parameters set by commandline
 * options.
 *
 * Returns number of files successfully read, or -1 on error.
 */
int
cov_read_files(const cov_project_params_t &params)
{
    unsigned int successes = 0;

    cov_init();

    for (list_iterator_t<char> itr = params.get_suppressed_calls().first() ; *itr ; ++itr)
    {
        cov_suppressions.add(new cov_suppression_t(*itr, cov_suppression_t::ARC_CALLS,
                             "commandline --suppress-call option"));
        cov_suppressions.add(new cov_suppression_t(*itr, cov_suppression_t::BLOCK_CALLS,
                             "commandline --suppress-call option"));
    }

    for (list_iterator_t<char> itr = params.get_suppressed_ifdefs().first() ; *itr ; ++itr)
    {
        cov_suppressions.add(new cov_suppression_t(*itr, cov_suppression_t::IFDEF,
                              "commandline -X/--suppress-ifdef option"));
    }

    for (list_iterator_t<char> itr = params.get_suppressed_comment_lines().first() ; *itr ; ++itr)
    {
        cov_suppressions.add(new cov_suppression_t(*itr, cov_suppression_t::COMMENT_LINE,
                              "commandline -Y/--suppress-comment option"));
    }

    list_iterator_t<char> itr = params.get_suppressed_comment_ranges().first();
    while (*itr)
    {
        const char *s = *itr;
        ++itr;
        const char *e = *itr;
        ++itr;
        if (!e)
        {
            _log.error("%s: -Z/--suppress-comment-between option requires pairs of words\n", argv0);
            return -1;
        }
        cov_suppression_t *sup;
        sup = new cov_suppression_t(s, cov_suppression_t::COMMENT_RANGE,
                                    "commandline -Z/--suppress-comment-between option");
        sup->set_word2(e);
        cov_suppressions.add(sup);
    }

    for (list_iterator_t<char> itr = params.get_suppressed_functions().first() ; *itr ; ++itr)
    {
        cov_suppressions.add(new cov_suppression_t(*itr, cov_suppression_t::FUNCTION,
                              "commandline --suppress-function option"));
    }

    cov_pre_read();

    if (params.get_object_directory())
	cov_add_search_directory(params.get_object_directory());
    if (params.get_gcda_prefix())
	cov_file_t::set_gcda_prefix(params.get_gcda_prefix());

    if (!params.num_files())
    {
	successes += cov_read_directory(".", params.get_recursive());
    }
    else
    {
	for (argparse::params_t::file_iterator_t itr = params.file_iter() ; *itr ; ++itr)
	{
	    const char *filename = *itr;

	    if (file_is_directory(filename) == 0)
		cov_add_search_directory(filename);
	}

	for (argparse::params_t::file_iterator_t itr = params.file_iter() ; *itr ; ++itr)
	{
	    const char *filename = *itr;

	    if (file_is_directory(filename) == 0)
	    {
		successes += cov_read_directory(filename, params.get_recursive());
	    }
	    else if (errno != ENOTDIR)
	    {
		perror(filename);
	    }
	    else if (file_is_regular(filename) == 0)
	    {
		if (cov_is_source_filename(filename))
		    successes += cov_read_source_file(filename);
		else
		    successes += cov_read_object_file(filename);
	    }
	    else
	    {
		_log.error("%s: don't know how to handle this filename\n", filename);
		return -1;
	    }
	}
    }

    if (!successes && !cov_file_t::length())
	return 0;   /* return 0 so we can pop up a file choice dialog */

    cov_post_read();
    return successes;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

list_t<cov_function_t> *
cov_list_all_functions()
{
    list_t<cov_function_t> *list = new list_t<cov_function_t>;

    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
	for (ptrarray_iterator_t<cov_function_t> fnitr = (*iter)->functions().first() ; *fnitr ; ++fnitr)
	    list->prepend(*fnitr);
    list->sort(cov_function_t::compare);
    return list;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *status_names[cov::NUM_STATUS] =
{
    "COVERED",
    "PARTCOVERED",
    "UNCOVERED",
    "UNINSTRUMENTED",
    "SUPPRESSED"
};

static const char *linkage_names[] =
{
    "UNKNOWN",
    "LOCAL",
    "GLOBAL"
};

static void
dump_callarcs(list_t<cov_callarc_t> &arcs)
{
    for (list_iterator_t<cov_callarc_t> itr = arcs.first() ; *itr ; ++itr)
    {
	cov_callarc_t *ca = *itr;

	dump_log.debug("            ARC {\n");
	dump_log.debug("                FROM=%s\n", ca->from->name.data());
	dump_log.debug("                TO=%s\n", ca->to->name.data());
	dump_log.debug("                COUNT=%llu\n", (unsigned long long)ca->count);
	dump_log.debug("            }\n");
    }
}

static void
dump_callspace(cov_callspace_t *space)
{
    dump_log.debug("CALLSPACE {\n");
    dump_log.debug("    NAME=%s\n", space->name());

    for (cov_callnode_iter_t cnitr = space->first() ; *cnitr ; ++cnitr)
    {
	cov_callnode_t *cn = *cnitr;

	dump_log.debug("    CALLNODE {\n");
	dump_log.debug("        NAME=%s\n", cn->name.data());
	if (cn->function == 0)
	    dump_log.debug("        FUNCTION=null\n");
	else
	    dump_log.debug("        FUNCTION=%s:%s\n", cn->function->file()->name(),
						    cn->function->name());
	dump_log.debug("        COUNT=%llu\n", (unsigned long long)cn->count);
	dump_log.debug("        OUT_ARCS={\n");
	dump_callarcs(cn->out_arcs);
	dump_log.debug("        }\n");
	dump_log.debug("        IN_ARCS={\n");
	dump_callarcs(cn->in_arcs);
	dump_log.debug("        }\n");
	dump_log.debug("    }\n");
    }

    dump_log.debug("}\n");
}

/*
 * This function is extern only because it needs to be a friend
 * of class cov_arc_t, i.e. no good reason. TODO: move the dump
 * functions into class members.
 */
void
dump_arc(cov_arc_t *a)
{
    dump_log.debug("                    ARC {\n");
    dump_log.debug("                        FROM=%s\n", a->from()->describe());
    dump_log.debug("                        TO=%s\n", a->to()->describe());
    dump_log.debug("                        COUNT=%llu\n", (unsigned long long)a->count());
    dump_log.debug("                        NAME=%s\n", a->name());
    dump_log.debug("                        ON_TREE=%s\n", boolstr(a->on_tree_));
    dump_log.debug("                        CALL=%s\n", boolstr(a->call_));
    dump_log.debug("                        FALL_THROUGH=%s\n", boolstr(a->fall_through_));
    dump_log.debug("                        STATUS=%s\n", status_names[a->status()]);
    dump_log.debug("                    }\n");
}

void
dump_block(cov_block_t *b)
{
    dump_log.debug("            BLOCK {\n");
    dump_log.debug("                IDX=%s\n", b->describe());
    dump_log.debug("                COUNT=%llu\n", (unsigned long long)b->count());
    dump_log.debug("                STATUS=%s\n", status_names[b->status()]);

    dump_log.debug("                OUT_ARCS {\n");
    for (list_iterator_t<cov_arc_t> aiter = b->first_arc() ; *aiter ; ++aiter)
	dump_arc(*aiter);
    dump_log.debug("                }\n");

    dump_log.debug("                LOCATIONS {\n");
    for (list_iterator_t<cov_location_t> liter = b->locations().first() ; *liter ; ++liter)
    {
	cov_location_t *loc = *liter;
	cov_line_t *ln = cov_file_t::find_line(loc);
	dump_log.debug("                    %s:%ld %s\n",
		loc->filename,
		loc->lineno,
		status_names[ln->status()]);
    }
    dump_log.debug("                }\n");

    dump_log.debug("                PURE_CALLS {\n");
    for (list_iterator_t<cov_block_t::call_t> citer = b->pure_calls_.first() ; *citer ; ++citer)
    {
	cov_block_t::call_t *call = *citer;
	dump_log.debug("                    %s @ %s\n",
		(call->name_.data() == 0 ? "-" : call->name_.data()),
		call->location_.describe());
    }
    dump_log.debug("                }\n");
    dump_log.debug("            }\n");
}

static void
dump_function(cov_function_t *fn)
{
    dump_log.debug("        FUNCTION {\n");
    dump_log.debug("            NAME=\"%s\"\n", fn->name());
    dump_log.debug("            STATUS=%s\n", status_names[fn->status()]);
    dump_log.debug("            LINKAGE=%s\n", linkage_names[fn->linkage()]);
    for (ptrarray_iterator_t<cov_block_t> itr = fn->blocks().first() ; *itr ; ++itr)
	dump_block(*itr);
    dump_log.debug("            DUP_COUNT=%u\n", fn->dup_count());
    dump_log.debug("    }\n");
}

static void
dump_file(cov_file_t *f)
{
    dump_log.debug("FILE {\n");
    dump_log.debug("    NAME=\"%s\"\n", f->name());
    dump_log.debug("    MINIMAL_NAME=\"%s\"\n", f->minimal_name());
    dump_log.debug("    STATUS=%s\n", status_names[f->status()]);
    for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
	dump_function(*fnitr);
    dump_log.debug("}\n");
}

void
cov_dump()
{
    if (dump_log.is_enabled(logging::DEBUG))
    {
	for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
	    dump_file(*iter);

	for (cov_callspace_iter_t csitr = cov_callgraph.first() ; *csitr ; ++csitr)
	    dump_callspace(*csitr);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void dump_row(FILE *fp, const char *key, const unsigned long *row)
{
    int i;
    fprintf(fp, "    %s: [", key);
    for (i = 0 ; i < cov::NUM_STATUS ; i++)
	fprintf(fp, "%s%lu", (i ? ", " : ""), row[i]);
    fprintf(fp, "]\n");
}

void cov_stats_t::dump(FILE *fp) const
{
    fprintf(fp, "cov_stats_t {\n");
    dump_row(fp, "blocks", blocks_);
    dump_row(fp, "lines", lines_);
    dump_row(fp, "functions", functions_);
    dump_row(fp, "calls", calls_);
    dump_row(fp, "branches", branches_);
    fprintf(fp, "}\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *cov::short_name(cov::status_t st)
{
    static const char *names[cov::NUM_STATUS] =
    {
	"CO",
	"PC",
	"UN",
	"UI",
	"SU"
    };
    return ((int)st >= 0 && (int)st < (int)cov::NUM_STATUS ? names[st] : "??");
}

const char *cov::long_name(cov::status_t st)
{
    static const char *names[cov::NUM_STATUS] =
    {
	"COVERED",
	"PARTCOVERED",
	"UNCOVERED",
	"UNINSTRUMENTED",
	"SUPPRESSED"
    };
    return ((int)st >= 0 && (int)st < (int)cov::NUM_STATUS ? names[st] : "?unknown?");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_stats_t::format_row_labels(const unsigned long *cc, estring &absbuf, estring &pcbuf)
{
    unsigned long numerator = cc[cov::COVERED] + cc[cov::PARTCOVERED];
    unsigned long denominator = cc[cov::COVERED] + cc[cov::PARTCOVERED] + cc[cov::UNCOVERED];

    absbuf.truncate();
    pcbuf.truncate();

    if (denominator == 0)
    {
	absbuf.append_string("0/0");
    }
    else if (cc[cov::PARTCOVERED] == 0)
    {
	absbuf.append_printf("%lu/%lu",
		numerator, denominator);
	pcbuf.append_printf("%.1f%%",
		(double)numerator * 100.0 / (double)denominator);
    }
    else
    {
	absbuf.append_printf("%lu+%ld/%lu",
		cc[cov::COVERED],
		cc[cov::PARTCOVERED],
		denominator);
	pcbuf.append_printf("%.1f+%.1f%%",
		(double)cc[cov::COVERED] * 100.0 / (double)denominator,
		(double)cc[cov::PARTCOVERED] * 100.0 / (double)denominator);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
