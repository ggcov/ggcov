/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2015-2020 Greg Banks <gnb@fastmail.fm>
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
#include "libgd_scenegen.H"
#include <gdfonts.h>
#include "filename.h"
#include "logging.H"

static logging::logger_t &_log = logging::find_logger("scene");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

libgd_scenegen_t::libgd_scenegen_t(unsigned int w, unsigned int h, const dbounds_t &bounds)
{
    image_ = gdImageCreateTrueColor(w, h);
    int bg = internalize_color(/*white*/0xffffff);
    gdImageFilledRectangle(image_, 0, 0, w-1, h-1, bg);
    font_ = gdFontGetSmall();
    text_color_ = internalize_color(/*black*/0U);

    _log.debug("libgd_scenegen_t(w=%u, h=%u, bounds={x1=%g, y1=%g, x2=%g, y2=%g})\n",
	       w, h, bounds.x1, bounds.y1, bounds.x2, bounds.y2);

    /* setup a matrix to transform from the diagram
     * coordinates as described by the dbounds_t
     * to the image coordinates */
    transform_.identity();
    transform_.translate(-bounds.x1, -bounds.y1);
    transform_.scale((double)w / bounds.width() ,
		     (double)h / bounds.height());


    points_ = 0;
    points_count_ = 0;
    points_allocated_ = 0;
}

libgd_scenegen_t::~libgd_scenegen_t()
{
    if (points_)
	g_free(points_);
    gdImageDestroy(image_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
libgd_scenegen_t::noborder()
{
    border_flag_ = FALSE;
}

void
libgd_scenegen_t::border(unsigned int rgb)
{
    border_flag_ = TRUE;
    border_ = internalize_color(rgb);
}

void
libgd_scenegen_t::nofill()
{
    fill_flag_ = FALSE;
}

void
libgd_scenegen_t::fill(unsigned int rgb)
{
    fill_flag_ = TRUE;
    fill_ = internalize_color(rgb);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
libgd_scenegen_t::box(double x, double y, double w, double h)
{
    int ix1 = transform_.itransx(x, y);
    int iy1 = transform_.itransy(x, y);
    int ix2 = transform_.itransx(x+w, y+h);
    int iy2 = transform_.itransy(x+w, y+h);
    _log.debug("box(world {x=%g, y=%g, w=%g, h=%g} "
	       "image {x=%d, y=%d, w=%d, h=%d})\n",
	       x, y, w, h, ix1, iy1, ix2-ix1, iy2-iy1);
    if (fill_flag_)
	gdImageFilledRectangle(image_, ix1, iy1, ix2, iy2, fill_);
    if (border_flag_)
	gdImageRectangle(image_, ix1, iy1, ix2, iy2, border_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
libgd_scenegen_t::textbox(
    double x,
    double y,
    double w,
    double h,
    const char *text)
{
    int ix1 = transform_.itransx(x, y);
    int iy1 = transform_.itransy(x, y);
    int ix2 = transform_.itransx(x+w, y+h);
    int iy2 = transform_.itransy(x+w, y+h);
    _log.debug("textbox(world {x=%g, y=%g, w=%g, h=%g} "
	       "image {x=%d, y=%d, w=%d, h=%d}, text=\"%s\"\n",
	       x, y, w, h, ix1, iy1, ix2-ix1, iy2-iy1, text);
    if (fill_flag_)
	gdImageFilledRectangle(image_, ix1, iy1, ix2, iy2, fill_);
    if (border_flag_)
	gdImageRectangle(image_, ix1, iy1, ix2, iy2, border_);
    if (text && *text)
	gdImageString(image_, font_, ix1, iy1, (unsigned char *)text, text_color_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
libgd_scenegen_t::polyline_begin(gboolean arrow)
{
    first_arrow_flag_ = arrow;
    points_count_ = 0;
}

void
libgd_scenegen_t::polyline_point(double x, double y)
{
    unsigned int newsize = sizeof(gdPoint) * (points_count_+1);
    if (newsize > points_allocated_)
    {
	/* round newsize up to a 256-byte boundary to reduce allocations */
	newsize = (newsize + 0xff) & ~0xff;
	/* TODO: use a new gnb_xrealloc */
	points_ = (gdPoint *)g_realloc(points_, newsize);
	points_allocated_ = newsize;
    }

    gdPoint *p = &points_[points_count_++];
    p->x = transform_.itransx(x, y);
    p->y = transform_.itransy(x, y);
}

void
libgd_scenegen_t::draw_arrow(const gdPoint *from, const gdPoint *to)
{
    gdPoint arrow[4];

    // Calculate the arrowhead geometry
    arrow[0].x = to->x;
    arrow[0].y = to->y;

    // calculate and normalise the vector along the arrowhead to the point
    double nx = to->x - from->x;
    double ny = to->y - from->y;
    double d = sqrt(nx*nx + ny*ny);
    nx /= d;
    ny /= d;

    // rotate the arrowhead vector to get the base normal
    double bnx = -ny;
    double bny = nx;

    // calculate a size for the arrow in image coords
    // as the average of the image coordinate width and height
    // of a square the nominal size of the arrow
    double size = (transform_.dtransw(arrow_size_, arrow_size_) +
		   transform_.dtransh(arrow_size_, arrow_size_)) / 2.0;

    // calculate the centre of the arrowhead base
    double bcx = to->x - size * arrow_shape_[1] * nx;
    double bcy = to->y - size * arrow_shape_[1] * ny;

    // calculate the first arrowhead barb point
    arrow[1].x = bcx + size * arrow_shape_[2] * bnx;
    arrow[1].y = bcy + size * arrow_shape_[2] * bny;

    // calculate the (possibly indented) arrowhead base centre point
    arrow[2].x = to->x - size * arrow_shape_[0] * nx;
    arrow[2].y = to->y - size * arrow_shape_[0] * ny;
    
    // calculate the second arrowhead barb point
    arrow[3].x = bcx - size * arrow_shape_[2] * bnx;
    arrow[3].y = bcy - size * arrow_shape_[2] * bny;
    
    gdImageFilledPolygon(image_, arrow, 4, fill_);
}

void
libgd_scenegen_t::polyline_end(gboolean arrow)
{
    if (points_count_ < 2)
	return;
    _log.debug("polyline([...%u...])\n", points_count_);

    gdImageOpenPolygon(image_, points_, points_count_, fill_);
    if (first_arrow_flag_)
	draw_arrow(&points_[1], &points_[0]);
    if (arrow)
	draw_arrow(&points_[points_count_-2], &points_[points_count_-1]);
    points_count_ = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

bool libgd_scenegen_t::save(const char *filename, const char *format)
{
    string_var newfilename = g_strconcat(filename, ".NEW", (char*)0);
    FILE *fp = fopen(newfilename, "w");
    if (!fp)
    {
	perror(newfilename);
	return false;
    }

    if (!format)
    {
	format = file_extension_c(filename);
	if (!format)
	{
	    _log.error("%s: cannot discover format from filename\n", filename);
	    goto error;
	}
	format++;   // skip the '.'
    }
    if (!strcasecmp(format, "png"))
    {
	gdImagePng(image_, fp);
    }
    else
    {
	_log.error("%s: unknown image format\n", format);
	goto error;
    }

    fclose(fp);
    fp = 0;
    if (rename(newfilename, filename) < 0)
    {
	perror(filename);
	goto error;
    }

    return true;

error:
    if (fp)
	fclose(fp);
    unlink(newfilename);
    return false;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
