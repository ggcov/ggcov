/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005-2020 Greg Banks <gnb@fastmail.fm>
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

#include "common.h"
#include "canvas_scenegen.H"
#include "ui.h"
#include "canvas_function_popup.H"
#include "cov_suppression.H"
#include "logging.H"

#define RGB_TO_STR(b, rgb) \
    snprintf((b), sizeof((b)), "#%02x%02x%02x", \
		((rgb)>>16)&0xff, ((rgb)>>8)&0xff, (rgb)&0xff)

static const char BLOCK_KEY[] = "ggcov-canvas-scenegen-block";
static logging::logger_t &_log = logging::find_logger("scene");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

canvas_scenegen_t::canvas_scenegen_t(GnomeCanvas *can)
{
    canvas_ = can;
    root_ = gnome_canvas_root(canvas_);

    points_.coords = (double *)0;
    points_.num_points = 0;
    points_.ref_count = 1;
    points_size_ = 0;
    registered_tooltip_ = FALSE;
}

canvas_scenegen_t::~canvas_scenegen_t()
{
    if (points_.coords != 0)
	g_free(points_.coords);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
canvas_scenegen_t::noborder()
{
    border_flag_ = FALSE;
}

void
canvas_scenegen_t::border(unsigned int rgb)
{
    border_flag_ = TRUE;
    RGB_TO_STR(border_buf_, rgb);
}

void
canvas_scenegen_t::nofill()
{
    fill_flag_ = FALSE;
}

void
canvas_scenegen_t::fill(unsigned int rgb)
{
    fill_flag_ = TRUE;
    RGB_TO_STR(fill_buf_, rgb);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
canvas_scenegen_t::handle_object(GnomeCanvasItem *item)
{
    cov_function_t *fn = get_function();
    if (fn)
    {
	canvas_function_popup_t *fpop = new canvas_function_popup_t(item, fn);
	fpop->set_foreground(border_color());
	fpop->set_background(fill_color());
	return;
    }

    cov_block_t *b = get_block();
    if (b)
    {
	if (!registered_tooltip_)
	{
	    GnomeCanvas *canvas = GNOME_CANVAS_ITEM(item)->canvas;
	    gtk_widget_set(GTK_WIDGET(canvas),
			   "has-tooltip", TRUE,
			   (char *)0);
	    g_signal_connect(G_OBJECT(canvas), "query-tooltip",
		    G_CALLBACK(on_query_tooltip), NULL);
	    registered_tooltip_ = TRUE;
	}
	gtk_object_set_data(GTK_OBJECT(item), BLOCK_KEY, (gpointer)b);
	return;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
canvas_scenegen_t::box(double x, double y, double w, double h)
{
    GnomeCanvasItem *item;

    if (_log.is_enabled(logging::DEBUG))
    {
	int cx1, cy1, cx2, cy2;
	gnome_canvas_w2c(canvas_, x, y, &cx1, &cy1);
	gnome_canvas_w2c(canvas_, x+w, y+h, &cx2, &cy2);
	_log.debug("box(world {x=%g, y=%g, w=%g, h=%g} "
		   "canvas {x=%d, y=%d, w=%d, h=%d})\n",
		   x, y, w, h, cx1, cy1, cx2-cx1, cy2-cy1);
    }

    item = gnome_canvas_item_new(root_, GNOME_TYPE_CANVAS_RECT,
		"x1",           x,
		"y1",           y,
		"x2",           x+w,
		"y2",           y+h,
		"fill_color",   fill_color(),
		"outline_color",border_color(),
		"width_pixels", (border_flag_ ? 1 : 0),
		(char *)0);
    handle_object(item);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
canvas_scenegen_t::textbox(
    double x,
    double y,
    double w,
    double h,
    const char *text)
{
    GnomeCanvasItem *item;

    if (_log.is_enabled(logging::DEBUG))
    {
	int cx1, cy1, cx2, cy2;
	gnome_canvas_w2c(canvas_, x, y, &cx1, &cy1);
	gnome_canvas_w2c(canvas_, x+w, y+h, &cx2, &cy2);
	_log.debug("textbox(world {x=%g, y=%g, w=%g, h=%g} "
		   "canvas {x=%d, y=%d, w=%d, h=%d}, text=\"%s\"\n",
		   x, y, w, h, cx1, cy1, cx2-cx1, cy2-cy1, text);
    }

    item = gnome_canvas_item_new(root_, GNOME_TYPE_CANVAS_TEXT,
		"text",         text,
		"font",         "fixed",
		"fill_color",   border_color(),
		"x",            x,
		"y",            y,
		"clip",         TRUE,
		"clip_width",   w,
		"clip_height",  h,
		"anchor",       GTK_ANCHOR_NORTH_WEST,
		(char *)0);
    handle_object(item);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
canvas_scenegen_t::polyline_begin(gboolean arrow)
{
    first_arrow_flag_ = arrow;
    points_.num_points = 0;
}

void
canvas_scenegen_t::polyline_point(double x, double y)
{
    unsigned int newsize = 2 * sizeof(double) * (points_.num_points+1);
    if (newsize > points_size_)
    {
	/* round newsize up to a 256-byte boundary to reduce allocations */
	newsize = (newsize + 0xff) & ~0xff;
	/* TODO: use a new gnb_xrealloc */
	points_.coords = (double *)g_realloc(points_.coords, newsize);
	points_size_ = newsize;
    }

    double *p = points_.coords + 2*points_.num_points;
    p[0] = x;
    p[1] = y;
    points_.num_points++;
}

void
canvas_scenegen_t::polyline_end(gboolean arrow)
{
    GnomeCanvasItem *item;

    if (!points_.num_points)
	return;
    _log.debug("polyline([...%u...])\n", points_.num_points);
    item = gnome_canvas_item_new(root_, GNOME_TYPE_CANVAS_LINE,
		"points",               &points_,
		"first_arrowhead",      first_arrow_flag_,
		"last_arrowhead",       arrow,
		"arrow_shape_a",        arrow_size_ * arrow_shape_[0],
		"arrow_shape_b",        arrow_size_ * arrow_shape_[1],
		"arrow_shape_c",        arrow_size_ * arrow_shape_[2],
		"fill_color",           fill_color(),
		/* setting width_pixels screws up the arrow heads !?!? */
		(char *)0);
    handle_object(item);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void describe_status(estring &txt, const cov_suppression_t *sup, count_t count)
{
    if (sup)
    {
	txt.append_printf("suppressed because %s\n", sup->describe());
    }
    else if (!count)
    {
	txt.append_printf("never executed\n");
    }
    else
    {
	txt.append_printf("executed %llu times\n", (unsigned long long)count);
    }
}

void
canvas_scenegen_t::format_tooltip(estring &txt, cov_block_t *b)
{
    txt.append_printf("Block %d", b->bindex());
    txt.append_printf(" of %s\n", b->function()->name());

    describe_status(txt, b->suppression(), b->count());
    for (list_iterator_t<cov_arc_t> aiter = b->first_arc() ; *aiter ; ++aiter)
    {
	cov_arc_t *a = *aiter;
	txt.append_printf("arc %u: ", a->aindex());
	if (a->is_call())
	{
	    const char *n = a->call_name();
	    txt.append_printf("call to %s", (n ? n : "unknown function"));
	}
	else if (a->is_return())
	{
	    txt.append_printf("return from function");
	}
	else if (a->is_fall_through())
	{
	    txt.append_printf("fall through to block %u", a->to()->bindex());
	}
	else
	{
	    txt.append_printf("branch to block %u", a->to()->bindex());
	}
	txt.append_string(", ");
	describe_status(txt, a->suppression(), a->count());
    }
}

gboolean
canvas_scenegen_t::on_query_tooltip(GtkWidget *w, gint x, gint y,
				    gboolean keyboard_mode, GtkTooltip *tooltip,
				    gpointer closure)
{
    GnomeCanvas *canvas = GNOME_CANVAS(w);

    double wx, wy;
    gnome_canvas_c2w(canvas, x, y, &wx, &wy);

    GnomeCanvasItem *item = gnome_canvas_get_item_at(canvas, wx, wy);
    if (!item)
	return FALSE;

    estring txt;

    cov_block_t *b = (cov_block_t *)gtk_object_get_data(GTK_OBJECT(item), BLOCK_KEY);
    if (b)
	format_tooltip(txt, b);

    while (txt.length() && txt.data()[txt.length()-1] == '\n')
	txt.truncate_to(txt.length()-1);
    if (txt.length())
    {
	gtk_tooltip_set_text(tooltip, txt.data());
	return TRUE;
    }
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
