/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_diagwin_H_
#define _ggcov_diagwin_H_ 1

#include "window.H"

#include "cov.H"
#include "diagram.H"

GLADE_CALLBACK void on_diag_zoom_in_activate(GtkWidget*, gpointer);
GLADE_CALLBACK void on_diag_zoom_out_activate(GtkWidget*, gpointer);
GLADE_CALLBACK void on_diag_show_all_activate(GtkWidget*, gpointer);

class diagwin_t : public window_t
{
public:
    diagwin_t(diagram_t *);
    ~diagwin_t();

private:
    void populate();
    void zoom_all();
    void zoom_to(double factor);
    static diagwin_t *from_widget(GtkWidget *w)
    {
    	return (diagwin_t *)window_t::from_widget(w);
    }
    
    diagram_t *diagram_;
    GtkWidget *canvas_;
    double zoom_;

    friend void on_diag_zoom_in_activate(GtkWidget*, gpointer);
    friend void on_diag_zoom_out_activate(GtkWidget*, gpointer);
    friend void on_diag_show_all_activate(GtkWidget*, gpointer);
};

#endif /* _ggcov_diagwin_H_ */
