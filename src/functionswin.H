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

#ifndef _ggcov_functionswin_H_
#define _ggcov_functionswin_H_ 1

#include "window.H"
#include "cov.H"

class functionswin_t : public window_t
{
public:
    functionswin_t();
    ~functionswin_t();
    
private:
    void populate();
    void update();
    void load_state();
    void save_state();
    void apply_toggles();
    void on_blocks_check_activate();
    void on_lines_check_activate();
    void on_calls_check_activate();
    void on_branches_check_activate();
    void on_percent_check_activate();

    list_t<cov_function_scope_t> functions_;
    
    GtkWidget *blocks_check_;
    GtkWidget *lines_check_;
    GtkWidget *calls_check_;
    GtkWidget *branches_check_;
    GtkWidget *percent_check_;
    GtkWidget *clist_;
#if GTK2
    GtkListStore *store_;
#endif
    gboolean populating_;
};

#endif /* _ggcov_functionswin_H_ */
