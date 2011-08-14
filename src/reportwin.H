/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2004 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_reportwin_H_
#define _ggcov_reportwin_H_ 1

#include "window.H"
#include "cov.H"
#include "report.H"

GLADE_CALLBACK void on_report_report_entry_changed(GtkWidget *w, gpointer data);

class reportwin_t : public window_t
{
public:
    reportwin_t();
    ~reportwin_t();

private:
    void populate_report_combo();
    void populate();
    void update();
    static reportwin_t *from_widget(GtkWidget *w)
    {
    	return (reportwin_t *)window_t::from_widget(w);
    }
    void grey_items();

    const report_t *report_;

    ui_combo_t *report_combo_;
    GtkWidget *text_;
    gboolean populating_;

    friend void on_report_report_entry_changed(GtkWidget *w, gpointer data);
};

#endif /* _ggcov_reportwin_H_ */
