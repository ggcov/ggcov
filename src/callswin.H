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

#ifndef _ggcov_callswin_H_
#define _ggcov_callswin_H_ 1

#include "window.H"
#include "cov.H"

GLADE_CALLBACK void on_calls_close_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_calls_exit_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_calls_from_function_entry_changed(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_calls_to_function_entry_changed(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_calls_from_function_view_clicked(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_calls_to_function_view_clicked(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_calls_clist_button_press_event(GtkWidget *w, GdkEvent *event, gpointer data);

class callswin_t : public window_t
{
public:
    callswin_t();
    ~callswin_t();

private:
    void populate();
    void update();
    void populate_function_combo(GtkCombo *combo);
    void update_for_func(cov_function_t *from_fn, cov_function_t *to_fn);
    static callswin_t *from_widget(GtkWidget *w)
    {
    	return (callswin_t *)window_t::from_widget(w);
    }


    GtkWidget *from_function_combo_;
    GtkWidget *from_function_view_;

    GtkWidget *to_function_combo_;
    GtkWidget *to_function_view_;

    GtkWidget *clist_;
    GList *functions_;	/* list of cov_function_t */

    friend void on_calls_close_activate(GtkWidget *w, gpointer data);
    friend void on_calls_exit_activate(GtkWidget *w, gpointer data);
    friend void on_calls_from_function_entry_changed(GtkWidget *w, gpointer data);
    friend void on_calls_to_function_entry_changed(GtkWidget *w, gpointer data);
    friend void on_calls_from_function_view_clicked(GtkWidget *w, gpointer data);
    friend void on_calls_to_function_view_clicked(GtkWidget *w, gpointer data);
    friend void on_calls_clist_button_press_event(GtkWidget *w, GdkEvent *event, gpointer data);
};

#endif /* _ggcov_callswin_H_ */
