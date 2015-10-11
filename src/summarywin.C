/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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
#include "utils.H"
#include "filename.h"
#include "cov.H"
#include "estring.H"
#include "prefs.H"
#include "uix.h"
#include "gnbstackedbar.h"

CVSID("$Id: summarywin.C,v 1.22 2010-05-09 05:37:15 gnb Exp $");

list_t<summarywin_t> summarywin_t::instances_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

summarywin_t::summarywin_t()
{
    GladeXML *xml;
    GtkWidget *w;

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

    w = glade_xml_get_widget(xml, "summary_filename_combo");
    filename_combo_ = init(UI_COMBO(w), "/");
    filename_view_ = glade_xml_get_widget(xml, "summary_filename_view");
    w = glade_xml_get_widget(xml, "summary_function_combo");
    function_combo_ = init(UI_COMBO(w));
    function_view_ = glade_xml_get_widget(xml, "summary_function_view");
    w = glade_xml_get_widget(xml, "summary_range_combo");
    range_combo_ = init(UI_COMBO(w), "/");
    range_start_spin_ = glade_xml_get_widget(xml, "summary_range_start_spin");
    range_end_spin_ = glade_xml_get_widget(xml, "summary_range_end_spin");
    range_view_ = glade_xml_get_widget(xml, "summary_range_view");

    lines_label_ = glade_xml_get_widget(xml, "summary_lines_label");
    lines_pc_label_ = glade_xml_get_widget(xml, "summary_lines_pc_label");
    lines_bar_ = glade_xml_get_widget(xml, "summary_lines_bar");
    functions_label_ = glade_xml_get_widget(xml, "summary_functions_label");
    functions_pc_label_ = glade_xml_get_widget(xml, "summary_functions_pc_label");
    functions_bar_ = glade_xml_get_widget(xml, "summary_functions_bar");
    calls_label_ = glade_xml_get_widget(xml, "summary_calls_label");
    calls_pc_label_ = glade_xml_get_widget(xml, "summary_calls_pc_label");
    calls_bar_ = glade_xml_get_widget(xml, "summary_calls_bar");
    branches_label_ = glade_xml_get_widget(xml, "summary_branches_label");
    branches_pc_label_ = glade_xml_get_widget(xml, "summary_branches_pc_label");
    branches_bar_ = glade_xml_get_widget(xml, "summary_branches_bar");
    blocks_label_ = glade_xml_get_widget(xml, "summary_blocks_label");
    blocks_pc_label_ = glade_xml_get_widget(xml, "summary_blocks_pc_label");
    blocks_bar_ = glade_xml_get_widget(xml, "summary_blocks_bar");

    ui_register_windows_menu(ui_get_dummy_menu(xml, "summary_windows_dummy"));

    /* NUM_STATUS-2 to ignore SUPPRESSED,UNINSTRUMENTED */
    gnb_stacked_bar_set_num_metrics(GNB_STACKED_BAR(lines_bar_),
				    cov::NUM_STATUS-2);
    gnb_stacked_bar_set_metric_colors(GNB_STACKED_BAR(lines_bar_),
				      (const GdkColor **)foregrounds_by_status);
    gnb_stacked_bar_set_num_metrics(GNB_STACKED_BAR(calls_bar_),
				    cov::NUM_STATUS-2);
    gnb_stacked_bar_set_metric_colors(GNB_STACKED_BAR(calls_bar_),
				      (const GdkColor **)foregrounds_by_status);
    gnb_stacked_bar_set_num_metrics(GNB_STACKED_BAR(functions_bar_),
				    cov::NUM_STATUS-2);
    gnb_stacked_bar_set_metric_colors(GNB_STACKED_BAR(functions_bar_),
				      (const GdkColor **)foregrounds_by_status);
    gnb_stacked_bar_set_num_metrics(GNB_STACKED_BAR(branches_bar_),
				    cov::NUM_STATUS-2);
    gnb_stacked_bar_set_metric_colors(GNB_STACKED_BAR(branches_bar_),
				      (const GdkColor **)foregrounds_by_status);
    gnb_stacked_bar_set_num_metrics(GNB_STACKED_BAR(blocks_bar_),
				     cov::NUM_STATUS-2);
    gnb_stacked_bar_set_metric_colors(GNB_STACKED_BAR(blocks_bar_),
				      (const GdkColor **)foregrounds_by_status);

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

void
summarywin_t::populate_filename_combo(ui_combo_t *cbox)
{
    clear(cbox);
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	cov_file_t *f = *iter;

	if (file_ == 0)
	    file_ = f;

	add(cbox, f->minimal_name(), (gpointer)f);
    }
    done(cbox);
    set_active(cbox, (gpointer)file_);
}

void
summarywin_t::populate_function_combo(ui_combo_t *cbox)
{
    list_t<cov_function_t> *list;

    list = cov_function_t::list_all();

    ::populate_function_combo(cbox, list, /*add_all_item*/FALSE, &function_);

    list->remove_all();
    delete list;
}

void
summarywin_t::populate()
{
    dprintf0(D_SUMMARYWIN, "summarywin_t::populate\n");

    populating_ = TRUE;     /* suppress combo entry callbacks */
    populate_filename_combo(filename_combo_);
    populate_function_combo(function_combo_);
    populate_filename_combo(range_combo_);
    populating_ = FALSE;

    spin_update();
    update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
summarywin_t::spin_update()
{
    GtkAdjustment *adj;

    assert(file_ != 0);
    start_ = 1;
    end_ = file_->num_lines();

    dprintf3(D_SUMMARYWIN, "summarywin_t::spin_update: %s[%lu-%lu]\n",
		file_->minimal_name(), start_, end_);

    adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(range_start_spin_));
    gtk_adjustment_configure(adj,
			     /*value*/ start_,
			     /*lower*/ start_,
			     /*upper*/ end_,
			     /*step_increment*/ 1,
			     /*page_increment*/ 1,
			     /*page_size*/ 0);

    adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(range_end_spin_));
    gtk_adjustment_configure(adj,
			     /*value*/ end_,
			     /*lower*/ start_,
			     /*upper*/ end_,
			     /*step_increment*/ 1,
			     /*page_increment*/ 1,
			     /*page_size*/ 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
summarywin_t::grey_items()
{
    gtk_widget_set_sensitive(GTK_WIDGET(filename_combo_), (scope_ == SU_FILENAME));
    gtk_widget_set_sensitive(filename_view_, (scope_ == SU_FILENAME));
    gtk_widget_set_sensitive(GTK_WIDGET(function_combo_), (scope_ == SU_FUNCTION));
    gtk_widget_set_sensitive(function_view_, (scope_ == SU_FUNCTION));
    gtk_widget_set_sensitive(GTK_WIDGET(range_combo_), (scope_ == SU_RANGE));
    gtk_widget_set_sensitive(range_start_spin_, (scope_ == SU_RANGE));
    gtk_widget_set_sensitive(range_end_spin_, (scope_ == SU_RANGE));
    gtk_widget_set_sensitive(range_view_, (scope_ == SU_RANGE));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static inline unsigned long
total(const unsigned long *values)
{
    return values[cov::COVERED] +
	   values[cov::PARTCOVERED] +
	   values[cov::UNCOVERED];
}


static void
set_label_values(
    GtkWidget *label,
    GtkWidget *pclabel,
    const unsigned long *values)
{
    unsigned long numerator, denominator;
    char buf[128];
    char pcbuf[128];

    numerator = values[cov::COVERED] + values[cov::PARTCOVERED];
    denominator = total(values);

    if (denominator == 0)
    {
	snprintf(buf, sizeof(buf), "0/0");
	pcbuf[0] = '\0';
    }
    else if (values[cov::PARTCOVERED] == 0)
    {
	snprintf(buf, sizeof(buf), "%ld/%ld",
		numerator, denominator);
	snprintf(pcbuf, sizeof(pcbuf), "%.1f%%",
		(double)numerator * 100.0 / (double)denominator);
    }
    else
    {
	snprintf(buf, sizeof(buf), "%ld+%ld/%ld",
		values[cov::COVERED],
		values[cov::PARTCOVERED],
		denominator);
	snprintf(pcbuf, sizeof(pcbuf), "%.1f+%.1f%%",
		(double)values[cov::COVERED] * 100.0 / (double)denominator,
		(double)values[cov::PARTCOVERED] * 100.0 / (double)denominator);
    }
    gtk_label_set_text(GTK_LABEL(label), buf);
    gtk_label_set_text(GTK_LABEL(pclabel), pcbuf);
}

static void
set_bar_values(GtkWidget *sbar, const unsigned long *values)
{
    if (total(values) == 0)
    {
	gtk_widget_hide(sbar);
    }
    else
    {
	gtk_widget_show(sbar);
	gnb_stacked_bar_set_metric_values(GNB_STACKED_BAR(sbar), values);
    }
}


void
summarywin_t::update()
{
    cov_scope_t *sc = 0;
    const cov_stats_t *stats;

    dprintf0(D_SUMMARYWIN, "summarywin_t::update\n");

    grey_items();

    populating_ = TRUE;
    assert(file_ != 0);
    assert(function_ != 0);
    set_active(filename_combo_, (gpointer)file_);
    set_active(range_combo_, (gpointer)file_);
    set_active(function_combo_, (gpointer)function_);
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
	sc = new cov_overall_scope_t;
	break;

    case SU_FILENAME:
	assert(file_ != 0);
	sc = new cov_file_scope_t(file_);
	break;

    case SU_FUNCTION:
	assert(function_ != 0);
	sc = new cov_function_scope_t(function_);
	break;

    case SU_RANGE:
	assert(file_ != 0);
	dprintf3(D_SUMMARYWIN, "summarywin_update: SU_RANGE %s %lu-%lu\n",
			file_->minimal_name(), start_, end_);
	sc = new cov_range_scope_t(file_, start_, end_);
	break;

    case SU_NSCOPES:
	break;
    }

    assert(sc != 0);
    set_title(sc->describe());
    stats = sc->get_stats();
    if (stats == 0)
    {
	static const cov_stats_t empty;
	stats = &empty;
    }

    set_label_values(lines_label_, lines_pc_label_, stats->lines_by_status());
    set_bar_values(lines_bar_, stats->lines_by_status());
    set_label_values(functions_label_, functions_pc_label_, stats->functions_by_status());
    set_bar_values(functions_bar_, stats->functions_by_status());
    set_label_values(calls_label_, calls_pc_label_, stats->calls_by_status());
    set_bar_values(calls_bar_, stats->calls_by_status());
    set_label_values(branches_label_, branches_pc_label_, stats->branches_by_status());
    set_bar_values(branches_bar_, stats->branches_by_status());
    set_label_values(blocks_label_, blocks_pc_label_, stats->blocks_by_status());
    set_bar_values(blocks_bar_, stats->blocks_by_status());

    delete sc;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
summarywin_t::on_overall_radio_toggled()
{
    if (populating_ || !shown_)
	return;
    scope_ = summarywin_t::SU_OVERALL;
    update();
}

GLADE_CALLBACK void
summarywin_t::on_filename_radio_toggled()
{
    if (populating_ || !shown_)
	return;
    scope_ = summarywin_t::SU_FILENAME;
    update();
}

GLADE_CALLBACK void
summarywin_t::on_filename_combo_changed()
{
    if (populating_ || !shown_)
	return;
    assert(scope_ == summarywin_t::SU_FILENAME);
    cov_file_t *f = (cov_file_t *)get_active(filename_combo_);
    if (f != 0)
    {
	/* stupid gtk2 */
	file_ = f;
	update();
    }
}

GLADE_CALLBACK void
summarywin_t::on_filename_view_clicked()
{
    assert(scope_ == summarywin_t::SU_FILENAME);
    assert(file_ != 0);
    assert(!populating_);
    sourcewin_t::show_file(file_);
}

GLADE_CALLBACK void
summarywin_t::on_function_radio_toggled()
{
    if (populating_ || !shown_)
	return;
    scope_ = summarywin_t::SU_FUNCTION;
    update();
}

GLADE_CALLBACK void
summarywin_t::on_function_combo_changed()
{
    if (populating_ || !shown_)
	return;
    assert(scope_ == summarywin_t::SU_FUNCTION);
    cov_function_t *fn = (cov_function_t *)get_active(function_combo_);
    if (fn != 0)
    {
	/* stupid gtk2 */
	function_ = fn;
	update();
    }
}

GLADE_CALLBACK void
summarywin_t::on_function_view_clicked()
{
    assert(scope_ == summarywin_t::SU_FUNCTION);
    assert(function_ != 0);
    assert(!populating_);
    sourcewin_t::show_function(function_);
}

GLADE_CALLBACK void
summarywin_t::on_range_radio_toggled()
{
    if (populating_ || !shown_)
	return;
    scope_ = summarywin_t::SU_RANGE;
    update();
}

GLADE_CALLBACK void
summarywin_t::on_range_combo_changed()
{
    if (populating_ || !shown_)
	return;
    assert(scope_ == summarywin_t::SU_RANGE);
    cov_file_t *f = (cov_file_t *)get_active(range_combo_);
    if (f != 0)
    {
	/* stupid gtk2 */
	file_ = f;
	spin_update();
	update();
    }
}

GLADE_CALLBACK void
summarywin_t::on_range_start_spin_changed()
{
    if (populating_ || !shown_)
	return;
    assert(scope_ == summarywin_t::SU_RANGE);
    start_ = gtk_spin_button_get_value_as_int(
			    GTK_SPIN_BUTTON(range_start_spin_));
    update();
}

GLADE_CALLBACK void
summarywin_t::on_range_end_spin_changed()
{
    if (populating_ || !shown_)
	return;
    assert(scope_ == summarywin_t::SU_RANGE);
    end_ = gtk_spin_button_get_value_as_int(
			    GTK_SPIN_BUTTON(range_end_spin_));
    update();
}

GLADE_CALLBACK void
summarywin_t::on_range_view_clicked()
{
    assert(scope_ == summarywin_t::SU_RANGE);
    assert(file_ != 0);
    assert(!populating_);
    sourcewin_t::show_lines(file_->name(), start_, end_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
