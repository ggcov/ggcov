/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2006 Greg Banks <gnb@alphalink.com.au>
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

#ifndef _ggcov_sourcewin_H_
#define _ggcov_sourcewin_H_ 1

#include "window.H"
#include "cov.H"
#include "string_var.H"

GLADE_CALLBACK void on_source_saveas_ok_button_clicked(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_saveas_cancel_button_clicked(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_save_as_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_column_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_colors_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_toolbar_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_titles_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_summarise_file_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_summarise_function_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_summarise_range_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_flow_diagram_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_filenames_entry_changed(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_functions_entry_changed(GtkWidget *w, gpointer data);

class sourcewin_t : public window_t
{
public:
    sourcewin_t();
    ~sourcewin_t();

    void set_filename(const char *filename, const char *display_fname);
    void select_region(unsigned long start, unsigned long end);
    void ensure_visible(unsigned long line);

    /*
     * Routines to create and show a source window with a particular
     * interesting range of contents selected.
     */
    static void show_lines(const char *filename, unsigned long startline,
			      unsigned long endline);
    static void show_function(const cov_function_t *);
    static void show_file(const cov_file_t *);
    static void show_filename(const char *filename);


private:
    enum columns
    {
	COL_FLOW,
    	COL_LINE,
    	COL_BLOCK,
    	COL_COUNT,
    	COL_SOURCE,
    	NUM_COLS
    };
#if GTK2
    struct flow_t
    {
	GtkWidget *canvas_;
	int bufy_;		    /* buffer y coord of canvas */
	unsigned int width_;	    /* width in pixels of canvas */
	cov_function_t *function_;
	unsigned int shown_;	    /* generation number for showing flows */
    };
#endif

    static sourcewin_t *instance();
    void setup_text();
    void populate_filenames();
    void populate_functions();
    void populate();
    void update();
    void update_title_buttons();
#if GTK2
    void delete_flows();
    void update_flow_window();
    void update_flows();
    flow_t *create_flow(cov_function_t *, int y, int h);
#endif
    cov_function_t *selected_function() const;
    gboolean save_with_annotations(const char *filename);
    static int delayed_function_activate(gpointer userdata);
    static void on_source_filename_activate(GtkWidget *w, gpointer userdata);
    static void on_source_function_activate(GtkWidget *w, gpointer userdata);
#if GTK2
    static void on_vadjustment_value_changed(GtkAdjustment *, gpointer);
#endif

    static sourcewin_t *from_widget(GtkWidget *w)
    {
    	return (sourcewin_t *)window_t::from_widget(w);
    }

    static gboolean initialised_;
    int font_width_;
    static const int column_widths_[NUM_COLS];
    static const char *column_names_[NUM_COLS];
    static list_t<sourcewin_t> instances_;
    
    GtkWidget *text_;	/* GtkText in gtk1.2, GtkTextView in gtk2.0 */
    ui_text_tag *text_tags_[cov::NUM_STATUS];
    GtkWidget *column_checks_[NUM_COLS];
    GtkWidget *colors_check_;
    GtkWidget *flow_diagram_item_;
    GtkWidget *toolbar_;
    GtkWidget *filenames_combo_;
    GtkWidget *functions_combo_;
    gboolean populating_;
    string_var filename_;

    GtkWidget *titles_hbox_;
    GtkWidget *left_pad_label_;
    GtkWidget *title_buttons_[NUM_COLS];
    GtkWidget *right_pad_label_;

    GtkWidget *saveas_dialog_;

#if GTK2
    list_t<flow_t> flows_;
    unsigned int flow_shown_;
    gboolean text_initialised_;
    unsigned int flow_width_;
    gboolean flow_width_dirty_;
#endif

    friend void on_source_saveas_ok_button_clicked(GtkWidget *w, gpointer data);
    friend void on_source_saveas_cancel_button_clicked(GtkWidget *w, gpointer data);
    friend void on_source_save_as_activate(GtkWidget *w, gpointer data);
    friend void on_source_column_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_colors_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_toolbar_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_titles_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_summarise_file_activate(GtkWidget *w, gpointer data);
    friend void on_source_summarise_function_activate(GtkWidget *w, gpointer data);
    friend void on_source_summarise_range_activate(GtkWidget *w, gpointer data);
    friend void on_source_flow_diagram_activate(GtkWidget *w, gpointer data);
    friend void on_source_filenames_entry_changed(GtkWidget *w, gpointer data);
    friend void on_source_functions_entry_changed(GtkWidget *w, gpointer data);
};

#endif /* _ggcov_sourcewin_H_ */
