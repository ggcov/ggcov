/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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

#ifndef _ggcov_prefs_H_
#define _ggcov_prefs_H_ 1

#include "common.h"
#include "ui.h"

struct prefs_t
{
    /* General */
    gboolean reuse_srcwin;
    gboolean reuse_summwin;
    
    /* Colors */
    GdkColor covered_foreground;
    GdkColor covered_background;
    GdkColor partcovered_foreground;
    GdkColor partcovered_background;
    GdkColor uncovered_foreground;
    GdkColor uncovered_background;
    GdkColor uninstrumented_foreground;
    GdkColor uninstrumented_background;
    GdkColor suppressed_foreground;
    GdkColor suppressed_background;

    void load();
    void post_load(GtkWidget *w);
    void save();
};

extern prefs_t prefs;

extern GdkColor *foregrounds_by_status[];
extern GdkColor *backgrounds_by_status[];

#endif /* _ggcov_prefs_H_ */
