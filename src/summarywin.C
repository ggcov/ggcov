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

#include "summarywin.H"
#include "sourcewin.H"
#include "filename.h"
#include "cov.H"
#include "estring.H"
#include "prefs.H"
#include "uix.h"
#include "gnbprogressbar.h"

CVSID("$Id: summarywin.C,v 1.6 2003-01-04 02:38:14 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

summarywin_t::summarywin_t()
{
    GladeXML *xml;
    
    /* load the interface & connect signals */
    xml = ui_load_tree("summary");
    
    set_window(glade_xml_get_widget(xml, "summary"));
    
    scope_radio_[SU_OVERALL] = glade_xml_get_widget(xml,
    	    	    	    	    	    	    "summary_overall_radio");
    scope_radio_[SU_FILENAME] = glade_xml_get_widget(xml,
    	    	    	    	    	    	    "summary_filename_radio");
    scope_radio_[SU_FUNCTION] = glade_xml_get_widget(xml,
    	    	    	    	    	    	    "summary_function_radio");
    scope_radio_[SU_RANGE] = glade_xml_get_widget(xml,
    	    	    	    	    	    	    "summary_range_radio");
    
    filename_combo_ = glade_xml_get_widget(xml, "summary_filename_combo");
    filename_view_ = glade_xml_get_widget(xml, "summary_filename_view");
    function_combo_ = glade_xml_get_widget(xml, "summary_function_combo");
    function_view_ = glade_xml_get_widget(xml, "summary_function_view");
    range_combo_ = glade_xml_get_widget(xml, "summary_range_combo");
    range_start_spin_ = glade_xml_get_widget(xml, "summary_range_start_spin");
    range_end_spin_ = glade_xml_get_widget(xml, "summary_range_end_spin");
    range_view_ = glade_xml_get_widget(xml, "summary_range_view");

    lines_label_ = glade_xml_get_widget(xml, "summary_lines_label");
    lines_progressbar_ = glade_xml_get_widget(xml, "summary_lines_progressbar");
    calls_label_ = glade_xml_get_widget(xml, "summary_calls_label");
    calls_progressbar_ = glade_xml_get_widget(xml, "summary_calls_progressbar");
    branches_executed_label_ = glade_xml_get_widget(xml,
    	    	    	    	    "summary_branches_executed_label");
    branches_executed_progressbar_ = glade_xml_get_widget(xml,
    	    	    	    	    "summary_branches_executed_progressbar");
    branches_taken_label_ = glade_xml_get_widget(xml,
    	    	    	    	    "summary_branches_taken_label");
    branches_taken_progressbar_ = glade_xml_get_widget(xml,
    	    	    	    	    "summary_branches_taken_progressbar");
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "summary_windows_dummy"));

    gtk_toggle_button_set_active(
    	    		    GTK_TOGGLE_BUTTON(scope_radio_[SU_OVERALL]),
			    TRUE);
    
#if 0
    gdk_color_parse("#d01010", &red);
    gdk_color_parse("#10d010", &green);
#endif

    gnb_progress_bar_set_trough_color(GNB_PROGRESS_BAR(lines_progressbar_),
	    	    	    	      &prefs.uncovered_foreground);
    gnb_progress_bar_set_thumb_color(GNB_PROGRESS_BAR(lines_progressbar_),
	    	    	    	     &prefs.covered_foreground);
    gnb_progress_bar_set_trough_color(GNB_PROGRESS_BAR(calls_progressbar_),
	    	    	    	      &prefs.uncovered_foreground);
    gnb_progress_bar_set_thumb_color(GNB_PROGRESS_BAR(calls_progressbar_),
	    	    	    	     &prefs.covered_foreground);
    gnb_progress_bar_set_trough_color(GNB_PROGRESS_BAR(branches_executed_progressbar_),
	    	    	    	      &prefs.uncovered_foreground);
    gnb_progress_bar_set_thumb_color(GNB_PROGRESS_BAR(branches_executed_progressbar_),
	    	    	    	     &prefs.covered_foreground);
    gnb_progress_bar_set_trough_color(GNB_PROGRESS_BAR(branches_taken_progressbar_),
	    	    	    	      &prefs.uncovered_foreground);
    gnb_progress_bar_set_thumb_color(GNB_PROGRESS_BAR(branches_taken_progressbar_),
	    	    	    	     &prefs.covered_foreground);
}


summarywin_t::~summarywin_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
populate_filename_combo(GtkCombo *combo)
{
    list_iterator_t<cov_file_t> iter;
    
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    {
    	cov_file_t *f = *iter;
	
    	ui_combo_add_data(combo, f->minimal_name(), (gpointer)f);
    }
}

static void
populate_function_combo(GtkCombo *combo)
{
    GList *list, *iter;
    estring label;
    
    list = cov_function_t::list_all();
    
    for (iter = list ; iter != 0 ; iter = iter->next)
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
    
    while (list != 0)
	list = g_list_remove_link(list, list);
}

void
summarywin_t::populate()
{
#if DEBUG
    fprintf(stderr, "summarywin_t::populate\n");
#endif

    populate_filename_combo(GTK_COMBO(filename_combo_));
    populate_function_combo(GTK_COMBO(function_combo_));
    populate_filename_combo(GTK_COMBO(range_combo_));
    update();
    spin_update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
summarywin_t::spin_update()
{
    GtkWidget *entry = GTK_COMBO(range_combo_)->entry;
    char *filename = gtk_entry_get_text(GTK_ENTRY(entry));
    GtkAdjustment *adj;
    unsigned long lastline;
    
    lastline = cov_file_t::find(filename)->get_last_location()->lineno;
    
#if DEBUG
    fprintf(stderr, "summarywin_t::spin_update: %s[1-%lu]\n", filename, lastline);
#endif

    adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(range_start_spin_));
    adj->lower = 1;
    adj->upper = lastline;
    gtk_adjustment_changed(adj);

    adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(range_end_spin_));
    adj->lower = 1;
    adj->upper = lastline;
    gtk_adjustment_changed(adj);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
summarywin_t::get_range(
    char **filenamep,
    unsigned long *startlinep,
    unsigned long *endlinep)
{
    *filenamep = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(range_combo_)->entry));
    *startlinep = gtk_spin_button_get_value_as_int(
	    	    	    GTK_SPIN_BUTTON(range_start_spin_));
    *endlinep = gtk_spin_button_get_value_as_int(
	    	    	    GTK_SPIN_BUTTON(range_end_spin_));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
summarywin_t::grey_items()
{
    gtk_widget_set_sensitive(filename_combo_, (scope_ == SU_FILENAME));
    gtk_widget_set_sensitive(filename_view_, (scope_ == SU_FILENAME));
    gtk_widget_set_sensitive(function_combo_, (scope_ == SU_FUNCTION));
    gtk_widget_set_sensitive(function_view_, (scope_ == SU_FUNCTION));
    gtk_widget_set_sensitive(range_combo_, (scope_ == SU_RANGE));
    gtk_widget_set_sensitive(range_start_spin_, (scope_ == SU_RANGE));
    gtk_widget_set_sensitive(range_end_spin_, (scope_ == SU_RANGE));
    gtk_widget_set_sensitive(range_view_, (scope_ == SU_RANGE));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
set_label_fraction(
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
set_progressbar_fraction(
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


void
summarywin_t::update()
{
    cov_stats_t stats;
    
#if DEBUG
    fprintf(stderr, "summarywin_t::update\n");
#endif
    
    grey_items();
    
    switch (scope_)
    {

    case SU_OVERALL:
    	set_title("Overall");
    	cov_overall_calc_stats(&stats);
	break;

    case SU_FILENAME:
    	{
	    cov_file_t *f = (cov_file_t *)ui_combo_get_current_data(
	    	    	    	    	    GTK_COMBO(filename_combo_));

	    set_title(f->minimal_name());
    	    f->calc_stats(&stats);
	}
	break;

    case SU_FUNCTION:
    	{
	    cov_function_t *fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	    	GTK_COMBO(function_combo_));

	    set_title(fn->name());
    	    fn->calc_stats(&stats);
	}
	break;

    case SU_RANGE:
    	{
	    char *filename;
    	    cov_location_t start, end;
	    char *title;

    	    get_range(&filename, &start.lineno, &end.lineno);
    	    end.filename = start.filename = cov_file_t::unminimise_name(filename);

#if DEBUG
    	    fprintf(stderr, "summarywin_update: SU_RANGE %s %ld-%ld\n",
	    	    	    	start.filename, start.lineno, end.lineno);
#endif
				
    	    title = g_strdup_printf("%s:%lu-%lu",
	    	    	    	    filename,
	    	    	    	    start.lineno, end.lineno);
	    set_title(title);
	    g_free(title);
	    
    	    cov_range_calc_stats(&start, &end, &stats);
	    g_free(start.filename);
	}
	break;
	
    case SU_NSCOPES:
    	break;
    }
    
    set_label_fraction(lines_label_,
    	    	    	 stats.lines_executed, stats.lines);
    set_progressbar_fraction(lines_progressbar_,
    	    	    	 stats.lines_executed, stats.lines);
    set_label_fraction(calls_label_,
    	    	    	 stats.calls_executed, stats.calls);
    set_progressbar_fraction(calls_progressbar_,
    	    	    	 stats.calls_executed, stats.calls);
    set_label_fraction(branches_executed_label_,
    	    	    	 stats.branches_executed, stats.branches);
    set_progressbar_fraction(branches_executed_progressbar_,
    	    	    	 stats.branches_executed, stats.branches);
    set_label_fraction(branches_taken_label_,
    	    	    	 stats.branches_taken, stats.branches);
    set_progressbar_fraction(branches_taken_progressbar_,
    	    	    	 stats.branches_taken, stats.branches);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_summary_close_activate(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);
    
    assert(sw != 0);
    delete sw;
}

GLADE_CALLBACK void
on_summary_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_summary_overall_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    sw->scope_ = SU_OVERALL;
    sw->update();   
}

GLADE_CALLBACK void
on_summary_filename_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    sw->scope_ = SU_FILENAME;
    sw->update();   
}

GLADE_CALLBACK void
on_summary_filename_entry_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    sw->update();   
}

GLADE_CALLBACK void
on_summary_filename_view_clicked(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);
    cov_file_t *f = (cov_file_t *)ui_combo_get_current_data(
	    	    	    	    GTK_COMBO(sw->filename_combo_));

    sourcewin_t::show_file(f);
}

GLADE_CALLBACK void
on_summary_function_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    sw->scope_ = SU_FUNCTION;
    sw->update();   
}

GLADE_CALLBACK void
on_summary_function_entry_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    sw->update();   
}

GLADE_CALLBACK void
on_summary_function_view_clicked(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);
    cov_function_t *fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	GTK_COMBO(sw->function_combo_));

    sourcewin_t::show_function(fn);
}

GLADE_CALLBACK void
on_summary_range_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    sw->scope_ = SU_RANGE;
    sw->update();   
}

GLADE_CALLBACK void
on_summary_range_entry_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);
    
    sw->spin_update();
    sw->update();   
}

GLADE_CALLBACK void
on_summary_range_start_spin_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    sw->update();   
}

GLADE_CALLBACK void
on_summary_range_end_spin_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    sw->update();   
}

GLADE_CALLBACK void
on_summary_range_view_clicked(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);
    unsigned long start, end;
    char *filename;
    
    sw->get_range(&filename, &start, &end);
    sourcewin_t::show_lines(filename, start, end);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
