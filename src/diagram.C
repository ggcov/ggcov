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

CVSID("$Id: diagram.C,v 1.2 2010-05-09 05:37:15 gnb Exp $");

diagram_factory_t *diagram_factory_t::all_ = 0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

diagram_t::diagram_t()
{
    static const rgb_t default_fg[cov::NUM_STATUS] =
    {
	rgb_t( COLOR_FG_COVERED ),
	rgb_t( COLOR_FG_PARTCOVERED ),
	rgb_t( COLOR_FG_UNCOVERED ),
	rgb_t( COLOR_FG_UNINSTRUMENTED ),
	rgb_t( COLOR_FG_SUPPRESSED )
    };
    static const rgb_t default_bg[cov::NUM_STATUS] =
    {
	rgb_t( COLOR_BG_COVERED ),
	rgb_t( COLOR_BG_PARTCOVERED ),
	rgb_t( COLOR_BG_UNCOVERED ),
	rgb_t( COLOR_BG_UNINSTRUMENTED ),
	rgb_t( COLOR_BG_SUPPRESSED )
    };

    memcpy(fg_by_status_, default_fg, sizeof(default_fg));
    memcpy(bg_by_status_, default_bg, sizeof(default_bg));
}

diagram_t::~diagram_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

diagram_t *
diagram_factory_t::create(const char *name)
{
    diagram_factory_t *df;

    for (df = all_ ; df ; df = df->next_)
    {
	if (!strcmp(name, df->name()))
	    return df->creator();
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
