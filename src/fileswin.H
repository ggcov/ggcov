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

#ifndef _ggcov_fileswin_H_
#define _ggcov_fileswin_H_ 1

#include "window.H"
#include "cov.H"

GLADE_CALLBACK void on_files_lines_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_files_calls_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_files_branches_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_files_percent_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_files_tree_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_files_collapse_all_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_files_expand_all_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_files_clist_button_press_event(GtkWidget *w, GdkEvent *event, gpointer data);

struct file_rec_t;

class fileswin_t : public window_t
{
public:
    fileswin_t();
    ~fileswin_t();

private:
    void populate();
    void update();
    void grey_items();
    void add_node(file_rec_t*, file_rec_t*, gboolean, gboolean);
    static fileswin_t *from_widget(GtkWidget *w)
    {
    	return (fileswin_t *)window_t::from_widget(w);
    }

    file_rec_t *root_;
    
    GtkWidget *lines_check_;
    GtkWidget *calls_check_;
    GtkWidget *branches_check_;
    GtkWidget *percent_check_;
    GtkWidget *tree_check_;
    GtkWidget *collapse_all_btn_;
    GtkWidget *expand_all_btn_;
    GtkWidget *ctree_;
#if GTK2
    GtkTreeStore *store_;
#endif

    friend void on_files_lines_check_activate(GtkWidget *w, gpointer data);
    friend void on_files_calls_check_activate(GtkWidget *w, gpointer data);
    friend void on_files_branches_check_activate(GtkWidget *w, gpointer data);
    friend void on_files_percent_check_activate(GtkWidget *w, gpointer data);
    friend void on_files_tree_check_activate(GtkWidget *w, gpointer data);
    friend void on_files_collapse_all_activate(GtkWidget *w, gpointer data);
    friend void on_files_expand_all_activate(GtkWidget *w, gpointer data);
    friend void on_files_clist_button_press_event(GtkWidget *w, GdkEvent *event, gpointer data);
};

#endif /* _ggcov_fileswin_H_ */
