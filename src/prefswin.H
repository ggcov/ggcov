/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2020 Greg Banks <gnb@fastmail.fm>
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

#ifndef _ggcov_prefswin_H_
#define _ggcov_prefswin_H_ 1

#include "window.H"

class prefswin_t : public window_t
{
public:
    ~prefswin_t();

    static prefswin_t *instance();

private:
    prefswin_t();

    void populate();
    void update_picker(int i, const GdkColor *);
    void update();
    void grey_items();
    void apply_picker(int i, GdkColor *);
    void apply();
    void on_ok_clicked();
    void on_apply_clicked();
    void on_cancel_clicked();

    static prefswin_t *instance_;

    GtkWidget *reuse_srcwin_check_;
    GtkWidget *reuse_summwin_check_;

    GtkWidget *color_pickers_[10];
};

#endif /* _ggcov_prefswin_H_ */
