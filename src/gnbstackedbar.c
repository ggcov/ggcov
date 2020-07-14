/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
 *
 * Derived from gtkprogress.c, gtkprogressbar.c which bore the message:
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
#  include <string.h>
#  include <stdio.h>
#else
#  include <stdio.h>
#endif
#if HAVE_MEMORY_H
#include <memory.h>
#endif

#include "uicommon.h"
#include "gnbstackedbar.h"
#include <gtk/gtkgc.h>

#ifndef STACKEDBAR_DEBUG
#define STACKEDBAR_DEBUG 0
#endif

static void gnb_stacked_bar_class_init(GnbStackedBarClass *klass);
static void gnb_stacked_bar_init(GnbStackedBar *sbar);
static void gnb_stacked_bar_paint(GnbStackedBar *sbar);
static void gnb_stacked_bar_finalize(GObject *object);
static void gnb_stacked_bar_realize(GtkWidget *widget);
static gint gnb_stacked_bar_expose(GtkWidget *widget, GdkEventExpose *event);
static void gnb_stacked_bar_size_allocate(GtkWidget *, GtkAllocation *);
static void gnb_stacked_bar_create_pixmap(GnbStackedBar *sbar);

static GtkWidgetClass *parent_class = NULL;


GtkType
gnb_stacked_bar_get_type(void)
{
    static GtkType stacked_bar_type = 0;

    if (!stacked_bar_type)
    {
	static const GtkTypeInfo stacked_bar_info =
	{
	    (char *)"GnbStackedBar",
	    sizeof (GnbStackedBar),
	    sizeof (GnbStackedBarClass),
	    (GtkClassInitFunc) gnb_stacked_bar_class_init,
	    (GtkObjectInitFunc) gnb_stacked_bar_init,
	    /* reserved_1 */ NULL,
	    /* reserved_2 */ NULL,
	    (GtkClassInitFunc) NULL
	};

	stacked_bar_type = gtk_type_unique(GTK_TYPE_WIDGET, &stacked_bar_info);
    }

    return stacked_bar_type;
}

static void
gnb_stacked_bar_class_init(GnbStackedBarClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;

    parent_class = (GtkWidgetClass *)gtk_type_class(GTK_TYPE_WIDGET);

    object_class->finalize = gnb_stacked_bar_finalize;

    widget_class->realize = gnb_stacked_bar_realize;
    widget_class->expose_event = gnb_stacked_bar_expose;
    widget_class->size_allocate = gnb_stacked_bar_size_allocate;
}

static void
gnb_stacked_bar_init(GnbStackedBar *sbar)
{
    sbar->nmetrics = 0;
    sbar->metrics = NULL;
    sbar->offscreen_pixmap = NULL;
}

static void
gnb_stacked_bar_realize(GtkWidget *widget)
{
    GnbStackedBar *sbar;
    GdkWindowAttr attr;
    gint mask;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(widget));

    sbar = GNB_STACKED_BAR(widget);
    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

    attr.window_type = GDK_WINDOW_CHILD;
    attr.x = widget->allocation.x;
    attr.y = widget->allocation.y;
    attr.width = widget->allocation.width;
    attr.height = widget->allocation.height;
    attr.wclass = GDK_INPUT_OUTPUT;
    attr.visual = gtk_widget_get_visual(widget);
    attr.colormap = gtk_widget_get_colormap(widget);
    attr.event_mask = gtk_widget_get_events(widget);
    attr.event_mask |= GDK_EXPOSURE_MASK;

    mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
				   &attr, mask);
    gdk_window_set_user_data(widget->window, sbar);

    widget->style = gtk_style_attach(widget->style, widget->window);
    gtk_style_set_background(widget->style, widget->window, GTK_STATE_ACTIVE);

    gnb_stacked_bar_create_pixmap(sbar);
}


static void
gnb_stacked_bar_finalize(GObject *object)
{
    GnbStackedBar *sbar;
    guint i;

    g_return_if_fail(object != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(object));

    sbar = GNB_STACKED_BAR(object);

    for (i = 0 ; i < sbar->nmetrics ; i++)
    {
	if (sbar->metrics[i].gc != NULL)
	    gtk_gc_release(sbar->metrics[i].gc);
    }
    if (sbar->metrics != NULL)
    {
	g_free(sbar->metrics);
	sbar->metrics = NULL;
    }
    sbar->nmetrics = 0;
}

static gint
gnb_stacked_bar_expose(GtkWidget *widget, GdkEventExpose *event)
{
    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(GNB_IS_STACKED_BAR(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    if (GTK_WIDGET_DRAWABLE(widget))
	gdk_draw_pixmap(widget->window,
		     widget->style->black_gc,
		     GNB_STACKED_BAR(widget)->offscreen_pixmap,
		     event->area.x, event->area.y,
		     event->area.x, event->area.y,
		     event->area.width, event->area.height);

    return FALSE;
}


static void
gnb_stacked_bar_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    g_return_if_fail(widget != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(widget));
    g_return_if_fail(allocation != NULL);

#if STACKEDBAR_DEBUG
    fprintf(stderr, "%s: size_allocate %d %d %d %d\n",
		widget->name,
		allocation->x, allocation->y,
		allocation->width, allocation->height);
#endif

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED(widget))
    {
	gdk_window_move_resize(widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

	gnb_stacked_bar_create_pixmap(GNB_STACKED_BAR(widget));
    }
}

static void
gnb_stacked_bar_create_pixmap(GnbStackedBar *sbar)
{
    GtkWidget *widget;

    g_return_if_fail(sbar != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(sbar));

    if (GTK_WIDGET_REALIZED(sbar))
    {
	widget = GTK_WIDGET(sbar);

	if (sbar->offscreen_pixmap != NULL)
	    gdk_pixmap_unref(sbar->offscreen_pixmap);

	sbar->offscreen_pixmap = gdk_pixmap_new(widget->window,
						widget->allocation.width,
						widget->allocation.height,
						-1);
	gnb_stacked_bar_paint(sbar);
    }
}


GLADE_CALLBACK GtkWidget *
gnb_stacked_bar_new(void)
{
    GtkWidget *widget;

    widget = gtk_widget_new(GNB_TYPE_STACKED_BAR, NULL);

    return widget;
}


static void
gnb_stacked_bar_paint(GnbStackedBar *sbar)
{
    GtkWidget *widget;
    guint x, xthick, ythick;
    unsigned long total, space;
    guint i;
    GnbStackedBarMetric *metric;

    g_return_if_fail(sbar != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(sbar));

    if (sbar->offscreen_pixmap == NULL)
	return;

    widget = GTK_WIDGET(sbar);

    /* The style [xy]thickness need to be halved for SHADOW_IN */
    xthick = widget->style->xthickness/2;
    ythick = widget->style->ythickness/2;
    space = widget->allocation.width - 2 * xthick;

    total = 0;
    for (i = 0 ; i < sbar->nmetrics ; i++)
	total += sbar->metrics[i].value;

#if STACKEDBAR_DEBUG
    fprintf(stderr, "%s: total=%lu xthick=%d ythick=%d\n",
		widget->name, total, xthick, ythick);
#endif

    /* divide screen space up proportionally */
    if (total == 0)
    {
	/* avoid division by zero: give all space to 0th metric */
	/* TODO: not right!! */
	for (i = 0 ; i < sbar->nmetrics ; i++)
	    sbar->metrics[i].length = (i ? 0 : space);
    }
    else
    {
	x = 0;
	for (i = 0 ; i < sbar->nmetrics ; i++)
	{
	    x += sbar->metrics[i].length =
		    (sbar->metrics[i].value * space + total/2) / total;
	}

	/* add the roundoff error to the last nonzero metric */
	x = space - x;
	if (x)
	{
#if STACKEDBAR_DEBUG
	    fprintf(stderr, "%s: cumulative rounding error %d\n",
		    widget->name, x);
#endif
	    for (i = sbar->nmetrics-1 ; i >= 0 ; --i)
	    {
		if (sbar->metrics[i].length > 0)
		{
#if STACKEDBAR_DEBUG
		    fprintf(stderr, "%s:   applied to metric %d\n",
			    widget->name, i);
#endif
		    sbar->metrics[i].length += x;
		    break;
		}
	    }
	}
    }

    /* paint the outside shadow */
#if STACKEDBAR_DEBUG
    fprintf(stderr, "%s: PAINTBOX(trough, %d, %d, %d, %d)\n",
		   widget->name,
		   0, 0,
		   widget->allocation.width,
		   widget->allocation.height);
#endif
    gtk_paint_box(widget->style,
		  sbar->offscreen_pixmap,
		  GTK_STATE_NORMAL, GTK_SHADOW_IN,
		  NULL, widget, "trough",
		  0, 0,
		  widget->allocation.width,
		  widget->allocation.height);

    if (widget->allocation.width <= (gint)(2*xthick) ||
	widget->allocation.height <= (gint)(2*ythick))
	return;

    x = xthick;
    for (i = 0 ; i < sbar->nmetrics ; i++)
    {
	metric = &sbar->metrics[i];

	if (!metric->length)
	    continue;

	if (!metric->color_allocated)
	{
	    metric->color_allocated = TRUE;
	    gdk_color_alloc(widget->style->colormap, &metric->color);
	}
	if (metric->gc == NULL)
	{
	    GdkGCValues gcvals;
	    unsigned long gcmask = 0;

	    gcvals.foreground = metric->color;
	    gcmask |= GDK_GC_FOREGROUND;
	    metric->gc = gtk_gc_get(widget->style->depth,
				  widget->style->colormap,
				  &gcvals,
				  (GdkGCValuesMask)gcmask);
	}

#if STACKEDBAR_DEBUG
	fprintf(stderr, "%s: metric[%d]length=%d fillrect(%u, %u, %u, %u)\n",
			widget->name,
			i, metric->length,
			x, ythick,
			metric->length,
			widget->allocation.height - 2*ythick);
#endif
	gdk_draw_rectangle(
		    sbar->offscreen_pixmap,
		    metric->gc,
		    /*filled*/TRUE,
		    x, ythick,
		    metric->length,
		    widget->allocation.height - 2*ythick);
	x += metric->length;
    }
}

/*******************************************************************/

void
gnb_stacked_bar_set_num_metrics(GnbStackedBar *sbar, guint nmetrics)
{
    guint i;
    GnbStackedBarMetric *metric;

    g_return_if_fail(sbar != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(sbar));

    if (sbar->metrics != NULL)
    {
	for (i = 0 ; i < sbar->nmetrics ; i++)
	{
	    if (sbar->metrics[i].gc != NULL)
		gtk_gc_release(sbar->metrics[i].gc);
	}
	g_free(sbar->metrics);
    }

    sbar->nmetrics = nmetrics;
    sbar->metrics = g_new(GnbStackedBarMetric, nmetrics);

    for (i = 0 ; i < sbar->nmetrics ; i++)
    {
	metric = &sbar->metrics[i];

	metric->value = 0;
	metric->length = 0;
	memset(&metric->color, 0, sizeof(GdkColor));
	metric->gc = 0;
	metric->color_set = 0;
	metric->color_allocated = 0;
    }
}

/*******************************************************************/

void
gnb_stacked_bar_set_metric_color(
    GnbStackedBar *sbar,
    guint idx,
    const GdkColor *color)
{
    gboolean changed = FALSE;
    GnbStackedBarMetric *metric;

    g_return_if_fail(sbar != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(sbar));
    g_return_if_fail(idx < sbar->nmetrics);

    metric = &sbar->metrics[idx];

    if (color != NULL)
    {
	changed = !metric->color_set;
	metric->color_set = TRUE;
	metric->color = *color;
    }
    else
    {
	changed = metric->color_set;
	metric->color_set = FALSE;
	/* TODO: unallocate the color */
    }

    if (changed)
    {
	if (metric->gc != NULL)
	{
	    gtk_gc_release(metric->gc);
	    metric->gc = NULL;
	}
	if (GTK_WIDGET_DRAWABLE(GTK_WIDGET(sbar)))
	    gtk_widget_queue_draw(GTK_WIDGET(sbar));
    }
}

/*******************************************************************/

void
gnb_stacked_bar_set_metric_colors(
    GnbStackedBar *sbar,
    const GdkColor **colorp)
{
    guint i;
    guint nchanged = 0;
    GnbStackedBarMetric *metric;

    g_return_if_fail(sbar != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(sbar));

    for (i = 0, metric = sbar->metrics ;
	 i < sbar->nmetrics ;
	 i++, metric++)
    {
	gboolean changed = FALSE;

	if (colorp != NULL && *colorp != NULL)
	{
	    changed = !metric->color_set;
	    metric->color_set = TRUE;
	    metric->color = **colorp;
	}
	else
	{
	    changed = metric->color_set;
	    metric->color_set = FALSE;
	    /* TODO: unallocate the color */
	}

	if (changed)
	{
	    nchanged++;
	    if (metric->gc != NULL)
	    {
		gtk_gc_release(metric->gc);
		metric->gc = NULL;
	    }
	}

	if (colorp != NULL)
	    colorp++;
    }

    if (nchanged && GTK_WIDGET_DRAWABLE(GTK_WIDGET(sbar)))
	gtk_widget_queue_draw(GTK_WIDGET(sbar));
}

/*******************************************************************/

void
gnb_stacked_bar_set_metric_value(
    GnbStackedBar *sbar,
    guint idx,
    unsigned long value)
{
    gboolean changed = FALSE;
    GnbStackedBarMetric *metric;

    g_return_if_fail(sbar != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(sbar));
    g_return_if_fail(idx < sbar->nmetrics);

    metric = &sbar->metrics[idx];

    if (metric->value != value)
    {
	changed = TRUE;
	metric->value = value;
    }

    if (changed && GTK_WIDGET_DRAWABLE(GTK_WIDGET(sbar)))
	gtk_widget_queue_draw(GTK_WIDGET(sbar));
}

/*******************************************************************/

void
gnb_stacked_bar_set_metric_values(
    GnbStackedBar *sbar,
    const unsigned long *valuep)
{
    gboolean changed = FALSE;
    guint i;
    GnbStackedBarMetric *metric;

    g_return_if_fail(sbar != NULL);
    g_return_if_fail(GNB_IS_STACKED_BAR(sbar));

    for (i = 0, metric = sbar->metrics ;
	 i < sbar->nmetrics ;
	 i++, metric++, valuep++)
    {
	if (metric->value != *valuep)
	{
	    changed = TRUE;
	    metric->value = *valuep;
	}
    }

    if (changed && GTK_WIDGET_DRAWABLE(GTK_WIDGET(sbar)))
	gtk_widget_queue_draw(GTK_WIDGET(sbar));
}

/*******************************************************************/
