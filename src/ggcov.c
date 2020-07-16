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

#include "common.h"
#include "cov.H"
#include "ui.h"
#include "filename.h"
#include "prefs.H"
#include "estring.H"
#include "tok.H"
#include "sourcewin.H"
#include "summarywin.H"
#include "callswin.H"
#include "callgraphwin.H"
#include "diagwin.H"
#include "callgraph_diagram.H"
#include "lego_diagram.H"
#include "functionswin.H"
#include "fileswin.H"
#include "reportwin.H"
#if HAVE_LIBGNOMEUI
#include <libgnomeui/libgnomeui.h>
#endif
#include "fakepopt.h"
#include "logging.H"

#define DEBUG_GTK 1

char *argv0;

static const char ** debug_argv;
static logging::logger_t &_log = logging::find_logger("uicore");
static logging::logger_t &dump_log = logging::find_logger("dump");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Construct and return a read-only copy of the given commandline
 * argument vector.  It's read-only because the whole vector is
 * collapsed into a single memory allocation for efficiency.
 */

static void
stash_argv(int argc, char **argv)
{
    int i, len;
    char **nargv, *p;

    len = 0;
    for (i = 0 ; i < argc ; i++)
	len += sizeof(char*) + strlen(argv[i]) + 1/*trailing nul char*/;
    len += sizeof(char*)/* trailing NULL ptr */ ;

    nargv = (char **)gnb_xmalloc(len);
    p = (char *)(nargv + argc + 1);

    for (i = 0 ; i < argc ; i++)
    {
	nargv[i] = p;
	strcpy(p, argv[i]);
	p += strlen(p) + 1;
    }
    nargv[i] = 0;

    debug_argv = (const char **)nargv;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
static void add_all_window_names(estring &);

class ggcov_params_t : public cov_project_params_t
{
public:
    ggcov_params_t();
    ~ggcov_params_t();

    ARGPARSE_STRING_PROPERTY(initial_windows);
    ARGPARSE_BOOL_PROPERTY(profile_mode);

protected:
    void setup_parser(argparse::parser_t &parser)
    {
	cov_project_params_t::setup_parser(parser);

	estring w_desc("list of windows to open initially, any of: ");
	add_all_window_names(w_desc);
	parser.add_option('w', "initial-windows")
	      .description(w_desc)
	      .setter((argparse::arg_setter_t)&ggcov_params_t::set_initial_windows)
              .metavar("WINDOW,...");
	parser.add_option(0, "profile")
	      .setter((argparse::noarg_setter_t)&ggcov_params_t::set_profile_mode);
    }

    void add_file(const char *file)
    {
	/* transparently handle file: URLs for Nautilus integration */
	if (!strncmp(file, "file://", 7))
	    file += 7;
	cov_project_params_t::add_file(file);
    }

    void
    post_args()
    {
	cov_project_params_t::post_args();
	if (dump_log.is_enabled(logging::DEBUG2))
	{
	    const char **p;
	    estring buf;

	    for (p = debug_argv ; *p ; p++)
	    {
		if (strpbrk(*p, " \t\"'") == 0)
		    buf.append_printf(" %s", *p);
		else
		    buf.append_printf(" \"%s\"", *p);
	    }
	    _log.debug2("argv[] = {%s }\n", buf.data());
	}
    }

};

ggcov_params_t::ggcov_params_t()
 :  initial_windows_("summary"),
    profile_mode_(FALSE)
{
}

ggcov_params_t::~ggcov_params_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Read a file from the File->Open dialog.
 */
gboolean
ggcov_read_file(const char *filename)
{
    if (file_is_directory(filename) == 0)
    {
	if (!cov_read_directory(filename, /*recursive*/FALSE))
	    return FALSE;
    }
    else if (errno != ENOTDIR)
    {
	perror(filename);
    }
    else if (file_is_regular(filename) == 0)
    {
	if (cov_is_source_filename(filename))
	{
	    if (!cov_read_source_file(filename))
		return FALSE;
	}
	else
	{
	    if (!cov_read_object_file(filename))
		return FALSE;
	}
    }
    else
    {
	_log.error("%s: don't know how to handle this filename\n", filename);
	return FALSE;
    }

    return TRUE;
}

static GtkWidget *open_window = 0;

GLADE_CALLBACK void
on_open_ok_button_clicked(GtkWidget *w, gpointer userdata)
{
    _log.debug("on_open_ok_button_clicked\n");

    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(open_window));

    if (filename && *filename)
    {
	cov_pre_read();
	if (ggcov_read_file(filename))
	    cov_post_read();
    }

    gtk_widget_hide(open_window);

    /*
     * No files loaded: user started ggcov without a commandline
     * argument and failed to load a file in the Open dialog.
     */
    if (!*cov_file_t::first())
	exit(1);

    if (filename)
        g_free(filename);
}

GLADE_CALLBACK void
on_open_cancel_button_clicked(GtkWidget *w, gpointer userdata)
{
    _log.debug("on_open_cancel_button_clicked\n");
    gtk_widget_hide(open_window);

    /*
     * No files loaded: user started ggcov without a commandline
     * argument and cancelled the Open dialog.
     */
    if (!*cov_file_t::first())
	exit(1);
}

GLADE_CALLBACK void
on_file_open_activate(GtkWidget *w, gpointer userdata)
{
    _log.debug("on_file_open_activate\n");

    if (open_window == 0)
    {
	GladeXML *xml;

	xml = ui_load_tree("open");
	open_window = glade_xml_get_widget(xml, "open");
    }

    gtk_widget_show(open_window);
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
    cgw->show();
}

static void
on_windows_new_callgraph2win_activated(GtkWidget *w, gpointer userdata)
{
    diagwin_t *lw;

    lw = new diagwin_t(new callgraph_diagram_t);
    lw->show();
}

static void
on_windows_new_legowin_activated(GtkWidget *w, gpointer userdata)
{
    diagwin_t *lw;

    lw = new diagwin_t(new lego_diagram_t);
    lw->show();
}

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

static void
on_windows_new_reportwin_activated(GtkWidget *w, gpointer userdata)
{
    reportwin_t *rw;

    rw = new reportwin_t();
    rw->show();
}

#include "ui/ggcov32.xpm"

static const struct window
{
    const char *name;
    const char *label;
    void (*create)(GtkWidget*, gpointer);
}
windows[] =
{
    {"summary", N_("New Summary..."), on_windows_new_summarywin_activated},
    {"files", N_("New File List..."), on_windows_new_fileswin_activated},
    {"functions", N_("New Function List..."), on_windows_new_functionswin_activated},
    {"calls", N_("New Calls List..."), on_windows_new_callswin_activated},
    {"callbutterfly", N_("New Call Butterfly..."), on_windows_new_callgraphwin_activated},
    {"callgraph", N_("New Call Graph..."), on_windows_new_callgraph2win_activated},
    {"lego", N_("New Lego Diagram..."), on_windows_new_legowin_activated},
    {"source", N_("New Source..."), on_windows_new_sourcewin_activated},
    {"reports", N_("New Report..."), on_windows_new_reportwin_activated},
    {0, 0, 0}
};

static void add_all_window_names(estring &s)
{
    const struct window *wp;
    for (wp = windows ; wp->name ; wp++)
    {
	if (wp > windows)
	    s.append_char(',');
	s.append_string(wp->name);
    }
}

static void
ui_create(ggcov_params_t &params, const char *full_argv0, int successes)
{
    /*
     * If we're being run from the source directory, fiddle the
     * glade search path to point to ../ui first.  This means
     * we can run ggcov before installation without having to
     * compile with UI_DEBUG=1.
     */
    estring dir = file_make_absolute(full_argv0);
    const char *p = strrchr(dir, '/');
    if (p != 0 &&
	(p -= 4) >= dir.data() &&
	!strncmp(p, "/src/", 5))
    {
	dir.truncate_to(p - dir);
	dir.append_string("/ui");
	if (!file_is_directory(dir))
	{
	    _log.info("running from source directory, "
		      "so prepending %s to glade search path\n",
		      dir.data());
	    ui_prepend_glade_path(dir);
	}
    }

    const struct window *wp;
    for (wp = windows ; wp->name ; wp++)
	ui_register_windows_entry(_(wp->label), wp->create, 0);

    ui_set_default_icon(ggcov32_xpm);

    prefs.load();

    if (!successes)
    {
	/* No files discovered from commandline...show the File->Open dialog to get some */
	on_file_open_activate(0, 0);
	while (!*cov_file_t::first())
	    gtk_main_iteration();
    }

    /* Possibly have files from commandline or dialog...show initial windows */
    tok_t tok(params.get_initial_windows(), ", \n\r");
    const char *name;
    int nwindows = 0;
    while ((name = tok.next()) != 0)
    {
	for (wp = windows ; wp->name ; wp++)
	{
	    if (!strcmp(name, wp->name))
	    {
		wp->create(0, 0);
		nwindows++;
		break;
	    }
	}
	if (!wp->name)
	    _log.warning("unknown window name \"%s\"\n", name);
    }

    if (!nwindows)
	on_windows_new_summarywin_activated(0, 0);

    if (params.get_profile_mode())
    {
	while (g_main_context_iteration(NULL, FALSE))
	    ;
	exit(0);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    ui_log_init();

    /* stash a copy of argv[] in case we want to dump it for debugging */
    stash_argv(argc, argv);

    ggcov_params_t params;

#if HAVE_GTK_INIT_WITH_ARGS
    argparse::goption_parser_t parser(params);
    GError *error = NULL;
    gboolean res = gtk_init_with_args(
	    &argc, &argv,
	    "sourcefile|executable|directory",
	    parser.get_goption_table(),
	    /*translation_domain*/NULL,
	    &error);
    if (error)
        _log.error(error->message);
    /* hopefully gtk either returned a GError or logged why it failed */
    if (error || !res)
        exit(1);
    parser.post_args();
#elif HAVE_GNOME_PROGRAM_INIT
    argparse::popt_parser_t parser(params);
    GnomeProgram *prog;
    poptContext popt_context;

    prog = gnome_program_init(PACKAGE, VERSION, LIBGNOMEUI_MODULE,
			      argc, argv,
			      GNOME_PARAM_POPT_TABLE, parser.get_popt_table(),
			      GNOME_PROGRAM_STANDARD_PROPERTIES,
			      GNOME_PARAM_NONE);
    g_object_get(prog, GNOME_PARAM_POPT_CONTEXT, &popt_context, (char *)0);
    parser.handle_popt_tail(popt_context);
#else
    argparse::popt_parser_t parser(params);
    gtk_init(&argc, &argv);
    /* As of 2.0 we don't need to explicitly initialise libGlade anymore */
    parser.parse(argc, argv);
#endif

    int r = cov_read_files(params);
    if (r < 0)
	exit(1);    /* error message in cov_read_files() */

    cov_dump();
    ui_create(params, argv[0], r);
    gtk_main();

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
