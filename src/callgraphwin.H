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

#ifndef _ggcov_callgraphwin_H_
#define _ggcov_callgraphwin_H_ 1

#include "window.H"
#include "cov.h"

GLADE_CALLBACK void on_callgraph_close_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_callgraph_exit_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_callgraph_function_entry_changed(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_callgraph_function_view_clicked(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_callgraph_ancestors_clist_button_press_event(
			  GtkWidget *w, GdkEventButton *event, gpointer data);
GLADE_CALLBACK void on_callgraph_descendants_clist_button_press_event(
			  GtkWidget *w, GdkEventButton *event, gpointer data);

class callgraphwin_t : public window_t
{
public:
    callgraphwin_t();
    ~callgraphwin_t();
    
    void set_node(cov_callnode_t *);

private:
    void populate_function_combo(GtkCombo *combo);
    void populate();
    static void init_clist(GtkCList *clist,
    	    	    	   int (*sortfn)(GtkCList *, const void*, const void*));
    static void update_clist(GtkWidget *clist, GList *arcs, gboolean isin);
    void update();
    static callgraphwin_t *from_widget(GtkWidget *w)
    {
    	return (callgraphwin_t *)window_t::from_widget(w);
    }
    
    cov_callnode_t *callnode_;

    GtkWidget *function_combo_;
    GtkWidget *function_view_;

    GtkWidget *ancestors_clist_;
    GtkWidget *descendants_clist_;

    friend void on_callgraph_close_activate(GtkWidget *w, gpointer data);
    friend void on_callgraph_exit_activate(GtkWidget *w, gpointer data);
    friend void on_callgraph_function_entry_changed(GtkWidget *w, gpointer data);
    friend void on_callgraph_function_view_clicked(GtkWidget *w, gpointer data);
    friend void on_callgraph_ancestors_clist_button_press_event(
			      GtkWidget *w, GdkEventButton *event, gpointer data);
    friend void on_callgraph_descendants_clist_button_press_event(
			      GtkWidget *w, GdkEventButton *event, gpointer data);
};

#endif /* _ggcov_callgraphwin_H_ */