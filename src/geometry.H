/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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

#ifndef _ggcov_geometry_H_
#define _ggcov_geometry_H_ 1

#include "common.h"
#include <math.h>

#ifndef HUGE
#define HUGE                (1.0e10)
#endif
#define EPSILON             (1.0e-06)

struct dbounds_t
{
    double x1, y1, x2, y2;

    double width() const
    {
	return x2 - x1;
    }
    double height() const
    {
	return y2 - y1;
    }

    inline void initialise()
    {
	x1 = HUGE;
	y1 = HUGE;
	x2 = -HUGE;
	y2 = -HUGE;
    }

    inline void expand(double deltax, double deltay)
    {
	x1 -= deltax;
	x2 += deltax;
	y1 -= deltay;
	y2 += deltay;
    }

    inline gboolean intersects(const dbounds_t &b) const
    {
	if (x1 > b.x2)
	    return FALSE;
	if (x2 < b.x1)
	    return FALSE;
	if (y1 > b.y2)
	    return FALSE;
	if (y2 < b.y1)
	    return FALSE;
	return TRUE;
    }

    inline void adjust(double bx1, double by1, double bx2, double by2)
    {
	if (bx1 < x1)
	    x1 = bx1;
	if (by1 < y1)
	    y1 = by1;
	if (bx2 > x2)
	    x2 = bx2;
	if (by2 > y2)
	    y2 = by2;
    }
    inline void adjust(double x1, double y1)
    {
	adjust(x1, y1, x1, y1);
    }
    inline void adjust(const dbounds_t &b)
    {
	adjust(b.x1, b.y1, b.x2, b.y2);
    }

};

struct matrix2_t
{
    // A 2-d to 2-d coordinate transform matrix as per the
    // PostScript(tm) imaging model, which is the 6 variable
    // components of a 3x3 matrix of the form
    // [  a  b  0 ]
    // [  c  d  0 ]
    // [ tx ty  1 ]
    double a, b, c, d, tx, ty;

    /* set the matrix to the identity matrix */
    void identity()
    {
	// set the matrix to
	// [  1  0  0 ]
	// [  0  1  0 ]
	// [  0  0  1 ]
	a = d = 1.0;
	b = c = tx = ty = 0.0;
    }
    void translate(double dx, double dy)
    {
	// equivalent of a premultiply by
	// [  1  0  0 ]
	// [  0  1  0 ]
	// [ dx dy  1 ]
	tx += a * dx + c * dy;
	ty += b * dx + d * dy;
    }
    void scale(double sx, double sy)
    {
	// equivalent of a premultiply by
	// [ sx  0  0 ]
	// [  0 sy  0 ]
	// [  0  0  1 ]
	a *= sx;
	b *= sy;
	c *= sx;
	d *= sy;
    }
    void rotate(double theta/*radians*/)
    {
	// equivalent of a premultiply by
	// [  cos(theta) sin(theta)  0 ]
	// [ -sin(theta) cos(theta)  0 ]
	// [           0          0  1 ]
	double cth, sth;
#ifdef HAVE_SINCOS
	sincos(theta, &sth, &cth);
#else
	sth = sin(theta);
	cth = cos(theta);
#endif

	double na =  a * cth + c * sth;
	double nb =  b * cth + d * sth;
	double nc = -a * sth + c * cth;
	double nd = -b * sth + d * cth;

	a = na;
	b = nb;
	c = nc;
	d = nd;
    }

    /* Transform and convert to integer */
    int itransx(double x, double y) const
    {
	return (int)(a * x + c * y + tx + 0.5);
    }
    int itransy(double x, double y) const
    {
	return (int)(b * x + d * y + ty + 0.5);
    }
    int itransw(double w, double h) const
    {
	return (int)(a * w + c * h + 0.5);
    }
    int itransh(double w, double h) const
    {
	return (int)(b * w + d * h + 0.5);
    }

    /* Transform leaving as double */
    double dtransx(double x, double y) const
    {
	return (a * x + c * y + tx);
    }
    double dtransy(double x, double y) const
    {
	return (b * x + d * y + ty);
    }
    double dtransw(double w, double h) const
    {
	return (a * w + c * h);
    }
    double dtransh(double w, double h) const
    {
	return (b * w + d * h);
    }
};

#endif /* _ggcov_geometry_H_ */
