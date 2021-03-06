/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2020 Greg Banks <gnb@fastmail.fm>
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

#include "window.H"
#include "cov.H"
#include "mvc.h"
#include "tok.H"
#include "string_var.H"
#include "confsection.H"
#include "prefs.H"
#include "logging.H"

static const char window_key[] = "ggcov_window_key";

static list_t<window_t> all;
static logging::logger_t &_log = logging::find_logger("uicore");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

window_t::window_t()
{
    mvc_listen(cov_file_t::files_model(), ~0, files_changed, this);
    all.prepend(this);
}

window_t::~window_t()
{
    /* JIC of strange gui stuff */
    assert(!deleting_);
    deleting_ = true;

    mvc_unlisten(cov_file_t::files_model(), ~0, files_changed, this);

    assert(window_);
    gtk_widget_destroy(window_);
    all.remove(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *
dnd_data_as_text(const void *data, unsigned int length)
{
    char *text;

    text = (char *)gnb_xmalloc(length+1);
    memcpy(text, data, length);
    text[length] = '\0';

    return text;
}

extern gboolean ggcov_read_file(const char *filename);

/*
 * Parse the given string as a list of URIs and load each file
 * into the global cov* data structures.
 */
static void
dnd_handle_uri_list(const void *data, unsigned int length)
{
    tok_t tok(dnd_data_as_text(data, length), "\n\r");
    int nfiles = 0;
    const char *uri;

    cov_pre_read();
    while ((uri = tok.next()) != 0)
    {
	_log.debug("dnd_handle_uri_list: uri=\"%s\"\n", uri);
	if (!strncmp(uri, "file:", 5) && ggcov_read_file(uri+5))
	    nfiles++;
    }
    if (nfiles)
	cov_post_read();
}

/*
 * Experiment shows that Nautilus on RH7.3 supports the following targets:
 *
 * x-special/gnome-icon-list
 *      some kind of (?) binary formatted list of URLs and other
 *      icon-related information which doesn't matter.
 * text/uri-list
 *      list of URLs one per line.
 * _NETSCAPE_URL
 *      single URL (no matter how many files are actually dragged).
 *
 * So we support text/uri-list because it's both simple and works.
 */
#define URI_LIST            2
static const GtkTargetEntry dnd_targets[] =
{
    {(char *)"text/uri-list", 0, URI_LIST}
};

void
dnd_drag_data_received(
    GtkWidget *widget,
    GdkDragContext *drag_context,
    gint x,
    gint y,
    GtkSelectionData *data,
    guint info,
    guint time,
    gpointer user_data)
{
    if (_log.is_enabled(logging::DEBUG))
    {
	string_var selection_str = gdk_atom_name(gtk_selection_data_get_selection(data));
	string_var target_str = gdk_atom_name(gtk_selection_data_get_target(data));
	string_var type_str = gdk_atom_name(gtk_selection_data_get_data_type(data));
	_log.debug("dnd_drag_data_received: info=%d, "
		  "data={selection=%s target=%s type=%s "
		  "data=0x%p length=%d format=%d}\n",
		  info,
		  selection_str.data(), target_str.data(), type_str.data(),
		  gtk_selection_data_get_data(data),
                  gtk_selection_data_get_length(data),
                  gtk_selection_data_get_format(data));
    }

    switch (info)
    {
    case URI_LIST:
	dnd_handle_uri_list(
          gtk_selection_data_get_data(data),
          gtk_selection_data_get_length(data));
	break;
    }
}

/* Define this to 1 to enable debugging of DnD targets from sources */
#define DEBUG_DND_TARGETS 0

#if DEBUG_DND_TARGETS
static gboolean
dnd_drag_motion(
    GtkWidget *widget,
    GdkDragContext *drag_context,
    gint x,
    gint y,
    guint time,
    gpointer user_data)
{
    GList *iter;
    estring buf;

    for (iter = drag_context->targets ; iter != 0 ; iter = iter->next)
    {
	gchar *name = gdk_atom_name((GdkAtom)GPOINTER_TO_INT(iter->data));
	buf.append_printf(" \"%s\"", name);
	g_free(name);
    }
    _log.debug("dnd_drag_motion: x=%d y=%d targets={%s }\n", x, y, buf.data());

    gdk_drag_status(drag_context, GDK_ACTION_COPY, GDK_CURRENT_TIME);

    return TRUE;
}
#endif /* DEBUG_DND_TARGETS */


static void
dnd_setup(GtkWidget *w)
{
    GtkDestDefaults defaults;

#if DEBUG_DND_TARGETS
    defaults = (GtkDestDefaults)
		(GTK_DEST_DEFAULT_ALL & ~GTK_DEST_DEFAULT_MOTION);
#else /* !DEBUG_DND_TARGETS */
    defaults = GTK_DEST_DEFAULT_ALL;
#endif /* !DEBUG_DND_TARGETS */

    gtk_drag_dest_set(w, defaults,
		      dnd_targets, sizeof(dnd_targets)/sizeof(dnd_targets[0]),
		      GDK_ACTION_COPY);

#if DEBUG_DND_TARGETS
    g_signal_connect(G_OBJECT(w), "drag_motion",
	    G_CALLBACK(dnd_drag_motion), 0);
#endif /* DEBUG_DND_TARGETS */

    g_signal_connect(G_OBJECT(w), "drag-data-received",
	    G_CALLBACK(dnd_drag_data_received), 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
window_t::attach(GtkWidget *w)
{
    g_object_set_data(G_OBJECT(w), window_key, this);

    g_signal_connect(G_OBJECT(w), "configure-event",
		     G_CALLBACK(on_configure_event), this);
}

void
window_t::set_window(GtkWidget *w)
{
    assert(GTK_IS_WINDOW(w));
    window_ = w;
    attach(window_);
    ui_register_window(window_);
    dnd_setup(w);
}

void
window_t::set_title(const char *file)
{
    ui_window_set_title(window_, file);
}

const char *
window_t::name() const
{
    if (window_)
	return gtk_widget_get_name(GTK_WIDGET(window_));
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

window_t *
window_t::from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : (window_t *)g_object_get_data(G_OBJECT(w), window_key));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
window_t::grey_items()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
window_t::populate()
{
}

/*
 * Call the populate() method when the set of files changes,
 * e.g. a new is loaded from the File->Open dialog.
 */
void
window_t::files_changed(void *obj, unsigned int features, void *closure)
{
    ((window_t *)closure)->populate();
}

void
window_t::load_geometry()
{
    assert(window_);
    confsection_t *cs = confsection_t::get("window-geometry");
    string_var geom = cs->get_string(name(), 0);
    if (!geom.data())
	return;
    gtk_window_parse_geometry(GTK_WINDOW(window_), geom);
}

void
window_t::save_geometry()
{
    if (!geom_dirty_)
	return;
    assert(window_);
    int x, y, w, h;
    gtk_window_get_size(GTK_WINDOW(window_), &w, &h);
    gtk_window_get_position(GTK_WINDOW(window_), &x, &y);
    estring geom;
    geom.append_printf("=%dx%d+%d+%d", w, h, x, y);
    confsection_t *cs = confsection_t::get("window-geometry");
    cs->set_string(name(), geom.data());
    confsection_t::sync();
    geom_dirty_ = FALSE;
}

void
window_t::show()
{
    if (!shown_)
    {
	load_state();
	populate();
	load_geometry();
    }
    gtk_widget_show(window_);
    if (shown_)
	gdk_window_raise(gtk_widget_get_window(window_));
    shown_ = true;

    prefs.post_load(window_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
window_t::on_close_activate()
{
    save_geometry();
    delete this;
}

GLADE_CALLBACK void
window_t::on_delete_event(GdkEvent *ev)
{
    save_geometry();
    delete this;
}

GLADE_CALLBACK void
window_t::on_exit_activate()
{
    for (list_iterator_t<window_t> itr = all.first() ; *itr ; ++itr)
	(*itr)->save_geometry();
    gtk_main_quit();
}

gboolean
window_t::on_configure_event(GtkWidget *w,
			     GdkEventConfigure *ev, gpointer closure)
{
    window_t *win = (window_t *)closure;

    if (win->geom_.w &&
	(win->geom_.x != ev->x ||
	 win->geom_.y != ev->y ||
	 win->geom_.w != ev->width ||
	 win->geom_.h != ev->height))
	win->geom_dirty_ = TRUE;
    win->geom_.x = ev->x;
    win->geom_.y = ev->y;
    win->geom_.w = ev->width;
    win->geom_.h = ev->height;
    return FALSE;       /* propagate the event please */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
window_t::load_state()
{
}

void
window_t::save_state()
{
}

void
load(GtkCheckMenuItem *cmi)
{
    confsection_t *cs = confsection_t::get("gtk-toggles");
    boolean b = cs->get_bool(
        gtk_widget_get_name(GTK_WIDGET(cmi)),
        gtk_check_menu_item_get_active(cmi));
    gtk_check_menu_item_set_active(cmi, b);
}

void
save(GtkCheckMenuItem *cmi)
{
    confsection_t *cs = confsection_t::get("gtk-toggles");
    boolean b = !!gtk_check_menu_item_get_active(cmi);
    cs->set_bool(gtk_widget_get_name(GTK_WIDGET(cmi)), b);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
