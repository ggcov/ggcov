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

CVSID("$Id: window.C,v 1.9 2003-07-18 13:36:55 gnb Exp $");

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
