/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2011 Greg Banks <gnb@users.sourceforge.net>
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

#include "gd_scenegen.H"
#include <gdfonts.h>
#include <math.h>

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gd_scenegen_t::gd_scenegen_t(int width, int height)
{
    width_ = width;
    height_ = height;
    sx_ = 1.0;
    ox_ = 0.0;
    sy_ = 1.0;
    oy_ = 0.0;
    image_ = gdImageCreateTrueColor(width_, height_);

    gdImageFilledRectangle(image_,
			   0, 0, width-1, height-1,
			   gdTrueColor(0xff, 0xff, 0xff));
}

gd_scenegen_t::~gd_scenegen_t()
{
    gdImageDestroy(image_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

color_t
gd_scenegen_t::color_rgb(unsigned char r, unsigned char g, unsigned char b)
{
    return (color_t)gdTrueColor(r, g, b);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
gd_scenegen_t::box(double x, double y, double w, double h)
{
    int ix1 = sx(x);
    int iy1 = sy(y);
    int ix2 = sx(x+w) - 1;
    int iy2 = sy(y+h) - 1;

    if (fill_flag_)
	gdImageFilledRectangle(image_, ix1, iy1, ix2, iy2, fill_color_);
    if (border_flag_)
	gdImageRectangle(image_, ix1, iy1, ix2, iy2, border_color_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
gd_scenegen_t::textbox(
    double x,
    double y,
    double w,
    double h,
    const char *text)
{
    if (!font_)
	font_ = gdFontGetSmall();
    gdImageString(image_, font_, sx(x), sy(y), (unsigned char *)text, border_color_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
gd_scenegen_t::polyline_begin(gboolean arrow)
{
    first_arrow_flag_ = arrow;
    points_.count = 0;
}

void
gd_scenegen_t::polyline_point(double x, double y)
{
    unsigned int newsize = sizeof(gdPoint) * (points_.count+1);
    if (newsize > points_.alloc)
    {
	/* round newsize up to a 256-byte boundary to reduce allocations */
	newsize = (newsize + 0xff) & ~0xff;
	/* TODO: use a new gnb_xrealloc */
	points_.data = (gdPoint *)g_realloc(points_.data, newsize);
	points_.alloc = newsize;
    }

    gdPoint *p = points_.data + points_.count;
    p->x = sx(x);
    p->y = sy(y);
    points_.count++;
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
gd_scenegen_t::polyline_end(gboolean arrow)
{
    if (points_.count < 2)
	return;

//     if (first_arrow_flag_)

    gdImageOpenPolygon(image_, points_.data, points_.count, fill_color_);

//     if (arrow)
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
gd_scenegen_t::bounds(const dbounds_t &b)
{
    sx_ = width_ / b.width();
    ox_ = b.x1;
    sy_ = height_ / b.height();
    oy_ = b.y1;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
gd_scenegen_t::get_data(estring &e) const
{
    int size = 0;
    void *buf = gdImagePngPtr(image_, &size);
    if (buf)
	buf = realloc(buf, size+1);	/* need room for the nul */
    if (buf)
	e.set((char *)buf, size);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
