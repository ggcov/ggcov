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

#include "legowin.H"
#include "cov.H"
#include "tok.H"
#include "prefs.H"

CVSID("$Id: legowin.C,v 1.1 2005-05-22 12:47:56 gnb Exp $");

#define WIDTH	(1.0)
#define HEIGHT	(1.0)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

legowin_t::legowin_t()
{
    GladeXML *xml;
    
    zoom_ = 1.0;

    /* load the interface & connect signals */
    xml = ui_load_tree("lego");
    
    set_window(glade_xml_get_widget(xml, "lego"));
    
    canvas_ = glade_xml_get_widget(xml, "lego_canvas");
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas_), zoom_);
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "lego_windows_dummy"));
}

legowin_t::~legowin_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
legowin_t::zoom_to(double factor)
{
    zoom_ = factor;
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas_), zoom_);
}

void
legowin_t::zoom_all()
{
    double zoomx, zoomy;

    zoomx = canvas_->allocation.width / WIDTH;
    zoomy = canvas_->allocation.height / HEIGHT;
    zoom_ = MIN(zoomx, zoomy);
    dprintf3(D_LEGOWIN, "legowin_t::zoom_all: zoomx=%g zoomy=%g zoom=%g\n",
    	    	zoomx, zoomy, zoom_);

    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas_), zoom_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
legowin_t::show_node(node_t *node)
{
    string_var label;
    list_iterator_t<node_t> niter;
    GnomeCanvasGroup *root = gnome_canvas_root(GNOME_CANVAS(canvas_));

    if (node->file_ != 0)
    {
	label = g_strdup_printf("%s %4.2f%%",
	    node->name_.data(),
	    100.0 * node->stats_.blocks_fraction());
    }
    else
    {
	label = strdup(node->name_);
    }

    node->cov_rect_item_ = gnome_canvas_item_new(
    	    	root,
    	    	GNOME_TYPE_CANVAS_RECT,
		"x1", 	    	node->x_,
		"y1",	    	node->y_,
		"x2", 	    	node->x_ + node->w_,
		"y2",	    	node->y_ + node->h_cov_,
		"fill_color_gdk",backgrounds_by_status[cov::COVERED],
		0);
    node->uncov_rect_item_ = gnome_canvas_item_new(
    	    	root,
    	    	GNOME_TYPE_CANVAS_RECT,
		"x1", 	    	node->x_,
		"y1",	    	node->y_ + node->h_cov_,
		"x2", 	    	node->x_ + node->w_,
		"y2",	    	node->y_ + node->h_,
		"fill_color_gdk",backgrounds_by_status[cov::UNCOVERED],
		0);
    node->border_rect_item_ = gnome_canvas_item_new(
    	    	root,
    	    	GNOME_TYPE_CANVAS_RECT,
		"x1", 	    	node->x_,
		"y1",	    	node->y_,
		"x2", 	    	node->x_ + node->w_,
		"y2",	    	node->y_ + node->h_,
		"outline_color","black",
#if GTK2
		"width_pixels", 1,
#endif
		0);
    node->text_item_ = gnome_canvas_item_new(
    	    	root,
    	    	GNOME_TYPE_CANVAS_TEXT,
		"text",     	label.data(),
		"font",     	"fixed",
		"fill_color",	"black",
		"x", 	    	node->x_,
		"y",	    	node->y_,
		"clip",     	TRUE,
		"clip_width",	node->x_ + node->w_,
		"clip_height",	node->y_ + node->h_,
		"anchor",   	GTK_ANCHOR_NORTH_WEST,
		0);

    for (niter = node->children_.first() ; niter != (node_t *)0 ; ++niter)
	show_node(*niter);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
legowin_t::dump_node(node_t *node, FILE *fp)
{
    unsigned int i;

    for (i = 0 ; i < node->depth_ ; i++)
	fputs("    ", fp);
    fprintf(fp, "%s depth=%u blocks=%lu/%lu geom=[%g %g %g %g]",
	node->name_.data(),
	node->depth_,
	node->stats_.blocks_executed(),
	node->stats_.blocks_total(),
	node->x_,
	node->y_,
	node->w_,
	node->h_);

    if (node->children_.head() != 0)
    {
	fputs(" {\n", fp);

	list_iterator_t<node_t> niter;
	for (niter = node->children_.first() ; niter != (node_t *)0 ; ++niter)
	    dump_node(*niter, fp);

	for (i = 0 ; i < node->depth_ ; i++)
	    fputs("    ", fp);
	fputc('}', fp);
    }
    fputc('\n', fp);
}

void
legowin_t::assign_geometry(
    node_t *node,
    double x,
    double y,
    double w,
    double h)
{
    node->x_ = x;
    node->y_ = y;
    node->w_ = w;
    node->h_ = h;
    node->h_cov_ = h * node->stats_.blocks_fraction();
    node->h_uncov_ = h - node->h_cov_;

    if (node->depth_)
	x += (WIDTH / (double)maxdepth_);
    h /= (double)node->stats_.blocks_total();

    list_iterator_t<node_t> niter;
    for (niter = node->children_.first() ; niter != (node_t *)0 ; ++niter)
    {
	node_t *child = *niter;
	double h2 = h * (double)child->stats_.blocks_total();
	assign_geometry(child, x, y, w, h2);
	y += h2;
    }
}

void
legowin_t::populate()
{
    list_iterator_t<cov_file_t> iter;
    GnomeCanvasGroup *root = gnome_canvas_root(GNOME_CANVAS(canvas_));

    dprintf0(D_LEGOWIN, "legowin_t::populate\n");

    while (root->item_list != 0)
    {
    	gtk_object_destroy(GTK_OBJECT(root->item_list->data));
    }

    root_ = new node_t();
    root_->name_ = "/";
    root_->depth_ = 0;
    maxdepth_ = 0;

    /* First pass: construct a tree of nodes */
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    {
	cov_file_t *f = *iter;

	dprintf1(D_LEGOWIN, "    adding file \"%s\"\n", f->minimal_name());

	cov_scope_t *sc = new cov_file_scope_t(f);
	const cov_stats_t *st = sc->get_stats();
	if (st->blocks_total() == 0)
	{
	    delete sc;
	    continue;
	}

	node_t *parent = root_;
	node_t *node = 0;
	tok_t tok(f->minimal_name(), "/");
	const char *comp;
	while ((comp = tok.next()) != 0)
	{
	    list_iterator_t<node_t> niter;
	    node = 0;

	    for (niter = parent->children_.first() ; 
		 niter != (node_t *)0 ;
		 ++niter)
	    {
		if (!strcmp((*niter)->name_, comp))
		{
		    node = *niter;
		    break;
		}
	    }
	    if (node == 0)
	    {
		node = new node_t();
		node->name_ = comp;
		parent->children_.append(node);
		node->depth_ = parent->depth_ + 1;
		if (node->depth_ > maxdepth_)
		    maxdepth_ = node->depth_;
	    }
	    parent->stats_.accumulate(st);
	    parent = node;
	}

	assert(node != 0);
	assert(node->file_ == 0);
	node->file_ = f;
	node->stats_ = *st;

	delete sc;
    }

    /* Second pass: assign geometry to the nodes */
    assign_geometry(root_, 0.0, 0.0, (WIDTH / (double)maxdepth_), HEIGHT);

    /* Third pass: create canvas items */
    show_node(root_);

    /* Debug: dump tree */
    if (debug_enabled(D_LEGOWIN|D_VERBOSE))
	dump_node(root_, stderr);

    /* setup the canvas' to show all the nodes */
    gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas_),
				    0.0, 0.0, WIDTH, HEIGHT);
    zoom_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_lego_zoom_in_activate(GtkWidget *w, gpointer data)
{
    legowin_t *cw = legowin_t::from_widget(w);

    dprintf0(D_LEGOWIN, "on_lego_zoom_in_activate\n");

    cw->zoom_to(cw->zoom_*2.0);
}


GLADE_CALLBACK void
on_lego_zoom_out_activate(GtkWidget *w, gpointer data)
{
    legowin_t *cw = legowin_t::from_widget(w);

    dprintf0(D_LEGOWIN, "on_lego_zoom_out_activate\n");

    cw->zoom_to(cw->zoom_/2.0);
}


GLADE_CALLBACK void
on_lego_show_all_activate(GtkWidget *w, gpointer data)
{
    legowin_t *cw = legowin_t::from_widget(w);

    dprintf0(D_LEGOWIN, "on_lego_show_all_activate\n");

    cw->zoom_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
