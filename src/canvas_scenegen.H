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

#ifndef _ggcov_canvas_scenegen_H_
#define _ggcov_canvas_scenegen_H_ 1

#include "common.h"
#include "uicommon.h"
#include "scenegen.H"
#include <libgnomeui/libgnomeui.h>

class canvas_scenegen_t : public scenegen_t
{
public:
    canvas_scenegen_t(GnomeCanvas *);
    ~canvas_scenegen_t();

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

private:
    inline const char *fill_color() const
    {
	return (fill_flag_ ? fill_buf_ : (const char *)0);
    }
    inline const char *border_color() const
    {
	return (border_flag_ ? border_buf_ : (const char *)0);
    }
    void handle_object(GnomeCanvasItem*);

    GnomeCanvas *canvas_;
    GnomeCanvasGroup *root_;

    gboolean fill_flag_;
    char fill_buf_[8];
    gboolean border_flag_;
    char border_buf_[8];

    /* state for polylines */
    double arrow_size_;
    gboolean first_arrow_flag_;
    GnomeCanvasPoints points_;
    unsigned int points_size_;
};

#endif /* _ggcov_canvas_scenegen_H_ */
