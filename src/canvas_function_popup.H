/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@alphalink.com.au>
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

#ifndef _ggcov_canvas_function_popup_H_
#define _ggcov_canvas_function_popup_H_ 1

#include "common.h"
#include "ui.h"
#include <libgnomeui/libgnomeui.h>

class cov_function_t;

GLADE_CALLBACK void on_canfn_view_summary_activate(GtkWidget *, GdkEvent *, gpointer);
GLADE_CALLBACK void on_canfn_view_call_butterfly_activate(GtkWidget *, GdkEvent *, gpointer);
GLADE_CALLBACK void on_canfn_view_source_activate(GtkWidget *, GdkEvent *, gpointer);

class canvas_function_popup_t
{
public:
    canvas_function_popup_t(GnomeCanvasItem *item, cov_function_t *fn);
    ~canvas_function_popup_t();

    void set_foreground(const char *);
    void set_background(const char *);

private:
    static gboolean on_leave_event(GtkWidget *, GdkEvent *, gpointer);
    static gboolean on_button_press_event(GtkWidget *, GdkEvent *, gpointer);
    static gboolean on_item_event(GnomeCanvasItem *, GdkEvent *, gpointer);
    static void on_item_destroy(GnomeCanvasItem *, gpointer);
    void show(GdkEvent *event);
    void show_menu(GdkEvent *event);

    struct widgets_t
    {
    	int refcount_;
	canvas_function_popup_t *current_;
	GtkWidget *popup_;
	GtkWidget *border_;
	GtkWidget *background_;
	GtkWidget *function_label_;
	GtkWidget *source_label_;
	GtkWidget *coverage_label_;
	GtkWidget *menu_;
    };
    static widgets_t widgets_;
    
    GnomeCanvasItem *item_;
    cov_function_t *function_;
    char foreground_[32];
    char background_[32];
    char coverage_[128];

    friend void on_canfn_view_summary_activate(GtkWidget *, GdkEvent *, gpointer);
    friend void on_canfn_view_call_butterfly_activate(GtkWidget *, GdkEvent *, gpointer);
    friend void on_canfn_view_source_activate(GtkWidget *, GdkEvent *, gpointer);
};

#endif /* _ggcov_canvas_function_popup_H_ */
