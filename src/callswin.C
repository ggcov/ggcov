/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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
#include "filename.h"
#include "cov.H"
#include "estring.H"

CVSID("$Id: callswin.C,v 1.17 2005-03-14 07:42:22 gnb Exp $");

#define COL_FROM    0
#define COL_TO	    1
#define COL_LINE    2
#define COL_ARC     3
#define COL_COUNT   4
#if !GTK2
#define NUM_COLS    	5
#else
#define COL_CLOSURE	5
#define NUM_COLS    	6
#define COL_TYPES \
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, \
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER
#endif

static const char all_functions[] = N_("All Functions");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
callswin_compare(cov_arc_t *a1, cov_arc_t *a2, int column)
{
    switch (column)
    {
    case COL_COUNT:
    	return u64cmp(a1->from()->count(), a2->from()->count());
	
    case COL_ARC:
#define arcno(a)    (((a)->from()->bindex() << 16)|(a)->aindex())
    	return u32cmp(arcno(a1), arcno(a2));
#undef arcno
	
    case COL_TO:
    	return strcmp(safestr(a1->name()), safestr(a2->name()));
	
    case COL_FROM:
    	return strcmp(safestr(a1->from()->function()->name()),
	    	      safestr(a2->from()->function()->name()));
	
    default:
	return 0;
    }
}

#if !GTK2
static int
callswin_clist_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    return callswin_compare(
    	(cov_arc_t *)((GtkCListRow *)ptr1)->data,
    	(cov_arc_t *)((GtkCListRow *)ptr2)->data,
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
    cov_arc_t *a1 = 0;
    cov_arc_t *a2 = 0;

    gtk_tree_model_get(tm, iter1, COL_CLOSURE, &a1, -1);
    gtk_tree_model_get(tm, iter2, COL_CLOSURE, &a2, -1);
    return callswin_compare(a1, a2, GPOINTER_TO_INT(data));
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
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_ARC);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_COUNT);
    gtk_clist_set_compare_func(GTK_CLIST(clist_), callswin_clist_compare);
    ui_clist_set_sort_column(GTK_CLIST(clist_), COL_ARC);
    ui_clist_set_sort_type(GTK_CLIST(clist_), GTK_SORT_ASCENDING);
#else
    store_ = gtk_list_store_new(NUM_COLS, COL_TYPES);
    /* default alphabetic sort is adequate for COL_FROM, COL_TO */
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_LINE,
	  callswin_tree_iter_compare, GINT_TO_POINTER(COL_LINE), 0);
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_ARC,
	  callswin_tree_iter_compare, GINT_TO_POINTER(COL_ARC), 0);
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
    
    col = gtk_tree_view_column_new_with_attributes(_("Arc"), rend,
    	    	"text", COL_ARC,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_ARC);
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

    ui_list_set_column_visibility(clist_, COL_ARC, FALSE);

    ui_register_windows_menu(ui_get_dummy_menu(xml, "calls_windows_dummy"));
}

callswin_t::~callswin_t()
{
#if GTK2
    g_object_unref(G_OBJECT(store_));
#endif
    listclear(functions_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callswin_t::populate_function_combo(GtkCombo *combo)
{
    GList *iter;
    estring label;
    
    ui_combo_clear(combo);    /* stupid glade2 */
    ui_combo_add_data(combo, _(all_functions), 0);

    for (iter = functions_ ; iter != 0 ; iter = iter->next)
    {
    	cov_function_t *fn = (cov_function_t *)iter->data;

    	label.truncate();
	label.append_string(fn->name());

    	/* see if we need to present some more scope to uniquify the name */
	if ((iter->next != 0 &&
	     !strcmp(((cov_function_t *)iter->next->data)->name(), fn->name())) ||
	    (iter->prev != 0 &&
	     !strcmp(((cov_function_t *)iter->prev->data)->name(), fn->name())))
	{
	    label.append_string(" (");
	    label.append_string(fn->file()->minimal_name());
	    label.append_string(")");
	}
	
    	ui_combo_add_data(combo, label.data(), fn);
    }
}

void
callswin_t::populate()
{
    dprintf0(D_CALLSWIN, "callswin_t::populate\n");
    functions_ = cov_function_t::list_all();
    populate_function_combo(GTK_COMBO(from_function_combo_));
    populate_function_combo(GTK_COMBO(to_function_combo_));
    update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callswin_t::update_for_func(cov_function_t *from_fn, cov_function_t *to_fn)
{
    unsigned int bidx;
#if GTK2
    GtkTreeIter titer;
#else
    int row;
#endif
    list_iterator_t<cov_arc_t> aiter;
    const cov_location_t *loc;
    char *text[NUM_COLS];
    char countbuf[32];
    char arcbuf[64];
    char linebuf[32];

    for (bidx = 0 ; bidx < from_fn->num_blocks()-2 ; bidx++)
    {
    	cov_block_t *b = from_fn->nth_block(bidx);
	
	if (b->get_first_location() == 0)
	    continue;	/* no source => can't be interesting to the user */

	for (aiter = b->out_arc_iterator() ; aiter != (cov_arc_t *)0 ; ++aiter)
	{
	    cov_arc_t *a = *aiter;
    	    
	    if (!a->is_call())
	    	continue;
	    if (a->is_suppressed())
	    	continue;
	    if (to_fn != 0 &&
	    	(a->name() == 0 || strcmp(to_fn->name(), a->name())))
	    	continue;
		
	    snprintf(countbuf, sizeof(countbuf), GNB_U64_DFMT, a->from()->count());
	    text[COL_COUNT] = countbuf;

	    snprintf(arcbuf, sizeof(arcbuf), "B%u A%u", b->bindex(), a->aindex());
	    text[COL_ARC] = arcbuf;

    	    if ((loc = a->get_from_location()) == 0)
	    	strncpy(linebuf, "WTF??", sizeof(linebuf));
	    else
		snprintf(linebuf, sizeof(linebuf), "%lu", loc->lineno);
	    text[COL_LINE] = linebuf;
	    
	    text[COL_FROM] = (char *)a->from()->function()->name();
	    if ((text[COL_TO] = (char *)a->name()) == 0)
	    	text[COL_TO] = "(unknown)";
		
#if !GTK2
	    row = gtk_clist_append(GTK_CLIST(clist_), text);
	    gtk_clist_set_row_data(GTK_CLIST(clist_), row, a);
#else
    	    gtk_list_store_append(store_, &titer);
	    gtk_list_store_set(store_,  &titer,
		COL_FROM, text[COL_FROM],
		COL_TO, text[COL_TO],
		COL_LINE, text[COL_LINE],
		COL_ARC, text[COL_ARC],
		COL_COUNT, text[COL_COUNT],
		COL_CLOSURE, (*aiter),
		-1);
#endif
	}
    }
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
	GList *iter;
	
	for (iter = functions_ ; iter != 0 ; iter = iter->next)
	{
	    from_fn = (cov_function_t *)iter->data;
	    
	    update_for_func(from_fn, to_fn);
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
on_calls_arc_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    ui_list_set_column_visibility(cw->clist_, COL_ARC,
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
    cov_arc_t *a;
    const cov_location_t *loc;

    a = (cov_arc_t *)ui_list_double_click_data(w, event, COL_CLOSURE);

    if (a != 0 && (loc = a->get_from_location()) != 0)
	sourcewin_t::show_lines(loc->filename, loc->lineno, loc->lineno);
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
