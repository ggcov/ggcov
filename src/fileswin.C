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

#include "fileswin.H"
#include <math.h>
#include "sourcewin.H"
#include "cov.h"

CVSID("$Id: fileswin.C,v 1.2 2002-12-22 02:10:44 gnb Exp $");


#define COL_LINES   	0
#define COL_CALLS   	1
#define COL_BRANCHES	2
#define COL_FILE	3
#define NUM_COLS    	4

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct file_rec_t
{
    cov_file_t *file;
    cov_stats_t stats;

    file_rec_t(cov_file_t *fn)
    {
	file = fn;
	cov_stats_init(&stats);
	cov_file_calc_stats(fn, &stats);
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
    	return strcmp(fr1->file->name, fr2->file->name);
	
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
    clist_ = glade_xml_get_widget(xml, "files_clist");

    gtk_clist_set_column_justification(GTK_CLIST(clist_), COL_LINES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(clist_), COL_CALLS,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(clist_), COL_BRANCHES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(clist_), COL_FILE,
    	    	    	    	       GTK_JUSTIFY_LEFT);
    gtk_clist_set_compare_func(GTK_CLIST(clist_), fileswin_compare);

    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_LINES);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_CALLS);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_BRANCHES);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_FILE);
    ui_clist_set_sort_column(GTK_CLIST(clist_), COL_LINES);
    ui_clist_set_sort_type(GTK_CLIST(clist_), GTK_SORT_DESCENDING);

    ui_register_windows_menu(ui_get_dummy_menu(xml, "files_windows_dummy"));
}

fileswin_t::~fileswin_t()
{
    listdelete(files_, file_rec_t, delete);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
fileswin_t::add_file(cov_file_t *f, void *userdata)
{
    fileswin_t *fw = (fileswin_t *)userdata;
    
    fw->files_ = g_list_prepend(fw->files_, new file_rec_t(f));
}

void
fileswin_t::populate()
{
#if DEBUG
    fprintf(stderr, "fileswin_t::populate\n");
#endif

    cov_file_foreach(add_file, this);
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
fileswin_t::update()
{
    GList *iter;
    gboolean percent_flag;
    char *text[NUM_COLS];
    char lines_pc_buf[16];
    char calls_pc_buf[16];
    char branches_pc_buf[16];
    
#if DEBUG
    fprintf(stderr, "fileswin_t::update\n");
#endif

    percent_flag = GTK_CHECK_MENU_ITEM(percent_check_)->active;

    gtk_clist_freeze(GTK_CLIST(clist_));
    gtk_clist_clear(GTK_CLIST(clist_));
    
    for (iter = files_ ; iter != 0 ; iter = iter->next)
    {
    	file_rec_t *fr = (file_rec_t *)iter->data;
	int row;

    	format_stat(lines_pc_buf, sizeof(lines_pc_buf), percent_flag,
	    	    fr->stats.lines_executed, fr->stats.lines);
	text[COL_LINES] = lines_pc_buf;
	
    	format_stat(calls_pc_buf, sizeof(calls_pc_buf), percent_flag,
	    	    fr->stats.calls_executed, fr->stats.calls);
	text[COL_CALLS] = calls_pc_buf;
	
    	format_stat(branches_pc_buf, sizeof(branches_pc_buf), percent_flag,
	    	    fr->stats.branches_executed, fr->stats.branches);
	text[COL_BRANCHES] = branches_pc_buf;
	
	text[COL_FILE] = (char *)cov_file_minimal_name(fr->file);
	
	row = gtk_clist_prepend(GTK_CLIST(clist_), text);
	gtk_clist_set_row_data(GTK_CLIST(clist_), row, fr);
    }
    
    gtk_clist_columns_autosize(GTK_CLIST(clist_));
    gtk_clist_sort(GTK_CLIST(clist_));
    gtk_clist_thaw(GTK_CLIST(clist_));
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

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist_), COL_LINES,
    	    	    GTK_CHECK_MENU_ITEM(fw->lines_check_)->active);
}

GLADE_CALLBACK void
on_files_calls_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist_), COL_CALLS,
    	    	    GTK_CHECK_MENU_ITEM(fw->calls_check_)->active);
}

GLADE_CALLBACK void
on_files_branches_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist_), COL_BRANCHES,
    	    	    GTK_CHECK_MENU_ITEM(fw->branches_check_)->active);
}

GLADE_CALLBACK void
on_files_percent_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_t::from_widget(w);

    fw->update();
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
	
	sourcewin_t::show_file(cov_file_minimal_name(fr->file));
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
