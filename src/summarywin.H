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

#ifndef _ggcov_summarywin_H_
#define _ggcov_summarywin_H_ 1

#include "window.H"

typedef enum
{
    SU_OVERALL,
    SU_FILENAME,
    SU_FUNCTION,
    SU_RANGE,

    SU_NSCOPES
} summarywin_scope_t;
    
GLADE_CALLBACK void on_summary_close_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_exit_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_overall_radio_toggled(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_filename_radio_toggled(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_filename_entry_changed(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_filename_view_clicked(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_function_radio_toggled(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_function_entry_changed(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_function_view_clicked(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_range_radio_toggled(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_range_entry_changed(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_range_start_spin_changed(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_range_end_spin_changed(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_summary_range_view_clicked(GtkWidget *w, gpointer data);

class summarywin_t : public window_t
{
public:
    summarywin_t();
    ~summarywin_t();

private:
    void populate();
    void spin_update();
    void update();
    static summarywin_t *from_widget(GtkWidget *w)
    {
    	return (summarywin_t *)window_t::from_widget(w);
    }
    void get_range(char **filenamep, unsigned long *startlinep,
    	    	   unsigned long *endlinep);
    void grey_items();

    summarywin_scope_t scope_;
    GtkWidget *scope_radio_[SU_NSCOPES];
    
    GtkWidget *filename_combo_;
    GtkWidget *filename_view_;
    GtkWidget *function_combo_;
    GtkWidget *function_view_;
    GtkWidget *range_combo_;
    GtkWidget *range_start_spin_;
    GtkWidget *range_end_spin_;
    GtkWidget *range_view_;

    GtkWidget *lines_label_;
    GtkWidget *lines_progressbar_;
    GtkWidget *calls_label_;
    GtkWidget *calls_progressbar_;
    GtkWidget *branches_executed_label_;
    GtkWidget *branches_executed_progressbar_;
    GtkWidget *branches_taken_label_;
    GtkWidget *branches_taken_progressbar_;

    friend void on_summary_close_activate(GtkWidget *w, gpointer data);
    friend void on_summary_exit_activate(GtkWidget *w, gpointer data);
    friend void on_summary_overall_radio_toggled(GtkWidget *w, gpointer data);
    friend void on_summary_filename_radio_toggled(GtkWidget *w, gpointer data);
    friend void on_summary_filename_entry_changed(GtkWidget *w, gpointer data);
    friend void on_summary_filename_view_clicked(GtkWidget *w, gpointer data);
    friend void on_summary_function_radio_toggled(GtkWidget *w, gpointer data);
    friend void on_summary_function_entry_changed(GtkWidget *w, gpointer data);
    friend void on_summary_function_view_clicked(GtkWidget *w, gpointer data);
    friend void on_summary_range_radio_toggled(GtkWidget *w, gpointer data);
    friend void on_summary_range_entry_changed(GtkWidget *w, gpointer data);
    friend void on_summary_range_start_spin_changed(GtkWidget *w, gpointer data);
    friend void on_summary_range_end_spin_changed(GtkWidget *w, gpointer data);
    friend void on_summary_range_view_clicked(GtkWidget *w, gpointer data);
};

#endif /* _ggcov_summarywin_H_ */
