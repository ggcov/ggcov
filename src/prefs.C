/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
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

CVSID("$Id: prefs.C,v 1.3 2003-03-11 21:33:23 gnb Exp $");

prefs_t prefs;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
colorstr(const char *val, GdkColor *col, const char *deflt)
{
    if (val == 0 || !gdk_color_parse(val, col))
    	gdk_color_parse(deflt, col);
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
    	     &covered_foreground, "#00c000");
    colorstr(cs->get_string("covered_background", 0),
    	     &covered_background, "#80d080");
    colorstr(cs->get_string("partcovered_foreground", 0),
    	     &partcovered_foreground, "#a0a000");
    colorstr(cs->get_string("partcovered_background", 0),
    	     &partcovered_background, "#d0d080");
    colorstr(cs->get_string("uncovered_foreground", 0),
    	     &uncovered_foreground, "#c00000");
    colorstr(cs->get_string("uncovered_background", 0),
    	     &uncovered_background, "#d08080");
    colorstr(cs->get_string("uninstrumented_foreground", 0),
    	     &uninstrumented_foreground, "#000000");
    colorstr(cs->get_string("uninstrumented_background", 0),
    	     &uninstrumented_background, "#a0a0a0");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
prefs_t::post_load(GtkWidget *w)
{
    GdkColormap *cmap;
    
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

    confsection_t::sync();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
