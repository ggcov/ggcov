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

#include "summarywin.h"
#include "sourcewin.h"
#include "cov.h"
#include "estring.h"
#include "uix.h"

CVSID("$Id: summarywin.c,v 1.5 2001-11-26 01:11:36 gnb Exp $");

extern GList *filenames;

static const char summarywin_window_key[] = "summarywin_key";

static void summarywin_populate(summarywin_t*);
static void summarywin_spin_update(summarywin_t *sw);
static void summarywin_update(summarywin_t*);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

summarywin_t *
summarywin_new(void)
{
    summarywin_t *sw;
    GladeXML *xml;
    
    sw = new(summarywin_t);

    /* load the interface & connect signals */
    xml = ui_load_tree("summary");
    
    sw->window = glade_xml_get_widget(xml, "summary");
    ui_register_window(sw->window);
    
    sw->scope_radio[SU_OVERALL] = glade_xml_get_widget(xml,
    	    	    	    	    	    	    "summary_overall_radio");
    sw->scope_radio[SU_FILENAME] = glade_xml_get_widget(xml,
    	    	    	    	    	    	    "summary_filename_radio");
    sw->scope_radio[SU_FUNCTION] = glade_xml_get_widget(xml,
    	    	    	    	    	    	    "summary_function_radio");
    sw->scope_radio[SU_RANGE] = glade_xml_get_widget(xml,
    	    	    	    	    	    	    "summary_range_radio");
    
    sw->filename_combo = glade_xml_get_widget(xml, "summary_filename_combo");
    sw->filename_view = glade_xml_get_widget(xml, "summary_filename_view");
    sw->function_combo = glade_xml_get_widget(xml, "summary_function_combo");
    sw->function_view = glade_xml_get_widget(xml, "summary_function_view");
    sw->range_combo = glade_xml_get_widget(xml, "summary_range_combo");
    sw->range_start_spin = glade_xml_get_widget(xml, "summary_range_start_spin");
    sw->range_end_spin = glade_xml_get_widget(xml, "summary_range_end_spin");
    sw->range_view = glade_xml_get_widget(xml, "summary_range_view");

    sw->lines_label = glade_xml_get_widget(xml, "summary_lines_label");
    sw->lines_progressbar = glade_xml_get_widget(xml, "summary_lines_progressbar");
    sw->calls_label = glade_xml_get_widget(xml, "summary_calls_label");
    sw->calls_progressbar = glade_xml_get_widget(xml, "summary_calls_progressbar");
    sw->branches_executed_label = glade_xml_get_widget(xml,
    	    	    	    	    "summary_branches_executed_label");
    sw->branches_executed_progressbar = glade_xml_get_widget(xml,
    	    	    	    	    "summary_branches_executed_progressbar");
    sw->branches_taken_label = glade_xml_get_widget(xml,
    	    	    	    	    "summary_branches_taken_label");
    sw->branches_taken_progressbar = glade_xml_get_widget(xml,
    	    	    	    	    "summary_branches_taken_progressbar");
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "summary_windows_dummy"));

    gtk_object_set_data(GTK_OBJECT(sw->window), summarywin_window_key, sw);
    
    summarywin_populate(sw);
    gtk_toggle_button_set_active(
    	    		    GTK_TOGGLE_BUTTON(sw->scope_radio[SU_OVERALL]),
			    TRUE);
    
    summarywin_update(sw);
    summarywin_spin_update(sw);
    gtk_widget_show(sw->window);
    
    return sw;
}

void
summarywin_delete(summarywin_t *sw)
{
    /* JIC of strange gui stuff */
    if (sw->deleting)
    	return;
    sw->deleting = TRUE;
    
    gtk_widget_destroy(sw->window);
    g_free(sw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static summarywin_t *
summarywin_from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : gtk_object_get_data(GTK_OBJECT(w), summarywin_window_key));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
summary_populate_filename_combo(GtkCombo *combo)
{
    GList *iter;
    
    for (iter = filenames ; iter != 0 ; iter = iter->next)
    {
    	const char *filename = (const char *)iter->data;
	
    	ui_combo_add_data(combo, filename, (gpointer)filename);
    }
}

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

static void
add_functions(cov_file_t *f, void *userdata)
{
    GList **listp = (GList **)userdata;
    unsigned fnidx;
    
    for (fnidx = 0 ; fnidx < f->functions->len ; fnidx++)
    {
    	cov_function_t *fn = cov_file_nth_function(f, fnidx);
	
	if (!strncmp(fn->name, "_GLOBAL_", 8))
	    continue;
	*listp = g_list_prepend(*listp, fn);
    }
}

static void
summary_populate_function_combo(GtkCombo *combo)
{
    GList *list = 0, *iter;
    estring label;
    
    cov_file_foreach(add_functions, &list);
    list = g_list_sort(list, compare_functions);
    
    estring_init(&label);
    for (iter = list ; iter != 0 ; iter = iter->next)
    {
    	cov_function_t *fn = (cov_function_t *)iter->data;

    	estring_truncate(&label);
	estring_append_string(&label, fn->name);

    	/* see if we need to present some more scope to uniquify the name */
	if ((iter->next != 0 &&
	     !strcmp(((cov_function_t *)iter->next->data)->name, fn->name)) ||
	    (iter->prev != 0 &&
	     !strcmp(((cov_function_t *)iter->prev->data)->name, fn->name)))
	{
	    estring_append_string(&label, " (");
	    estring_append_string(&label, fn->file->name);
	    estring_append_string(&label, ")");
	}
	
    	ui_combo_add_data(combo, label.data, fn);
    }
    estring_free(&label);
    
    while (list != 0)
	list = g_list_remove_link(list, list);
}

static void
summarywin_populate(summarywin_t *sw)
{
#if DEBUG
    fprintf(stderr, "summarywin_populate\n");
#endif
    
    summary_populate_filename_combo(GTK_COMBO(sw->filename_combo));
    summary_populate_function_combo(GTK_COMBO(sw->function_combo));
    summary_populate_filename_combo(GTK_COMBO(sw->range_combo));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
summarywin_spin_update(summarywin_t *sw)
{
    GtkWidget *entry = GTK_COMBO(sw->range_combo)->entry;
    char *filename = gtk_entry_get_text(GTK_ENTRY(entry));
    GtkAdjustment *adj;
    unsigned long lastline;
    
    lastline = cov_file_get_last_location(cov_file_find(filename))->lineno;
    
#if DEBUG
    fprintf(stderr, "summarywin_spin_update: %s[1-%lu]\n", filename, lastline);
#endif

    adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(sw->range_start_spin));
    adj->lower = 1;
    adj->upper = lastline;
    gtk_adjustment_changed(adj);

    adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(sw->range_end_spin));
    adj->lower = 1;
    adj->upper = lastline;
    gtk_adjustment_changed(adj);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
summarywin_get_range(
    summarywin_t *sw,
    char **filenamep,
    unsigned long *startlinep,
    unsigned long *endlinep)
{
    *filenamep = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(sw->range_combo)->entry));
    *startlinep = gtk_spin_button_get_value_as_int(
	    	    	    GTK_SPIN_BUTTON(sw->range_start_spin));
    *endlinep = gtk_spin_button_get_value_as_int(
	    	    	    GTK_SPIN_BUTTON(sw->range_end_spin));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
summarywin_set_label(
    GtkWidget *label,
    unsigned long numerator,
    unsigned long denominator)
{
    char buf[128];

    if (denominator == 0)
	snprintf(buf, sizeof(buf), "%ld/%ld", numerator, denominator);
    else
	snprintf(buf, sizeof(buf), "%ld/%ld  %.2f%%", numerator, denominator,
    	    		(double)numerator * 100.0 / (double)denominator);
    gtk_label_set_text(GTK_LABEL(label), buf);
}

static void
summarywin_set_progressbar(
    GtkWidget *progressbar,
    unsigned long numerator,
    unsigned long denominator)
{
    if (denominator == 0)
    {
    	gtk_widget_hide(progressbar);
    }
    else
    {
    	gtk_widget_show(progressbar);
	gtk_progress_configure(GTK_PROGRESS(progressbar),
	    	    /*value*/(gfloat)numerator,
		    /*min*/0.0, /*max*/(gfloat)denominator);
    }
}


static void
summarywin_update(summarywin_t *sw)
{
    cov_stats_t stats;
    
#if DEBUG
    fprintf(stderr, "summarywin_update\n");
#endif
    
    gtk_widget_set_sensitive(sw->filename_combo, (sw->scope == SU_FILENAME));
    gtk_widget_set_sensitive(sw->filename_view, (sw->scope == SU_FILENAME));
    gtk_widget_set_sensitive(sw->function_combo, (sw->scope == SU_FUNCTION));
    gtk_widget_set_sensitive(sw->function_view, (sw->scope == SU_FUNCTION));
    gtk_widget_set_sensitive(sw->range_combo, (sw->scope == SU_RANGE));
    gtk_widget_set_sensitive(sw->range_start_spin, (sw->scope == SU_RANGE));
    gtk_widget_set_sensitive(sw->range_end_spin, (sw->scope == SU_RANGE));
    gtk_widget_set_sensitive(sw->range_view, (sw->scope == SU_RANGE));
    
    cov_stats_init(&stats);
    switch (sw->scope)
    {

    case SU_OVERALL:
    	ui_window_set_title(sw->window, "Overall");
    	cov_overall_calc_stats(&stats);
	break;

    case SU_FILENAME:
    	{
	    GtkWidget *entry = GTK_COMBO(sw->filename_combo)->entry;
	    char *filename = gtk_entry_get_text(GTK_ENTRY(entry));
	    cov_file_t *f = cov_file_find(filename);

	    ui_window_set_title(sw->window, f->name);
    	    cov_file_calc_stats(f, &stats);
	}
	break;

    case SU_FUNCTION:
    	{
	    cov_function_t *fn = ui_combo_get_current_data(
	    	    	    	    	GTK_COMBO(sw->function_combo));

	    ui_window_set_title(sw->window, fn->name);
    	    cov_function_calc_stats(fn, &stats);
	}
	break;

    case SU_RANGE:
    	{
    	    cov_location_t start, end;
	    char *title;

    	    summarywin_get_range(sw, &start.filename, &start.lineno, &end.lineno);
    	    end.filename = start.filename;

#if DEBUG
    	    fprintf(stderr, "summarywin_update: SU_RANGE %s %ld-%ld\n",
	    	    	    	start.filename, start.lineno, end.lineno);
#endif
				
    	    title = g_strdup_printf("%s:%lu-%lu", start.filename,
	    	    	    	    	    	  start.lineno, end.lineno);
	    ui_window_set_title(sw->window, title);
	    g_free(title);
	    
    	    cov_range_calc_stats(&start, &end, &stats);
	}
	break;
	
    case SU_NSCOPES:
    	break;
    }
    
    summarywin_set_label(sw->lines_label,
    	    	    	 stats.lines_executed, stats.lines);
    summarywin_set_progressbar(sw->lines_progressbar,
    	    	    	 stats.lines_executed, stats.lines);
    summarywin_set_label(sw->calls_label,
    	    	    	 stats.calls_executed, stats.calls);
    summarywin_set_progressbar(sw->calls_progressbar,
    	    	    	 stats.calls_executed, stats.calls);
    summarywin_set_label(sw->branches_executed_label,
    	    	    	 stats.branches_executed, stats.branches);
    summarywin_set_progressbar(sw->branches_executed_progressbar,
    	    	    	 stats.branches_executed, stats.branches);
    summarywin_set_label(sw->branches_taken_label,
    	    	    	 stats.branches_taken, stats.branches);
    summarywin_set_progressbar(sw->branches_taken_progressbar,
    	    	    	 stats.branches_taken, stats.branches);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
summarywin_show_source(
    const char *filename,
    unsigned long startline,
    unsigned long endline)
{
    sourcewin_t *srcw = sourcewin_new();
    
    sourcewin_set_filename(srcw, filename);
    if (startline > 0)
    {
	sourcewin_ensure_visible(srcw, startline);
	sourcewin_select_region(srcw, startline, endline);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_summary_close_activate(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);
    
    if (sw != 0)
	summarywin_delete(sw);
}

GLADE_CALLBACK void
on_summary_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_summary_overall_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);

    sw->scope = SU_OVERALL;
    summarywin_update(sw);    
}

GLADE_CALLBACK void
on_summary_filename_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);

    sw->scope = SU_FILENAME;
    summarywin_update(sw);    
}

GLADE_CALLBACK void
on_summary_filename_entry_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);

    summarywin_update(sw);    
}

GLADE_CALLBACK void
on_summary_filename_view_clicked(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);
    GtkWidget *entry = GTK_COMBO(sw->filename_combo)->entry;
    char *filename = gtk_entry_get_text(GTK_ENTRY(entry));

    summarywin_show_source(filename, 0, 0);    
}

GLADE_CALLBACK void
on_summary_function_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);

    sw->scope = SU_FUNCTION;
    summarywin_update(sw);    
}

GLADE_CALLBACK void
on_summary_function_entry_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);

    summarywin_update(sw);    
}

GLADE_CALLBACK void
on_summary_function_view_clicked(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);
    cov_function_t *fn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(sw->function_combo));
    const cov_location_t *start, *end;

    start = cov_function_get_first_location(fn);
    end = cov_function_get_last_location(fn);

    summarywin_show_source(start->filename, start->lineno, end->lineno);
}

GLADE_CALLBACK void
on_summary_range_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);

    sw->scope = SU_RANGE;
    summarywin_update(sw);    
}

GLADE_CALLBACK void
on_summary_range_entry_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);
    
    summarywin_spin_update(sw);
    summarywin_update(sw);    
}

GLADE_CALLBACK void
on_summary_range_start_spin_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);

    summarywin_update(sw);    
}

GLADE_CALLBACK void
on_summary_range_end_spin_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);

    summarywin_update(sw);    
}

GLADE_CALLBACK void
on_summary_range_view_clicked(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_from_widget(w);
    unsigned long start, end;
    char *filename;
    
    summarywin_get_range(sw, &filename, &start, &end);
    summarywin_show_source(filename, start, end);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
