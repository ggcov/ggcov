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

#include "php_scenegen.H"
#include <math.h>

CVSID("$Id: php_scenegen.C,v 1.4 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

php_scenegen_t::php_scenegen_t()
{
    ser_.begin_array();
}

php_scenegen_t::~php_scenegen_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

color_t
php_scenegen_t::color_rgb(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned int i;
    unsigned int rgb = (r<<16)|(g<<8)|(b);

    for (i = 0 ; i < ncolors_ ; i++)
    {
	if (colors_[i] == rgb)
	    return i;
    }

    ncolors_++;
    colors_ = (unsigned int *)g_realloc(colors_, sizeof(unsigned int)*ncolors_);
    i = ncolors_-1;
    colors_[i] = rgb;

    ser_.next_key();
    ser_.begin_array(5);
    ser_.next_key(); ser_.integer(CODE_COLOR);
    ser_.next_key(); ser_.integer(i);
    ser_.next_key(); ser_.integer(r);
    ser_.next_key(); ser_.integer(g);
    ser_.next_key(); ser_.integer(b);
    ser_.end_array();

    return i;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
php_scenegen_t::box(double x, double y, double w, double h)
{
    ser_.next_key();
    ser_.begin_array(7);
    ser_.next_key(); ser_.integer(CODE_RECTANGLE);
    ser_.next_key(); ser_.floating(x);
    ser_.next_key(); ser_.floating(y);
    ser_.next_key(); ser_.floating(x+w);
    ser_.next_key(); ser_.floating(y+h);

    ser_.next_key();
    if (fill_flag_)
	ser_.integer(fill_color_);
    else
	ser_.null();

    ser_.next_key();
    if (border_flag_)
	ser_.integer(border_color_);
    else
	ser_.null();

    ser_.end_array();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
php_scenegen_t::textbox(
    double x,
    double y,
    double w,
    double h,
    const char *text)
{
    ser_.next_key();
    ser_.begin_array(5);
    ser_.next_key(); ser_.integer(CODE_TEXT);
    ser_.next_key(); ser_.floating(x);
    ser_.next_key(); ser_.floating(y);
    ser_.next_key(); ser_.string(text);
    ser_.next_key(); ser_.integer(border_color_);
    ser_.end_array();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
php_scenegen_t::polyline_begin(gboolean arrow)
{
    first_arrow_flag_ = arrow;
    points_.num_points = 0;
}

void
php_scenegen_t::polyline_point(double x, double y)
{
    unsigned int newsize = 2 * sizeof(double) * (points_.num_points+1);
    if (newsize > points_.size)
    {
	/* round newsize up to a 256-byte boundary to reduce allocations */
	newsize = (newsize + 0xff) & ~0xff;
	/* TODO: use a new gnb_xrealloc */
	points_.coords = (double *)g_realloc(points_.coords, newsize);
	points_.size = newsize;
    }

    double *p = points_.coords + 2*points_.num_points;
    p[0] = x;
    p[1] = y;
    points_.num_points++;
}

#if 0
    // Serialize a polygon
    // (This code was used in the first attempt at rendering
    // arrowheads, but testing showed that the arrowhead
    // geometry calculations needed to be performed in image
    // space to avoid skewing.  It might be useful again).
    ser_.next_key();
    ser_.begin_array(4);
    ser_.next_key(); ser_.integer(CODE_POLYGON);
    ser_.next_key(); ser_.begin_array(6);
    for (i = 0 ; i < 6 ; i++)
    {
	ser_.next_key(); ser_.floating(c[i]);
    }
    ser_.end_array();
    ser_.next_key(); ser_.integer(fill_color_);	// fill
    ser_.next_key(); ser_.integer(fill_color_);	// border
    ser_.end_array();
#endif

/*
 * TODO: need to make use of the arrow_shape_
 */

void
php_scenegen_t::polyline_end(gboolean arrow)
{
    unsigned int i;
    unsigned int nc = 2*points_.num_points;

    if (points_.num_points < 2)
	return;

    ser_.next_key();
    ser_.begin_array(5);
    ser_.next_key(); ser_.integer(CODE_POLYLINE);

    ser_.next_key(); ser_.begin_array(nc);
    for (i = 0 ; i < nc ; i++)
    {
	ser_.next_key(); ser_.floating(points_.coords[i]);
    }
    ser_.end_array();

    ser_.next_key();
    if (first_arrow_flag_)
	ser_.floating(arrow_size_);
    else
	ser_.null();

    ser_.next_key();
    if (arrow)
	ser_.floating(arrow_size_);
    else
	ser_.null();

    ser_.next_key(); ser_.integer(fill_color_);
    ser_.end_array();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
php_scenegen_t::bounds(double x, double y, double w, double h)
{
    ser_.next_key();
    ser_.begin_array(5);
    ser_.next_key(); ser_.integer(0);
    ser_.next_key(); ser_.floating(x);
    ser_.next_key(); ser_.floating(y);
    ser_.next_key(); ser_.floating(w);
    ser_.next_key(); ser_.floating(h);
    ser_.end_array();
}

const estring &
php_scenegen_t::data()
{
    ser_.end_array();
    return ser_.data();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
