/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2003 Greg Banks <gnb@users.sourceforge.net>
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
    static void files_changed(void *, unsigned int, void *);
    static window_t *from_widget(GtkWidget *);
    virtual void grey_items();
    void attach(GtkWidget *);
    void set_window(GtkWidget *);
    void set_title(const char *);
    virtual const char *name() const;

    virtual void load_state();
    virtual void save_state();

    GtkWidget *window_;
    boolean deleting_:1;    /* handle possible GUI recursion crap */
    boolean shown_:1;       /* has been shown()n at least once */
    boolean geom_dirty_:1;  /* geometry needs to be saved */

private:
    void on_close_activate();
    void on_delete_event(GdkEvent *);
    void on_exit_activate();
    void load_geometry();
    void save_geometry();
    struct { int x, y, w, h; } geom_;

    static gboolean on_configure_event(GtkWidget *, GdkEventConfigure *, gpointer);
};

void load(GtkCheckMenuItem *);
void save(GtkCheckMenuItem *);

#endif /* _ggcov_window_H_ */
