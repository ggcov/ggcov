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

/*
 * tggcov is a libcov-based reimplementation of the gcov commandline
 * program.  It's primary use is for regression testing, to allow
 * automated comparisons of the statistics generated by libcov and
 * the real ones from gcov.
 */

#include "common.h"
#include "cov.H"
#include "filename.h"
#include "estring.H"
#include "fakepopt.h"

CVSID("$Id: tggcov.c,v 1.5 2003-07-14 15:57:17 gnb Exp $");

char *argv0;
GList *files;	    /* incoming specification from commandline */

static int recursive = FALSE;	/* needs to be int (not gboolean) for popt */
static int header_flag = FALSE;
static int blocks_flag = FALSE;
static int lines_flag = FALSE;
static int new_format_flag = FALSE;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* 
 * TODO: This function should be common with the identical code in ggcov.c
 */

static void
read_gcov_files(void)
{
    GList *iter;
    
    cov_init();
    cov_pre_read();
    
    if (files == 0)
    {
    	if (!cov_read_directory(".", recursive))
	    exit(1);
    }
    else
    {
	for (iter = files ; iter != 0 ; iter = iter->next)
	{
	    const char *filename = (const char *)iter->data;
	    
	    if (file_is_directory(filename) == 0)
	    	cov_add_search_directory(filename);
    	}

	for (iter = files ; iter != 0 ; iter = iter->next)
	{
	    const char *filename = (const char *)iter->data;
	    
	    if (file_is_directory(filename) == 0)
	    {
	    	if (!cov_read_directory(filename, recursive))
		    exit(1);
	    }
	    else if (file_is_regular(filename) == 0)
	    {
	    	if (cov_is_source_filename(filename))
		{
		    if (!cov_read_source_file(filename))
			exit(1);
		}
		else
		{
		    if (!cov_read_object_file(filename))
			exit(1);
		}
	    }
	    else
	    {
	    	fprintf(stderr, "%s: don't know how to handle this filename\n",
		    	filename);
		exit(1);
	    }
	}
    }
    
    cov_post_read();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BLOCKS_WIDTH	8

static void
annotate_file(cov_file_t *f)
{
    const char *cfilename = f->name();
    FILE *infp, *outfp;
    unsigned long lineno;
    cov_line_t *ln;
    char *ggcov_filename;
    char buf[1024];
    
    if ((infp = fopen(cfilename, "r")) == 0)
    {
    	perror(cfilename);
	return;
    }

    ggcov_filename = g_strconcat(cfilename, ".tggcov", 0);
    fprintf(stderr, "Writing %s\n", ggcov_filename);
    if ((outfp = fopen(ggcov_filename, "w")) == 0)
    {
    	perror(ggcov_filename);
	g_free(ggcov_filename);
	fclose(infp);
	return;
    }
    g_free(ggcov_filename);
    
    if (header_flag)
    {
    	fprintf(outfp, "    Count       ");
	if (blocks_flag)
    	    fprintf(outfp, "Block(s)");
	if (lines_flag)
    	    fprintf(outfp, " Line   ");
    	fprintf(outfp, " Source\n");

    	fprintf(outfp, "============    ");
	if (blocks_flag)
    	    fprintf(outfp, "======= ");
	if (lines_flag)
    	    fprintf(outfp, "======= ");
    	fprintf(outfp, "=======\n");
    }
    
    lineno = 0;
    while (fgets(buf, sizeof(buf), infp) != 0)
    {
    	++lineno;
	ln = f->nth_line(lineno);

	if (new_format_flag)
	{
	    if (ln->status() != cov_line_t::UNINSTRUMENTED)
	    {
		if (ln->count())
		    fprintf(outfp, "%9llu:%5lu:", ln->count(), lineno);
		else
		    fprintf(outfp, "    #####:%5lu:", lineno);
	    }
	    else
		fprintf(outfp, "        -:%5lu:", lineno);
	}
	else
	{
	    if (ln->status() != cov_line_t::UNINSTRUMENTED)
	    {
		if (ln->count())
		    fprintf(outfp, "%12lld    ", ln->count());
		else
		    fputs("      ######    ", outfp);
	    }
	    else
		fputs("\t\t", outfp);
	}
	if (blocks_flag)
	{
    	    char blocks_buf[BLOCKS_WIDTH];
    	    ln->format_blocks(blocks_buf, BLOCKS_WIDTH-1);
	    fprintf(outfp, "%*s ", BLOCKS_WIDTH-1, blocks_buf);
	}
	if (lines_flag)
	{
	    fprintf(outfp, "%7lu ", lineno);
	}
	fputs(buf, outfp);
    }
    
    fclose(infp);
    fclose(outfp);
}

static void
annotate(void)
{
    list_iterator_t<cov_file_t> iter;
    
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    	annotate_file(*iter);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * With the old GTK, we're forced to parse our own arguments the way
 * the library wants, with popt and in such a way that we can't use the
 * return from poptGetNextOpt() to implement multiple-valued options
 * (e.g. -o dir1 -o dir2).  This limits our ability to parse arguments
 * for both old and new GTK builds.  Worse, gtk2 doesn't depend on popt
 * at all, so some systems will have gtk2 and not popt, so we have to
 * parse arguments in a way which avoids potentially buggy duplicate
 * specification of options, i.e. we simulate popt in fakepopt.c!
 */
static poptContext popt_context;
static struct poptOption popt_options[] =
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
    	"blocks",	    	    	    	/* longname */
	'B',  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	&blocks_flag,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"display block numbers",    	    	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"header",	    	    	    	/* longname */
	'H',  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	&header_flag,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"display header line",	    	    	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"lines",	    	    	    	/* longname */
	'L',  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	&lines_flag,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"display line numbers",	    	    	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"new-format",	    	    	    	/* longname */
	'N',  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	&new_format_flag,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"display count in new gcc 3.3 format",	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    POPT_AUTOHELP
    { 0, 0, 0, 0, 0, 0, 0 }
};

static void
parse_args(int argc, char **argv)
{
    const char *file;
    
    argv0 = argv[0];
    
    popt_context = poptGetContext(PACKAGE, argc, (const char**)argv,
    	    	    	    	  popt_options, 0);
    poptSetOtherOptionHelp(popt_context,
    	    	           "[OPTIONS] [executable|source|directory]...");

    int rc;
    while ((rc = poptGetNextOpt(popt_context)) > 0)
    	;
    if (rc < -1)
    {
    	fprintf(stderr, "%s:%s at or near %s\n",
	    argv[0],
	    poptStrerror(rc),
	    poptBadOption(popt_context, POPT_BADOPTION_NOALIAS));
    	exit(1);
    }
    
    while ((file = poptGetArg(popt_context)) != 0)
	files = g_list_append(files, (gpointer)file);
	
    poptFreeContext(popt_context);
    
#if 0
    {
    	GList *iter;

	fprintf(stderr, "parse_args: recursive=%d\n", recursive);
	fprintf(stderr, "parse_args: blocks_flag=%d\n", blocks_flag);
	fprintf(stderr, "parse_args: header_flag=%d\n", header_flag);
	fprintf(stderr, "parse_args: lines_flag=%d\n", lines_flag);

	fprintf(stderr, "parse_args: files = {\n");
	for (iter = files ; iter != 0 ; iter = iter->next)
	    fprintf(stderr, "parse_args:     %s\n", (char *)iter->data);
	fprintf(stderr, "parse_args: }\n");

	exit(1);
    }
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define DEBUG_GLIB 1
#if DEBUG_GLIB

static const char *
log_level_to_str(GLogLevelFlags level)
{
    static char buf[32];

    switch (level & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR: return "ERROR";
    case G_LOG_LEVEL_CRITICAL: return "CRITICAL";
    case G_LOG_LEVEL_WARNING: return "WARNING";
    case G_LOG_LEVEL_MESSAGE: return "MESSAGE";
    case G_LOG_LEVEL_INFO: return "INFO";
    case G_LOG_LEVEL_DEBUG: return "DEBUG";
    default:
    	snprintf(buf, sizeof(buf), "%d", level);
	return buf;
    }
}

void
log_func(
    const char *domain,
    GLogLevelFlags level,
    const char *msg,
    gpointer user_data)
{
    fprintf(stderr, "%s:%s:%s\n",
    	(domain == 0 ? PACKAGE : domain),
	log_level_to_str(level),
	msg);
    if (level & G_LOG_FLAG_FATAL)
    	exit(1);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
#if DEBUG_GLIB
    g_log_set_handler("GLib",
    	    	      (GLogLevelFlags)(G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL),
    	    	      log_func, /*user_data*/0);
#endif

    parse_args(argc, argv);
    read_gcov_files();

    cov_dump(stderr);
    annotate();

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
