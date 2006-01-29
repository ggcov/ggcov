/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@alphalink.com.au>
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

#define HUGE    	    (1.0e10)
#define EPSILON		    (1.0e-06)

struct dbounds_t
{
    double x1, y1, x2, y2;
    
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

#endif /* _ggcov_geometry_H_ */
