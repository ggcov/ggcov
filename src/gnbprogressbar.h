/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
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

#ifndef __GNB_PROGRESS_BAR_H__
#define __GNB_PROGRESS_BAR_H__


#include <gdk/gdk.h>
#include <gtk/gtkprogressbar.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GNB_TYPE_PROGRESS_BAR            (gnb_progress_bar_get_type ())
#define GNB_PROGRESS_BAR(obj)            (GTK_CHECK_CAST ((obj), GNB_TYPE_PROGRESS_BAR, GnbProgressBar))
#define GNB_PROGRESS_BAR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNB_TYPE_PROGRESS_BAR, GnbProgressBarClass))
#define GNB_IS_PROGRESS_BAR(obj)         (GTK_CHECK_TYPE ((obj), GNB_TYPE_PROGRESS_BAR))
#define GNB_IS_PROGRESS_BAR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNB_TYPE_PROGRESS_BAR))


typedef struct _GnbProgressBar       GnbProgressBar;
typedef struct _GnbProgressBarClass  GnbProgressBarClass;

struct _GnbProgressBar
{
  GtkProgressBar progressbar;

  GdkColor thumb_color;
  GdkColor trough_color;

  GdkGC *thumb_gc;
  GdkGC *trough_gc;
  
  gboolean thumb_color_set:1;
  gboolean trough_color_set:1;
  gboolean thumb_color_allocated:1;
  gboolean trough_color_allocated:1;
};

struct _GnbProgressBarClass
{
  GtkProgressBarClass parent_class;
};


GtkType    gnb_progress_bar_get_type             (void);
GtkWidget* gnb_progress_bar_new                  (void);
GtkWidget* gnb_progress_bar_new_with_adjustment  (GtkAdjustment  *adjustment);

void       gnb_progress_bar_set_trough_color     (GnbProgressBar *pbar,
    	    	    	    	    	    	  const GdkColor *);
void       gnb_progress_bar_set_thumb_color      (GnbProgressBar *pbar,
    	    	    	    	    	    	  const GdkColor *);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GNB_PROGRESS_BAR_H__ */
