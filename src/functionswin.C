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

#include "functionswin.H"
#include <math.h>
#include "sourcewin.H"
#include "cov.H"
#include "prefs.H"
#include "confsection.H"
#include "logging.H"


#define COL_BLOCKS      0
#define COL_LINES       1
#define COL_CALLS       2
#define COL_BRANCHES    3
#define COL_FUNCTION    4
#define COL_CLOSURE     5
#define COL_FG_GDK      6
#define NUM_COLS        7
#define COL_TYPES \
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, \
    G_TYPE_POINTER, GDK_TYPE_COLOR

static logging::logger_t &_log = logging::find_logger("funcswin");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
ratiocmp(double r1, double r2)
{
    return (fabs(r1 - r2) < 0.00001 ? 0 : (r1 < r2) ? -1 : 1);
}

static int
functionswin_compare(
    cov_function_scope_t *fs1,
    cov_function_scope_t *fs2,
    int column)
{
    const cov_stats_t *s1 = fs1->get_stats();
    const cov_stats_t *s2 = fs2->get_stats();
    int ret = 0;

    _log.debug2("functionswin_compare: fs1=\"%s\" fs2=\"%s\"\n",
		fs1->describe(), fs2->describe());

    switch (column)
    {
    case COL_BLOCKS:
	ret = ratiocmp(s1->blocks_sort_fraction(),
		       s2->blocks_sort_fraction());
	break;

    case COL_LINES:
	ret = ratiocmp(s1->lines_sort_fraction(),
		       s2->lines_sort_fraction());
	break;

    case COL_CALLS:
	ret = ratiocmp(s1->calls_sort_fraction(),
		       s2->calls_sort_fraction());
	break;

    case COL_BRANCHES:
	ret = ratiocmp(s1->branches_sort_fraction(),
		       s2->branches_sort_fraction());
	break;

    case COL_FUNCTION:
	ret = 0;
	break;

    default:
	return 0;
    }

    if (ret == 0)
	ret = strcmp(fs1->function()->name(), fs2->function()->name());

    return ret;
}

static int
functionswin_tree_iter_compare(
    GtkTreeModel *tm,
    GtkTreeIter *iter1,
    GtkTreeIter *iter2,
    gpointer data)
{
    cov_function_scope_t *fs1 = 0;
    cov_function_scope_t *fs2 = 0;

    gtk_tree_model_get(tm, iter1, COL_CLOSURE, &fs1, -1);
    gtk_tree_model_get(tm, iter2, COL_CLOSURE, &fs2, -1);
    return functionswin_compare(fs1, fs2, GPOINTER_TO_INT(data));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

functionswin_t::functionswin_t()
{
    GladeXML *xml;
    GtkTreeViewColumn *col;
    GtkCellRenderer *rend;

    /* load the interface & connect signals */
    xml = ui_load_tree("functions");

    set_window(glade_xml_get_widget(xml, "functions"));

    blocks_check_ = glade_xml_get_widget(xml, "functions_blocks_check");
    lines_check_ = glade_xml_get_widget(xml, "functions_lines_check");
    calls_check_ = glade_xml_get_widget(xml, "functions_calls_check");
    branches_check_ = glade_xml_get_widget(xml, "functions_branches_check");
    percent_check_ = glade_xml_get_widget(xml, "functions_percent_check");
    clist_ = glade_xml_get_widget(xml, "functions_clist");
    store_ = gtk_list_store_new(NUM_COLS, COL_TYPES);
    /* default alphabetic sort is adequate for COL_FUNCTION */
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_BLOCKS,
	  functionswin_tree_iter_compare, GINT_TO_POINTER(COL_BLOCKS), 0);
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_LINES,
	  functionswin_tree_iter_compare, GINT_TO_POINTER(COL_LINES), 0);
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_CALLS,
	  functionswin_tree_iter_compare, GINT_TO_POINTER(COL_CALLS), 0);
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_BRANCHES,
	  functionswin_tree_iter_compare, GINT_TO_POINTER(COL_BRANCHES), 0);

    gtk_tree_view_set_model(GTK_TREE_VIEW(clist_), GTK_TREE_MODEL(store_));

    rend = gtk_cell_renderer_text_new();

    col = gtk_tree_view_column_new_with_attributes(_("Blocks"), rend,
		"text", COL_BLOCKS,
		"foreground-gdk", COL_FG_GDK,/* only needed on 1st column */
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_BLOCKS);
    gtk_tree_view_append_column(GTK_TREE_VIEW(clist_), col);

    col = gtk_tree_view_column_new_with_attributes(_("Lines"), rend,
		"text", COL_LINES,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_LINES);
    gtk_tree_view_append_column(GTK_TREE_VIEW(clist_), col);

    col = gtk_tree_view_column_new_with_attributes(_("Calls"), rend,
		"text", COL_CALLS,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_CALLS);
    gtk_tree_view_append_column(GTK_TREE_VIEW(clist_), col);

    col = gtk_tree_view_column_new_with_attributes(_("Branches"), rend,
		"text", COL_BRANCHES,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_BRANCHES);
    gtk_tree_view_append_column(GTK_TREE_VIEW(clist_), col);

    col = gtk_tree_view_column_new_with_attributes(_("Function"), rend,
		"text", COL_FUNCTION,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_FUNCTION);
    gtk_tree_view_append_column(GTK_TREE_VIEW(clist_), col);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(clist_), TRUE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(clist_), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(clist_), COL_FUNCTION);

    ui_register_windows_menu(ui_get_dummy_menu(xml, "functions_windows_dummy"));
}

functionswin_t::~functionswin_t()
{
    g_object_unref(G_OBJECT(store_));
    functions_.delete_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
functionswin_t::populate()
{
    _log.debug("functionswin_t::populate\n");

    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	for (ptrarray_iterator_t<cov_function_t> fnitr = (*iter)->functions().first() ; *fnitr ; ++fnitr)
	{
	    cov_function_t *fn = *fnitr;
	    cov::status_t st = fn->status();

	    if (st != cov::SUPPRESSED && st != cov::UNINSTRUMENTED)
		functions_.prepend(new cov_function_scope_t(fn));
	}
    }

    update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
format_stat(
    char *buf,
    unsigned int maxlen,
    gboolean percent_flag,
    unsigned long numerator,
    unsigned long denominator)
{
    if (denominator == 0)
	*buf = '\0';
    else if (percent_flag)
	snprintf(buf, maxlen, "%.2f", (double)numerator * 100.0 / denominator);
    else
	snprintf(buf, maxlen, "%lu/%lu", numerator, denominator);
}


void
functionswin_t::update()
{
    gboolean percent_flag;
    GdkColor *color;
    char *text[NUM_COLS];
    char blocks_pc_buf[16];
    char lines_pc_buf[16];
    char calls_pc_buf[16];
    char branches_pc_buf[16];

    _log.debug("functionswin_t::update\n");

    percent_flag = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(percent_check_));

    gtk_list_store_clear(store_);

    for (list_iterator_t<cov_function_scope_t> iter = functions_.first() ; *iter ; ++iter)
    {
	const cov_stats_t *stats = (*iter)->get_stats();
	GtkTreeIter titer;

	format_stat(blocks_pc_buf, sizeof(blocks_pc_buf), percent_flag,
		       stats->blocks_executed(), stats->blocks_total());
	text[COL_BLOCKS] = blocks_pc_buf;

	format_stat(lines_pc_buf, sizeof(lines_pc_buf), percent_flag,
		       stats->lines_executed(), stats->lines_total());
	text[COL_LINES] = lines_pc_buf;

	format_stat(calls_pc_buf, sizeof(calls_pc_buf), percent_flag,
		       stats->calls_executed(), stats->calls_total());
	text[COL_CALLS] = calls_pc_buf;

	format_stat(branches_pc_buf, sizeof(branches_pc_buf), percent_flag,
		       stats->branches_executed(), stats->branches_total());
	text[COL_BRANCHES] = branches_pc_buf;

	text[COL_FUNCTION] = (char *)(*iter)->function()->unambiguous_name();

	color = foregrounds_by_status[(*iter)->function()->status()];

	gtk_list_store_append(store_, &titer);
	gtk_list_store_set(store_,  &titer,
	    COL_FUNCTION, text[COL_FUNCTION],
	    COL_BLOCKS, text[COL_BLOCKS],
	    COL_LINES, text[COL_LINES],
	    COL_CALLS, text[COL_CALLS],
	    COL_BRANCHES, text[COL_BRANCHES],
	    COL_CLOSURE, (*iter),
	    COL_FG_GDK, color,
	    -1);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
functionswin_t::load_state()
{
    populating_ = TRUE; /* suppress check menu item callback */
    load(GTK_CHECK_MENU_ITEM(blocks_check_));
    load(GTK_CHECK_MENU_ITEM(lines_check_));
    load(GTK_CHECK_MENU_ITEM(calls_check_));
    load(GTK_CHECK_MENU_ITEM(branches_check_));
    load(GTK_CHECK_MENU_ITEM(percent_check_));
    apply_toggles();
    populating_ = FALSE;
}

void
functionswin_t::save_state()
{
    save(GTK_CHECK_MENU_ITEM(blocks_check_));
    save(GTK_CHECK_MENU_ITEM(lines_check_));
    save(GTK_CHECK_MENU_ITEM(calls_check_));
    save(GTK_CHECK_MENU_ITEM(branches_check_));
    save(GTK_CHECK_MENU_ITEM(percent_check_));
    confsection_t::sync();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
functionswin_t::apply_toggles()
{
    ui_list_set_column_visibility(clist_, COL_BLOCKS,
		    gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(blocks_check_)));
    ui_list_set_column_visibility(clist_, COL_LINES,
		    gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(lines_check_)));
    ui_list_set_column_visibility(clist_, COL_CALLS,
		    gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(calls_check_)));
    ui_list_set_column_visibility(clist_, COL_BRANCHES,
		    gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(branches_check_)));
}

GLADE_CALLBACK void
functionswin_t::on_blocks_check_activate()
{
    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
functionswin_t::on_lines_check_activate()
{
    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
functionswin_t::on_calls_check_activate()
{
    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
functionswin_t::on_branches_check_activate()
{
    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
functionswin_t::on_percent_check_activate()
{
    if (populating_)
	return;
    update();
    save_state();
}

GLADE_CALLBACK gboolean
functionswin_t::on_clist_button_press_event(GdkEvent *event)
{
    cov_function_scope_t *fs = (cov_function_scope_t *)
	ui_list_double_click_data(clist_, event, COL_CLOSURE);

    if (fs)
	sourcewin_t::show_function(fs->function());
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
