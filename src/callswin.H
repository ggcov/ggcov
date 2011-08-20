/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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

class callswin_t : public window_t
{
public:
    callswin_t();
    ~callswin_t();

private:
    void populate();
    void update();
    void update_for_func(cov_function_t *from_fn, cov_function_t *to_fn);
    void apply_toggles();
    void load_state();
    void save_state();
    void on_call_from_check_activate();
    void on_call_to_check_activate();
    void on_line_check_activate();
    void on_arc_check_activate();
    void on_count_check_activate();
    void on_from_function_combo_changed();
    void on_to_function_combo_changed();
    void on_from_function_view_clicked();
    void on_to_function_view_clicked();

    ui_combo_t *from_function_combo_;
    GtkWidget *from_function_view_;

    ui_combo_t *to_function_combo_;
    GtkWidget *to_function_view_;

    GtkWidget *from_check_;
    GtkWidget *to_check_;
    GtkWidget *line_check_;
    GtkWidget *count_check_;
    gboolean populating_;

    GtkWidget *clist_;
#if GTK2
    GtkListStore *store_;
#endif
    list_t<cov_function_t> *functions_;
};

#endif /* _ggcov_callswin_H_ */
