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

#ifndef _ggcov_callgraph2win_H_
#define _ggcov_callgraph2win_H_ 1

#include "window.H"

#if GTK2
/*
 * TODO: someone with access to a RH 8.0 box needs to
 * port this feature to GTK2/libgnomecanvas.
 */
#define CALLGRAPH2 0
#else
#define CALLGRAPH2 1

#include "cov.H"
#include <libgnomeui/libgnomeui.h>

typedef struct
{
    double x1, y1, x2, y2;
} dbounds_t;

GLADE_CALLBACK void on_callgraph2_close_activate(GtkWidget*, gpointer);
GLADE_CALLBACK void on_callgraph2_zoom_in_activate(GtkWidget*, gpointer);
GLADE_CALLBACK void on_callgraph2_zoom_out_activate(GtkWidget*, gpointer);
GLADE_CALLBACK void on_callgraph2_show_all_activate(GtkWidget*, gpointer);

class callgraph2win_t : public window_t
{
public:
    callgraph2win_t();
    ~callgraph2win_t();

private:
    struct node_t
    {
    	cov_callnode_t *callnode_;
	unsigned int rank_;
	unsigned int file_;
	unsigned long generation_;
	unsigned int spread_;
	double x_, y_;
	GnomeCanvasItem *rect_item_;
	GnomeCanvasItem *text_item_;

    	static node_t *from_callnode(cov_callnode_t *cn)
	{
	    return (node_t *)cn->userdata;
	}
    	node_t(cov_callnode_t *);
	~node_t();
    };

    void populate();
    void zoom_all();
    void zoom_to(double factor);
    static callgraph2win_t *from_widget(GtkWidget *w)
    {
    	return (callgraph2win_t *)window_t::from_widget(w);
    }
    void adjust_rank(node_t *n, int delta);
    node_t *build_node(cov_callnode_t*, int rank);
    void add_spread(node_t *n);
    void build_ranks(node_t *, GPtrArray *ranks);
    void show_box(node_t *, double ystart, double yend);


    GtkWidget *canvas_;
    node_t *main_node_;
    double zoom_;
    dbounds_t bounds_;
    unsigned long generation_;
    unsigned int max_file_;

    friend void on_callgraph2_close_activate(GtkWidget*, gpointer);
    friend void on_callgraph2_zoom_in_activate(GtkWidget*, gpointer);
    friend void on_callgraph2_zoom_out_activate(GtkWidget*, gpointer);
    friend void on_callgraph2_show_all_activate(GtkWidget*, gpointer);
};

#endif /* !GTK2 */
#endif /* _ggcov_callgraph2win_H_ */
