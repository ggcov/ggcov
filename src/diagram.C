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

#include "diagram.H"
#include "colors.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

diagram_t::diagram_t()
{
    fg_rgb_by_status_[cov::COVERED] = RGB(COLOR_FG_COVERED);
    fg_rgb_by_status_[cov::PARTCOVERED] = RGB(COLOR_FG_PARTCOVERED);
    fg_rgb_by_status_[cov::UNCOVERED] = RGB(COLOR_FG_UNCOVERED);
    fg_rgb_by_status_[cov::UNINSTRUMENTED] = RGB(COLOR_FG_UNINSTRUMENTED);
    fg_rgb_by_status_[cov::SUPPRESSED] = RGB(COLOR_FG_SUPPRESSED);

    bg_rgb_by_status_[cov::COVERED] = RGB(COLOR_BG_COVERED);
    bg_rgb_by_status_[cov::PARTCOVERED] = RGB(COLOR_BG_PARTCOVERED);
    bg_rgb_by_status_[cov::UNCOVERED] = RGB(COLOR_BG_UNCOVERED);
    bg_rgb_by_status_[cov::UNINSTRUMENTED] = RGB(COLOR_BG_UNINSTRUMENTED);
    bg_rgb_by_status_[cov::SUPPRESSED] = RGB(COLOR_BG_SUPPRESSED);
}

diagram_t::~diagram_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
diagram_t::set_fg(cov::status_t st, unsigned int rgb)
{
    fg_rgb_by_status_[st] = rgb;
}

void
diagram_t::set_bg(cov::status_t st, unsigned int rgb)
{
    bg_rgb_by_status_[st] = rgb;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
