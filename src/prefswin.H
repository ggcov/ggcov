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

#ifndef _ggcov_prefswin_H_
#define _ggcov_prefswin_H_ 1

#include "window.H"

GLADE_CALLBACK void on_preferences_ok_clicked(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_preferences_apply_clicked(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_preferences_cancel_clicked(GtkWidget *w, gpointer data);

class prefswin_t : public window_t
{
public:
    ~prefswin_t();

    static prefswin_t *instance();

private:
    prefswin_t();

    void populate();
    static prefswin_t *from_widget(GtkWidget *w)
    {
    	return (prefswin_t *)window_t::from_widget(w);
    }
    void update_picker(int i, const GdkColor *);
    void update();
    void grey_items();
    void apply_picker(int i, GdkColor *);
    void apply();
    
    static prefswin_t *instance_;

    GtkWidget *reuse_srcwin_check_;
    GtkWidget *reuse_summwin_check_;

    GtkWidget *color_pickers_[10];

    friend void on_preferences_ok_clicked(GtkWidget *w, gpointer data);
    friend void on_preferences_apply_clicked(GtkWidget *w, gpointer data);
    friend void on_preferences_cancel_clicked(GtkWidget *w, gpointer data);
};

#endif /* _ggcov_prefswin_H_ */
