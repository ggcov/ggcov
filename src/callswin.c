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

#include "callswin.h"
#include "sourcewin.h"
#include "cov.h"
#include "estring.h"

CVSID("$Id: callswin.c,v 1.2 2002-09-28 00:13:01 gnb Exp $");

#define COL_FROM    0
#define COL_TO	    1
#define COL_LINE    2
#define COL_ARC     3
#define COL_COUNT   4
#define COL_NUM     5

static const char callswin_window_key[] = "callswin_key";
static const char all_functions[] = N_("All Functions");

static void callswin_populate(callswin_t*);
static void callswin_update(callswin_t*);


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
callswin_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    cov_arc_t *a1 = (cov_arc_t *)((GtkCListRow *)ptr1)->data;
    cov_arc_t *a2 = (cov_arc_t *)((GtkCListRow *)ptr2)->data;

    switch (clist->sort_column)
    {
    case COL_COUNT:
    	return ullcmp(a1->from->count, a2->from->count);
	
    case COL_ARC:
#define arcno(a)    (((a)->from->idx << 16)|(a)->idx)
    	return ulcmp(arcno(a1), arcno(a2));
#undef arcno
	
    case COL_TO:
    	return strcmp(safestr(a1->name), safestr(a2->name));
	
    case COL_FROM:
    	return strcmp(safestr(a1->from->function->name),
	    	      safestr(a2->from->function->name));
	
    default:
	return 0;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callswin_t *
callswin_new(void)
{
    callswin_t *cw;
    GladeXML *xml;
    
    cw = new(callswin_t);

    /* load the interface & connect signals */
    xml = ui_load_tree("calls");
    
    cw->window = glade_xml_get_widget(xml, "calls");
    ui_register_window(cw->window);
    
    cw->from_function_combo = glade_xml_get_widget(xml, "calls_from_function_combo");
    cw->from_function_view = glade_xml_get_widget(xml, "calls_from_function_view");

    cw->to_function_combo = glade_xml_get_widget(xml, "calls_to_function_combo");
    cw->to_function_view = glade_xml_get_widget(xml, "calls_to_function_view");

    cw->clist = glade_xml_get_widget(xml, "calls_clist");
    gtk_clist_column_titles_passive(GTK_CLIST(cw->clist));
    ui_clist_init_column_arrow(GTK_CLIST(cw->clist), COL_FROM);
    ui_clist_init_column_arrow(GTK_CLIST(cw->clist), COL_TO);
    ui_clist_init_column_arrow(GTK_CLIST(cw->clist), COL_ARC);
    ui_clist_init_column_arrow(GTK_CLIST(cw->clist), COL_COUNT);
    gtk_clist_set_compare_func(GTK_CLIST(cw->clist), callswin_compare);
    ui_clist_set_sort_column(GTK_CLIST(cw->clist), COL_ARC);
    ui_clist_set_sort_type(GTK_CLIST(cw->clist), GTK_SORT_ASCENDING);
    
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "calls_windows_dummy"));

    gtk_object_set_data(GTK_OBJECT(cw->window), callswin_window_key, cw);
    
    callswin_populate(cw);
    callswin_update(cw);
    gtk_widget_show(cw->window);
    
    return cw;
}

void
callswin_delete(callswin_t *cw)
{
    /* JIC of strange gui stuff */
    if (cw->deleting)
    	return;
    cw->deleting = TRUE;
    
    gtk_widget_destroy(cw->window);
    g_free(cw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static callswin_t *
callswin_from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : gtk_object_get_data(GTK_OBJECT(w), callswin_window_key));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
calls_populate_function_combo(callswin_t *cw, GtkCombo *combo)
{
    GList *iter;
    estring label;
    
    ui_combo_add_data(combo, _(all_functions), 0);

    estring_init(&label);
    for (iter = cw->functions ; iter != 0 ; iter = iter->next)
    {
    	cov_function_t *fn = (cov_function_t *)iter->data;

    	estring_truncate(&label);
	estring_append_string(&label, fn->name);

    	/* see if we need to present some more scope to uniquify the name */
	if ((iter->next != 0 &&
	     !strcmp(((cov_function_t *)iter->next->data)->name, fn->name)) ||
	    (iter->prev != 0 &&
	     !strcmp(((cov_function_t *)iter->prev->data)->name, fn->name)))
	{
	    estring_append_string(&label, " (");
	    estring_append_string(&label, fn->file->name);
	    estring_append_string(&label, ")");
	}
	
    	ui_combo_add_data(combo, label.data, fn);
    }
    estring_free(&label);
}

static void
callswin_populate(callswin_t *cw)
{
#if DEBUG
    fprintf(stderr, "callswin_populate\n");
#endif
    cw->functions = cov_list_functions();
    calls_populate_function_combo(cw, GTK_COMBO(cw->from_function_combo));
    calls_populate_function_combo(cw, GTK_COMBO(cw->to_function_combo));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
callswin_update_for_func(
    callswin_t *cw,
    cov_function_t *from_fn,
    cov_function_t *to_fn)
{
    unsigned int bidx;
    int row;
    GList *aiter;
    char *text[COL_NUM];
    char countbuf[32];
    char arcbuf[64];
    char linebuf[32];

    for (bidx = 0 ; bidx < from_fn->blocks->len-2 ; bidx++)
    {
    	cov_block_t *b = cov_function_nth_block(from_fn, bidx);

	for (aiter = b->out_arcs ; aiter != 0 ; aiter = aiter->next)
	{
	    cov_arc_t *a = (cov_arc_t *)aiter->data;
    	    
	    if (!a->fake)
	    	continue;
	    if (to_fn != 0 &&
	    	(a->name == 0 || strcmp(to_fn->name, a->name)))
	    	continue;
		
	    snprintf(countbuf, sizeof(countbuf), "%llu", a->from->count);
	    text[COL_COUNT] = countbuf;

	    snprintf(arcbuf, sizeof(arcbuf), "B%u A%u", b->idx, a->idx);
	    text[COL_ARC] = arcbuf;
	    
	    snprintf(linebuf, sizeof(linebuf), "%u",
	    	    cov_arc_get_from_location(a)->lineno);
	    text[COL_LINE] = linebuf;
	    
	    text[COL_FROM] = a->from->function->name;
	    if ((text[COL_TO] = a->name) == 0)
	    	text[COL_TO] = "(unknown)";
		
	    row = gtk_clist_append(GTK_CLIST(cw->clist), text);
	    gtk_clist_set_row_data(GTK_CLIST(cw->clist), row, a);
	}
    }
}

static void
callswin_update(callswin_t *cw)
{
    cov_function_t *from_fn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->from_function_combo));
    cov_function_t *to_fn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->to_function_combo));
    estring title;
    
#if DEBUG
    fprintf(stderr, "callswin_update\n");
#endif
    estring_init(&title);
    switch ((from_fn == 0 ? 0 : 2)|(to_fn == 0 ? 0 : 1))
    {
    case 0:
	break;
    case 1:
    	estring_append_printf(&title, _("to %s"), to_fn->name);
	break;
    case 2:
    	estring_append_printf(&title, _("from %s"), from_fn->name);
	break;
    case 3:
    	estring_append_printf(&title, _("from %s to %s"),
	    	    	      from_fn->name, to_fn->name);
	break;
    }
    ui_window_set_title(cw->window, title.data);
    estring_free(&title);


    gtk_clist_freeze(GTK_CLIST(cw->clist));
    gtk_clist_clear(GTK_CLIST(cw->clist));

    if (from_fn != 0)
    {
    	callswin_update_for_func(cw, from_fn, to_fn);
    }
    else
    {
    	/* all functions */
	GList *iter;
	
	for (iter = cw->functions ; iter != 0 ; iter = iter->next)
	{
	    from_fn = (cov_function_t *)iter->data;
	    
	    callswin_update_for_func(cw, from_fn, to_fn);
	}
    }
    
    gtk_clist_columns_autosize(GTK_CLIST(cw->clist));
    gtk_clist_thaw(GTK_CLIST(cw->clist));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: this needs to be common code too */
static void
callswin_show_range(
    const cov_location_t *start,
    const cov_location_t *end)
{
    sourcewin_t *srcw = sourcewin_new();
    
    if (end == 0)
    	end = start;

    sourcewin_set_filename(srcw, start->filename);
    if (start->lineno > 0)
    {
	sourcewin_ensure_visible(srcw, start->lineno);
	sourcewin_select_region(srcw, start->lineno, end->lineno);
    }
}

static void
callswin_show_function(cov_function_t *fn)
{
    if (fn != 0)
	callswin_show_range(
    	    cov_function_get_first_location(fn),
    	    cov_function_get_last_location(fn));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_calls_close_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_from_widget(w);
    
    if (cw != 0)
	callswin_delete(cw);
}

GLADE_CALLBACK void
on_calls_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_calls_from_function_entry_changed(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_from_widget(w);

    callswin_update(cw);    
}

GLADE_CALLBACK void
on_calls_to_function_entry_changed(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_from_widget(w);

    callswin_update(cw);    
}

GLADE_CALLBACK void
on_calls_from_function_view_clicked(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_from_widget(w);
    cov_function_t *fn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->from_function_combo));
    callswin_show_function(fn);
}

GLADE_CALLBACK void
on_calls_to_function_view_clicked(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_from_widget(w);
    cov_function_t *fn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->to_function_combo));
    callswin_show_function(fn);
}

GLADE_CALLBACK void
on_calls_clist_button_press_event(
    GtkWidget *w,
    GdkEvent *event,
    gpointer data)
{
    callswin_t *cw = callswin_from_widget(w);
    int row, col;
    cov_arc_t *a;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     event->button.x, event->button.y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_calls_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	a = gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	callswin_show_range(cov_arc_get_from_location(a), 0);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
