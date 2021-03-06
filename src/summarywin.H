/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
#include "cov.H"

class summarywin_t : public window_t
{
public:
    summarywin_t();
    ~summarywin_t();

    enum scope_t
    {
	SU_OVERALL,
	SU_FILENAME,
	SU_FUNCTION,
	SU_RANGE,

	SU_NSCOPES
    };

    static void show_overall();
    static void show_file(const cov_file_t *);
    static void show_function(const cov_function_t *);
    static void show_lines(const char *filename, unsigned long start,
			   unsigned long end);

private:
    static summarywin_t *instance();
    void populate_filename_combo(ui_combo_t *);
    void populate_function_combo(ui_combo_t *);
    void populate();
    void spin_update();
    void update();
    void grey_items();
    void on_overall_radio_toggled();
    void on_filename_radio_toggled();
    void on_filename_combo_changed();
    void on_filename_view_clicked();
    void on_function_radio_toggled();
    void on_function_combo_changed();
    void on_function_view_clicked();
    void on_range_radio_toggled();
    void on_range_combo_changed();
    void on_range_start_spin_changed();
    void on_range_end_spin_changed();
    void on_range_view_clicked();

    static list_t<summarywin_t> instances_;

    scope_t scope_;
    const cov_file_t *file_;            /* SU_FILENAME, SU_RANGE */
    const cov_function_t *function_;    /* SU_FUNCTION */
    unsigned long start_, end_;         /* SU_RANGE */

    GtkWidget *scope_radio_[SU_NSCOPES];

    ui_combo_t *filename_combo_;
    GtkWidget *filename_view_;
    ui_combo_t *function_combo_;
    GtkWidget *function_view_;
    ui_combo_t *range_combo_;
    GtkWidget *range_start_spin_;
    GtkWidget *range_end_spin_;
    GtkWidget *range_view_;
    gboolean populating_;

    GtkWidget *lines_label_;
    GtkWidget *lines_pc_label_;
    GtkWidget *lines_bar_;
    GtkWidget *functions_label_;
    GtkWidget *functions_pc_label_;
    GtkWidget *functions_bar_;
    GtkWidget *calls_label_;
    GtkWidget *calls_pc_label_;
    GtkWidget *calls_bar_;
    GtkWidget *branches_label_;
    GtkWidget *branches_pc_label_;
    GtkWidget *branches_bar_;
    GtkWidget *blocks_label_;
    GtkWidget *blocks_pc_label_;
    GtkWidget *blocks_bar_;
};

#endif /* _ggcov_summarywin_H_ */
