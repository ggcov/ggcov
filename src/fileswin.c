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

#include "fileswin.h"
#include <math.h>
#include "sourcewin.h"
#include "cov.h"
#include "estring.h"

CVSID("$Id: fileswin.c,v 1.1 2001-11-30 01:05:26 gnb Exp $");

typedef struct
{
    cov_file_t *file;
    cov_stats_t stats;
} file_rec_t;


static const char fileswin_window_key[] = "fileswin_key";

static void fileswin_populate(fileswin_t*);
static void fileswin_update(fileswin_t*);

#define COL_LINES   	0
#define COL_CALLS   	1
#define COL_BRANCHES	2
#define COL_FILE	3
#define NUM_COLS    	4

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static file_rec_t *
file_rec_new(cov_file_t *fn)
{
    file_rec_t *fr;
    
    fr = new(file_rec_t);
    
    fr->file = fn;

    cov_stats_init(&fr->stats);
    cov_file_calc_stats(fn, &fr->stats);
    
    return fr;
}

static void
file_rec_delete(file_rec_t *fr)
{
    g_free(fr);
}

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

fileswin_t *
fileswin_new(void)
{
    fileswin_t *fw;
    GladeXML *xml;
    
    fw = new(fileswin_t);

    /* load the interface & connect signals */
    xml = ui_load_tree("files");
    
    fw->window = glade_xml_get_widget(xml, "files");
    ui_register_window(fw->window);
    
    fw->lines_check = glade_xml_get_widget(xml, "files_lines_check");
    fw->calls_check = glade_xml_get_widget(xml, "files_calls_check");
    fw->branches_check = glade_xml_get_widget(xml, "files_branches_check");
    fw->percent_check = glade_xml_get_widget(xml, "files_percent_check");
    fw->clist = glade_xml_get_widget(xml, "files_clist");

    gtk_clist_set_column_justification(GTK_CLIST(fw->clist), COL_LINES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(fw->clist), COL_CALLS,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(fw->clist), COL_BRANCHES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(fw->clist), COL_FILE,
    	    	    	    	       GTK_JUSTIFY_LEFT);
    gtk_clist_set_compare_func(GTK_CLIST(fw->clist), fileswin_compare);

    ui_clist_init_column_arrow(GTK_CLIST(fw->clist), COL_LINES);
    ui_clist_init_column_arrow(GTK_CLIST(fw->clist), COL_CALLS);
    ui_clist_init_column_arrow(GTK_CLIST(fw->clist), COL_BRANCHES);
    ui_clist_init_column_arrow(GTK_CLIST(fw->clist), COL_FILE);
    ui_clist_set_sort_column(GTK_CLIST(fw->clist), COL_LINES);
    ui_clist_set_sort_type(GTK_CLIST(fw->clist), GTK_SORT_DESCENDING);

    ui_register_windows_menu(ui_get_dummy_menu(xml, "files_windows_dummy"));

    gtk_object_set_data(GTK_OBJECT(fw->window), fileswin_window_key, fw);
    
    fileswin_populate(fw);
    fileswin_update(fw);

    gtk_widget_show(fw->window);
    
    return fw;
}

void
fileswin_delete(fileswin_t *fw)
{
    /* JIC of strange gui stuff */
    if (fw->deleting)
    	return;
    fw->deleting = TRUE;
    
    gtk_widget_destroy(fw->window);
    listdelete(fw->files, file_rec_t, file_rec_delete);
    g_free(fw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static fileswin_t *
fileswin_from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : gtk_object_get_data(GTK_OBJECT(w), fileswin_window_key));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
add_file(cov_file_t *f, void *userdata)
{
    fileswin_t *fw = (fileswin_t *)userdata;
    
    fw->files = g_list_prepend(fw->files, file_rec_new(f));
}

static void
fileswin_populate(fileswin_t *fw)
{
#if DEBUG
    fprintf(stderr, "fileswin_populate\n");
#endif

    cov_file_foreach(add_file, fw);
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


static void
fileswin_update(fileswin_t *fw)
{
    GList *iter;
    gboolean percent_flag;
    char *text[NUM_COLS];
    char lines_pc_buf[16];
    char calls_pc_buf[16];
    char branches_pc_buf[16];
    
#if DEBUG
    fprintf(stderr, "fileswin_update\n");
#endif

    percent_flag = GTK_CHECK_MENU_ITEM(fw->percent_check)->active;

    gtk_clist_freeze(GTK_CLIST(fw->clist));
    gtk_clist_clear(GTK_CLIST(fw->clist));
    
    for (iter = fw->files ; iter != 0 ; iter = iter->next)
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
	
	text[COL_FILE] = fr->file->name;
	
	row = gtk_clist_prepend(GTK_CLIST(fw->clist), text);
	gtk_clist_set_row_data(GTK_CLIST(fw->clist), row, fr);
    }
    
    gtk_clist_columns_autosize(GTK_CLIST(fw->clist));
    gtk_clist_sort(GTK_CLIST(fw->clist));
    gtk_clist_thaw(GTK_CLIST(fw->clist));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_files_close_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_from_widget(w);
    
    if (fw != 0)
	fileswin_delete(fw);
}

GLADE_CALLBACK void
on_files_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_files_lines_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist), COL_LINES,
    	    	    GTK_CHECK_MENU_ITEM(fw->lines_check)->active);
}

GLADE_CALLBACK void
on_files_calls_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist), COL_CALLS,
    	    	    GTK_CHECK_MENU_ITEM(fw->calls_check)->active);
}

GLADE_CALLBACK void
on_files_branches_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist), COL_BRANCHES,
    	    	    GTK_CHECK_MENU_ITEM(fw->branches_check)->active);
}

GLADE_CALLBACK void
on_files_percent_check_activate(GtkWidget *w, gpointer data)
{
    fileswin_t *fw = fileswin_from_widget(w);

    fileswin_update(fw);
}

GLADE_CALLBACK void
on_files_clist_button_press_event(
    GtkWidget *w,
    GdkEvent *event,
    gpointer data)
{
    int row, col;
    file_rec_t *fr;
    sourcewin_t *srcw;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     event->button.x, event->button.y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_files_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	fr = gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	srcw = sourcewin_new();
	sourcewin_set_filename(srcw, fr->file->name);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
