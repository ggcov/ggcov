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

#ifndef _ggcov_window_H_
#define _ggcov_window_H_ 1

#include "ui.h"

GLADE_CALLBACK void on_window_close_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_window_exit_activate(GtkWidget *w, gpointer data);

class window_t
{
protected:
    window_t();
public:
    virtual ~window_t();

    virtual void show();

    GtkWidget *get_window() const
    {
    	return window_;
    }

protected:
    virtual void populate();
    static window_t *from_widget(GtkWidget *);
    virtual void grey_items();
    void attach(GtkWidget *);
    void set_window(GtkWidget *);
    void set_title(const char *);

    GtkWidget *window_;
    gboolean deleting_:1;    /* handle possible GUI recursion crap */
    gboolean shown_:1;	     /* has been shown()n at least once */

    friend void on_window_close_activate(GtkWidget *w, gpointer data);
    friend void on_window_exit_activate(GtkWidget *w, gpointer data);
};

#endif /* _ggcov_window_H_ */
