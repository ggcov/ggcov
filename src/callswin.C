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

#include "callswin.H"
#include "sourcewin.H"
#include "utils.H"
#include "filename.h"
#include "cov.H"
#include "estring.H"
#include "confsection.H"
#include "logging.H"

#define COL_FROM    0
#define COL_TO      1
#define COL_LINE    2
#define COL_COUNT   3
#define COL_CLOSURE     4
#define NUM_COLS        5
#define COL_TYPES \
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, \
    G_TYPE_STRING, G_TYPE_POINTER

static logging::logger_t &_log = logging::find_logger("callswin");

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
		if (!r)
		    r =  strcmp(safestr(a->from_->file()->name()),
				safestr(b->from_->file()->name()));
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callswin_t::callswin_t()
{
    GladeXML *xml;
    GtkTreeViewColumn *col;
    GtkCellRenderer *rend;
    GtkWidget *w;

    /* load the interface & connect signals */
    xml = ui_load_tree("calls");

    set_window(glade_xml_get_widget(xml, "calls"));

    w = glade_xml_get_widget(xml, "calls_from_function_combo");
    from_function_combo_ = init(UI_COMBO(w));
    from_function_view_ = glade_xml_get_widget(xml, "calls_from_function_view");

    w = glade_xml_get_widget(xml, "calls_to_function_combo");
    to_function_combo_ = init(UI_COMBO(w));
    to_function_view_ = glade_xml_get_widget(xml, "calls_to_function_view");

    from_check_ = glade_xml_get_widget(xml, "calls_call_from_check");
    to_check_ = glade_xml_get_widget(xml, "calls_call_to_check");
    line_check_ = glade_xml_get_widget(xml, "calls_line_check");
    count_check_ = glade_xml_get_widget(xml, "calls_count_check");

    clist_ = glade_xml_get_widget(xml, "calls_clist");
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

    ui_register_windows_menu(ui_get_dummy_menu(xml, "calls_windows_dummy"));
}

callswin_t::~callswin_t()
{
    g_object_unref(G_OBJECT(store_));
    functions_->remove_all();
    delete functions_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callswin_t::populate()
{
    _log.debug("callswin_t::populate\n");
    functions_ = cov_list_all_functions();
    ::populate_function_combo(from_function_combo_, functions_,
			      /*add_all_item*/TRUE, /*currentp*/0);
    ::populate_function_combo(to_function_combo_, functions_,
			      /*add_all_item*/TRUE, /*currentp*/0);
    set_active(from_function_combo_, 0);
    set_active(to_function_combo_, 0);
    update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callswin_t::update_for_func(cov_function_t *from_fn, cov_function_t *to_fn)
{
    GtkTreeIter titer;
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

	snprintf(countbuf, sizeof(countbuf), "%llu", (unsigned long long)itr->count());
	text[COL_COUNT] = countbuf;

	if ((loc = itr->location()) == 0)
	    strncpy(linebuf, "WTF??", sizeof(linebuf));
	else
	    snprintf(linebuf, sizeof(linebuf), "%lu", loc->lineno);
	text[COL_LINE] = linebuf;

	text[COL_FROM] = (char *)from_fn->unambiguous_name();
	if ((text[COL_TO] = (char *)itr->name()) == 0)
	    text[COL_TO] = "(unknown)";

	call = new callswin_call_t(from_fn, itr);

	gtk_list_store_append(store_, &titer);
	gtk_list_store_set(store_,  &titer,
	    COL_FROM, text[COL_FROM],
	    COL_TO, text[COL_TO],
	    COL_LINE, text[COL_LINE],
	    COL_COUNT, text[COL_COUNT],
	    COL_CLOSURE, call,
	    -1);
    }
    delete itr;
}

void
callswin_t::update()
{
    cov_function_t *from_fn = (cov_function_t *)get_active(from_function_combo_);
    cov_function_t *to_fn = (cov_function_t *)get_active(to_function_combo_);
    estring title;

    _log.debug("callswin_t::update\n");
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

    gtk_list_store_clear(store_);

    if (from_fn != 0)
    {
	update_for_func(from_fn, to_fn);
    }
    else
    {
	/* all functions */
	for (list_iterator_t<cov_function_t> iter = functions_->first() ; *iter ; ++iter)
	    update_for_func(*iter, to_fn);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callswin_t::apply_toggles()
{
    ui_list_set_column_visibility(clist_, COL_FROM,
				  GTK_CHECK_MENU_ITEM(from_check_)->active);
    ui_list_set_column_visibility(clist_, COL_TO,
				  GTK_CHECK_MENU_ITEM(to_check_)->active);
    ui_list_set_column_visibility(clist_, COL_LINE,
				  GTK_CHECK_MENU_ITEM(line_check_)->active);
    ui_list_set_column_visibility(clist_, COL_COUNT,
				  GTK_CHECK_MENU_ITEM(count_check_)->active);
}

void
callswin_t::load_state()
{
    populating_ = TRUE; /* suppress check menu item callback */
    load(GTK_CHECK_MENU_ITEM(from_check_));
    load(GTK_CHECK_MENU_ITEM(to_check_));
    load(GTK_CHECK_MENU_ITEM(line_check_));
    load(GTK_CHECK_MENU_ITEM(count_check_));
    apply_toggles();
    populating_ = FALSE;
}

void
callswin_t::save_state()
{
    save(GTK_CHECK_MENU_ITEM(from_check_));
    save(GTK_CHECK_MENU_ITEM(to_check_));
    save(GTK_CHECK_MENU_ITEM(line_check_));
    save(GTK_CHECK_MENU_ITEM(count_check_));
    confsection_t::sync();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
callswin_t::on_call_from_check_activate()
{
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
callswin_t::on_call_to_check_activate()
{
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
callswin_t::on_line_check_activate()
{
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
callswin_t::on_count_check_activate()
{
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
callswin_t::on_from_function_combo_changed()
{
    update();
}

GLADE_CALLBACK void
callswin_t::on_to_function_combo_changed()
{
    update();
}

GLADE_CALLBACK void
callswin_t::on_from_function_view_clicked()
{
    cov_function_t *fn = (cov_function_t *)get_active(from_function_combo_);
    if (fn)
	sourcewin_t::show_function(fn);
}

GLADE_CALLBACK void
callswin_t::on_to_function_view_clicked()
{
    cov_function_t *fn = (cov_function_t *)get_active(to_function_combo_);
    if (fn)
	sourcewin_t::show_function(fn);
}

GLADE_CALLBACK gboolean
callswin_t::on_clist_button_press_event(GdkEvent *event)
{
    callswin_call_t *call = (callswin_call_t *)
	ui_list_double_click_data(clist_, event, COL_CLOSURE);
    const cov_location_t *loc;

    if (call && (loc = call->location_))
	sourcewin_t::show_lines(loc->filename, loc->lineno, loc->lineno);
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
