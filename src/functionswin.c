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

#include "functionswin.h"
#include <math.h>
#include "sourcewin.h"
#include "cov.h"
#include "estring.h"

CVSID("$Id: functionswin.c,v 1.1 2001-11-25 15:17:46 gnb Exp $");

typedef struct
{
    cov_function_t *function;
    cov_stats_t stats;
} func_rec_t;


static const char functionswin_window_key[] = "functionswin_key";

static void functionswin_populate(functionswin_t*);
static void functionswin_update(functionswin_t*);

#define COL_LINES   	0
#define COL_CALLS   	1
#define COL_BRANCHES	2
#define COL_FUNCTION	3
#define NUM_COLS    	4

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static func_rec_t *
func_rec_new(cov_function_t *fn)
{
    func_rec_t *fr;
    
    fr = new(func_rec_t);
    
    fr->function = fn;

    cov_stats_init(&fr->stats);
    cov_function_calc_stats(fn, &fr->stats);
    
    return fr;
}

static void
func_rec_delete(func_rec_t *fr)
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
functionswin_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    func_rec_t *fr1 = (func_rec_t *)((GtkCListRow *)ptr1)->data;
    func_rec_t *fr2 = (func_rec_t *)((GtkCListRow *)ptr2)->data;

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
	
    case COL_FUNCTION:
    	return strcmp(fr1->function->name, fr2->function->name);
	
    default:
	return 0;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

functionswin_t *
functionswin_new(void)
{
    functionswin_t *fw;
    GladeXML *xml;
    
    fw = new(functionswin_t);

    /* load the interface & connect signals */
    xml = ui_load_tree("functions");
    
    fw->window = glade_xml_get_widget(xml, "functions");
    ui_register_window(fw->window);
    
    fw->lines_check = glade_xml_get_widget(xml, "functions_lines_check");
    fw->calls_check = glade_xml_get_widget(xml, "functions_calls_check");
    fw->branches_check = glade_xml_get_widget(xml, "functions_branches_check");
    fw->percent_check = glade_xml_get_widget(xml, "functions_percent_check");
    fw->clist = glade_xml_get_widget(xml, "functions_clist");

    gtk_clist_set_column_justification(GTK_CLIST(fw->clist), COL_LINES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(fw->clist), COL_CALLS,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(fw->clist), COL_BRANCHES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(fw->clist), COL_FUNCTION,
    	    	    	    	       GTK_JUSTIFY_LEFT);
    gtk_clist_set_compare_func(GTK_CLIST(fw->clist), functionswin_compare);

    ui_clist_init_column_arrow(GTK_CLIST(fw->clist), COL_LINES);
    ui_clist_init_column_arrow(GTK_CLIST(fw->clist), COL_CALLS);
    ui_clist_init_column_arrow(GTK_CLIST(fw->clist), COL_BRANCHES);
    ui_clist_init_column_arrow(GTK_CLIST(fw->clist), COL_FUNCTION);
    ui_clist_set_sort_column(GTK_CLIST(fw->clist), COL_LINES);
    ui_clist_set_sort_type(GTK_CLIST(fw->clist), GTK_SORT_DESCENDING);

    ui_register_windows_menu(ui_get_dummy_menu(xml, "functions_windows_dummy"));

    gtk_object_set_data(GTK_OBJECT(fw->window), functionswin_window_key, fw);
    
    functionswin_populate(fw);
    functionswin_update(fw);

    gtk_widget_show(fw->window);
    
    return fw;
}

void
functionswin_delete(functionswin_t *fw)
{
    /* JIC of strange gui stuff */
    if (fw->deleting)
    	return;
    fw->deleting = TRUE;
    
    gtk_widget_destroy(fw->window);
    listdelete(fw->functions, func_rec_t, func_rec_delete);
    g_free(fw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static functionswin_t *
functionswin_from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : gtk_object_get_data(GTK_OBJECT(w), functionswin_window_key));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if 0

static int
compare_functions(const void *a, const void *b)
{
    const cov_function_t *fa = (const cov_function_t *)a;
    const cov_function_t *fb = (const cov_function_t *)b;
    int ret;
    
    ret = strcmp(fa->name, fb->name);
    if (ret == 0)
    	ret = strcmp(fa->file->name, fb->file->name);
    return ret;
}
#endif

static void
add_functions(cov_file_t *f, void *userdata)
{
    functionswin_t *fw = (functionswin_t *)userdata;
    unsigned fnidx;
    
    for (fnidx = 0 ; fnidx < f->functions->len ; fnidx++)
    {
    	cov_function_t *fn = cov_file_nth_function(f, fnidx);
	
	if (!strncmp(fn->name, "_GLOBAL_", 8))
	    continue;
	
	fw->functions = g_list_prepend(fw->functions, func_rec_new(fn));
    }
}

static void
functionswin_populate(functionswin_t *fw)
{
#if DEBUG
    fprintf(stderr, "functionswin_populate\n");
#endif

    cov_file_foreach(add_functions, fw);
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
functionswin_update(functionswin_t *fw)
{
    GList *iter;
    gboolean percent_flag;
    char *text[NUM_COLS];
    char lines_pc_buf[16];
    char calls_pc_buf[16];
    char branches_pc_buf[16];
    
#if DEBUG
    fprintf(stderr, "functionswin_update\n");
#endif

    percent_flag = GTK_CHECK_MENU_ITEM(fw->percent_check)->active;

    gtk_clist_freeze(GTK_CLIST(fw->clist));
    gtk_clist_clear(GTK_CLIST(fw->clist));
    
    for (iter = fw->functions ; iter != 0 ; iter = iter->next)
    {
    	func_rec_t *fr = (func_rec_t *)iter->data;
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
	
	text[COL_FUNCTION] = fr->function->name;
	
	row = gtk_clist_prepend(GTK_CLIST(fw->clist), text);
	gtk_clist_set_row_data(GTK_CLIST(fw->clist), row, fr);
    }
    
    gtk_clist_columns_autosize(GTK_CLIST(fw->clist));
    gtk_clist_sort(GTK_CLIST(fw->clist));
    gtk_clist_thaw(GTK_CLIST(fw->clist));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_functions_close_activate(GtkWidget *w, gpointer data)
{
    functionswin_t *fw = functionswin_from_widget(w);
    
    if (fw != 0)
	functionswin_delete(fw);
}

GLADE_CALLBACK void
on_functions_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_functions_lines_check_activate(GtkWidget *w, gpointer data)
{
    functionswin_t *fw = functionswin_from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist), COL_LINES,
    	    	    GTK_CHECK_MENU_ITEM(fw->lines_check)->active);
}

GLADE_CALLBACK void
on_functions_calls_check_activate(GtkWidget *w, gpointer data)
{
    functionswin_t *fw = functionswin_from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist), COL_CALLS,
    	    	    GTK_CHECK_MENU_ITEM(fw->calls_check)->active);
}

GLADE_CALLBACK void
on_functions_branches_check_activate(GtkWidget *w, gpointer data)
{
    functionswin_t *fw = functionswin_from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist), COL_BRANCHES,
    	    	    GTK_CHECK_MENU_ITEM(fw->branches_check)->active);
}

GLADE_CALLBACK void
on_functions_percent_check_activate(GtkWidget *w, gpointer data)
{
    functionswin_t *fw = functionswin_from_widget(w);

    functionswin_update(fw);
}

GLADE_CALLBACK void
on_functions_clist_button_press_event(
    GtkWidget *w,
    GdkEvent *event,
    gpointer data)
{
    int row, col;
    func_rec_t *fr;
    sourcewin_t *srcw;
    const cov_location_t *start, *end;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     event->button.x, event->button.y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_functions_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	fr = gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	start = cov_function_get_first_location(fr->function);
	end = cov_function_get_last_location(fr->function);
	
	srcw = sourcewin_new();
	sourcewin_set_filename(srcw, start->filename);
	sourcewin_ensure_visible(srcw, start->lineno);
	sourcewin_select_region(srcw, start->lineno, end->lineno);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
