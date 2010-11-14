/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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

CVSID("$Id: callgraphwin.C,v 1.17 2010-05-09 05:37:14 gnb Exp $");

#define COL_COUNT   0
#define COL_NAME    1
#if !GTK2
#define COL_CLOSURE	0
#define NUM_COLS    	2
#else
#define COL_CLOSURE	2
#define NUM_COLS    	3
#define COL_TYPES \
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if !GTK2
static int
callgraphwin_ancestors_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    cov_callgraph_t::arc_t *ca1 = (cov_callgraph_t::arc_t *)((GtkCListRow *)ptr1)->data;
    cov_callgraph_t::arc_t *ca2 = (cov_callgraph_t::arc_t *)((GtkCListRow *)ptr2)->data;

    switch (clist->sort_column)
    {
    case COL_COUNT:
    	return u64cmp(ca1->count, ca2->count);
	
    case COL_NAME:
    	return strcmp(ca1->from->name, ca2->from->name);
	
    default:
	return 0;
    }
}

static int
callgraphwin_descendants_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    cov_callgraph_t::arc_t *ca1 = (cov_callgraph_t::arc_t *)((GtkCListRow *)ptr1)->data;
    cov_callgraph_t::arc_t *ca2 = (cov_callgraph_t::arc_t *)((GtkCListRow *)ptr2)->data;

    switch (clist->sort_column)
    {
    case COL_COUNT:
    	return u64cmp(ca1->count, ca2->count);
	
    case COL_NAME:
    	return strcmp(ca1->to->name, ca2->to->name);
	
    default:
	return 0;
    }
}

#else

static int
callgraphwin_count_compare(
    GtkTreeModel *tm,
    GtkTreeIter *iter1,
    GtkTreeIter *iter2,
    gpointer data)
{
    cov_callgraph_t::arc_t *ca1 = 0;
    cov_callgraph_t::arc_t *ca2 = 0;

    gtk_tree_model_get(tm, iter1, COL_CLOSURE, &ca1, -1);
    gtk_tree_model_get(tm, iter2, COL_CLOSURE, &ca2, -1);

    return u64cmp(ca1->count, ca2->count);
}

#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if !GTK2
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
#else
void
callgraphwin_t::init_tree_view(GtkTreeView *tv)
{
    GtkListStore *store;
    GtkTreeViewColumn *col;
    GtkCellRenderer *rend;

    store = gtk_list_store_new(NUM_COLS, COL_TYPES);

    /* default alphabetic sort is adequate for COL_NAME */
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store, COL_COUNT,
	  callgraphwin_count_compare, 0, 0);
    
    gtk_tree_view_set_model(tv, GTK_TREE_MODEL(store));

    rend = gtk_cell_renderer_text_new();

    col = gtk_tree_view_column_new_with_attributes(_("Count"), rend,
    	    	"text", COL_COUNT,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_COUNT);
    gtk_tree_view_append_column(tv, col);
    
    col = gtk_tree_view_column_new_with_attributes(_("Function"), rend,
    	    	"text", COL_NAME,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_NAME);
    gtk_tree_view_append_column(tv, col);
    
    /* the TreeView has another reference, so we can safely drop ours */
    g_object_unref(G_OBJECT(store));
}
#endif

callgraphwin_t::callgraphwin_t()
{
    GladeXML *xml;
    
    /* load the interface & connect signals */
    xml = ui_load_tree("callgraph");
    
    set_window(glade_xml_get_widget(xml, "callgraph"));
    
    function_combo_ = glade_xml_get_widget(xml, "callgraph_function_combo");
    function_view_ = glade_xml_get_widget(xml, "callgraph_function_view");

    ancestors_clist_ = glade_xml_get_widget(xml, "callgraph_ancestors_clist");
#if !GTK2
    init_clist(GTK_CLIST(ancestors_clist_),
    	    	    	    callgraphwin_ancestors_compare);
#else
    init_tree_view(GTK_TREE_VIEW(ancestors_clist_));
#endif

    descendants_clist_ = glade_xml_get_widget(xml, "callgraph_descendants_clist");
#if !GTK2
    init_clist(GTK_CLIST(descendants_clist_),
    	    	    	    callgraphwin_descendants_compare);
#else
    init_tree_view(GTK_TREE_VIEW(descendants_clist_));
#endif
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "callgraph_windows_dummy"));


    hpaned_ = glade_xml_get_widget(xml, "callgraph_hpaned");
    gtk_signal_connect(GTK_OBJECT(get_window()), "show",
    	GTK_SIGNAL_FUNC(on_callgraph_show), 0);
}

callgraphwin_t::~callgraphwin_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
compare_callnodes(const void *a, const void *b)
{
    const cov_callgraph_t::node_t *cna = (const cov_callgraph_t::node_t *)a;
    const cov_callgraph_t::node_t *cnb = (const cov_callgraph_t::node_t *)b;
    int ret;
    
    ret = strcmp(cna->name, cnb->name);
    if (ret == 0 && cna->function != 0 && cnb->function != 0)
    	ret = strcmp(cna->function->file()->name(), cnb->function->file()->name());
    return ret;
}

static void
add_callnode(cov_callgraph_t::node_t *cn, void *userdata)
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
    
    ui_combo_clear(combo);    /* stupid glade2 */

    cov_project_t::current()->callgraph().foreach_node(add_callnode, &list);
    list = g_list_sort(list, compare_callnodes);
    
    for (iter = list ; iter != 0 ; iter = iter->next)
    {
    	cov_callgraph_t::node_t *cn = (cov_callgraph_t::node_t *)iter->data;

    	label.truncate();
	label.append_string(cn->name);

    	/* see if we need to present some more scope to uniquify the name */
	if ((iter->next != 0 &&
	     !strcmp(((cov_callgraph_t::node_t *)iter->next->data)->name, cn->name)) ||
	    (iter->prev != 0 &&
	     !strcmp(((cov_callgraph_t::node_t *)iter->prev->data)->name, cn->name)))
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
    dprintf0(D_GRAPHWIN, "callgraphwin_t::populate\n");
    
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
    	total += ((cov_callgraph_t::arc_t *)list->data)->count;
    
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
    	snprintf(buf, maxlen, GNB_U64_DFMT"/"GNB_U64_DFMT, numerator, denominator);
}

void
callgraphwin_t::update_clist(GtkWidget *clist, GList *arcs, gboolean isin)
{
    GList *iter;
#if !GTK2    
    int row;
#else
    GtkListStore *store = (GtkListStore *)gtk_tree_view_get_model(
    	    	    	    	    	    	    	GTK_TREE_VIEW(clist));
    GtkTreeIter titer;
#endif
    count_t total;
    char *text[NUM_COLS];
    char countbuf[32];

    total = cov_callarcs_total(arcs);

#if !GTK2    
    gtk_clist_freeze(GTK_CLIST(clist));
    gtk_clist_clear(GTK_CLIST(clist));
#else
    gtk_list_store_clear(store);
#endif

    for (iter = arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callgraph_t::arc_t *ca = (cov_callgraph_t::arc_t *)iter->data;

    	format_stat(countbuf, sizeof(countbuf), /*percent*/FALSE,
	    	    /*numerator*/ca->count, /*denominator*/total);
	text[COL_COUNT] = countbuf;

	text[COL_NAME] = (char *)(isin ? ca->from : ca->to)->name.data();

#if !GTK2
	row = gtk_clist_prepend(GTK_CLIST(clist), text);
	gtk_clist_set_row_data(GTK_CLIST(clist), row, ca);
#else
    	gtk_list_store_append(store, &titer);
	gtk_list_store_set(store,  &titer,
	    COL_COUNT, text[COL_COUNT],
	    COL_NAME, text[COL_NAME],
	    COL_CLOSURE, ca,
	    -1);
#endif
    }

#if !GTK2    
    gtk_clist_sort(GTK_CLIST(clist));
    gtk_clist_columns_autosize(GTK_CLIST(clist));
    gtk_clist_thaw(GTK_CLIST(clist));
#endif
}

void
callgraphwin_t::update()
{
    cov_callgraph_t::node_t *cn = callnode_;
    
    dprintf0(D_GRAPHWIN, "callgraphwin_t::update\n");
    gtk_widget_set_sensitive(function_view_, (cn->function != 0));
    set_title(cn->name);

    update_clist(ancestors_clist_, cn->in_arcs, TRUE);
    update_clist(descendants_clist_, cn->out_arcs, FALSE);
}

void
callgraphwin_t::set_node(cov_callgraph_t::node_t *cn)
{
    callnode_ = cn;
    assert(callnode_ != 0);
    if (shown_)
    	update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_callgraph_function_entry_changed(GtkWidget *w, gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);
    cov_callgraph_t::node_t *cn;

    cn = (cov_callgraph_t::node_t *)ui_combo_get_current_data(
    	    	    	    	    	GTK_COMBO(cw->function_combo_));
    if (cn != 0)
    {
    	/* stupid gtk2 */
	cw->set_node(cn);
    }
}

GLADE_CALLBACK void
on_callgraph_function_view_clicked(GtkWidget *w, gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);

    sourcewin_t::show_function(cw->callnode_->function);
}

GLADE_CALLBACK gboolean
on_callgraph_ancestors_clist_button_press_event(
    GtkWidget *w,
    GdkEvent *event,
    gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);
    cov_callgraph_t::arc_t *ca;

    ca = (cov_callgraph_t::arc_t *)ui_list_double_click_data(w, event, COL_CLOSURE);

    if (ca != 0)
	cw->set_node(ca->from);
    return FALSE;
}

GLADE_CALLBACK gboolean
on_callgraph_descendants_clist_button_press_event(
    GtkWidget *w,
    GdkEvent *event,
    gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);
    cov_callgraph_t::arc_t *ca;

    ca = (cov_callgraph_t::arc_t *)ui_list_double_click_data(w, event, COL_CLOSURE);

    if (ca != 0)	
	cw->set_node(ca->to);
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraphwin_t::on_callgraph_show(GtkWidget *w, gpointer data)
{
    callgraphwin_t *cw = callgraphwin_t::from_widget(w);
    GtkPaned *paned = GTK_PANED(cw->hpaned_);

    gtk_paned_set_position(paned,
    	    	    	   (paned->max_position + paned->min_position) / 2);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
