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

#include "fileswin.H"
#include <math.h>
#include "sourcewin.H"
#include "cov.H"
#include "prefs.H"

CVSID("$Id: fileswin.C,v 1.8 2003-03-30 04:47:56 gnb Exp $");


#define COL_FILE	0
#define COL_LINES   	1
#define COL_CALLS   	2
#define COL_BRANCHES	3
#define NUM_COLS    	4

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct file_rec_t
{
    cov_file_t *file;
    cov_stats_t stats;

    file_rec_t()
    {
    }
    
    file_rec_t(cov_file_t *f)
    {
	file = f;
	f->calc_stats(&stats);
    }
    
    ~file_rec_t()
    {
    }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define ratio(n,d) \
    ((d) == 0 ? -1.0 : (double)(n) / (double)(d))
    
static int
ratiocmp(double r1, double r2)
{
    return (fabs(r1 - r2) < 0.00001 ? 0 : (r1 < r2) ? -1 : 1);
}

static int
fileswin_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    file_rec_t *fr1 = (file_rec_t *)((GtkCListRow *)ptr1)->data;
    file_rec_t *fr2 = (file_rec_t *)((GtkCListRow *)ptr2)->data;
    static file_rec_t nullfr;
    
    if (fr1 == 0)
    	fr1 = &nullfr;
    if (fr2 == 0)
    	fr2 = &nullfr;

    switch (clist->sort_column)
    {
    case COL_LINES:
    	return ratiocmp(
    	    	    ratio(fr1->stats.lines_executed, fr1->stats.lines),
    	    	    ratio(fr2->stats.lines_executed, fr2->stats.lines));
	
    case COL_CALLS:
    	return ratiocmp(
    	    	    ratio(fr1->stats.calls_executed, fr1->stats.calls),
    	    	    ratio(fr2->stats.calls_executed, fr2->stats.calls));
	
    case COL_BRANCHES:
    	return ratiocmp(
    	    	    ratio(fr1->stats.branches_executed, fr1->stats.branches),
    	    	    ratio(fr2->stats.branches_executed, fr2->stats.branches));
	
    case COL_FILE:
    	return strcmp(fr1->file == 0 ? "" : fr1->file->name(),
	    	      fr2->file == 0 ? "" : fr2->file->name());
	
    default:
	return 0;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

fileswin_t::fileswin_t()
{
    GladeXML *xml;
    
    /* load the interface & connect signals */
    xml = ui_load_tree("files");
    
    set_window(glade_xml_get_widget(xml, "files"));
    
    lines_check_ = glade_xml_get_widget(xml, "files_lines_check");
    calls_check_ = glade_xml_get_widget(xml, "files_calls_check");
    branches_check_ = glade_xml_get_widget(xml, "files_branches_check");
    percent_check_ = glade_xml_get_widget(xml, "files_percent_check");
    tree_check_ = glade_xml_get_widget(xml, "files_tree_check");
    collapse_all_btn_ = glade_xml_get_widget(xml, "files_collapse_all");
    expand_all_btn_ = glade_xml_get_widget(xml, "files_expand_all");
    ctree_ = glade_xml_get_widget(xml, "files_ctree");

    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_LINES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_CALLS,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_BRANCHES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(ctree_), COL_FILE,
    	    	    	    	       GTK_JUSTIFY_LEFT);
    gtk_clist_set_compare_func(GTK_CLIST(ctree_), fileswin_compare);

    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_LINES);
    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_CALLS);
    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_BRANCHES);
    ui_clist_init_column_arrow(GTK_CLIST(ctree_), COL_FILE);
    ui_clist_set_sort_column(GTK_CLIST(ctree_), COL_LINES);
    ui_clist_set_sort_type(GTK_CLIST(ctree_), GTK_SORT_DESCENDING);

    ui_register_windows_menu(ui_get_dummy_menu(xml, "files_windows_dummy"));
}

fileswin_t::~fileswin_t()
{
    listdelete(files_, file_rec_t, delete);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileswin_t::populate()
{
    list_iterator_t<cov_file_t> iter;

#if DEBUG
    fprintf(stderr, "fileswin_t::populate\n");
#endif
    
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    	files_ = g_list_prepend(files_, new file_rec_t(*iter));

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

static GtkCTreeNode *
add_node(
    GtkCTree *ctree,
    GtkCTreeNode *parent,
    char **text,
    gboolean is_leaf,
    void *data,
    GdkColor *color)
{
    GtkCTreeNode *node;

    node = gtk_ctree_insert_node(
	    ctree,
	    parent,
	    (GtkCTreeNode *)0,	    /* sibling */
	    text,
	    4,     	    	    /* spacing */
	    (GdkPixmap *)0,	    /* pixmap_closed */
	    (GdkBitmap *)0,	    /* mask_closed */
	    (GdkPixmap *)0,	    /* pixmap_opened */
	    (GdkBitmap *)0,	    /* mask_opened */
	    is_leaf,  	    	    /* is_leaf */
	    TRUE);  	    	    /* expanded*/
    if (data != 0)
    	gtk_ctree_node_set_row_data(ctree, node, data);
    if (color != 0)
    	gtk_ctree_node_set_foreground(ctree, node, color);
    return node;
}

void
fileswin_t::update()
{
    GList *iter;
    gboolean percent_flag;
    gboolean tree_flag;
    GdkColor *color;
    char *text[NUM_COLS];
    char lines_pc_buf[16];
    char calls_pc_buf[16];
    char branches_pc_buf[16];
    
#if DEBUG
    fprintf(stderr, "fileswin_t::update\n");
#endif

    percent_flag = GTK_CHECK_MENU_ITEM(percent_check_)->active;
    tree_flag = GTK_CHECK_MENU_ITEM(tree_check_)->active;

    gtk_clist_freeze(GTK_CLIST(ctree_));
    gtk_clist_clear(GTK_CLIST(ctree_));
    root_ = 0;
    
    gtk_ctree_set_line_style(GTK_CTREE(ctree_),
    	    tree_flag ? GTK_CTREE_LINES_SOLID : GTK_CTREE_LINES_NONE);
    
    for (iter = files_ ; iter != 0 ; iter = iter->next)
    {
    	file_rec_t *fr = (file_rec_t *)iter->data;
	GtkCTreeNode *node;

    	format_stat(lines_pc_buf, sizeof(lines_pc_buf), percent_flag,
	    	    fr->stats.lines_executed, fr->stats.lines);
	text[COL_LINES] = lines_pc_buf;
	
    	format_stat(calls_pc_buf, sizeof(calls_pc_buf), percent_flag,
	    	    fr->stats.calls_executed, fr->stats.calls);
	text[COL_CALLS] = calls_pc_buf;
	
    	format_stat(branches_pc_buf, sizeof(branches_pc_buf), percent_flag,
	    	    fr->stats.branches_executed, fr->stats.branches);
	text[COL_BRANCHES] = branches_pc_buf;
		
	if (fr->stats.lines == 0)
	    color = &prefs.uninstrumented_foreground;
	else if (fr->stats.lines_executed == 0)
	    color = &prefs.uncovered_foreground;
	else if (fr->stats.lines_executed < fr->stats.lines)
	    color = &prefs.partcovered_foreground;
	else
	    color = &prefs.covered_foreground;

    	if (tree_flag)
	{
	    GtkCTreeNode *parent;
	    char *buf = g_strdup(fr->file->minimal_name());
	    char *end = buf + strlen(buf);
	    char *part;
	    char *text2[NUM_COLS];

	    memset(text2, 0, sizeof(text2));

    	    if (root_ == 0)
	    {
		text2[COL_FILE] = (char *)cov_file_t::common_path();
    		root_ = add_node(
				GTK_CTREE(ctree_),
				(GtkCTreeNode *)0,  /* parent */
				text2,
				FALSE,	    	    /* is_leaf */
				0,  	    	    /* data */
				0); 	    	    /* color */
	    }
    	    parent = root_;

	    for (part = strtok(buf, "/") ; 
		 part != 0 ; 
		 part = strtok(0, "/"))
	    {
		if (part + strlen(part) == end)
		{
	    	    text[COL_FILE] = part;
    		    node = add_node(
				    GTK_CTREE(ctree_),
				    parent,
				    text,
				    TRUE,  	    	/* is_leaf */
				    fr,	    	    	/* data */
				    color);     	/* color */
		}
		else
		{
	    	    for (node = GTK_CTREE_ROW(parent)->children ;
			 node != 0 ;
			 node = GTK_CTREE_NODE_NEXT(node))
		    {
			char *name = ((GtkCellText *)GTK_CTREE_ROW(node)->row.cell)->text;

			if (!strcmp(name, part))
		    	    break;
		    }
		    if (node == 0)
		    {
			text2[COL_FILE] = part;
    			node = add_node(
					GTK_CTREE(ctree_),
					parent,
					text2,
					FALSE,	    	/* is_leaf */
					0,	    	/* data */
					0);     	/* color */
		    }

		}
		parent = node;
	    }
	    g_free(buf);
	}
	else
	{
	    text[COL_FILE] = (char *)fr->file->minimal_name();
    	    add_node(
		    GTK_CTREE(ctree_),
		    (GtkCTreeNode *)0,	/* parent */
		    text,
		    TRUE,  	    	/* is_leaf */
		    fr,	    	    	/* data */
		    color);     	/* color */
	}
    }
    
    gtk_clist_columns_autosize(GTK_CLIST(ctree_));
    gtk_clist_sort(GTK_CLIST(ctree_));
    gtk_clist_thaw(GTK_CLIST(ctree_));
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

GLADE_CALLBACK void
on_files_close_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);
    
    assert(fw != 0);
    delete fw;
}

GLADE_CALLBACK void
on_files_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_files_lines_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->ctree_), COL_LINES,
    	    	    GTK_CHECK_MENU_ITEM(fw->lines_check_)->active);
}

GLADE_CALLBACK void
on_files_calls_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->ctree_), COL_CALLS,
    	    	    GTK_CHECK_MENU_ITEM(fw->calls_check_)->active);
}

GLADE_CALLBACK void
on_files_branches_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->ctree_), COL_BRANCHES,
    	    	    GTK_CHECK_MENU_ITEM(fw->branches_check_)->active);
}

GLADE_CALLBACK void
on_files_percent_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    fw->update();
}

GLADE_CALLBACK void
on_files_tree_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    fw->update();
}

GLADE_CALLBACK void
on_files_collapse_all_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    gtk_ctree_collapse_recursive(GTK_CTREE(fw->ctree_), fw->root_);
    gtk_ctree_expand(GTK_CTREE(fw->ctree_), fw->root_);
}

GLADE_CALLBACK void
on_files_expand_all_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    gtk_ctree_expand_recursive(GTK_CTREE(fw->ctree_), fw->root_);
}

GLADE_CALLBACK void
on_files_clist_button_press_event(
    GtkWidget *w,
    GdkEvent *event,
    gpointer data)
{
    int row, col;
    file_rec_t *fr;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     (int)event->button.x,
				     (int)event->button.y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_files_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	fr = (file_rec_t *)gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	sourcewin_t::show_file(fr->file);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
