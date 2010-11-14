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

#include "callswin.H"
#include "sourcewin.H"
#include "utils.H"
#include "filename.h"
#include "cov.H"
#include "estring.H"

CVSID("$Id: callswin.C,v 1.24 2010-05-09 05:37:14 gnb Exp $");

#define COL_FROM    0
#define COL_TO	    1
#define COL_LINE    2
#define COL_COUNT   3
#if !GTK2
#define COL_CLOSURE	0
#define NUM_COLS    	4
#else
#define COL_CLOSURE	4
#define NUM_COLS    	5
#define COL_TYPES \
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, \
    G_TYPE_STRING, G_TYPE_POINTER
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct callswin_call_t
{
    cov_function_t *from_;
    const char *to_;
    const cov_location_t *location_;
    count_t count_;

    callswin_call_t(cov_function_t *fn, cov_call_iterator_t *itr)
     :  from_(fn),
     	to_(itr->name()),
	location_(itr->location()),
	count_(itr->count())
    {
    }

    static int compare(
    	const callswin_call_t *a,
	const callswin_call_t *b,
    	int column)
    {
    	int cols[NUM_COLS+1];
	int i, n = 0, r;
	
	/* primary sort key */
	cols[n++] = column;
	/* secondary sort keys */
	cols[n++] = COL_FROM;
	cols[n++] = COL_LINE;
	cols[n++] = COL_TO;
	
	for (i = 0 ; i < n ; i++)
	{
	    switch (cols[i])
	    {
	    case COL_FROM:
    		r =  strcmp(safestr(a->from_->name()),
	    		    safestr(b->from_->name()));
		break;

	    case COL_TO:
    		r = strcmp(safestr(a->to_), safestr(b->to_));
		break;

	    case COL_LINE:
    		r = u32cmp(a->location_->lineno, b->location_->lineno);
		break;

	    case COL_COUNT:
    		r = u64cmp(a->count_, b->count_);
		break;

	    default:
		r = 0;
		break;
	    }
	    if (r)
	    	return r;
	}
	return 0;
    }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if !GTK2
static int
callswin_clist_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    return callswin_call_t::compare(
    	(callswin_call_t *)((GtkCListRow *)ptr1)->data,
    	(callswin_call_t *)((GtkCListRow *)ptr2)->data,
    	clist->sort_column);
}
#else
static int
callswin_tree_iter_compare(
    GtkTreeModel *tm,
    GtkTreeIter *iter1,
    GtkTreeIter *iter2,
    gpointer data)
{
    callswin_call_t *a = 0;
    callswin_call_t *b = 0;

    gtk_tree_model_get(tm, iter1, COL_CLOSURE, &a, -1);
    gtk_tree_model_get(tm, iter2, COL_CLOSURE, &b, -1);
    return callswin_call_t::compare(a, b, GPOINTER_TO_INT(data));
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callswin_t::callswin_t()
{
    GladeXML *xml;
#if GTK2
    GtkTreeViewColumn *col;
    GtkCellRenderer *rend;
#endif
    
    /* load the interface & connect signals */
    xml = ui_load_tree("calls");
    
    set_window(glade_xml_get_widget(xml, "calls"));
    
    from_function_combo_ = glade_xml_get_widget(xml, "calls_from_function_combo");
    from_function_view_ = glade_xml_get_widget(xml, "calls_from_function_view");

    to_function_combo_ = glade_xml_get_widget(xml, "calls_to_function_combo");
    to_function_view_ = glade_xml_get_widget(xml, "calls_to_function_view");

    clist_ = glade_xml_get_widget(xml, "calls_clist");
#if !GTK2
    gtk_clist_column_titles_passive(GTK_CLIST(clist_));
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_FROM);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_TO);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_COUNT);
    gtk_clist_set_compare_func(GTK_CLIST(clist_), callswin_clist_compare);
    ui_clist_set_sort_type(GTK_CLIST(clist_), GTK_SORT_ASCENDING);
#else
    store_ = gtk_list_store_new(NUM_COLS, COL_TYPES);
    /* default alphabetic sort is adequate for COL_FROM, COL_TO */
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_LINE,
	  callswin_tree_iter_compare, GINT_TO_POINTER(COL_LINE), 0);
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_COUNT,
	  callswin_tree_iter_compare, GINT_TO_POINTER(COL_COUNT), 0);
    
    gtk_tree_view_set_model(GTK_TREE_VIEW(clist_), GTK_TREE_MODEL(store_));

    rend = gtk_cell_renderer_text_new();

    col = gtk_tree_view_column_new_with_attributes(_("From"), rend,
    	    	"text", COL_FROM,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_FROM);
    gtk_tree_view_append_column(GTK_TREE_VIEW(clist_), col);
    
    col = gtk_tree_view_column_new_with_attributes(_("To"), rend,
    	    	"text", COL_TO,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_TO);
    gtk_tree_view_append_column(GTK_TREE_VIEW(clist_), col);
    
    col = gtk_tree_view_column_new_with_attributes(_("Line"), rend,
    	    	"text", COL_LINE,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_LINE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(clist_), col);
    
    col = gtk_tree_view_column_new_with_attributes(_("Count"), rend,
    	    	"text", COL_COUNT,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_COUNT);
    gtk_tree_view_append_column(GTK_TREE_VIEW(clist_), col);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(clist_), TRUE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(clist_), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(clist_), COL_FROM);
#endif

    ui_register_windows_menu(ui_get_dummy_menu(xml, "calls_windows_dummy"));
}

callswin_t::~callswin_t()
{
#if GTK2
    g_object_unref(G_OBJECT(store_));
#endif
    functions_->remove_all();
    delete functions_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callswin_t::populate()
{
    dprintf0(D_CALLSWIN, "callswin_t::populate\n");
    functions_ = cov_function_t::list_all();
    ::populate_function_combo(GTK_COMBO(from_function_combo_), functions_,
    	    	    	      /*add_all_item*/TRUE, /*currentp*/0);
    ::populate_function_combo(GTK_COMBO(to_function_combo_), functions_,
    	    	    	      /*add_all_item*/TRUE, /*currentp*/0);
    update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callswin_t::update_for_func(cov_function_t *from_fn, cov_function_t *to_fn)
{
#if GTK2
    GtkTreeIter titer;
#else
    int row;
#endif
    const cov_location_t *loc;
    callswin_call_t *call;
    const char *text[NUM_COLS];
    char countbuf[32];
    char linebuf[32];

    cov_call_iterator_t *itr = new cov_function_call_iterator_t(from_fn);
    
    while (itr->next())
    {
	if (to_fn != 0 &&
	    (itr->name() == 0 || strcmp(to_fn->name(), itr->name())))
	    continue;

	snprintf(countbuf, sizeof(countbuf), GNB_U64_DFMT, itr->count());
	text[COL_COUNT] = countbuf;

    	if ((loc = itr->location()) == 0)
	    strncpy(linebuf, "WTF??", sizeof(linebuf));
	else
	    snprintf(linebuf, sizeof(linebuf), "%lu", loc->lineno);
	text[COL_LINE] = linebuf;

	text[COL_FROM] = (char *)from_fn->name();
	if ((text[COL_TO] = (char *)itr->name()) == 0)
	    text[COL_TO] = "(unknown)";
	    
	call = new callswin_call_t(from_fn, itr);

#if !GTK2
	row = gtk_clist_append(GTK_CLIST(clist_), (char **)text);
	gtk_clist_set_row_data(GTK_CLIST(clist_), row, call);
#else
    	gtk_list_store_append(store_, &titer);
	gtk_list_store_set(store_,  &titer,
	    COL_FROM, text[COL_FROM],
	    COL_TO, text[COL_TO],
	    COL_LINE, text[COL_LINE],
	    COL_COUNT, text[COL_COUNT],
	    COL_CLOSURE, call,
	    -1);
#endif
    }
    delete itr;
}

void
callswin_t::update()
{
    cov_function_t *from_fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	GTK_COMBO(from_function_combo_));
    cov_function_t *to_fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	GTK_COMBO(to_function_combo_));
    estring title;
    
    dprintf0(D_CALLSWIN, "callswin_t::update\n");
    switch ((from_fn == 0 ? 0 : 2)|(to_fn == 0 ? 0 : 1))
    {
    case 0:
	break;
    case 1:
    	title.append_printf(_("to %s"), to_fn->name());
	break;
    case 2:
    	title.append_printf(_("from %s"), from_fn->name());
	break;
    case 3:
    	title.append_printf(_("from %s to %s"),
	    	    	      from_fn->name(), to_fn->name());
	break;
    }
    set_title(title.data());
    
    gtk_widget_set_sensitive(from_function_view_, (from_fn != 0));
    gtk_widget_set_sensitive(to_function_view_, (to_fn != 0));    

#if !GTK2
    gtk_clist_freeze(GTK_CLIST(clist_));
    gtk_clist_clear(GTK_CLIST(clist_));
#else
    gtk_list_store_clear(store_);
#endif

    if (from_fn != 0)
    {
    	update_for_func(from_fn, to_fn);
    }
    else
    {
    	/* all functions */
	list_iterator_t<cov_function_t> iter;
	
	for (iter = functions_->first() ; iter != (cov_function_t *)0 ; ++iter)
	{
	    update_for_func(*iter, to_fn);
	}
    }
    
#if !GTK2
    gtk_clist_columns_autosize(GTK_CLIST(clist_));
    gtk_clist_thaw(GTK_CLIST(clist_));
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_calls_call_from_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    ui_list_set_column_visibility(cw->clist_, COL_FROM,
    	    	    	          GTK_CHECK_MENU_ITEM(w)->active);
}

GLADE_CALLBACK void
on_calls_call_to_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    ui_list_set_column_visibility(cw->clist_, COL_TO,
    	    	    	          GTK_CHECK_MENU_ITEM(w)->active);
}

GLADE_CALLBACK void
on_calls_line_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    ui_list_set_column_visibility(cw->clist_, COL_LINE,
    	    	    	          GTK_CHECK_MENU_ITEM(w)->active);
}

GLADE_CALLBACK void
on_calls_count_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    ui_list_set_column_visibility(cw->clist_, COL_COUNT,
    	    	    	          GTK_CHECK_MENU_ITEM(w)->active);
}

GLADE_CALLBACK void
on_calls_from_function_entry_changed(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);

    cw->update();
}

GLADE_CALLBACK void
on_calls_to_function_entry_changed(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);

    cw->update();
}

GLADE_CALLBACK void
on_calls_from_function_view_clicked(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    cov_function_t *fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->from_function_combo_));
    g_return_if_fail(fn != 0);
    sourcewin_t::show_function(fn);
}

GLADE_CALLBACK void
on_calls_to_function_view_clicked(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    cov_function_t *fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->to_function_combo_));
    g_return_if_fail(fn != 0);
    sourcewin_t::show_function(fn);
}

GLADE_CALLBACK gboolean
on_calls_clist_button_press_event(GtkWidget *w, GdkEvent *event, gpointer data)
{
    callswin_call_t *call;
    const cov_location_t *loc;

    call = (callswin_call_t *)ui_list_double_click_data(w, event, COL_CLOSURE);

    if (call != 0 && (loc = call->location_) != 0)
	sourcewin_t::show_location(loc);
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
