/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
 *
 * Derived from GnbProgressBar.h which bore the message:
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

#if HAVE_CONFIG_H
#  include <config.h>
#  if STDC_HEADERS
#    include <string.h>
#    include <stdio.h>
#  endif
#else
#  include <stdio.h>
#endif
#if HAVE_MEMORY_H
#include <memory.h>
#endif

#include "uicommon.h"
#include "gnbprogressbar.h"
#include <gtk/gtkgc.h>


static void gnb_progress_bar_class_init    (GnbProgressBarClass *klass);
static void gnb_progress_bar_init          (GnbProgressBar      *pbar);
static void gnb_progress_bar_paint         (GtkProgress         *progress);
static void gnb_progress_bar_real_update   (GtkProgress         *progress);
#if GTK2
static void gnb_progress_bar_finalize	   (GObject	        *object);
#else
static void gnb_progress_bar_finalize	   (GtkObject	        *object);
#endif

GtkType
gnb_progress_bar_get_type (void)
{
    static GtkType progress_bar_type = 0;

    if (!progress_bar_type)
    {
	static const GtkTypeInfo progress_bar_info =
	{
	    "GnbProgressBar",
	    sizeof (GnbProgressBar),
	    sizeof (GnbProgressBarClass),
	    (GtkClassInitFunc) gnb_progress_bar_class_init,
	    (GtkObjectInitFunc) gnb_progress_bar_init,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL
	};

	progress_bar_type = gtk_type_unique (GTK_TYPE_PROGRESS_BAR, &progress_bar_info);
    }

    return progress_bar_type;
}

static void
gnb_progress_bar_class_init (GnbProgressBarClass *klass)
{
    GtkProgressClass *progress_class;

    progress_class = (GtkProgressClass *) klass;

#if GTK2
    ((GObjectClass *)klass)->finalize = gnb_progress_bar_finalize;
#else
    ((GtkObjectClass *)klass)->finalize = gnb_progress_bar_finalize;
#endif
    progress_class->paint = gnb_progress_bar_paint;
    progress_class->update = gnb_progress_bar_real_update;
}

static void
gnb_progress_bar_init (GnbProgressBar *pbar)
{
    memset(&pbar->thumb_color, 0, sizeof(GdkColor));
    memset(&pbar->trough_color, 0, sizeof(GdkColor));
    pbar->thumb_gc = NULL;
    pbar->trough_gc = NULL;
    pbar->thumb_color_set = FALSE;
    pbar->trough_color_set = FALSE;
    pbar->thumb_color_allocated = FALSE;
    pbar->trough_color_allocated = FALSE;
}

static void
gnb_progress_bar_finalize(
#if GTK2
    GObject *object
#else
    GtkObject *object
#endif
    )
{
    GnbProgressBar *pbar = (GnbProgressBar *)object;
    
    if (pbar->trough_gc != NULL)
    	gtk_gc_release(pbar->trough_gc);
    if (pbar->thumb_gc != NULL)
    	gtk_gc_release(pbar->thumb_gc);

#if 0
    if (pbar->trough_color_allocated)
	gdk_colormap_free_colors(widget->style->colormap, &pbar->trough_color, 1);
    if (pbar->thumb_color_allocated)
	gdk_colormap_free_colors(widget->style->colormap, &pbar->thumb_color, 1);
#endif
}


GtkWidget*
gnb_progress_bar_new (void)
{
    GtkWidget *pbar;

    pbar = gtk_widget_new (GNB_TYPE_PROGRESS_BAR, "adjustment", NULL, NULL);

    return pbar;
}

GtkWidget*
gnb_progress_bar_new_with_adjustment (GtkAdjustment *adjustment)
{
    GtkWidget *pbar;

    g_return_val_if_fail (adjustment != NULL, NULL);
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);

    pbar = gtk_widget_new (GNB_TYPE_PROGRESS_BAR,
			   "adjustment", adjustment,
			   NULL);

    return pbar;
}

#if GTK2
#define xthickness(w)	((w)->style->xthickness)
#define ythickness(w)	((w)->style->ythickness)
#else
#define xthickness(w)	((w)->style->klass->xthickness)
#define ythickness(w)	((w)->style->klass->ythickness)
#endif

static void
gnb_progress_bar_paint (GtkProgress *progress)
{
    GnbProgressBar *pbar;
    GtkProgressBar *ppbar;
    GtkWidget *widget;
    gint amount;
    gint space = 0;
    gfloat percentage;
    GdkRectangle rect;

    g_return_if_fail (progress != NULL);
    g_return_if_fail (GNB_IS_PROGRESS_BAR (progress));

    pbar = GNB_PROGRESS_BAR (progress);
    ppbar = GTK_PROGRESS_BAR (progress);
    widget = GTK_WIDGET (progress);

    if (ppbar->orientation == GTK_PROGRESS_LEFT_TO_RIGHT ||
	ppbar->orientation == GTK_PROGRESS_RIGHT_TO_LEFT)
	space = widget->allocation.width - 2 * xthickness(widget);
    else
	space = widget->allocation.height - 2 * ythickness(widget);

    percentage = gtk_progress_get_current_percentage(progress);
    
#if DEBUG > 3
    fprintf(stderr, "\n");
#endif

    if (progress->offscreen_pixmap)
    {
    	/* paint the outside shadow */
#if DEBUG > 3
	fprintf(stderr, "PAINTBOX(trough, %d, %d, %d, %d)\n",
		       0, 0,
		       widget->allocation.width,
		       widget->allocation.height);
#endif
	gtk_paint_box (widget->style,
		       progress->offscreen_pixmap,
		       GTK_STATE_NORMAL, GTK_SHADOW_IN, 
		       NULL, widget, "trough",
		       0, 0,
		       widget->allocation.width,
		       widget->allocation.height);
		       
	/* fill in the trough colour */
	if (pbar->trough_color_set &&
	    widget->allocation.width > 4 &&
	    widget->allocation.height > 4)
	{
	    if (!pbar->trough_color_allocated)
	    {
		pbar->trough_color_allocated = TRUE;
	    	gdk_color_alloc(widget->style->colormap, &pbar->trough_color);
	    }
	    if (pbar->trough_gc == NULL)
	    {
	    	GdkGCValues gcvals;
		unsigned long gcmask = 0;
		
		gcvals.foreground = pbar->trough_color;
		gcmask |= GDK_GC_FOREGROUND;
		pbar->trough_gc = gtk_gc_get(widget->style->depth,
		    	    	    	     widget->style->colormap,
					     &gcvals,
					     (GdkGCValuesMask)gcmask);
	    }
	    
#if DEBUG > 3
	    fprintf(stderr, "FILLRECT(trough, 2, 2, %d, %d)\n",
			widget->allocation.width - 4,
			widget->allocation.height - 4);
#endif
    	    gdk_draw_rectangle(
	    	    	progress->offscreen_pixmap,
			pbar->trough_gc,
			/*filled*/TRUE,
			2, 2,
			widget->allocation.width - 4,
			widget->allocation.height - 4);
    	}

    	/* draw the thumb */	
	amount = (int)(percentage * space + 0.5);

	if (amount > 0)
	{
	    switch (ppbar->orientation)
	    {

	      case GTK_PROGRESS_LEFT_TO_RIGHT:

		rect.x = xthickness(widget);
		rect.y = ythickness(widget);
		rect.width = amount;
		rect.height = widget->allocation.height - ythickness(widget) * 2;
		break;

	      case GTK_PROGRESS_RIGHT_TO_LEFT:

		rect.x = widget->allocation.width - xthickness(widget) - amount;
		rect.y = ythickness(widget);
		rect.width = amount;
		rect.height = widget->allocation.height - ythickness(widget) * 2;
		break;

	      case GTK_PROGRESS_BOTTOM_TO_TOP:

		rect.x = xthickness(widget);
		rect.y = widget->allocation.height - ythickness(widget) - amount;
		rect.width = widget->allocation.width - xthickness(widget) * 2;
		rect.height = amount;
		break;

	      case GTK_PROGRESS_TOP_TO_BOTTOM:

		rect.x = xthickness(widget);
		rect.y = ythickness(widget);
		rect.width = widget->allocation.width - xthickness(widget) * 2;
		rect.height = amount;
		break;

	      default:
		break;
	    }

	    if (pbar->thumb_color_set)
	    {
		if (!pbar->thumb_color_allocated)
		{
		    pbar->thumb_color_allocated = TRUE;
	    	    gdk_color_alloc(widget->style->colormap, &pbar->thumb_color);
		}
		if (pbar->thumb_gc == NULL)
		{
	    	    GdkGCValues gcvals;
		    unsigned long gcmask = 0;

		    gcvals.foreground = pbar->thumb_color;
		    gcmask |= GDK_GC_FOREGROUND;
		    pbar->thumb_gc = gtk_gc_get(widget->style->depth,
		    	    	    		 widget->style->colormap,
						 &gcvals,
						 (GdkGCValuesMask)gcmask);
		}

#if DEBUG > 3
		fprintf(stderr, "FILLRECT(bar, %d, %d, %d, %d)\n",
	    	    	    (int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
#endif
    		gdk_draw_rectangle(
	    	    	    progress->offscreen_pixmap,
			    pbar->thumb_gc,
			    /*filled*/TRUE,
			   rect.x, rect.y, rect.width, rect.height);
	    }
	    else
	    {
#if DEBUG > 3
		fprintf(stderr, "PAINTBOX(bar, %d, %d, %d, %d)\n",
	    	    	    (int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
#endif
		gtk_paint_box (widget->style,
			   progress->offscreen_pixmap,
			   GTK_STATE_PRELIGHT, GTK_SHADOW_OUT,
			   NULL, widget, "bar",
			   rect.x, rect.y, rect.width, rect.height);
	    }
	}
    }
}

static void
gnb_progress_bar_real_update(GtkProgress *progress)
{
    GtkProgressClass *progress_class;
    
#if DEBUG > 3
    fprintf(stderr, "gnb_progress_bar_real_update\n");
#endif

#if GTK2
    progress_class = (GtkProgressClass *)g_type_class_peek_parent(g_type_class_peek(GNB_TYPE_PROGRESS_BAR));
#else
    progress_class = (GtkProgressClass *)gtk_type_parent_class(GNB_TYPE_PROGRESS_BAR);
#endif
    progress_class->update(progress);
    gnb_progress_bar_paint(progress);
}


/*******************************************************************/

void
gnb_progress_bar_set_trough_color (GnbProgressBar     *pbar,
				const GdkColor *color)
{
    gboolean changed = FALSE;
    g_return_if_fail (pbar != NULL);
    g_return_if_fail (GNB_IS_PROGRESS_BAR (pbar));

    if (color != NULL)
    {
    	changed = !pbar->trough_color_set;
	pbar->trough_color_set = TRUE;
	pbar->trough_color = *color;
    }
    else
    {
    	changed = pbar->trough_color_set;
	pbar->trough_color_set = FALSE;
	/* TODO: unallocate the color */
    }

    if (changed)
    {
    	if (pbar->trough_gc != NULL)
	{
	    gtk_gc_release(pbar->trough_gc);
	    pbar->trough_gc = NULL;
	}
	if (GTK_WIDGET_DRAWABLE(GTK_WIDGET(pbar)))
	    gtk_widget_queue_draw(GTK_WIDGET(pbar));
    }
}

void
gnb_progress_bar_set_thumb_color (GnbProgressBar     *pbar,
				const GdkColor *color)
{
    gboolean changed = FALSE;
    g_return_if_fail (pbar != NULL);
    g_return_if_fail (GNB_IS_PROGRESS_BAR (pbar));

    if (color != NULL)
    {
    	changed = !pbar->thumb_color_set;
	pbar->thumb_color_set = TRUE;
	pbar->thumb_color = *color;
    }
    else
    {
    	changed = pbar->thumb_color_set;
	pbar->thumb_color_set = FALSE;
	/* TODO: unallocate the color */
    }

    if (changed)
    {
    	if (pbar->thumb_gc != NULL)
	{
	    gtk_gc_release(pbar->thumb_gc);
	    pbar->thumb_gc = NULL;
	}
	if (GTK_WIDGET_DRAWABLE(GTK_WIDGET(pbar)))
	    gtk_widget_queue_draw(GTK_WIDGET(pbar));
    }
}

/*******************************************************************/
