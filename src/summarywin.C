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

#include "summarywin.H"
#include "sourcewin.H"
#include "filename.h"
#include "cov.H"
#include "estring.H"
#include "prefs.H"
#include "uix.h"
#include "gnbprogressbar.h"

CVSID("$Id: summarywin.C,v 1.8 2003-03-17 03:54:49 gnb Exp $");

list_t<summarywin_t> summarywin_t::instances_;

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

    instances_.append(this);
}


summarywin_t::~summarywin_t()
{
    instances_.remove(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

summarywin_t *
summarywin_t::instance()
{
    summarywin_t *sw = 0;

    if (prefs.reuse_summwin)
    	sw = instances_.head();
    if (sw == 0)
    	sw = new summarywin_t;
    return sw;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
summarywin_t::show_overall()
{
    summarywin_t *sw = instance();
    
    sw->scope_ = SU_OVERALL;
    if (sw->shown_)
    	sw->update();
    sw->show();
}

void
summarywin_t::show_file(const cov_file_t *f)
{
    summarywin_t *sw = instance();
    
    sw->scope_ = SU_FILENAME;
    sw->file_ = f;
    if (sw->shown_)
    	sw->update();
    sw->show();
}

void
summarywin_t::show_function(const cov_function_t *fn)
{
    summarywin_t *sw = instance();
    
    sw->scope_ = SU_FUNCTION;
    sw->function_ = fn;
    if (sw->shown_)
    	sw->update();
    sw->show();
}

void
summarywin_t::show_lines(
    const char *filename,
    unsigned long start,
    unsigned long end)
{
    summarywin_t *sw;
    cov_file_t *f;
    
    if ((f = cov_file_t::find(filename)) == 0)
    	return;
	
    sw = instance();
    sw->scope_ = SU_RANGE;
    sw->file_ = f;
    sw->start_ = start;
    sw->end_ = end;
    if (sw->shown_)
    	sw->update();
    sw->show();
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

    populating_ = TRUE;     /* suppress combo entry callbacks */
    populate_filename_combo(GTK_COMBO(filename_combo_));
    populate_function_combo(GTK_COMBO(function_combo_));
    populate_filename_combo(GTK_COMBO(range_combo_));
    populating_ = FALSE;
    
    if (file_ == 0)
    	file_ = (cov_file_t *)ui_combo_get_current_data(
	    	    	    	    GTK_COMBO(filename_combo_));
    if (function_ == 0)
    	function_ = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	    GTK_COMBO(function_combo_));

    update();
    spin_update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
summarywin_t::spin_update()
{
    GtkAdjustment *adj;
    unsigned long lastline;
    
    assert(file_ != 0);
    lastline = file_->get_last_location()->lineno;
    
#if DEBUG
    fprintf(stderr, "summarywin_t::spin_update: %s[1-%lu]\n",
    	    	file_->minimal_name(), lastline);
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

    populating_ = TRUE;
    assert(file_ != 0);    
    ui_combo_set_current_data(GTK_COMBO(filename_combo_), (gpointer)file_);
    ui_combo_set_current_data(GTK_COMBO(range_combo_), (gpointer)file_);
    assert(function_ != 0);    
    ui_combo_set_current_data(GTK_COMBO(function_combo_), (gpointer)function_);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scope_radio_[scope_]), TRUE);
    if (start_ > 0)
    	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_start_spin_),
	    	    	    	  (gfloat)start_);
    if (end_ > 0)
    	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_end_spin_),
	    	    	    	  (gfloat)end_);
    populating_ = FALSE;

    switch (scope_)
    {

    case SU_OVERALL:
    	set_title("Overall");
    	cov_overall_calc_stats(&stats);
	break;

    case SU_FILENAME:
    	assert(file_ != 0);
	set_title(file_->minimal_name());
    	file_->calc_stats(&stats);
	break;

    case SU_FUNCTION:
    	assert(function_ != 0);
	set_title(function_->name());
    	function_->calc_stats(&stats);
	break;

    case SU_RANGE:
    	{
    	    cov_location_t start, end;
	    char *title;

    	    assert(file_ != 0);
            start.lineno = start_;
	    end.lineno = end_;
    	    end.filename = start.filename = (char *)file_->name();

#if DEBUG
    	    fprintf(stderr, "summarywin_update: SU_RANGE %s %ld-%ld\n",
	    	    	    file_->minimal_name(), start.lineno, end.lineno);
#endif
				
    	    title = g_strdup_printf("%s:%lu-%lu",
	    	    	    	    file_->minimal_name(),
	    	    	    	    start.lineno, end.lineno);
	    set_title(title);
	    g_free(title);
	    
    	    cov_range_calc_stats(&start, &end, &stats);
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

    if (sw->populating_)
    	return;
    sw->scope_ = summarywin_t::SU_OVERALL;
    sw->update();   
}

GLADE_CALLBACK void
on_summary_filename_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    if (sw->populating_)
    	return;
    sw->scope_ = summarywin_t::SU_FILENAME;
    sw->update();   
}

GLADE_CALLBACK void
on_summary_filename_entry_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    if (sw->populating_)
    	return;
    assert(sw->scope_ == summarywin_t::SU_FILENAME);
    sw->file_ = (cov_file_t *)ui_combo_get_current_data(
	    	    	    	    GTK_COMBO(sw->filename_combo_));
    sw->update();   
}

GLADE_CALLBACK void
on_summary_filename_view_clicked(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    assert(sw->scope_ == summarywin_t::SU_FILENAME);
    assert(sw->file_ != 0);
    assert(!sw->populating_);
    sourcewin_t::show_file(sw->file_);
}

GLADE_CALLBACK void
on_summary_function_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    if (sw->populating_)
    	return;
    sw->scope_ = summarywin_t::SU_FUNCTION;
    sw->update();   
}

GLADE_CALLBACK void
on_summary_function_entry_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    if (sw->populating_)
    	return;
    assert(sw->scope_ == summarywin_t::SU_FUNCTION);
    sw->function_ = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	    	GTK_COMBO(sw->function_combo_));
    sw->update();
}

GLADE_CALLBACK void
on_summary_function_view_clicked(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    assert(sw->scope_ == summarywin_t::SU_FUNCTION);
    assert(sw->function_ != 0);
    assert(!sw->populating_);
    sourcewin_t::show_function(sw->function_);
}

GLADE_CALLBACK void
on_summary_range_radio_toggled(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    if (sw->populating_)
    	return;
    sw->scope_ = summarywin_t::SU_RANGE;
    sw->update();   
}

GLADE_CALLBACK void
on_summary_range_entry_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);
    
    if (sw->populating_)
    	return;
    assert(sw->scope_ == summarywin_t::SU_RANGE);
    sw->file_ = (cov_file_t *)ui_combo_get_current_data(
	    	    	    	    	GTK_COMBO(sw->range_combo_));
    sw->spin_update();
    sw->update();   
}

GLADE_CALLBACK void
on_summary_range_start_spin_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    if (sw->populating_)
    	return;
    assert(sw->scope_ == summarywin_t::SU_RANGE);
    sw->start_ = gtk_spin_button_get_value_as_int(
	    	    	    GTK_SPIN_BUTTON(sw->range_start_spin_));
    sw->update();   
}

GLADE_CALLBACK void
on_summary_range_end_spin_changed(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);

    if (sw->populating_)
    	return;
    assert(sw->scope_ == summarywin_t::SU_RANGE);
    sw->end_ = gtk_spin_button_get_value_as_int(
	    	    	    GTK_SPIN_BUTTON(sw->range_end_spin_));
    sw->update();   
}

GLADE_CALLBACK void
on_summary_range_view_clicked(GtkWidget *w, gpointer data)
{
    summarywin_t *sw = summarywin_t::from_widget(w);
    
    assert(sw->scope_ == summarywin_t::SU_RANGE);
    assert(sw->file_ != 0);
    assert(!sw->populating_);
    sourcewin_t::show_lines(sw->file_->name(), sw->start_, sw->end_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
