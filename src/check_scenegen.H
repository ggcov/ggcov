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

#ifndef _ggcov_check_scenegen_H_
#define _ggcov_check_scenegen_H_ 1

#include "common.h"
#include "scenegen.H"
#include "geometry.H"

class check_scenegen_t : public scenegen_t
{
public:
    check_scenegen_t();
    ~check_scenegen_t();

    void noborder();
    void border(unsigned int rgb);
    void nofill();
    void fill(unsigned int rgb);
    void box(double x, double y, double w, double h);
    void textbox(double x, double y, double w, double h,
			 const char *text);
    void arrow_size(double as);
    void polyline_begin(gboolean arrow);
    void polyline_point(double x, double y);
    void polyline_end(gboolean arrow);

    void check();

    void object(cov_function_t *);

private:
    struct box_t
    {
    	string_var name_;
	dbounds_t bbox_;
    };

    struct polyline_t
    {
	int npoints_;
	double *points_;
	dbounds_t bbox_;
    };

    void check_box_intersections();

    /* state for boxes */
    string_var name_;

    /* state for polylines */
    double *points_;
    unsigned int npoints_;
    unsigned int points_size_;
    dbounds_t bbox_;

    /* stored geometry */
    list_t<box_t> boxes_;
    list_t<polyline_t> polylines_;
};

#endif /* _ggcov_check_scenegen_H_ */
