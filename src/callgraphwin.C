/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
#include "cov_priv.H"
#include "estring.H"
#include "logging.H"

#define COL_COUNT   0
#define COL_NAME    1
#define COL_CLOSURE     2
#define NUM_COLS        3
#define COL_TYPES \
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER

static logging::logger_t &_log = logging::find_logger("graphwin");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
callgraphwin_count_compare(
    GtkTreeModel *tm,
    GtkTreeIter *iter1,
    GtkTreeIter *iter2,
    gpointer data)
{
    cov_callarc_t *ca1 = 0;
    cov_callarc_t *ca2 = 0;

    gtk_tree_model_get(tm, iter1, COL_CLOSURE, &ca1, -1);
    gtk_tree_model_get(tm, iter2, COL_CLOSURE, &ca2, -1);

    return u64cmp(ca1->count, ca2->count);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

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

callgraphwin_t::callgraphwin_t()
{
    GladeXML *xml;
    GtkWidget *w;

    /* load the interface & connect signals */
    xml = ui_load_tree("callgraph");

    set_window(glade_xml_get_widget(xml, "callgraph"));

    w = glade_xml_get_widget(xml, "callgraph_function_combo");
    function_combo_ = init(UI_COMBO(w));
    function_view_ = glade_xml_get_widget(xml, "callgraph_function_view");

    ancestors_clist_ = glade_xml_get_widget(xml, "callgraph_ancestors_clist");
    init_tree_view(GTK_TREE_VIEW(ancestors_clist_));

    descendants_clist_ = glade_xml_get_widget(xml, "callgraph_descendants_clist");
    init_tree_view(GTK_TREE_VIEW(descendants_clist_));

    ui_register_windows_menu(ui_get_dummy_menu(xml, "callgraph_windows_dummy"));


    hpaned_ = glade_xml_get_widget(xml, "callgraph_hpaned");
    g_signal_connect(G_OBJECT(get_window()), "show",
                     G_CALLBACK(on_callgraph_show), 0);

    callnode_ = cov_callgraph.default_node();
}

callgraphwin_t::~callgraphwin_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraphwin_t::populate_function_combo(GtkComboBox *combo)
{
    list_t<cov_callnode_t> list;

    clear(combo);

    for (cov_callspace_iter_t csitr = cov_callgraph.first() ; *csitr ; ++csitr)
    {
	for (cov_callnode_iter_t cnitr = (*csitr)->first() ; *cnitr ; ++cnitr)
	    list.prepend(*cnitr);
    }
    list.sort(cov_callnode_t::compare_by_name_and_file);

    for (list_iterator_t<cov_callnode_t> itr = list.first() ; *itr ; ++itr)
    {
	cov_callnode_t *cn = *itr;
	add(combo, cn->unambiguous_name(), cn);
    }
    done(combo);

    list.remove_all();
}

void
callgraphwin_t::populate()
{
    _log.debug("callgraphwin_t::populate\n");

    populate_function_combo(function_combo_);

    set_active(function_combo_, callnode_);
    update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static count_t
cov_callarcs_total(list_t<cov_callarc_t> &arcs)
{
    count_t total = 0;

    for (list_iterator_t<cov_callarc_t> itr = arcs.first() ; *itr ; ++itr)
	total += (*itr)->count;

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
	snprintf(buf, maxlen, "%llu/%llu",
		 (unsigned long long)numerator,
		 (unsigned long long)denominator);
}

void
callgraphwin_t::update_clist(GtkWidget *clist, list_t<cov_callarc_t> &arcs, gboolean isin)
{
    GtkListStore *store = (GtkListStore *)gtk_tree_view_get_model(
							GTK_TREE_VIEW(clist));
    GtkTreeIter titer;
    count_t total;
    char *text[NUM_COLS];
    char countbuf[32];

    total = cov_callarcs_total(arcs);

    gtk_list_store_clear(store);

    for (list_iterator_t<cov_callarc_t> itr = arcs.first() ; *itr ; ++itr)
    {
	cov_callarc_t *ca = *itr;

	format_stat(countbuf, sizeof(countbuf), /*percent*/FALSE,
		    /*numerator*/ca->count, /*denominator*/total);
	text[COL_COUNT] = countbuf;

	text[COL_NAME] = (char *)(isin ? ca->from : ca->to)->unambiguous_name();

	gtk_list_store_append(store, &titer);
	gtk_list_store_set(store,  &titer,
	    COL_COUNT, text[COL_COUNT],
	    COL_NAME, text[COL_NAME],
	    COL_CLOSURE, ca,
	    -1);
    }
}

void
callgraphwin_t::update()
{
    cov_callnode_t *cn = callnode_;

    _log.debug("callgraphwin_t::update\n");
    gtk_widget_set_sensitive(function_view_, (cn->function != 0));

    set_title(cn->unambiguous_name());

    update_clist(ancestors_clist_, cn->in_arcs, TRUE);
    update_clist(descendants_clist_, cn->out_arcs, FALSE);
}

void
callgraphwin_t::set_node(cov_callnode_t *cn)
{
    callnode_ = cn;
    assert(callnode_ != 0);
    if (shown_)
	update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
callgraphwin_t::on_function_combo_changed()
{
    cov_callnode_t *cn  = (cov_callnode_t *)get_active(function_combo_);
    if (cn)
	set_node(cn);
}

GLADE_CALLBACK void
callgraphwin_t::on_function_view_clicked()
{
    sourcewin_t::show_function(callnode_->function);
}

GLADE_CALLBACK gboolean
callgraphwin_t::on_ancestors_clist_button_press_event(GdkEvent *event)
{
    cov_callarc_t *ca = (cov_callarc_t *)
	ui_list_double_click_data(ancestors_clist_, event, COL_CLOSURE);
    if (ca)
	set_node(ca->from);
    return FALSE;
}

GLADE_CALLBACK gboolean
callgraphwin_t::on_descendants_clist_button_press_event(GdkEvent *event)
{
    cov_callarc_t *ca = (cov_callarc_t *)
	    ui_list_double_click_data(descendants_clist_, event, COL_CLOSURE);
    if (ca)
	set_node(ca->to);
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
