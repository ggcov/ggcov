/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2004 Greg Banks <gnb@users.sourceforge.net>
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

CVSID("$Id: cov.C,v 1.31 2010-05-09 05:37:14 gnb Exp $");

static gboolean cov_read_one_object_file(const char *exefilename, int depth);
static void cov_calculate_duplicate_counts(void);
extern char *argv0;

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
    dprintf1(D_FILES, "Adding search directory \"%s\"\n", dir);
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
    dprintf1(D_FILES, "Handling source file %s\n", filename);

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

    new cov_callgraph_t();      // becomes singleton instance
    cov_file_t::init();
    cov_suppression_t::init_builtins();
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
    cov_callgraph_t *callgraph = cov_callgraph_t::instance();
    for (iter = cov_file_t::first() ; *iter ; ++iter)
	callgraph->add_nodes(*iter);
    for (iter = cov_file_t::first() ; *iter ; ++iter)
	callgraph->add_arcs(*iter);

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

    dprintf1(D_FILES, "Scanning \"%s\" for shared libraries\n",
	     b->filename());

    do
    {
	dprintf1(D_FILES, "Trying scanner %s\n", factory.name());
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
	dprintf1(D_FILES, "Trying filename %s\n", file.data());
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

    dprintf1(D_FILES, "Scanning object or exe file \"%s\"\n", exefilename);

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
	dprintf1(D_FILES, "Trying scanner %s\n", factory.name());
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
	dprintf1(D_FILES, "Trying filename %s\n", file.data());
	if (cov_is_source_filename(file) &&
	    file_is_regular(file) == 0 &&
	    cov_read_source_file_2(file, /*quiet*/TRUE))
	    successes++;
    }

    delete fs;

    successes += cov_read_shlibs(b, depth+1);

    if (depth == 0 && successes == 0)
	fprintf(stderr, "found no coveraged source files in executable \"%s\"\n",
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
    dprintf1(D_FILES, "Scanning directory \"%s\"\n", child.data());

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
	    fprintf(stderr, "found no coveraged source files in or under directory \"%s\"\n",
		    dirname);
	else
	    fprintf(stderr, "found no coveraged source files in directory \"%s\"\n",
		    dirname);
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

    if (params.get_suppressed_calls() != 0)
    {
	tok_t tok(params.get_suppressed_calls(), ", \t");
	const char *v;

	while ((v = tok.next()) != 0)
	    new cov_suppression_t(v, cov_suppression_t::BLOCK_CALLS,
				  "commandline --suppress-call option");
    }

    if (params.get_suppressed_ifdefs() != 0)
    {
	tok_t tok(params.get_suppressed_ifdefs(), ", \t");
	const char *v;

	while ((v = tok.next()) != 0)
	    new cov_suppression_t(v, cov_suppression_t::IFDEF,
				  "commandline -X option");
    }

    if (params.get_suppressed_comment_lines() != 0)
    {
	tok_t tok(params.get_suppressed_comment_lines(), ", \t");
	const char *v;

	while ((v = tok.next()) != 0)
	    new cov_suppression_t(v, cov_suppression_t::COMMENT_LINE,
				  "commandline -Y option");
    }

    if (params.get_suppressed_comment_ranges() != 0)
    {
	tok_t tok(params.get_suppressed_comment_ranges(), ", \t");
	const char *s, *e;

	while ((s = tok.next()) != 0)
	{
	    if ((e = tok.next()) == 0)
	    {
		fprintf(stderr, "%s: -Z option requires pairs of words\n", argv0);
		return -1;
	    }
	    cov_suppression_t *sup;
	    sup = new cov_suppression_t(s, cov_suppression_t::COMMENT_RANGE,
					"commandline -Z option");
	    sup->set_word2(e);
	}
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
		fprintf(stderr, "%s: don't know how to handle this filename\n",
			filename);
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
dump_callarcs(FILE *fp, list_t<cov_callarc_t> &arcs)
{
    for (list_iterator_t<cov_callarc_t> itr = arcs.first() ; *itr ; ++itr)
    {
	cov_callarc_t *ca = *itr;

	fprintf(fp, "            ARC {\n");
	fprintf(fp, "                FROM=%s\n", ca->from->name.data());
	fprintf(fp, "                TO=%s\n", ca->to->name.data());
	fprintf(fp, "                COUNT=%llu\n", (unsigned long long)ca->count);
	fprintf(fp, "            }\n");
    }
}

static void
dump_callspace(cov_callspace_t *space, FILE *fp)
{
    fprintf(fp, "CALLSPACE {\n");
    fprintf(fp, "    NAME=%s\n", space->name());

    for (cov_callnode_iter_t cnitr = space->first() ; *cnitr ; ++cnitr)
    {
	cov_callnode_t *cn = *cnitr;

	fprintf(fp, "    CALLNODE {\n");
	fprintf(fp, "        NAME=%s\n", cn->name.data());
	if (cn->function == 0)
	    fprintf(fp, "        FUNCTION=null\n");
	else
	    fprintf(fp, "        FUNCTION=%s:%s\n", cn->function->file()->name(),
						    cn->function->name());
	fprintf(fp, "        COUNT=%llu\n", (unsigned long long)cn->count);
	fprintf(fp, "        OUT_ARCS={\n");
	dump_callarcs(fp, cn->out_arcs);
	fprintf(fp, "        }\n");
	fprintf(fp, "        IN_ARCS={\n");
	dump_callarcs(fp, cn->in_arcs);
	fprintf(fp, "        }\n");
	fprintf(fp, "    }\n");
    }

    fprintf(fp, "}\n");
}

/*
 * This function is extern only because it needs to be a friend
 * of class cov_arc_t, i.e. no good reason. TODO: move the dump
 * functions into class members.
 */
void
dump_arc(FILE *fp, cov_arc_t *a)
{
    fprintf(fp, "                    ARC {\n");
    fprintf(fp, "                        FROM=%s\n", a->from()->describe());
    fprintf(fp, "                        TO=%s\n", a->to()->describe());
    fprintf(fp, "                        COUNT=%llu\n", (unsigned long long)a->count());
    fprintf(fp, "                        NAME=%s\n", a->name());
    fprintf(fp, "                        ON_TREE=%s\n", boolstr(a->on_tree_));
    fprintf(fp, "                        CALL=%s\n", boolstr(a->call_));
    fprintf(fp, "                        FALL_THROUGH=%s\n", boolstr(a->fall_through_));
    fprintf(fp, "                        STATUS=%s\n", status_names[a->status()]);
    fprintf(fp, "                    }\n");
}

void
dump_block(FILE *fp, cov_block_t *b)
{
    fprintf(fp, "            BLOCK {\n");
    fprintf(fp, "                IDX=%s\n", b->describe());
    fprintf(fp, "                COUNT=%llu\n", (unsigned long long)b->count());
    fprintf(fp, "                STATUS=%s\n", status_names[b->status()]);

    fprintf(fp, "                OUT_ARCS {\n");
    for (list_iterator_t<cov_arc_t> aiter = b->first_arc() ; *aiter ; ++aiter)
	dump_arc(fp, *aiter);
    fprintf(fp, "                }\n");

    fprintf(fp, "                LOCATIONS {\n");
    for (list_iterator_t<cov_location_t> liter = b->locations().first() ; *liter ; ++liter)
    {
	cov_location_t *loc = *liter;
	cov_line_t *ln = cov_line_t::find(loc);
	fprintf(fp, "                    %s:%ld %s\n",
		loc->filename,
		loc->lineno,
		status_names[ln->status()]);
    }
    fprintf(fp, "                }\n");

    fprintf(fp, "                PURE_CALLS {\n");
    for (list_iterator_t<cov_block_t::call_t> citer = b->pure_calls_.first() ; *citer ; ++citer)
    {
	cov_block_t::call_t *call = *citer;
	fprintf(fp, "                    %s @ %s\n",
		(call->name_.data() == 0 ? "-" : call->name_.data()),
		call->location_.describe());
    }
    fprintf(fp, "                }\n");
    fprintf(fp, "            }\n");
}

static void
dump_function(FILE *fp, cov_function_t *fn)
{
    fprintf(fp, "        FUNCTION {\n");
    fprintf(fp, "            NAME=\"%s\"\n", fn->name());
    fprintf(fp, "            STATUS=%s\n", status_names[fn->status()]);
    fprintf(fp, "            LINKAGE=%s\n", linkage_names[fn->linkage()]);
    for (ptrarray_iterator_t<cov_block_t> itr = fn->blocks().first() ; *itr ; ++itr)
	dump_block(fp, *itr);
    fprintf(fp, "            DUP_COUNT=%u\n", fn->dup_count());
    fprintf(fp, "    }\n");
}

static void
dump_file(FILE *fp, cov_file_t *f)
{
    fprintf(fp, "FILE {\n");
    fprintf(fp, "    NAME=\"%s\"\n", f->name());
    fprintf(fp, "    MINIMAL_NAME=\"%s\"\n", f->minimal_name());
    fprintf(fp, "    STATUS=%s\n", status_names[f->status()]);
    for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
	dump_function(fp, *fnitr);
    fprintf(fp, "}\n");
}

void
cov_dump(FILE *fp)
{
    if (debug_enabled(D_DUMP))
    {
	if (fp == 0)
	    fp = stderr;

	for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
	    dump_file(fp, *iter);

	cov_callgraph_t *callgraph = cov_callgraph_t::instance();
	for (cov_callspace_iter_t csitr = callgraph->first() ; *csitr ; ++csitr)
	    dump_callspace(*csitr, fp);
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
/*END*/
