/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2015 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_libgd_scenegen_H_
#define _ggcov_libgd_scenegen_H_ 1

#include "common.h"
#include <gd.h>
#include "geometry.H"
#include "scenegen.H"

class libgd_scenegen_t : public scenegen_t
{
public:
    libgd_scenegen_t(unsigned int w, unsigned int h, const dbounds_t &);
    ~libgd_scenegen_t();

    void noborder();
    void border(unsigned int rgb);
    void nofill();
    void fill(unsigned int rgb);
    void box(double x, double y, double w, double h);
    void textbox(double x, double y, double w, double h,
			 const char *text);
    void polyline_begin(gboolean arrow);
    void polyline_point(double x, double y);
    void polyline_end(gboolean arrow);

    bool save(const char *filename, const char *format = 0);

private:
    int internalize_color(unsigned int rgb) const
    {
	return gdImageColorExact(image_, (rgb>>16)&0xff, (rgb>>8)&0xff, rgb&0xff);
    }
    void draw_arrow(const gdPoint *from, const gdPoint *to);

    gdImage *image_;
    matrix2_t transform_;
    gdFont *font_;
    int text_color_;

    gboolean fill_flag_;
    int fill_;
    gboolean border_flag_;
    int border_;

    /* state for polylines */
    gboolean first_arrow_flag_;
    gdPoint *points_;
    unsigned int points_count_;	    // number of valid points
    size_t points_allocated_;	    // size in bytes of points array
};

#endif /* _ggcov_libgd_scenegen_H_ */
