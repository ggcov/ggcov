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

#include "window.H"
#include "cov.H"
#include "mvc.h"
#include "tok.H"

CVSID("$Id: window.C,v 1.11 2003-07-19 09:59:11 gnb Exp $");

static const char window_key[] = "ggcov_window_key";

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

window_t::window_t()
{
    mvc_listen(cov_file_t::files_model(), ~0, files_changed, this);
}

window_t::~window_t()
{
    /* JIC of strange gui stuff */
    assert(!deleting_);
    deleting_ = TRUE;

    mvc_unlisten(cov_file_t::files_model(), ~0, files_changed, this);
    
    assert(window_ != 0);
    gtk_widget_destroy(window_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern gboolean ggcov_read_file(const char *filename);

/*
 * Parse the given string as a list of URIs and load each file
 * into the global cov* data structures.
 */
static void
dnd_handle_uri_list(char *uri_list)
{
    tok_t tok(uri_list, "\n\r");
    int nfiles = 0;
    const char *uri;

    cov_pre_read();
    while ((uri = tok.next()) != 0)
    {
#if DEBUG
    	fprintf(stderr, "uri=\"%s\"\n", uri);
#endif
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
 *  	some kind of (?) binary formatted list of URLs and other
 *  	icon-related information which doesn't matter.
 * text/uri-list
 *  	list of URLs one per line.
 * _NETSCAPE_URL
 *  	single URL (no matter how many files are actually dragged).
 *
 * So we support text/uri-list because it's both simple and works.
 */
#define URI_LIST     	    2
static const GtkTargetEntry dnd_targets[] = 
{
    {"text/uri-list", 0, URI_LIST}
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
    char *text;
    
#if GTK2
    text = (char *)gtk_selection_data_get_text(data);
#else
    text = (char *)g_malloc(data->length+1);
    memcpy(text, data->data, data->length);
    text[data->length] = '\0';
#endif

#if DEBUG
    fprintf(stderr, "dnd_drag_data_received: info=%d, text=\"%s\"\n", info, text);
#endif

    switch (info)
    {
    case URI_LIST:
	dnd_handle_uri_list(text);
	break;
    default:
    	g_free(text);
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
    
    fprintf(stderr, "dnd_drag_motion: x=%d y=%d targets={", x, y);

    for (iter = drag_context->targets ; iter != 0 ; iter = iter->next)
    {
	gchar *name = gdk_atom_name(GPOINTER_TO_INT(iter->data));
	fprintf(stderr, "\"%s\"%s", name, (iter->next == 0 ? "" : ", "));
	g_free (name);
    }
    fprintf(stderr, "}\n");
    
    gdk_drag_status(drag_context, GDK_ACTION_COPY, GDK_CURRENT_TIME);

    return TRUE;
}
#endif /* DEBUG_DND_TARGETS */


static void
dnd_setup(GtkWidget *w)
{
    GtkDestDefaults defaults;
    
    defaults = GTK_DEST_DEFAULT_ALL;
#if DEBUG_DND_TARGETS
    defaults &= ~GTK_DEST_DEFAULT_MOTION,
#endif /* DEBUG_DND_TARGETS */

    gtk_drag_dest_set(w, defaults,
    	    	      dnd_targets, sizeof(dnd_targets)/sizeof(dnd_targets[0]),
		      GDK_ACTION_COPY);

#if DEBUG_DND_TARGETS
    gtk_signal_connect(GTK_OBJECT(w), "drag_motion",
	    GTK_SIGNAL_FUNC(dnd_drag_motion), 0);
#endif /* DEBUG_DND_TARGETS */

    gtk_signal_connect(GTK_OBJECT(w), "drag-data-received",
	    GTK_SIGNAL_FUNC(dnd_drag_data_received), 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
window_t::attach(GtkWidget *w)
{
    gtk_object_set_data(GTK_OBJECT(w), window_key, this);
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

window_t *
window_t::from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : (window_t *)gtk_object_get_data(GTK_OBJECT(w), window_key));
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
window_t::show()
{
    if (!shown_)
	populate();
    gtk_widget_show(window_);
    if (shown_)
    	gdk_window_raise(window_->window);
    shown_ = TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_window_close_activate(GtkWidget *w, gpointer data)
{
    window_t *win = window_t::from_widget(w);
    
    assert(win != 0);
    delete win;
}

GLADE_CALLBACK void
on_window_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
