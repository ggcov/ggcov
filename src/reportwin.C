/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2004 Greg Banks <gnb@alphalink.com.au>
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

#include "reportwin.H"
#include "sourcewin.H"
#include "filename.h"
#include "cov.H"
#include "estring.H"
#include "prefs.H"
#include "uix.h"
#include "gnbstackedbar.h"

CVSID("$Id: reportwin.C,v 1.3 2005-03-14 07:49:16 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
null_report_func(FILE *)
{
    return 0;
}

static const report_t null_report =  { "none", N_("None"), null_report_func };

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

reportwin_t::reportwin_t()
{
    GladeXML *xml;
    
    /* load the interface & connect signals */
    xml = ui_load_tree("report");
    
    set_window(glade_xml_get_widget(xml, "report"));
    
    report_combo_ = glade_xml_get_widget(xml, "report_report_combo");
    text_ = glade_xml_get_widget(xml, "report_text");
    ui_text_setup(text_);

    ui_register_windows_menu(ui_get_dummy_menu(xml, "report_windows_dummy"));

    report_ = &null_report;
}


reportwin_t::~reportwin_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
reportwin_t::populate_report_combo()
{
    const report_t *rep;
    GtkCombo *combo = GTK_COMBO(report_combo_);
    
    ui_combo_clear(combo);    /* stupid glade2 */
    ui_combo_add_data(combo, _(null_report.label), (gpointer)&null_report);
    for (rep = all_reports ; rep->name != 0 ; rep++)
    	ui_combo_add_data(combo, _(rep->label), (gpointer)rep);
    ui_combo_set_current_data(combo, (gpointer)report_);
}

void
reportwin_t::populate()
{
    dprintf0(D_REPORTWIN, "reportwin_t::populate\n");

    populating_ = TRUE;     /* suppress combo entry callbacks */
    populate_report_combo();
    populating_ = FALSE;

    update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
reportwin_t::grey_items()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static FILE *
open_temp_file()
{
    char *fname;
    int fd;
    FILE *fp = 0;
    
    fname = g_strdup("/tmp/gcov-reportXXXXXX");
    if ((fd = mkstemp(fname)) < 0)
    {
    	perror(fname);
    }
    else if ((fp = fdopen(fd, "w+")) == 0)
    {
    	perror(fname);
	close(fd);
    }
    g_free(fname);
    return fp;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
reportwin_t::update()
{
    FILE *fp;
    int n;
    char buf[1024];

    dprintf0(D_REPORTWIN, "reportwin_t::update\n");
    
    grey_items();

    populating_ = TRUE;
    assert(report_ != 0);
    ui_combo_set_current_data(GTK_COMBO(report_combo_), (gpointer)report_);
    populating_ = FALSE;

    set_title(_(report_->label));

    if ((fp = open_temp_file()) == 0)
    	return;
	
    report_->func(fp);
    fflush(fp);
    fseek(fp, 0L, SEEK_SET);

    ui_text_begin(text_);
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
	ui_text_add(text_, 0, buf, n);
    ui_text_end(text_);
    
    fclose(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_report_report_entry_changed(GtkWidget *w, gpointer data)
{
    reportwin_t *rw = reportwin_t::from_widget(w);
    const report_t *rep;

    if (rw->populating_ || !rw->shown_)
    	return;
    rep = (const report_t *)ui_combo_get_current_data(
	    	    	    	    GTK_COMBO(rw->report_combo_));
    if (rep != 0)
    {
    	/* stupid gtk2 */
    	rw->report_ = rep;
	rw->update();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
