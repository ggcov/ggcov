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

#include "cov.H"
#include "canvas_function_popup.H"
#include "summarywin.H"
#include "sourcewin.H"
#include "ui.h"

CVSID("$Id: canvas_function_popup.C,v 1.2 2010-05-09 05:37:14 gnb Exp $");

canvas_function_popup_t::widgets_t canvas_function_popup_t::widgets_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

canvas_function_popup_t::canvas_function_popup_t(
    GnomeCanvasItem *item,
    cov_function_t *fn)
 :  item_(item),
    function_(fn)
{
    cov_scope_t *scope = new cov_function_scope_t(fn);
    snprintf(coverage_, sizeof(coverage_), "%lu/%lu %4.2f%%",
		    scope->get_stats()->blocks_executed(),
		    scope->get_stats()->blocks_total(),
		    100.0 * scope->get_stats()->blocks_fraction());
    delete scope;

    gtk_signal_connect(GTK_OBJECT(item), "event",
	GTK_SIGNAL_FUNC(on_item_event), this);
    gtk_signal_connect(GTK_OBJECT(item), "destroy",
	GTK_SIGNAL_FUNC(on_item_destroy), this);
    widgets_.refcount_++;
}

canvas_function_popup_t::~canvas_function_popup_t()
{
    if (--widgets_.refcount_ == 0 && widgets_.popup_ != 0)
    {
	gtk_widget_destroy(widgets_.popup_);
	widgets_.popup_ = 0;
	gtk_widget_destroy(widgets_.menu_);
	widgets_.menu_ = 0;
    }
    if (widgets_.current_ == this)
	widgets_.current_ = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
canvas_function_popup_t::set_foreground(const char *s)
{
    strncpy(foreground_, s, sizeof(foreground_));
    foreground_[sizeof(foreground_)-1] = '\0';
}

void
canvas_function_popup_t::set_background(const char *s)
{
    strncpy(background_, s, sizeof(background_));
    background_[sizeof(background_)-1] = '\0';
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0
static const char *
describe_event(GnomeCanvas *canvas, const GdkEvent *event)
{
    static char buf[256];

    switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
	return "ENTER";
    case GDK_LEAVE_NOTIFY:
	return "LEAVE";
    case GDK_MOTION_NOTIFY:

	int cx, cy;
	if (canvas != 0)
	    gnome_canvas_w2c(canvas, event->motion.x, event->motion.y, &cx, &cy);

	int cox, coy;
	cox = (int)event->motion.x_root - cx;
	coy = (int)event->motion.y_root - cy;

	snprintf(buf, sizeof(buf), "MOTION(%g,%g = %u,%u) + %u,%u",
	    event->motion.x,
	    event->motion.y,
	    cx, cy,
	    cox, coy);

	return buf;
    case GDK_BUTTON_PRESS:
	snprintf(buf, sizeof(buf), "BUTTON_PRESS(button=%u)", event->button.button);
	return buf;
    case GDK_2BUTTON_PRESS:
	snprintf(buf, sizeof(buf), "BUTTON_2PRESS(button=%u)", event->button.button);
	return buf;
    case GDK_3BUTTON_PRESS:
	snprintf(buf, sizeof(buf), "BUTTON_3PRESS(button=%u)", event->button.button);
	return buf;
    case GDK_BUTTON_RELEASE:
	snprintf(buf, sizeof(buf), "BUTTON_RELEASE(button=%u)", event->button.button);
	return buf;
    default:
	snprintf(buf, sizeof(buf), "%d", event->type);
	return buf;
    }
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_canfn_view_summary_activate(GtkWidget *w, GdkEvent *event, gpointer closure)
{
//    fprintf(stderr, "on_canfn_view_summary_activate:\n");
    cov_function_t *fn = canvas_function_popup_t::widgets_.current_->function_;
    summarywin_t::show_function(fn);
}

GLADE_CALLBACK void
on_canfn_view_call_butterfly_activate(GtkWidget *w, GdkEvent *event, gpointer closure)
{
    fprintf(stderr, "on_canfn_view_call_butterfly_activate:\n");
//    cov_function_t *fn = canvas_function_popup_t::widgets_.current_->function_;
}

GLADE_CALLBACK void
on_canfn_view_source_activate(GtkWidget *w, GdkEvent *event, gpointer closure)
{
//    fprintf(stderr, "on_canfn_view_source_activate:\n");
    cov_function_t *fn = canvas_function_popup_t::widgets_.current_->function_;
    sourcewin_t::show_function(fn);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
canvas_function_popup_t::on_button_press_event(
    GtkWidget *w,
    GdkEvent *event,
    gpointer closure)
{
//    fprintf(stderr, "on_button_press_event: %s\n",
//          describe_event(0, event));
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 3)
	widgets_.current_->show_menu(event);
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
canvas_function_popup_t::on_item_event(
    GnomeCanvasItem *item,
    GdkEvent *event,
    gpointer closure)
{
    canvas_function_popup_t *fpop = (canvas_function_popup_t *)closure;

#if 0
    fprintf(stderr, "on_item_event: %s\n",
	    describe_event(item->canvas, event));
#endif

    switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
	fpop->show(event);
	break;
    default:
	break;
    }

    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
canvas_function_popup_t::on_leave_event(
    GtkWidget *window,
    GdkEvent *event,
    gpointer closure)
{
//    canvas_function_popup_t *fpop = (canvas_function_popup_t *)closure;
//    fprintf(stderr, "on_canvas_function_popup_leave_event\n");
    gtk_widget_hide(window);
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
canvas_function_popup_t::on_item_destroy(GnomeCanvasItem *item, gpointer closure)
{
    canvas_function_popup_t *fpop = (canvas_function_popup_t *)closure;

    delete fpop;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
canvas_function_popup_t::show(GdkEvent *event)
{
    GdkColor fg, bg;
    widgets_t *w = &widgets_;

    if (w->popup_ == 0)
    {
	GladeXML *xml;

	/* load the interface & connect signals */
	xml = ui_load_tree("canvas_function_popup");

	w->popup_ = glade_xml_get_widget(xml, "canvas_function_popup");
	w->border_ = w->popup_;
	w->background_ = glade_xml_get_widget(xml, "canfn_eventbox");
	w->function_label_ = glade_xml_get_widget(xml, "canfn_function_label");
	w->source_label_ = glade_xml_get_widget(xml, "canfn_source_label");
	w->coverage_label_ = glade_xml_get_widget(xml, "canfn_coverage_label");

	gtk_signal_connect(GTK_OBJECT(w->popup_), "leave_notify_event",
	    GTK_SIGNAL_FUNC(on_leave_event), (gpointer)0);
	gtk_signal_connect(GTK_OBJECT(w->background_), "button_press_event",
	    GTK_SIGNAL_FUNC(on_button_press_event), (gpointer)0);

	/* load the interface & connect signals */
	xml = ui_load_tree("canvas_function_menu");

	w->menu_ = glade_xml_get_widget(xml, "canvas_function_menu");
    }

    w->current_ = this;

    gtk_label_set_text(GTK_LABEL(w->function_label_), function_->name());
    gtk_label_set_text(GTK_LABEL(w->source_label_), function_->file()->minimal_name());
    gtk_label_set_text(GTK_LABEL(w->coverage_label_), coverage_);

    gnome_canvas_get_color(item_->canvas, foreground_, &fg);
    gtk_widget_modify_bg(w->border_, GTK_STATE_NORMAL, &fg);
    gtk_widget_modify_fg(w->background_, GTK_STATE_NORMAL, &fg);

    gnome_canvas_get_color(item_->canvas, background_, &bg);
    gtk_widget_modify_bg(w->background_, GTK_STATE_NORMAL, &bg);

    int cx, cy;
    gnome_canvas_w2c(item_->canvas,
		     event->crossing.x, event->crossing.y,
		     &cx, &cy);
    cx = (int)item_->x1 + ((int)event->crossing.x_root - cx);
    cy = (int)item_->y1 + ((int)event->crossing.y_root - cy);
//    fprintf(stderr, "cx,cy=%u,%u\n", cx, cy);

    gtk_window_move(GTK_WINDOW(w->popup_), cx, cy);
    gtk_widget_show(w->popup_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
canvas_function_popup_t::show_menu(GdkEvent *event)
{
    gtk_menu_popup(GTK_MENU(widgets_.menu_),
		   /*parent_menu_shell*/0,
		   /*parent_menu_item*/0,
		   (GtkMenuPositionFunc)0,
		   /*data*/(gpointer)0,
		   event->button.button,
		   event->button.time);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
