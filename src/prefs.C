/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2005 Greg Banks <gnb@users.sourceforge.net>
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

#include "confsection.H"
#include "prefs.H"
#include "colors.h"

CVSID("$Id: prefs.C,v 1.11 2010-05-09 05:37:15 gnb Exp $");

prefs_t prefs;

/*
 * Note: depends implicitly on order of declarations in cov::status_t
 */
GdkColor *foregrounds_by_status[] =
{
    &prefs.covered_foreground,
    &prefs.partcovered_foreground,
    &prefs.uncovered_foreground,
    &prefs.uninstrumented_foreground,
    &prefs.suppressed_foreground

};
GdkColor *backgrounds_by_status[] = 
{
    &prefs.covered_background,
    &prefs.partcovered_background,
    &prefs.uncovered_background,
    &prefs.uninstrumented_background,
    &prefs.suppressed_background
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
colorstr(
    char *val,
    GdkColor *col,
    unsigned def_r,
    unsigned def_g,
    unsigned def_b)
{
    if (val == 0 || !gdk_color_parse(val, col))
    {
	col->red = (def_r<<8)|def_r;
	col->green = (def_g<<8)|def_g;
	col->blue = (def_b<<8)|def_b;
    }
    if (val != 0)
	g_free(val);
}

void
prefs_t::load()
{
    confsection_t *cs;
    
    cs = confsection_t::get("general");
    
    reuse_srcwin = cs->get_bool("reuse_srcwin", FALSE);
    reuse_summwin = cs->get_bool("reuse_summwin", FALSE);


    cs = confsection_t::get("colors");

    colorstr(cs->get_string("covered_foreground", 0),
    	     &covered_foreground, COLOR_FG_COVERED);
    colorstr(cs->get_string("covered_background", 0),
    	     &covered_background, COLOR_BG_COVERED);
    colorstr(cs->get_string("partcovered_foreground", 0),
    	     &partcovered_foreground, COLOR_FG_PARTCOVERED);
    colorstr(cs->get_string("partcovered_background", 0),
    	     &partcovered_background, COLOR_BG_PARTCOVERED);
    colorstr(cs->get_string("uncovered_foreground", 0),
    	     &uncovered_foreground, COLOR_FG_UNCOVERED);
    colorstr(cs->get_string("uncovered_background", 0),
    	     &uncovered_background, COLOR_BG_UNCOVERED);
    colorstr(cs->get_string("uninstrumented_foreground", 0),
    	     &uninstrumented_foreground, COLOR_FG_UNINSTRUMENTED);
    colorstr(cs->get_string("uninstrumented_background", 0),
    	     &uninstrumented_background, COLOR_BG_UNINSTRUMENTED);
    colorstr(cs->get_string("suppressed_foreground", 0),
    	     &suppressed_foreground, COLOR_FG_SUPPRESSED);
    colorstr(cs->get_string("suppressed_background", 0),
    	     &suppressed_background, COLOR_BG_SUPPRESSED);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
prefs_t::post_load(GtkWidget *w)
{
    GdkColormap *cmap;
    static gboolean first = TRUE;
    
    if (!first)
	return;
    first = FALSE;

    gtk_widget_realize(w);
    
    cmap = gtk_widget_get_colormap(w);

    gdk_colormap_alloc_color(cmap, &covered_foreground,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
    gdk_colormap_alloc_color(cmap, &covered_background,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
    gdk_colormap_alloc_color(cmap, &partcovered_foreground,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
    gdk_colormap_alloc_color(cmap, &partcovered_background,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
    gdk_colormap_alloc_color(cmap, &uncovered_foreground,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
    gdk_colormap_alloc_color(cmap, &uncovered_background,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
    gdk_colormap_alloc_color(cmap, &uninstrumented_foreground,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
    gdk_colormap_alloc_color(cmap, &uninstrumented_background,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
    gdk_colormap_alloc_color(cmap, &suppressed_foreground,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
    gdk_colormap_alloc_color(cmap, &suppressed_background,
    	    	    	     /*writeable*/FALSE, /*best_match*/TRUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *
strcolor(const GdkColor *col)
{
    static char buf[16];
    
    snprintf(buf, sizeof(buf), "#%04x%04x%04x", col->red, col->green, col->blue);
    return buf;
}

void
prefs_t::save()
{
    confsection_t *cs;
    
    cs = confsection_t::get("general");
    
    cs->set_bool("reuse_srcwin", reuse_srcwin);
    cs->set_bool("reuse_summwin", reuse_summwin);


    cs = confsection_t::get("colors");
    
    cs->set_string("covered_foreground", strcolor(&covered_foreground));
    cs->set_string("covered_background", strcolor(&covered_background));
    cs->set_string("partcovered_foreground", strcolor(&partcovered_foreground));
    cs->set_string("partcovered_background", strcolor(&partcovered_background));
    cs->set_string("uncovered_foreground", strcolor(&uncovered_foreground));
    cs->set_string("uncovered_background", strcolor(&uncovered_background));
    cs->set_string("uninstrumented_foreground", strcolor(&uninstrumented_foreground));
    cs->set_string("uninstrumented_background", strcolor(&uninstrumented_background));
    cs->set_string("suppressed_foreground", strcolor(&suppressed_foreground));
    cs->set_string("suppressed_background", strcolor(&suppressed_background));

    confsection_t::sync();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
