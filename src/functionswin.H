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

#ifndef _ggcov_functionswin_H_
#define _ggcov_functionswin_H_ 1

#include "window.H"
#include "cov.H"

GLADE_CALLBACK void on_functions_close_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_functions_exit_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_functions_lines_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_functions_calls_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_functions_branches_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_functions_percent_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_functions_clist_button_press_event(GtkWidget *w, GdkEvent *event, gpointer data);

class functionswin_t : public window_t
{
public:
    functionswin_t();
    ~functionswin_t();
    
private:
    void populate();
    void update();
    static void add_functions(cov_file_t *f, void *userdata);
    static functionswin_t *from_widget(GtkWidget *w)
    {
    	return (functionswin_t *)window_t::from_widget(w);
    }


    GList *functions_;	    /* list of func_rec_t */
    
    GtkWidget *lines_check_;
    GtkWidget *calls_check_;
    GtkWidget *branches_check_;
    GtkWidget *percent_check_;
    GtkWidget *clist_;

    friend void on_functions_close_activate(GtkWidget *w, gpointer data);
    friend void on_functions_exit_activate(GtkWidget *w, gpointer data);
    friend void on_functions_lines_check_activate(GtkWidget *w, gpointer data);
    friend void on_functions_calls_check_activate(GtkWidget *w, gpointer data);
    friend void on_functions_branches_check_activate(GtkWidget *w, gpointer data);
    friend void on_functions_percent_check_activate(GtkWidget *w, gpointer data);
    friend void on_functions_clist_button_press_event(GtkWidget *w, GdkEvent *event, gpointer data);
};

#endif /* _ggcov_functionswin_H_ */
