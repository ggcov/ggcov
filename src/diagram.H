/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_diagram_H_
#define _ggcov_diagram_H_ 1

#include "common.h"
#include "cov.H"
#include "scenegen.H"
#include "geometry.H"

class diagram_t
{
public:
    diagram_t();
    virtual ~diagram_t();

    virtual const char *name() = 0;
    virtual const char *title() = 0;
    virtual void prepare() = 0;
    virtual void render(scenegen_t *) = 0;
    virtual void get_bounds(dbounds_t *) = 0;

    void set_fg(cov::status_t, unsigned int rgb);
    void set_bg(cov::status_t, unsigned int rgb);

protected:
    unsigned int    fg_rgb_by_status_[cov::NUM_STATUS];
    unsigned int    bg_rgb_by_status_[cov::NUM_STATUS];

private:
};

#endif /* _ggcov_diagram_H_ */
