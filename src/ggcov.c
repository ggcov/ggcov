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

#include "common.h"
#include "cov.h"
#include "ui.h"
#include "filename.h"
#include "estring.h"
#include "sourcewin.h"
#include "summarywin.h"
#include "callswin.h"
#include "callgraphwin.h"
#include "callgraph2win.h"
#include "functionswin.h"
#include "fileswin.h"
#include <libgnomeui/libgnomeui.h>

CVSID("$Id: ggcov.c,v 1.10 2002-12-12 00:10:27 gnb Exp $");

char *argv0;
GList *files;	    /* incoming specification from commandline */
GList *filenames;   /* filenames of all .c files which have .bb etc read */
int screenshot_mode = 0;

static poptContext popt_context;
static struct poptOption popt_options[] =
{
    {
    	"glade-path",	    	    	    	/* longname */
	0,  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&ui_glade_path,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"where to locate ggcov.glade",	    	/* descrip */
	"colon-seperated list of directories"	/* argDescrip */
    },
    {
    	"screenshot-mode",  	    	    	/* longname */
	0,  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	&screenshot_mode,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"behaviour tweaks for screenshots", 	/* descrip */
	0   	    	    	    		/* argDescrip */
    },
    { 0, 0, 0, 0, 0, 0, 0 }
};

static void summarise(void);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
append_one_filename(cov_file_t *f, void *userdata)
{
    filenames = g_list_append(filenames, f->name);
}

static int
compare_filenames(gconstpointer a, gconstpointer b)
{
    return strcmp((const char *)a, (const char *)b);
}

static void
read_gcov_files(void)
{
    GList *iter;
    
    cov_init();
    cov_pre_read();
    
    if (files == 0)
    {
    	if (!cov_read_directory("."))
	    exit(1);
    }
    else
    {
	for (iter = files ; iter != 0 ; iter = iter->next)
	{
	    const char *filename = (const char *)iter->data;
	    const char *ext;
	    
	    if (file_is_directory(filename) == 0)
	    {
	    	if (!cov_read_directory(filename))
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
    
    cov_file_foreach(append_one_filename, 0);
    filenames = g_list_sort(filenames, compare_filenames);
    
    cov_post_read();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0

static void
annotate_file(cov_file_t *f, void *userdata)
{
    const char *cfilename = f->name;
    FILE *infp, *outfp;
    cov_location_t loc;
    count_t count;
    gboolean have_count;
    char *ggcov_filename;
    char buf[1024];
    
    if ((infp = fopen(cfilename, "r")) == 0)
    {
    	perror(cfilename);
	return;
    }

    ggcov_filename = g_strconcat(cfilename, ".ggcov", 0);
    fprintf(stderr, "Writing %s\n", ggcov_filename);
    if ((outfp = fopen(ggcov_filename, "w")) == 0)
    {
    	perror(ggcov_filename);
	g_free(ggcov_filename);
	fclose(infp);
	return;
    }
    g_free(ggcov_filename);
    
    loc.filename = cfilename;
    loc.lineno = 0;
    while (fgets(buf, sizeof(buf), infp) != 0)
    {
    	++loc.lineno;
	
	cov_get_count_by_location(&loc, &count, &have_count);
	if (have_count)
	{
	    if (count)
		fprintf(outfp, "%12lld    ", count);
	    else
		fputs("      ######    ", outfp);
	}
	else
	    fputs("\t\t", outfp);
	fputs(buf, outfp);
    }
    
    fclose(infp);
    fclose(outfp);
}

static void
annotate(void)
{
    cov_file_foreach(annotate_file, 0);
}
#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG

static void
dump_callarcs(GList *arcs)
{
    for ( ; arcs != 0 ; arcs = arcs->next)
    {
    	cov_callarc_t *ca = (cov_callarc_t *)arcs->data;
	
	fprintf(stderr, "        ARC {\n");
	fprintf(stderr, "            FROM=%s\n", ca->from->name);
	fprintf(stderr, "            TO=%s\n", ca->to->name);
	fprintf(stderr, "            COUNT=%llu\n", ca->count);
	fprintf(stderr, "        }\n");
    }
}

static void
dump_callnode(cov_callnode_t *cn, void *userdata)
{
    fprintf(stderr, "CALLNODE {\n");
    fprintf(stderr, "    NAME=%s\n", cn->name);
    if (cn->function == 0)
	fprintf(stderr, "    FUNCTION=null\n");
    else
	fprintf(stderr, "    FUNCTION=%s:%s\n", cn->function->file->name,
	    	    	    	    	    	cn->function->name);
    fprintf(stderr, "    COUNT=%llu\n", cn->count);
    fprintf(stderr, "    OUT_ARCS={\n");
    dump_callarcs(cn->out_arcs);
    fprintf(stderr, "    }\n");
    fprintf(stderr, "    IN_ARCS={\n");
    dump_callarcs(cn->in_arcs);
    fprintf(stderr, "    }\n");
    fprintf(stderr, "}\n");
    
}

static void
dump_arc(cov_arc_t *a)
{
    fprintf(stderr, "                    ARC {\n");
    fprintf(stderr, "                        FROM=%s:%u\n",
    	    	    	    	a->from->function->name,
				a->from->idx);
    fprintf(stderr, "                        TO=%s:%u\n",
    	    	    	    	a->to->function->name,
				a->to->idx);
    fprintf(stderr, "                        COUNT=%lld\n", a->count);
    fprintf(stderr, "                        NAME=%s\n", a->name);
    fprintf(stderr, "                        ON_TREE=%s\n", boolstr(a->on_tree));
    fprintf(stderr, "                        FAKE=%s\n", boolstr(a->fake));
    fprintf(stderr, "                        FALL_THROUGH=%s\n", boolstr(a->fall_through));
    fprintf(stderr, "                    }\n");
}

static void
dump_block(cov_block_t *b)
{
    GList *iter;
    
    fprintf(stderr, "            BLOCK {\n");
    fprintf(stderr, "                IDX=%s:%u\n", b->function->name, b->idx);
    fprintf(stderr, "                COUNT=%lld\n",b->count);

    fprintf(stderr, "                OUT_ARCS {\n");
    for (iter = b->out_arcs ; iter != 0 ; iter = iter->next)
    	dump_arc((cov_arc_t *)iter->data);
    fprintf(stderr, "                }\n");

    fprintf(stderr, "                IN_ARCS {\n");
    for (iter = b->in_arcs ; iter != 0 ; iter = iter->next)
    	dump_arc((cov_arc_t *)iter->data);
    fprintf(stderr, "                }\n");
    
    fprintf(stderr, "                LOCATIONS {\n");
    for (iter = b->locations ; iter != 0 ; iter = iter->next)
    {
    	cov_location_t *loc = (cov_location_t *)iter->data;
	fprintf(stderr, "                    %s:%ld\n", loc->filename, loc->lineno);
    }
    fprintf(stderr, "                }\n");
    fprintf(stderr, "            }\n");
}

static void
dump_function(cov_function_t *fn)
{
    int i;
    
    fprintf(stderr, "        FUNCTION {\n");
    fprintf(stderr, "            NAME=\"%s\"\n", fn->name);
    for (i = 0 ; i < fn->blocks->len ; i++)
    	dump_block((cov_block_t *)g_ptr_array_index(fn->blocks, i));
    fprintf(stderr, "    }\n");
}

static void
dump_file(cov_file_t *f, void *userdata)
{
    int i;
    
    fprintf(stderr, "FILE {\n");
    fprintf(stderr, "    NAME=\"%s\"\n", f->name);
    for (i = 0 ; i < f->functions->len ; i++)
    	dump_function((cov_function_t *)g_ptr_array_index(f->functions, i));
    fprintf(stderr, "}\n");
}

static void
summarise(void)
{
    cov_file_foreach(dump_file, 0);
    cov_callnode_foreach(dump_callnode, 0);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
ui_init(int argc, char **argv)
{
    gnome_init_with_popt_table(PACKAGE, VERSION, argc, argv,
			       popt_options, /*popt flags*/0,
			       &popt_context);

    /* initialise libGlade */
    glade_gnome_init();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
on_windows_new_summarywin_activated(GtkWidget *w, gpointer userdata)
{
    summarywin_new();
}

static void
on_windows_new_fileswin_activated(GtkWidget *w, gpointer userdata)
{
    fileswin_new();
}

static void
on_windows_new_functionswin_activated(GtkWidget *w, gpointer userdata)
{
    functionswin_new();
}

static void
on_windows_new_callswin_activated(GtkWidget *w, gpointer userdata)
{
    callswin_new();
}

static void
on_windows_new_callgraphwin_activated(GtkWidget *w, gpointer userdata)
{
    callgraphwin_t *cgw;
    
    cgw = callgraphwin_new();
    callgraphwin_set_node(cgw, cov_callnode_find("main"));
}

static void
on_windows_new_callgraph2win_activated(GtkWidget *w, gpointer userdata)
{
    callgraph2win_t *cgw;
    
    cgw = callgraph2win_new();
}

static void
on_windows_new_sourcewin_activated(GtkWidget *w, gpointer userdata)
{
    sourcewin_t *srcw;

    srcw = sourcewin_new();
    sourcewin_set_filename(srcw, (const char *)filenames->data);
}

#include "ui/icon.xpm"

static void
ui_create(void)
{
    ui_register_windows_entry("New Summary...",
    			      on_windows_new_summarywin_activated, 0);
    ui_register_windows_entry("New File List...",
    			      on_windows_new_fileswin_activated, 0);
    ui_register_windows_entry("New Function List...",
    			      on_windows_new_functionswin_activated, 0);
    ui_register_windows_entry("New Calls List...",
    			      on_windows_new_callswin_activated, 0);
    ui_register_windows_entry("New Call Graph...",
    			      on_windows_new_callgraphwin_activated, 0);
    ui_register_windows_entry("New Call Graph2...",
    			      on_windows_new_callgraph2win_activated, 0);
    ui_register_windows_entry("New Source...",
    			      on_windows_new_sourcewin_activated, 0);

    ui_set_default_icon(icon_xpm);

    summarywin_new();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
parse_args(int argc, char **argv)
{
    int rc;
    const char *file;
    
    argv0 = argv[0];
    
    while ((rc = poptGetNextOpt(popt_context)) > 0)
    	;
    
    while ((file = poptGetArg(popt_context)) != 0)
	files = g_list_append(files, (gpointer)file);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    ui_init(argc, argv);
    parse_args(argc, argv);
    read_gcov_files();

#if DEBUG
    summarise();
#endif
#if 0
    annotate();
#endif
    ui_create();
    gtk_main();
    
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
