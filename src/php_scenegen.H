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

#ifndef _ggcov_php_scenegen_H_
#define _ggcov_php_scenegen_H_ 1

#include "common.h"
#include "scenegen.H"
#include "php_serializer.H"

class php_scenegen_t : public scenegen_t
{
public:
    php_scenegen_t();
    ~php_scenegen_t();

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

    void bounds(double x, double y, double w, double h);
    const estring &data();

private:
    unsigned int get_color(unsigned int rgb);

    enum _codes
    {
	CODE_COLOR = 1,
	CODE_RECTANGLE,
	CODE_TEXT,
	CODE_POLYLINE,
	CODE_POLYGON
    };

    php_serializer_t ser_;

    unsigned int ncolors_;
    unsigned int *colors_;

    gboolean fill_flag_;
    unsigned int fill_idx_;
    gboolean border_flag_;
    unsigned int border_idx_;

    /* state for polylines */
    gboolean first_arrow_flag_;
    struct
    {
	unsigned int num_points;
	unsigned int size;
	double *coords;
    } points_;
};

#endif /* _ggcov_php_scenegen_H_ */
