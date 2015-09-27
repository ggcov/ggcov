/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2004 Greg Banks <gnb@users.sourceforge.net>
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
    void load_state();
    void save_state();
    void apply_toggles();
    void on_blocks_check_activate();
    void on_lines_check_activate();
    void on_functions_check_activate();
    void on_calls_check_activate();
    void on_branches_check_activate();
    void on_percent_check_activate();
    void on_tree_check_activate();
    void on_collapse_all_activate();
    void on_expand_all_activate();
    gboolean on_ctree_button_press_event(GdkEvent *event);

    file_rec_t *root_;

    GtkWidget *blocks_check_;
    GtkWidget *lines_check_;
    GtkWidget *functions_check_;
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
    gboolean populating_;
};

#endif /* _ggcov_fileswin_H_ */
