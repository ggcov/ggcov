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

#ifndef _ggcov_scenegen_H_
#define _ggcov_scenegen_H_ 1

#include "common.h"
#include "cov.H"

/* 
 * Scenegen is an abstract base class used as an interface between code
 * that generates diagrams and the code that talks to window systems
 * or PostScript.  It behaves as a streaming interface which generates
 * one scene in a single pass, then another object needs to be created.
 * Both graphical primitives and cov object feedback primitives are
 * provided; the latter can be used to attach double-click callbacks or
 * URLs or whatever to graphical primitives.
 */

inline unsigned int RGB(unsigned int r, unsigned int g, unsigned int b)
{
    return (r<<16) | (g<<8) | (b);
}

class scenegen_t
{
public:
    scenegen_t();
    virtual ~scenegen_t();

    virtual void noborder() = 0;
    virtual void border(unsigned int rgb) = 0;
    virtual void nofill() = 0;
    virtual void fill(unsigned int rgb) = 0;
    virtual void box(double x, double y, double w, double h) = 0;
    virtual void textbox(double x, double y, double w, double h,
			 const char *text) = 0;
    virtual void arrow_size(double) = 0;
    virtual void polyline_begin(gboolean arrow) = 0;
    virtual void polyline_point(double x, double y) = 0;
    virtual void polyline_end(gboolean arrow) = 0;

    virtual void file(cov_file_t *);
    virtual void function(cov_function_t *);

private:
};

#endif /* _ggcov_scenegen_H_ */