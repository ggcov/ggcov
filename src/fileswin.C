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

#include "fileswin.H"
#include <math.h>
#include "sourcewin.H"
#include "cov.H"
#include "filename.h"
#include "list.H"
#include "prefs.H"
#include "tok.H"
#include "confsection.H"
#include "filerec.H"
#include "logging.H"


#define COL_FILE        0
#define COL_BLOCKS      1
#define COL_LINES       2
#define COL_FUNCTIONS   3
#define COL_CALLS       4
#define COL_BRANCHES    5
#if !GTK2
#define NUM_COLS        6
#else
#define COL_CLOSURE     6
#define COL_FG_GDK      7
#define COL_ICON        8
#define NUM_COLS        9
#define COL_TYPES \
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, \
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, GDK_TYPE_COLOR, \
    G_TYPE_STRING
#endif

static logging::logger_t &_log = logging::find_logger("fileswin");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
ratiocmp(double r1, double r2)
{
    return (fabs(r1 - r2) < 0.00001 ? 0 : (r1 < r2) ? -1 : 1);
}

static int
fileswin_compare(file_rec_t *fr1, file_rec_t *fr2, int column)
{
    const cov_stats_t *s1 = fr1->get_scope()->get_stats();
    const cov_stats_t *s2 = fr2->get_scope()->get_stats();

    _log.debug2("fileswin_compare: fr1=\"%s\" fr2=\"%s\"\n",
		fr1->get_name(), fr2->get_name());
    switch (column)
    {
    case COL_BLOCKS:
	return ratiocmp(s1->blocks_sort_fraction(),
			s2->blocks_sort_fraction());

    case COL_LINES:
	return ratiocmp(s1->lines_sort_fraction(),
			s2->lines_sort_fraction());

    case COL_FUNCTIONS:
	return ratiocmp(s1->functions_sort_fraction(),
			s2->functions_sort_fraction());

    case COL_CALLS:
	return ratiocmp(s1->calls_sort_fraction(),
			s2->calls_sort_fraction());

    case COL_BRANCHES:
	return ratiocmp(s1->branches_sort_fraction(),
			s2->branches_sort_fraction());

    case COL_FILE:
	return strcmp(fr1->get_name(), fr2->get_name());

    default:
	return 0;
    }
}

#if !GTK2
static int
fileswin_clist_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    return fileswin_compare(
	(file_rec_t *)((GtkCListRow *)ptr1)->data,
	(file_rec_t *)((GtkCListRow *)ptr2)->data,
	clist->sort_column);
}
#else
static int
fileswin_tree_iter_compare(
    GtkTreeModel *tm,
    GtkTreeIter *iter1,
    GtkTreeIter *iter2,
    gpointer data)
{
    file_rec_t *fr1 = 0;
    file_rec_t *fr2 = 0;

    gtk_tree_model_get(tm, iter1, COL_CLOSURE, &fr1, -1);
    gtk_tree_model_get(tm, iter2, COL_CLOSURE, &fr2, -1);
    return fileswin_compare(fr1, fr2, GPOINTER_TO_INT(data));
}
#endif


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileswin_t::fileswin_t()
{
    GladeXML *xml;
#if GTK2
    GtkTreeViewColumn *col;
    GtkCellRenderer *rend;
#endif

    /* load the interface & connect signals */
    xml = ui_load_tree("files");

    set_window(glade_xml_get_widget(xml, "files"));

    blocks_check_ = glade_xml_get_widget(xml, "files_blocks_check");
    lines_check_ = glade_xml_get_widget(xml, "files_lines_check");
    functions_check_ = glade_xml_get_widget(xml, "files_functions_check");
    calls_check_ = glade_xml_get_widget(xml, "files_calls_check");
    branches_check_ = glade_xml_get_widget(xml, "files_branches_check");
    percent_check_ = glade_xml_get_widget(xml, "files_percent_check");
    tree_check_ = glade_xml_get_widget(xml, "files_tree_check");
    collapse_all_btn_ = glade_xml_get_widget(xml, "files_collapse_all");
    expand_all_btn_ = glade_xml_get_widget(xml, "files_expand_all");

    ctree_ = glade_xml_get_widget(xml, "files_ctree");
#if !GTK2
    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_BLOCKS,
				       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_LINES,
				       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_FUNCTIONS,
				       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_CALLS,
				       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_BRANCHES,
				       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_FILE,
				       GTK_JUSTIFY_LEFT);
    gtk_clist_set_compare_func(GTK_CLIST(ctree_), fileswin_clist_compare);

    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_BLOCKS);
    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_LINES);
    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_FUNCTIONS);
    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_CALLS);
    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_BRANCHES);
    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_FILE);
    ui_clist_set_sort_column(GTK_CLIST(ctree_), COL_BLOCKS);
    ui_clist_set_sort_type(GTK_CLIST(ctree_), GTK_SORT_DESCENDING);
#else
    store_ = gtk_tree_store_new(NUM_COLS, COL_TYPES);
    /* default alphabetic sort is adequate for COL_FILE */
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_BLOCKS,
	  fileswin_tree_iter_compare, GINT_TO_POINTER(COL_BLOCKS), 0);
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_LINES,
	  fileswin_tree_iter_compare, GINT_TO_POINTER(COL_LINES), 0);
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_FUNCTIONS,
	  fileswin_tree_iter_compare, GINT_TO_POINTER(COL_FUNCTIONS), 0);
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_CALLS,
	  fileswin_tree_iter_compare, GINT_TO_POINTER(COL_CALLS), 0);
    gtk_tree_sortable_set_sort_func((GtkTreeSortable *)store_, COL_BRANCHES,
	  fileswin_tree_iter_compare, GINT_TO_POINTER(COL_BRANCHES), 0);

    gtk_tree_view_set_model(GTK_TREE_VIEW(ctree_), GTK_TREE_MODEL(store_));

    rend = gtk_cell_renderer_text_new();

    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, _("File"));

    GtkCellRenderer *pixrend = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(col, pixrend, FALSE);
    gtk_tree_view_column_set_attributes(col, pixrend,
		"stock-id", COL_ICON,
		(char *)0);
    GtkCellRenderer *textrend = gtk_cell_renderer_text_new();
    gtk_object_set(GTK_OBJECT(textrend), "xalign", 0.0, (char *)0);
    gtk_tree_view_column_pack_start(col, textrend, TRUE);
    gtk_tree_view_column_set_attributes(col, textrend,
		"text", COL_FILE,
		"foreground-gdk", COL_FG_GDK,/* only needed on 1st column */
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_FILE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ctree_), col);

    col = gtk_tree_view_column_new_with_attributes(_("Blocks"), rend,
		"text", COL_BLOCKS,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_BLOCKS);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ctree_), col);

    col = gtk_tree_view_column_new_with_attributes(_("Lines"), rend,
		"text", COL_LINES,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_LINES);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ctree_), col);

    col = gtk_tree_view_column_new_with_attributes(_("Functions"), rend,
		"text", COL_FUNCTIONS,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_FUNCTIONS);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ctree_), col);

    col = gtk_tree_view_column_new_with_attributes(_("Calls"), rend,
		"text", COL_CALLS,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_CALLS);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ctree_), col);

    col = gtk_tree_view_column_new_with_attributes(_("Branches"), rend,
		"text", COL_BRANCHES,
		(char *)0);
    gtk_tree_view_column_set_sort_column_id(col, COL_BRANCHES);
    gtk_tree_view_append_column(GTK_TREE_VIEW(ctree_), col);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(ctree_), TRUE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ctree_), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(ctree_), COL_FILE);
#endif

    ui_register_windows_menu(ui_get_dummy_menu(xml, "files_windows_dummy"));
}

fileswin_t::~fileswin_t()
{
#if GTK2
    g_object_unref(G_OBJECT(store_));
#endif
    delete root_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileswin_t::populate()
{
    _log.debug("fileswin_t::populate\n");

    if (root_ != 0)
	delete root_;
    // common_path() includes a trailing /, we just
    // want the last directory component
    estring rootname = cov_file_t::common_path();
    rootname.truncate_to(rootname.length()-1);
    rootname.remove(0, rootname.find_last_char('/')+1);
    root_ = new file_rec_t(rootname, 0);
    root_->add_descendents(cov_file_t::first());
    if (_log.is_enabled(logging::DEBUG2))
	root_->dump(0, _log);

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
fileswin_t::add_node(
    file_rec_t *fr,
    file_rec_t *parent,
#if GTK2
    GtkTreeIter *parent_iter,
#endif
    gboolean percent_flag,
    gboolean tree_flag)
{
    GdkColor *color;
    char *text[NUM_COLS];
    char blocks_pc_buf[16];
    char lines_pc_buf[16];
    char functions_pc_buf[16];
    char calls_pc_buf[16];
    char branches_pc_buf[16];
    gboolean is_leaf;

    is_leaf = fr->is_file();
    if (tree_flag || is_leaf)
    {
	const cov_stats_t *stats = fr->get_scope()->get_stats();

	text[COL_FILE] = (char *)fr->get_name();

	format_stat(blocks_pc_buf, sizeof(blocks_pc_buf), percent_flag,
		    stats->blocks_executed(), stats->blocks_total());
	text[COL_BLOCKS] = blocks_pc_buf;

	format_stat(lines_pc_buf, sizeof(lines_pc_buf), percent_flag,
		    stats->lines_executed(), stats->lines_total());
	text[COL_LINES] = lines_pc_buf;

	format_stat(functions_pc_buf, sizeof(functions_pc_buf), percent_flag,
		    stats->functions_executed(), stats->functions_total());
	text[COL_FUNCTIONS] = functions_pc_buf;

	format_stat(calls_pc_buf, sizeof(calls_pc_buf), percent_flag,
		    stats->calls_executed(), stats->calls_total());
	text[COL_CALLS] = calls_pc_buf;

	format_stat(branches_pc_buf, sizeof(branches_pc_buf), percent_flag,
		    stats->branches_executed(), stats->branches_total());
	text[COL_BRANCHES] = branches_pc_buf;

	color = foregrounds_by_status[fr->get_scope()->status()];

#if !GTK2
	GtkCTreeNode *node = gtk_ctree_insert_node(
		GTK_CTREE(ctree_),
		(parent == 0 || !tree_flag ? 0 : (GtkCTreeNode *)parent->get_client_data()),
		(GtkCTreeNode *)0,          /* sibling */
		text,
		4,                  /* spacing */
		(GdkPixmap *)0,     /* pixmap_closed */
		(GdkBitmap *)0,     /* mask_closed */
		(GdkPixmap *)0,     /* pixmap_opened */
		(GdkBitmap *)0,     /* mask_opened */
		is_leaf,                    /* is_leaf */
		TRUE);                      /* expanded*/
	fr->set_client_data(node);
	gtk_ctree_node_set_row_data(GTK_CTREE(ctree_), node, fr);
	if (color != 0)
	    gtk_ctree_node_set_foreground(GTK_CTREE(ctree_), node, color);
	for (list_iterator_t<file_rec_t> friter = fr->first_child() ; *friter ; ++friter)
	    add_node((*friter), fr, percent_flag, tree_flag);
#else
	GtkTreeIter iter;
	gtk_tree_store_append(store_, &iter,
	    (parent == 0 || !tree_flag ? 0 : parent_iter));
	gtk_tree_store_set(store_,  &iter,
	    COL_FILE, text[COL_FILE],
	    COL_BLOCKS, text[COL_BLOCKS],
	    COL_LINES, text[COL_LINES],
	    COL_FUNCTIONS, text[COL_FUNCTIONS],
	    COL_CALLS, text[COL_CALLS],
	    COL_BRANCHES, text[COL_BRANCHES],
	    COL_CLOSURE, fr,
	    COL_FG_GDK, color,
	    COL_ICON, (is_leaf ? GTK_STOCK_FILE : GTK_STOCK_DIRECTORY),
	    -1);
	for (list_iterator_t<file_rec_t> friter = fr->first_child() ; *friter ; ++friter)
	    add_node((*friter), fr, &iter, percent_flag, tree_flag);
#endif
    }
}


void
fileswin_t::update()
{
    gboolean percent_flag;
    gboolean tree_flag;

    _log.debug("fileswin_t::update\n");

    percent_flag = GTK_CHECK_MENU_ITEM(percent_check_)->active;
    tree_flag = GTK_CHECK_MENU_ITEM(tree_check_)->active;

#if !GTK2
    gtk_clist_freeze(GTK_CLIST(ctree_));
    gtk_clist_clear(GTK_CLIST(ctree_));

    gtk_ctree_set_line_style(GTK_CTREE(ctree_),
	    tree_flag ? GTK_CTREE_LINES_SOLID : GTK_CTREE_LINES_NONE);
#else
    gtk_tree_store_clear(store_);
#endif

#if !GTK2
    add_node(root_, 0, percent_flag, tree_flag);
    gtk_clist_columns_autosize(GTK_CLIST(ctree_));
    gtk_clist_sort(GTK_CLIST(ctree_));
    gtk_clist_thaw(GTK_CLIST(ctree_));
#else
    add_node(root_, 0, 0, percent_flag, tree_flag);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(ctree_));
#endif
    grey_items();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileswin_t::grey_items()
{
    gboolean tree_flag = GTK_CHECK_MENU_ITEM(tree_check_)->active;

    gtk_widget_set_sensitive(collapse_all_btn_, tree_flag);
    gtk_widget_set_sensitive(expand_all_btn_, tree_flag);
    window_t::grey_items();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileswin_t::apply_toggles()
{
    ui_list_set_column_visibility(ctree_, COL_BLOCKS,
		    GTK_CHECK_MENU_ITEM(blocks_check_)->active);
    ui_list_set_column_visibility(ctree_, COL_LINES,
		    GTK_CHECK_MENU_ITEM(lines_check_)->active);
    ui_list_set_column_visibility(ctree_, COL_FUNCTIONS,
		    GTK_CHECK_MENU_ITEM(functions_check_)->active);
    ui_list_set_column_visibility(ctree_, COL_CALLS,
		    GTK_CHECK_MENU_ITEM(calls_check_)->active);
    ui_list_set_column_visibility(ctree_, COL_BRANCHES,
		    GTK_CHECK_MENU_ITEM(branches_check_)->active);
}

GLADE_CALLBACK void
fileswin_t::on_blocks_check_activate()
{
    _log.debug("fileswin_t::on_blocks_check_activate\n");

    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
fileswin_t::on_lines_check_activate()
{
    _log.debug("fileswin_t::on_lines_check_activate\n");

    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
fileswin_t::on_functions_check_activate()
{
    _log.debug("fileswin_t::on_functions_check_activate\n");

    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
fileswin_t::on_calls_check_activate()
{
    _log.debug("fileswin_t::on_calls_check_activate\n");

    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
fileswin_t::on_branches_check_activate()
{
    _log.debug("fileswin_t::on_branches_check_activate\n");

    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
fileswin_t::on_percent_check_activate()
{
    if (populating_)
	return;
    update();
    save_state();
}

GLADE_CALLBACK void
fileswin_t::on_tree_check_activate()
{
    if (populating_)
	return;
    update();
    save_state();
}

GLADE_CALLBACK void
fileswin_t::on_collapse_all_activate()
{
#if !GTK2
    gtk_ctree_collapse_recursive(GTK_CTREE(ctree_), root_->node);
    gtk_ctree_expand(GTK_CTREE(ctree_), root_->node);
#else
    gtk_tree_view_collapse_all(GTK_TREE_VIEW(ctree_));
#endif
}

GLADE_CALLBACK void
fileswin_t::on_expand_all_activate()
{
#if !GTK2
    gtk_ctree_expand_recursive(GTK_CTREE(ctree_), (GtkCTreeNode *)root_->get_client_data());
#else
    gtk_tree_view_expand_all(GTK_TREE_VIEW(ctree_));
#endif
}

GLADE_CALLBACK gboolean
fileswin_t::on_ctree_button_press_event(GdkEvent *event)
{
    file_rec_t *fr = (file_rec_t *)
	ui_list_double_click_data(ctree_, event, COL_CLOSURE);

    if (fr && fr->is_file())
	sourcewin_t::show_file(fr->get_file());
    return FALSE;
}

void
fileswin_t::load_state()
{
    populating_ = TRUE; /* suppress check menu item callback */
    load(GTK_CHECK_MENU_ITEM(blocks_check_));
    load(GTK_CHECK_MENU_ITEM(lines_check_));
    load(GTK_CHECK_MENU_ITEM(functions_check_));
    load(GTK_CHECK_MENU_ITEM(calls_check_));
    load(GTK_CHECK_MENU_ITEM(branches_check_));
    load(GTK_CHECK_MENU_ITEM(percent_check_));
    load(GTK_CHECK_MENU_ITEM(tree_check_));
    populating_ = FALSE;
    apply_toggles();
}

void
fileswin_t::save_state()
{
    save(GTK_CHECK_MENU_ITEM(blocks_check_));
    save(GTK_CHECK_MENU_ITEM(lines_check_));
    save(GTK_CHECK_MENU_ITEM(functions_check_));
    save(GTK_CHECK_MENU_ITEM(calls_check_));
    save(GTK_CHECK_MENU_ITEM(branches_check_));
    save(GTK_CHECK_MENU_ITEM(percent_check_));
    save(GTK_CHECK_MENU_ITEM(tree_check_));
    confsection_t::sync();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
