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

#include "common.h"
#include "cov.H"
#include "ui.h"
#include "filename.h"
#include "prefs.H"
#include "estring.H"
#include "sourcewin.H"
#include "summarywin.H"
#include "callswin.H"
#include "callgraphwin.H"
#include "callgraph2win.H"
#include "functionswin.H"
#include "fileswin.H"
#if !GTK2
#include <libgnomeui/libgnomeui.h>
#endif
#include "fakepopt.h"

CVSID("$Id: ggcov.c,v 1.29 2003-06-28 10:21:13 gnb Exp $");

#define DEBUG_GTK 1

char *argv0;
GList *files;	    /* incoming specification from commandline */
int screenshot_mode = 0;

static int recursive = FALSE;	/* needs to be int (not gboolean) for popt */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

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

static void
on_windows_new_summarywin_activated(GtkWidget *w, gpointer userdata)
{
    summarywin_t *sw;
    
    sw = new summarywin_t();
    sw->show();
}

static void
on_windows_new_fileswin_activated(GtkWidget *w, gpointer userdata)
{
    fileswin_t *fw;
    
    fw = new fileswin_t();
    fw->show();
}

static void
on_windows_new_functionswin_activated(GtkWidget *w, gpointer userdata)
{
    functionswin_t *fw;
    
    fw = new functionswin_t();
    fw->show();
}

static void
on_windows_new_callswin_activated(GtkWidget *w, gpointer userdata)
{
    callswin_t *cw;
    
    cw = new callswin_t();
    cw->show();
}

static void
on_windows_new_callgraphwin_activated(GtkWidget *w, gpointer userdata)
{
    callgraphwin_t *cgw;
    
    cgw = new callgraphwin_t();
    cgw->set_node(cov_callnode_t::find("main"));
    cgw->show();
}

#if CALLGRAPH2
static void
on_windows_new_callgraph2win_activated(GtkWidget *w, gpointer userdata)
{

    callgraph2win_t *cgw;
    
    cgw = new callgraph2win_t;
    cgw->show();
}
#endif

static void
on_windows_new_sourcewin_activated(GtkWidget *w, gpointer userdata)
{
    sourcewin_t *srcw;
    list_iterator_t<cov_file_t> iter = cov_file_t::first();
    cov_file_t *f = *iter;

    srcw = new sourcewin_t();
    srcw->set_filename(f->name(), f->minimal_name());
    srcw->show();
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
    ui_register_windows_entry("New Call Butterfly...",
    			      on_windows_new_callgraphwin_activated, 0);
#if CALLGRAPH2
    ui_register_windows_entry("New Call Graph...",
    			      on_windows_new_callgraph2win_activated, 0);
#endif
    ui_register_windows_entry("New Source...",
    			      on_windows_new_sourcewin_activated, 0);

    ui_set_default_icon(icon_xpm);

    prefs.load();
    summarywin_t *sw = new summarywin_t();
    sw->show();
    prefs.post_load(sw->get_window());
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
    { 0, 0, 0, 0, 0, 0, 0 }
};

static void
parse_args(int argc, char **argv)
{
    const char *file;
    
    argv0 = argv[0];
    
#if GTK2
    popt_context = poptGetContext(PACKAGE, argc, (const char**)argv,
    	    	    	    	  popt_options, 0);
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
#endif
    
    while ((file = poptGetArg(popt_context)) != 0)
	files = g_list_append(files, (gpointer)file);
	
    poptFreeContext(popt_context);
    
#if 0
    {
    	GList *iter;

	fprintf(stderr, "parse_args: recursive=%d\n", recursive);

	fprintf(stderr, "parse_args: files = {\n");
	for (iter = files ; iter != 0 ; iter = iter->next)
	    fprintf(stderr, "parse_args:     %s\n", (char *)iter->data);
	fprintf(stderr, "parse_args: }\n");

	exit(1);
    }
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if DEBUG_GTK

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
#if DEBUG_GTK
    g_log_set_handler("GLib",
    	    	      (GLogLevelFlags)(G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL),
    	    	      log_func, /*user_data*/0);
    g_log_set_handler("Gtk",
    	    	      (GLogLevelFlags)(G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL),
    	    	      log_func, /*user_data*/0);
    g_log_set_handler("libglade",
    	    	      (GLogLevelFlags)(G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL),
    	    	      log_func, /*user_data*/0);
    g_log_set_handler(0,
    	    	      (GLogLevelFlags)(G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL),
    	    	      log_func, /*user_data*/0);
#endif

#if GTK2
    gtk_init(&argc, &argv);
    /* As of 2.0 we don't need to explicitly initialise libGlade anymore */
#else
    gnome_init_with_popt_table(PACKAGE, VERSION, argc, argv,
			       popt_options, /*popt flags*/0,
			       &popt_context);
    glade_gnome_init();
#endif

    parse_args(argc, argv);
    read_gcov_files();

    cov_dump(stderr);
    ui_create();
    gtk_main();

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
