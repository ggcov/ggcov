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

#include "functionswin.H"
#include <math.h>
#include "sourcewin.H"
#include "cov.H"
#include "prefs.H"

CVSID("$Id: functionswin.C,v 1.8 2003-05-31 14:39:22 gnb Exp $");


#define COL_LINES   	0
#define COL_CALLS   	1
#define COL_BRANCHES	2
#define COL_FUNCTION	3
#define NUM_COLS    	4

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
    cov_function_scope_t *fs1 = (cov_function_scope_t *)((GtkCListRow *)ptr1)->data;
    cov_function_scope_t *fs2 = (cov_function_scope_t *)((GtkCListRow *)ptr2)->data;
    const cov_stats_t *s1 = fs1->get_stats();
    const cov_stats_t *s2 = fs2->get_stats();

    switch (clist->sort_column)
    {
    case COL_LINES:
    	return ratiocmp(
    	    	    ratio(s1->lines_executed, s1->lines),
    	    	    ratio(s2->lines_executed, s2->lines));
	
    case COL_CALLS:
    	return ratiocmp(
    	    	    ratio(s1->calls_executed, s1->calls),
    	    	    ratio(s2->calls_executed, s2->calls));
	
    case COL_BRANCHES:
    	return ratiocmp(
    	    	    ratio(s1->branches_executed, s1->branches),
    	    	    ratio(s2->branches_executed, s2->branches));
	
    case COL_FUNCTION:
    	return strcmp(fs1->function()->name(), fs2->function()->name());
	
    default:
	return 0;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

functionswin_t::functionswin_t()
{
    GladeXML *xml;
    
    /* load the interface & connect signals */
    xml = ui_load_tree("functions");
    
    set_window(glade_xml_get_widget(xml, "functions"));
    
    lines_check_ = glade_xml_get_widget(xml, "functions_lines_check");
    calls_check_ = glade_xml_get_widget(xml, "functions_calls_check");
    branches_check_ = glade_xml_get_widget(xml, "functions_branches_check");
    percent_check_ = glade_xml_get_widget(xml, "functions_percent_check");
    clist_ = glade_xml_get_widget(xml, "functions_clist");

    gtk_clist_set_column_justification(GTK_CLIST(clist_), COL_LINES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(clist_), COL_CALLS,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(clist_), COL_BRANCHES,
    	    	    	    	       GTK_JUSTIFY_RIGHT);
    gtk_clist_set_column_justification(GTK_CLIST(clist_), COL_FUNCTION,
    	    	    	    	       GTK_JUSTIFY_LEFT);
    gtk_clist_set_compare_func(GTK_CLIST(clist_), functionswin_compare);

    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_LINES);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_CALLS);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_BRANCHES);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_FUNCTION);
    ui_clist_set_sort_column(GTK_CLIST(clist_), COL_LINES);
    ui_clist_set_sort_type(GTK_CLIST(clist_), GTK_SORT_DESCENDING);

    ui_register_windows_menu(ui_get_dummy_menu(xml, "functions_windows_dummy"));
}

functionswin_t::~functionswin_t()
{
    functions_.delete_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
functionswin_t::populate()
{
    list_iterator_t<cov_file_t> iter;
    unsigned int fnidx;

#if DEBUG
    fprintf(stderr, "functionswin_t::populate\n");
#endif
    
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    {
    	cov_file_t *f = *iter;

	for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
	{
    	    cov_function_t *fn = f->nth_function(fnidx);

	    if (!fn->is_suppressed())
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
    list_iterator_t<cov_function_scope_t> iter;
    gboolean percent_flag;
    GdkColor *color;
    char *text[NUM_COLS];
    char lines_pc_buf[16];
    char calls_pc_buf[16];
    char branches_pc_buf[16];
    
#if DEBUG
    fprintf(stderr, "functionswin_t::update\n");
#endif

    percent_flag = GTK_CHECK_MENU_ITEM(percent_check_)->active;

    gtk_clist_freeze(GTK_CLIST(clist_));
    gtk_clist_clear(GTK_CLIST(clist_));
    
    for (iter = functions_.first() ; iter != (cov_function_scope_t *)0 ; ++iter)
    {
	const cov_stats_t *stats = (*iter)->get_stats();
	int row;

    	format_stat(lines_pc_buf, sizeof(lines_pc_buf), percent_flag,
	    	       stats->lines_executed, stats->lines);
	text[COL_LINES] = lines_pc_buf;
	
    	format_stat(calls_pc_buf, sizeof(calls_pc_buf), percent_flag,
	    	       stats->calls_executed, stats->calls);
	text[COL_CALLS] = calls_pc_buf;
	
    	format_stat(branches_pc_buf, sizeof(branches_pc_buf), percent_flag,
	    	       stats->branches_executed, stats->branches);
	text[COL_BRANCHES] = branches_pc_buf;
	
	text[COL_FUNCTION] = (char *)(*iter)->function()->name();
	
	if (stats->lines == 0)
	    color = &prefs.uninstrumented_foreground;
	else if (stats->lines_executed == 0)
	    color = &prefs.uncovered_foreground;
	else if (stats->lines_executed < stats->lines)
	    color = &prefs.partcovered_foreground;
	else
	    color = &prefs.covered_foreground;
	
	row = gtk_clist_prepend(GTK_CLIST(clist_), text);
	gtk_clist_set_row_data(GTK_CLIST(clist_), row, (*iter));
	gtk_clist_set_foreground(GTK_CLIST(clist_), row, color);
    }
    
    gtk_clist_columns_autosize(GTK_CLIST(clist_));
    gtk_clist_sort(GTK_CLIST(clist_));
    gtk_clist_thaw(GTK_CLIST(clist_));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_functions_lines_check_activate(GtkWidget *w, gpointer data)
{
    functionswin_t *fw = functionswin_t::from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist_), COL_LINES,
    	    	    GTK_CHECK_MENU_ITEM(fw->lines_check_)->active);
}

GLADE_CALLBACK void
on_functions_calls_check_activate(GtkWidget *w, gpointer data)
{
    functionswin_t *fw = functionswin_t::from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist_), COL_CALLS,
    	    	    GTK_CHECK_MENU_ITEM(fw->calls_check_)->active);
}

GLADE_CALLBACK void
on_functions_branches_check_activate(GtkWidget *w, gpointer data)
{
    functionswin_t *fw = functionswin_t::from_widget(w);

    gtk_clist_set_column_visibility(GTK_CLIST(fw->clist_), COL_BRANCHES,
    	    	    GTK_CHECK_MENU_ITEM(fw->branches_check_)->active);
}

GLADE_CALLBACK void
on_functions_percent_check_activate(GtkWidget *w, gpointer data)
{
    functionswin_t *fw = functionswin_t::from_widget(w);

    fw->update();
}

GLADE_CALLBACK void
on_functions_clist_button_press_event(
    GtkWidget *w,
    GdkEvent *event,
    gpointer data)
{
    int row, col;
    cov_function_scope_t *fs;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     (int)event->button.x,
				     (int)event->button.y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_functions_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	fs = (cov_function_scope_t *)gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	sourcewin_t::show_function(fs->function());
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
