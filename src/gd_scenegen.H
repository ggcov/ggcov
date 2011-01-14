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

#ifndef _ggcov_gd_scenegen_H_
#define _ggcov_gd_scenegen_H_ 1

#include "common.h"
#include "scenegen.H"
#include "geometry.H"
#include "estring.H"
#include <gd.h>

class gd_scenegen_t : public scenegen_t
{
public:
    gd_scenegen_t(int w, int h);
    ~gd_scenegen_t();

    color_t color_rgb(unsigned char r, unsigned char g, unsigned char b);
    void box(double x, double y, double w, double h);
    void textbox(double x, double y, double w, double h,
			 const char *text);
    void polyline_begin(gboolean arrow);
    void polyline_point(double x, double y);
    void polyline_end(gboolean arrow);

    void bounds(const dbounds_t &);

    void get_data(estring &) const;

private:
    int sx(double x) { return (int)((x - ox_) * sx_ + 0.5); }
    int sy(double y) { return (int)((y - oy_) * sy_ + 0.5); }

    int width_, height_;
    gdImage *image_;
    double sx_, ox_, sy_, oy_;
    gdFont *font_;

    /* state for polylines */
    gboolean first_arrow_flag_;
    struct
    {
	unsigned int count;
	unsigned int alloc;
	gdPoint *data;
    } points_;
};

#endif /* _ggcov_gd_scenegen_H_ */
