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

#include "callgraphwin.H"
#include "sourcewin.H"
#include "cov.H"
#include "estring.H"

CVSID("$Id: callgraphwin.C,v 1.4 2002-12-29 14:08:33 gnb Exp $");

#define COL_COUNT   0
#define COL_NAME    1
#define COL_MAX     2

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

void
callgraphwin_t::init_clist(
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

callgraphwin_t::callgraphwin_t()
{
    GladeXML *xml;
    
    /* load the interface & connect signals */
    xml = ui_load_tree("callgraph");
    
    set_window(glade_xml_get_widget(xml, "callgraph"));
    
    function_combo_ = glade_xml_get_widget(xml, "callgraph_function_combo");
    function_view_ = glade_xml_get_widget(xml, "callgraph_function_view");

    ancestors_clist_ = glade_xml_get_widget(xml, "callgraph_ancestors_clist");
    init_clist(GTK_CLIST(ancestors_clist_),
    	    	    	    callgraphwin_ancestors_compare);

    descendants_clist_ = glade_xml_get_widget(xml, "callgraph_descendants_clist");
    init_clist(GTK_CLIST(descendants_clist_),
    	    	    	    callgraphwin_descendants_compare);
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "callgraph_windows_dummy"));
}

callgraphwin_t::~callgraphwin_t()
{
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
    	ret = strcmp(cna->function->file()->name(), cnb->function->file()->name());
    return ret;
}

static void
add_callnode(cov_callnode_t *cn, void *userdata)
{
    GList **listp = (GList **)userdata;
    
    *listp = g_list_prepend(*listp, cn);
}


/*
 * TODO: invert this function and make the loop with the unambiguous
 * label building a library function which calls a callback.
 */
void
callgraphwin_t::populate_function_combo(GtkCombo *combo)
{
    GList *list = 0, *iter;
    estring label;
    
    cov_callnode_t::foreach(add_callnode, &list);
    list = g_list_sort(list, compare_callnodes);
    
    for (iter = list ; iter != 0 ; iter = iter->next)
    {
    	cov_callnode_t *cn = (cov_callnode_t *)iter->data;

    	label.truncate();
	label.append_string(cn->name);

    	/* see if we need to present some more scope to uniquify the name */
	if ((iter->next != 0 &&
	     !strcmp(((cov_callnode_t *)iter->next->data)->name, cn->name)) ||
	    (iter->prev != 0 &&
	     !strcmp(((cov_callnode_t *)iter->prev->data)->name, cn->name)))
	{
	    label.append_string(" (");
	    if (cn->function != 0)
		label.append_string(cn->function->file()->minimal_name());
	    else
		label.append_string("library");
	    label.append_string(")");
	}
	
    	ui_combo_add_data(combo, label.data(), cn);
    }
    
    while (list != 0)
	list = g_list_remove_link(list, list);
}

void
callgraphwin_t::populate()
{
#if DEBUG
    fprintf(stderr, "callgraphwin_t::populate\n");
#endif
    
    populate_function_combo(GTK_COMBO(function_combo_));

    ui_combo_set_current_data(GTK_COMBO(function_combo_), callnode_);
    update();
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

void
callgraphwin_t::update_clist(GtkWidget *clist, GList *arcs, gboolean isin)
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

void
callgraphwin_t::update()
{
    cov_callnode_t *cn = callnode_;
    
#if DEBUG
    fprintf(stderr, "callgraphwin_t::update\n");
#endif
    gtk_widget_set_sensitive(function_view_, (cn->function != 0));
    set_title(cn->name);

    update_clist(ancestors_clist_, cn->in_arcs, TRUE);
    update_clist(descendants_clist_, cn->out_arcs, FALSE);
}

void
callgraphwin_t::set_node(cov_callnode_t *cn)
{
    callnode_ = cn;
    if (shown_)
    	update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_callgraph_close_activate(GtkWidget *w, gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);
    
    assert(cw != 0);
    delete cw;
}

GLADE_CALLBACK void
on_callgraph_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_callgraph_function_entry_changed(GtkWidget *w, gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);

    cw->callnode_ = (cov_callnode_t *)ui_combo_get_current_data(
    	    	    	    	    	GTK_COMBO(cw->function_combo_));
    cw->update();
}

GLADE_CALLBACK void
on_callgraph_function_view_clicked(GtkWidget *w, gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);

    sourcewin_t::show_function(cw->callnode_->function);
}

GLADE_CALLBACK void
on_callgraph_ancestors_clist_button_press_event(
    GtkWidget *w,
    GdkEventButton *event,
    gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);
    int row, col;
    cov_callarc_t *ca;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     (int)event->x, (int)event->y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_callgraph_ancestors_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	ca = (cov_callarc_t *)gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	cw->set_node(ca->from);
    }
}

GLADE_CALLBACK void
on_callgraph_descendants_clist_button_press_event(
    GtkWidget *w,
    GdkEventButton *event,
    gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);
    int row, col;
    cov_callarc_t *ca;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     (int)event->x, (int)event->y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_callgraph_descendants_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	ca = (cov_callarc_t *)gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	cw->set_node(ca->to);
    }
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
