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

CVSID("$Id: window.C,v 1.6 2003-03-17 03:54:49 gnb Exp $");

static const char window_key[] = "callgraph2win_key";

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

window_t::window_t()
{
}

window_t::~window_t()
{
    /* JIC of strange gui stuff */
    assert(!deleting_);
    deleting_ = TRUE;
    
    assert(window_ != 0);
    gtk_widget_destroy(window_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
window_t::set_window(GtkWidget *w)
{
    assert(GTK_IS_WINDOW(w));
    window_ = w;
    gtk_object_set_data(GTK_OBJECT(window_), window_key, this);
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
/*END*/
