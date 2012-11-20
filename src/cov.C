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
	".c", ".cc", ".cxx", ".cpp", ".C",
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
	return FALSE;
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

    new cov_callgraph_t();	// becomes singleton instance
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
    	return FALSE;	/* no scanner can open this file */

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
    	return FALSE;	/* no scanner can open this file */
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

static int recursive = FALSE;	/* needs to be int (not gboolean) for popt */
static char *suppressed_calls = 0;
static char *suppressed_ifdefs = 0;
static char *suppressed_comment_lines = 0;
static char *suppressed_comment_ranges = 0;
static char *object_dir = 0;
static char *gcda_prefix = 0;
static const char *debug_str = 0;
static int print_version_flag = FALSE;

const struct poptOption cov_popt_options[] =
{
    {
    	"recursive",	    	    	    	/* longname */
	'r',  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	&recursive,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"recursively scan directories for source", /* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
	"suppress-call",	    	    	/* longname */
	0,  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&suppressed_calls,			/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"suppress blocks which call this function", /* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"suppress-ifdef",	    	    	/* longname */
	'X',  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&suppressed_ifdefs,     	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"suppress source which is conditional on this cpp define", /* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"suppress-comment",	    	    	/* longname */
	'Y',  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&suppressed_comment_lines,     	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"suppress source on lines containing this comment", /* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"suppress-comment-between",	   	/* longname */
	'Z',  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&suppressed_comment_ranges,     	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"suppress source between lines containing these start and end comments", /* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
	"gcda-prefix",    	    	    	/* longname */
	'p',  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&gcda_prefix,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"directory underneath which to find .da files", /* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"object-directory",    	    	    	/* longname */
	'o',  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&object_dir,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"directory in which to find .o,.bb,.bbg,.da files", /* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
	"solve-fuzzy",    	    	    	/* longname */
	'F',  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	NULL,	         	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"(silently ignored for compatibility)", /* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"debug",	    	    	    	/* longname */
	'D',  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&debug_str,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"enable ggcov debugging features",  	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"version",	    	    	    	/* longname */
	'v',  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	&print_version_flag,     	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"print version and exit",  	    	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    { 0, 0, 0, 0, 0, 0, 0 }
};

void
cov_post_args(void)
{
    if (debug_str != 0)
    	debug_set(debug_str);

    if (print_version_flag)
    {
    	fputs(PACKAGE " version " VERSION "\n", stdout);
	exit(0);
    }

    if (debug_enabled(D_DUMP|D_VERBOSE))
    {
	string_var token_str = debug_enabled_tokens();

	duprintf1("cov_post_args: recursive=%d\n", recursive);
	duprintf1("cov_post_args: suppressed_calls=%s\n", suppressed_calls);
	duprintf1("cov_post_args: suppressed_ifdefs=%s\n", suppressed_ifdefs);
	duprintf1("cov_post_args: suppressed_comment_lines=%s\n", suppressed_comment_lines);
	duprintf1("cov_post_args: suppressed_comment_ranges=%s\n", suppressed_comment_ranges);
	duprintf2("cov_post_args: debug = 0x%lx (%s)\n", debug, token_str.data());

	static const struct { const char *name; int value; } build_options[] =
	{
	    { "HAVE_LIBBFD",
	    #ifdef HAVE_LIBBFD
	    1
	    #endif
	    }, { "HAVE_LIBPOPT",
	    #ifdef HAVE_LIBPOPT
	    1
	    #endif
	    }, { "HAVE_LIBGCONF",
	    #ifdef HAVE_LIBGCONF
	    1
	    #endif
	    }, { "COV_I386",
	    #ifdef COV_I386
	    1
	    #endif
	    }, { "COV_AMD64",
	    #ifdef COV_AMD64
	    1
	    #endif
	    }, { "UI_DEBUG",
	    #ifdef UI_DEBUG
	    1
	    #endif
	    }, { "HAVE_GNOME_PROGRAM_INIT",
	    #ifdef HAVE_GNOME_PROGRAM_INIT
	    1
	    #endif
	    }, { "HAVE_GTK_TEXT_BUFFER_SELECT_RANGE",
	    #ifdef HAVE_GTK_TEXT_BUFFER_SELECT_RANGE
	    1
	    #endif
	    }, { "HAVE_G_HASH_TABLE_ITER",
	    #ifdef HAVE_G_HASH_TABLE_ITER
	    1
	    #endif
	    }, { NULL, 0 }
	};
	duprintf0("cov_post_args: built with");
	for (int i = 0 ; build_options[i].name ; i++)
	    duprintf2(" %s%s", build_options[i].value ? "" : "!", build_options[i].name);
	duprintf0("\n");
    }
}


void
cov_read_files(const list_t<const char> &files)
{
    unsigned int successes = 0;

    if (debug_enabled(D_DUMP|D_VERBOSE))
    {
	duprintf0("cov_post_args: files = ");
	for (list_iterator_t<const char> itr = files.first() ; *itr ; ++itr)
	    duprintf1(" \"%s\"", *itr);
	duprintf0(" }\n");
    }

    cov_init();

    if (suppressed_calls != 0)
    {
	tok_t tok(/*force copy*/(const char *)suppressed_calls, ", \t");
	const char *v;

	while ((v = tok.next()) != 0)
	    new cov_suppression_t(v, cov_suppression_t::BLOCK_CALLS,
				  "commandline --suppress-call option");
    }

    if (suppressed_ifdefs != 0)
    {
    	tok_t tok(/*force copy*/(const char *)suppressed_ifdefs, ", \t");
	const char *v;
	
	while ((v = tok.next()) != 0)
	    new cov_suppression_t(v, cov_suppression_t::IFDEF,
				  "commandline -X option");
    }

    if (suppressed_comment_lines != 0)
    {
    	tok_t tok(/*force copy*/(const char *)suppressed_comment_lines, ", \t");
	const char *v;
	
	while ((v = tok.next()) != 0)
	    new cov_suppression_t(v, cov_suppression_t::COMMENT_LINE,
				  "commandline -Y option");
    }

    if (suppressed_comment_ranges != 0)
    {
    	tok_t tok(/*force copy*/(const char *)suppressed_comment_ranges, ", \t");
	const char *s, *e;
	
	while ((s = tok.next()) != 0)
	{
	    if ((e = tok.next()) == 0)
	    {
		fprintf(stderr, "%s: -Z option requires pairs of words\n", argv0);
		exit(1);
	    }
	    cov_suppression_t *sup;
	    sup = new cov_suppression_t(s, cov_suppression_t::COMMENT_RANGE,
					"commandline -Z option");
	    sup->set_word2(e);
	}
    }

    cov_pre_read();
    
    if (object_dir != 0)
    	cov_add_search_directory(object_dir);
    if (gcda_prefix)
	cov_file_t::set_gcda_prefix(gcda_prefix);

    if (!files.head())
    {
	successes += cov_read_directory(".", recursive);
    }
    else
    {
	for (list_iterator_t<const char> itr = files.first() ; *itr ; ++itr)
	{
	    const char *filename = *itr;

	    if (file_is_directory(filename) == 0)
	    	cov_add_search_directory(filename);
    	}

	for (list_iterator_t<const char> itr = files.first() ; *itr ; ++itr)
	{
	    const char *filename = *itr;

	    if (file_is_directory(filename) == 0)
	    {
		successes += cov_read_directory(filename, recursive);
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
		exit(1);
	    }
	}
    }

    if (!successes && !cov_file_t::length())
	exit(1);

    cov_post_read();
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
	fprintf(fp, "                COUNT="GNB_U64_DFMT"\n", ca->count);
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
	fprintf(fp, "        COUNT="GNB_U64_DFMT"\n", cn->count);
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
    fprintf(fp, "                        COUNT="GNB_U64_DFMT"\n", a->count());
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
    fprintf(fp, "                COUNT="GNB_U64_DFMT"\n", b->count());
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
/*END*/
