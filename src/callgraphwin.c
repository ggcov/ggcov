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

#include "callgraphwin.h"
#include "sourcewin.h"
#include "cov.h"
#include "estring.h"

CVSID("$Id: callgraphwin.c,v 1.1 2001-12-03 01:05:15 gnb Exp $");

#define COL_COUNT   0
#define COL_NAME    1
#define COL_MAX     2

static const char callgraphwin_window_key[] = "callgraphwin_key";

static void callgraphwin_populate(callgraphwin_t*);


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
callgraphwin_ancestors_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    cov_callarc_t *ca1 = (cov_callarc_t *)((GtkCListRow *)ptr1)->data;
    cov_callarc_t *ca2 = (cov_callarc_t *)((GtkCListRow *)ptr2)->data;

    switch (clist->sort_column)
    {
    case COL_COUNT:
    	return ullcmp(ca1->count, ca2->count);
	
    case COL_NAME:
    	return strcmp(ca1->from->name, ca2->from->name);
	
    default:
	return 0;
    }
}

static int
callgraphwin_descendants_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    cov_callarc_t *ca1 = (cov_callarc_t *)((GtkCListRow *)ptr1)->data;
    cov_callarc_t *ca2 = (cov_callarc_t *)((GtkCListRow *)ptr2)->data;

    switch (clist->sort_column)
    {
    case COL_COUNT:
    	return ullcmp(ca1->count, ca2->count);
	
    case COL_NAME:
    	return strcmp(ca1->to->name, ca2->to->name);
	
    default:
	return 0;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
callgraphwin_init_clist(
    GtkCList *clist,
    int (*sortfn)(GtkCList *, const void*, const void*))
{
    gtk_clist_column_titles_passive(clist);
    ui_clist_init_column_arrow(clist, COL_COUNT);
    ui_clist_init_column_arrow(clist, COL_NAME);
    gtk_clist_set_compare_func(clist, sortfn);
    ui_clist_set_sort_column(clist, COL_COUNT);
    ui_clist_set_sort_type(clist, GTK_SORT_DESCENDING);
}

callgraphwin_t *
callgraphwin_new(void)
{
    callgraphwin_t *cw;
    GladeXML *xml;
    
    cw = new(callgraphwin_t);

    /* load the interface & connect signals */
    xml = ui_load_tree("callgraph");
    
    cw->window = glade_xml_get_widget(xml, "callgraph");
    ui_register_window(cw->window);
    
    cw->function_combo = glade_xml_get_widget(xml, "callgraph_function_combo");
    cw->function_view = glade_xml_get_widget(xml, "callgraph_function_view");

    cw->ancestors_clist = glade_xml_get_widget(xml, "callgraph_ancestors_clist");
    callgraphwin_init_clist(GTK_CLIST(cw->ancestors_clist),
    	    	    	    callgraphwin_ancestors_compare);

    cw->descendants_clist = glade_xml_get_widget(xml, "callgraph_descendants_clist");
    callgraphwin_init_clist(GTK_CLIST(cw->descendants_clist),
    	    	    	    callgraphwin_descendants_compare);
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "callgraph_windows_dummy"));

    gtk_object_set_data(GTK_OBJECT(cw->window), callgraphwin_window_key, cw);
    
    callgraphwin_populate(cw);
    
    return cw;
}

void
callgraphwin_delete(callgraphwin_t *cw)
{
    /* JIC of strange gui stuff */
    if (cw->deleting)
    	return;
    cw->deleting = TRUE;
    
    gtk_widget_destroy(cw->window);
    g_free(cw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static callgraphwin_t *
callgraphwin_from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : gtk_object_get_data(GTK_OBJECT(w), callgraphwin_window_key));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
compare_callnodes(const void *a, const void *b)
{
    const cov_callnode_t *cna = (const cov_callnode_t *)a;
    const cov_callnode_t *cnb = (const cov_callnode_t *)b;
    int ret;
    
    ret = strcmp(cna->name, cnb->name);
    if (ret == 0 && cna->function != 0 && cnb->function != 0)
    	ret = strcmp(cna->function->file->name, cnb->function->file->name);
    return ret;
}

static void
add_callnode(cov_callnode_t *cn, void *userdata)
{
    GList **listp = (GList **)userdata;
    
    *listp = g_list_prepend(*listp, cn);
}

static void
callgraph_populate_function_combo(GtkCombo *combo)
{
    GList *list = 0, *iter;
    estring label;
    
    cov_callnode_foreach(add_callnode, &list);
    list = g_list_sort(list, compare_callnodes);
    
    estring_init(&label);
    for (iter = list ; iter != 0 ; iter = iter->next)
    {
    	cov_callnode_t *cn = (cov_callnode_t *)iter->data;

    	estring_truncate(&label);
	estring_append_string(&label, cn->name);

    	/* see if we need to present some more scope to uniquify the name */
	if ((iter->next != 0 &&
	     !strcmp(((cov_callnode_t *)iter->next->data)->name, cn->name)) ||
	    (iter->prev != 0 &&
	     !strcmp(((cov_callnode_t *)iter->prev->data)->name, cn->name)))
	{
	    estring_append_string(&label, " (");
	    if (cn->function != 0)
		estring_append_string(&label, cn->function->file->name);
	    else
		estring_append_string(&label, "library");
	    estring_append_string(&label, ")");
	}
	
    	ui_combo_add_data(combo, label.data, cn);
    }
    estring_free(&label);
    
    while (list != 0)
	list = g_list_remove_link(list, list);
}

static void
callgraphwin_populate(callgraphwin_t *cw)
{
#if DEBUG
    fprintf(stderr, "callgraphwin_populate\n");
#endif
    
    callgraph_populate_function_combo(GTK_COMBO(cw->function_combo));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static count_t
cov_callarcs_total(GList *list)
{
    count_t total = 0;
    
    for ( ; list != 0 ; list = list->next)
    	total += ((cov_callarc_t *)list->data)->count;
    
    return total;
}

/* TODO: this is common code too */
static void
format_stat(
    char *buf,
    unsigned int maxlen,
    gboolean percent_flag,
    count_t numerator,
    count_t denominator)
{
    if (denominator == 0)
    	*buf = '\0';
    else if (percent_flag)
    	snprintf(buf, maxlen, "%.2f", (double)numerator * 100.0 / denominator);
    else
    	snprintf(buf, maxlen, "%llu/%llu", numerator, denominator);
}

static void
callgraphwin_update_clist(GtkWidget *clist, GList *arcs, gboolean isin)
{
    GList *iter;
    int row;
    count_t total;
    char *text[COL_MAX];
    char countbuf[32];

    total = cov_callarcs_total(arcs);
    
    gtk_clist_freeze(GTK_CLIST(clist));
    gtk_clist_clear(GTK_CLIST(clist));
    
    for (iter = arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callarc_t *ca = (cov_callarc_t *)iter->data;

    	format_stat(countbuf, sizeof(countbuf), /*percent*/FALSE,
	    	    /*numerator*/ca->count, /*denominator*/total);
	text[COL_COUNT] = countbuf;

	text[COL_NAME] = (isin ? ca->from : ca->to)->name;

	row = gtk_clist_prepend(GTK_CLIST(clist), text);
	gtk_clist_set_row_data(GTK_CLIST(clist), row, ca);
    }
    
    gtk_clist_sort(GTK_CLIST(clist));
    gtk_clist_columns_autosize(GTK_CLIST(clist));
    gtk_clist_thaw(GTK_CLIST(clist));
}

static void
callgraphwin_update(callgraphwin_t *cw)
{
    cov_callnode_t *cn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->function_combo));
    
#if DEBUG
    fprintf(stderr, "callgraphwin_update\n");
#endif
    gtk_widget_set_sensitive(cw->function_view, (cn->function != 0));
    ui_window_set_title(cw->window, cn->name);

    callgraphwin_update_clist(cw->ancestors_clist, cn->in_arcs, TRUE);
    callgraphwin_update_clist(cw->descendants_clist, cn->out_arcs, FALSE);
}

void
callgraphwin_set_node(callgraphwin_t *cw, cov_callnode_t *cn)
{
    ui_combo_set_current_data(GTK_COMBO(cw->function_combo), cn);
    callgraphwin_update(cw);
    gtk_widget_show(cw->window);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: this needs to be common code too */
static void
callgraphwin_show_source(
    const char *filename,
    unsigned long startline,
    unsigned long endline)
{
    sourcewin_t *srcw = sourcewin_new();
    
    sourcewin_set_filename(srcw, filename);
    if (startline > 0)
    {
	sourcewin_ensure_visible(srcw, startline);
	sourcewin_select_region(srcw, startline, endline);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_callgraph_close_activate(GtkWidget *w, gpointer data)
{
    callgraphwin_t *cw = callgraphwin_from_widget(w);
    
    if (cw != 0)
	callgraphwin_delete(cw);
}

GLADE_CALLBACK void
on_callgraph_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_callgraph_function_entry_changed(GtkWidget *w, gpointer data)
{
    callgraphwin_t *cw = callgraphwin_from_widget(w);

    callgraphwin_update(cw);
}

GLADE_CALLBACK void
on_callgraph_function_view_clicked(GtkWidget *w, gpointer data)
{
    callgraphwin_t *cw = callgraphwin_from_widget(w);
    cov_callnode_t *cn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->function_combo));
    const cov_location_t *start, *end;

    start = cov_function_get_first_location(cn->function);
    end = cov_function_get_last_location(cn->function);

    callgraphwin_show_source(start->filename, start->lineno, end->lineno);
}

GLADE_CALLBACK void
on_callgraph_ancestors_clist_button_press_event(
    GtkWidget *w,
    GdkEventButton *event,
    gpointer data)
{
    callgraphwin_t *cw = callgraphwin_from_widget(w);
    int row, col;
    cov_callarc_t *ca;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     event->x, event->y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_callgraph_ancestors_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	ca = gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	callgraphwin_set_node(cw, ca->from);
    }
}

GLADE_CALLBACK void
on_callgraph_descendants_clist_button_press_event(
    GtkWidget *w,
    GdkEventButton *event,
    gpointer data)
{
    callgraphwin_t *cw = callgraphwin_from_widget(w);
    int row, col;
    cov_callarc_t *ca;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     event->x, event->y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_callgraph_descendants_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	ca = gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	callgraphwin_set_node(cw, ca->to);
    }
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
