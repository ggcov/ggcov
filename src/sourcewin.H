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

#ifndef _ggcov_sourcewin_H_
#define _ggcov_sourcewin_H_ 1

#include "window.H"
#include "cov.H"

GLADE_CALLBACK void on_source_close_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_exit_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_count_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_number_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_block_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_source_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_colors_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_toolbar_check_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_summarise_file_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_summarise_function_activate(GtkWidget *w, gpointer data);
GLADE_CALLBACK void on_source_summarise_range_activate(GtkWidget *w, gpointer data);
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
    static void init(GtkWidget *);
#if GTK2
    void initialise_tags();
#endif
    static sourcewin_t *instance();
    void populate_filenames();
    void populate_functions();
    void populate();
    void update();
    void get_selected_lines(unsigned long *startp, unsigned long *endp);
#if !GTK2
    unsigned long offset_to_lineno(unsigned int offset);
#endif
    static int delayed_function_activate(gpointer userdata);
    static void on_source_filename_activate(GtkWidget *w, gpointer userdata);
    static void on_source_function_activate(GtkWidget *w, gpointer userdata);

    static sourcewin_t *from_widget(GtkWidget *w)
    {
    	return (sourcewin_t *)window_t::from_widget(w);
    }

    static gboolean initialised_;
#if GTK2
    static PangoFontDescription *font_;
#else
    static GdkFont *font_;
    static int font_width_, font_height_;
#endif
    static list_t<sourcewin_t> instances_;
    
#if GTK2
    GtkWidget *text_view_;
    GtkTextTag *text_tags_[4];
#else
    GtkWidget *text_;
#endif
    GtkWidget *number_check_;
    GtkWidget *block_check_;
    GtkWidget *count_check_;
    GtkWidget *source_check_;
    GtkWidget *colors_check_;
    GtkWidget *toolbar_;
    GtkWidget *filenames_combo_;
    GtkWidget *functions_combo_;
    gboolean populating_;
    char *filename_;
    unsigned int max_lineno_;	/* the largest linenumber */
#if !GTK2
    GArray *offsets_by_line_;
#endif

    friend void on_source_close_activate(GtkWidget *w, gpointer data);
    friend void on_source_exit_activate(GtkWidget *w, gpointer data);
    friend void on_source_count_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_number_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_block_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_source_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_colors_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_toolbar_check_activate(GtkWidget *w, gpointer data);
    friend void on_source_summarise_file_activate(GtkWidget *w, gpointer data);
    friend void on_source_summarise_function_activate(GtkWidget *w, gpointer data);
    friend void on_source_summarise_range_activate(GtkWidget *w, gpointer data);
    friend void on_source_filenames_entry_changed(GtkWidget *w, gpointer data);
    friend void on_source_functions_entry_changed(GtkWidget *w, gpointer data);
};

#endif /* _ggcov_sourcewin_H_ */
