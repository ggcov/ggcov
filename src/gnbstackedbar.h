/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2004 Greg Banks <gnb@alphalink.com.au>
 *
 * Derived from gtkprogressbar.h which bore the message:
 * GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GNBSTACKEDBAR_H__
#define __GNBSTACKEDBAR_H__


#include <gdk/gdk.h>
#include <gtk/gtkprogressbar.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GNB_TYPE_STACKED_BAR	    	(gnb_stacked_bar_get_type ())
#define GNB_STACKED_BAR(obj)            (GTK_CHECK_CAST ((obj), GNB_TYPE_STACKED_BAR, GnbStackedBar))
#define GNB_STACKED_BAR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNB_TYPE_STACKED_BAR, GnbStackedBarClass))
#define GNB_IS_STACKED_BAR(obj)         (GTK_CHECK_TYPE ((obj), GNB_TYPE_STACKED_BAR))
#define GNB_IS_STACKED_BAR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNB_TYPE_STACKED_BAR))


typedef struct _GnbStackedBar       	GnbStackedBar;
typedef struct _GnbStackedBarMetric 	GnbStackedBarMetric;
typedef struct _GnbStackedBarClass  	GnbStackedBarClass;

struct _GnbStackedBarMetric
{
    unsigned long value;    	/* client value */
    guint length;     	    	/* length in pixels */
    GdkColor color;
    GdkGC *gc;
    gboolean color_set;
    gboolean color_allocated;
};

struct _GnbStackedBar
{
    GtkWidget widget;

    guint nmetrics;
    GnbStackedBarMetric *metrics;
    GdkPixmap *offscreen_pixmap;
    
    /* orientation is hardcoded for GTK_PROGRESS_LEFT_TO_RIGHT */
};

struct _GnbStackedBarClass
{
    GtkWidgetClass parent_class;
};


GtkType    gnb_stacked_bar_get_type             (void);
GtkWidget* gnb_stacked_bar_new                  (void);


void	   gnb_stacked_bar_set_num_metrics 	(GnbStackedBar *sbar,
    	    	    	    	    	    	 guint nmetrics);
void       gnb_stacked_bar_set_metric_color 	(GnbStackedBar *sbar,
    	    	    	    	    	    	 guint metric,
    	    	    	    	    	    	 const GdkColor *);
void       gnb_stacked_bar_set_metric_colors 	(GnbStackedBar *sbar,
    	    	    	    	    	    	 const GdkColor **);
void       gnb_stacked_bar_set_metric_value 	(GnbStackedBar *sbar,
    	    	    	    	    	    	 guint metric,
    	    	    	    	    	    	 unsigned long value);
void       gnb_stacked_bar_set_metric_values 	(GnbStackedBar *sbar,
    	    	    	    	    	    	 const unsigned long *values);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GNBSTACKEDBAR_H__ */
