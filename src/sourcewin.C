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

#include "sourcewin.H"
#include "summarywin.H"
#include "cov.H"
#include "estring.H"
#include "prefs.H"

CVSID("$Id: sourcewin.C,v 1.22 2003-07-19 06:26:28 gnb Exp $");

#ifndef GTK_SCROLLED_WINDOW_GET_CLASS
#define GTK_SCROLLED_WINDOW_GET_CLASS(obj) \
	GTK_SCROLLED_WINDOW_CLASS(GTK_OBJECT_CLASS(GTK_OBJECT(obj)->klass))
#endif


list_t<sourcewin_t> sourcewin_t::instances_;

/* column widths, in *characters* */
const int sourcewin_t::column_widths_[sourcewin_t::NUM_COLS] = { 8, 16, 8, -1 };
const char *sourcewin_t::column_names_[sourcewin_t::NUM_COLS] = {
    "Line", "Blocks", "Count", "Source"
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::setup_text()
{
    ui_text_setup(text_);
    font_width_ = ui_text_font_width(text_);

    text_tags_[cov_line_t::COVERED] =
	ui_text_create_tag(text_, "covered", &prefs.covered_foreground);
    
    text_tags_[cov_line_t::PARTCOVERED] =
	ui_text_create_tag(text_, "partcovered", &prefs.partcovered_foreground);
    
    text_tags_[cov_line_t::UNCOVERED] =
	ui_text_create_tag(text_, "uncovered", &prefs.uncovered_foreground);
    
    text_tags_[cov_line_t::UNINSTRUMENTED] =
	ui_text_create_tag(text_, "uninstrumented", &prefs.uninstrumented_foreground);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


sourcewin_t::sourcewin_t()
{
    GladeXML *xml;
    
    /* load the interface & connect signals */
    xml = ui_load_tree("source");
    
    set_window(glade_xml_get_widget(xml, "source"));

    text_ = glade_xml_get_widget(xml, "source_text");
    setup_text();
    column_checks_[COL_LINE] = glade_xml_get_widget(xml, "source_line_check");
    column_checks_[COL_BLOCK] = glade_xml_get_widget(xml, "source_block_check");
    column_checks_[COL_COUNT] = glade_xml_get_widget(xml, "source_count_check");
    column_checks_[COL_SOURCE] = glade_xml_get_widget(xml, "source_source_check");
    colors_check_ = glade_xml_get_widget(xml, "source_colors_check");
    toolbar_ = glade_xml_get_widget(xml, "source_toolbar");
    filenames_combo_ = glade_xml_get_widget(xml, "source_filenames_combo");
    functions_combo_ = glade_xml_get_widget(xml, "source_functions_combo");
    titles_hbox_ = glade_xml_get_widget(xml, "source_titles_hbox");
    left_pad_label_ = glade_xml_get_widget(xml, "source_left_pad_label");
    title_buttons_[COL_LINE] = glade_xml_get_widget(xml, "source_col_line_button");
    title_buttons_[COL_BLOCK] = glade_xml_get_widget(xml, "source_col_block_button");
    title_buttons_[COL_COUNT] = glade_xml_get_widget(xml, "source_col_count_button");
    title_buttons_[COL_SOURCE] = glade_xml_get_widget(xml, "source_col_source_button");
    right_pad_label_ = glade_xml_get_widget(xml, "source_right_pad_label");
    ui_register_windows_menu(ui_get_dummy_menu(xml, "source_windows_dummy"));
    
    xml = ui_load_tree("source_saveas");
    saveas_dialog_ = glade_xml_get_widget(xml, "source_saveas");
    attach(saveas_dialog_);

    instances_.append(this);
}

sourcewin_t::~sourcewin_t()
{
    instances_.remove(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_source_filenames_entry_changed(GtkWidget *w, gpointer userdata)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    cov_file_t *f = (cov_file_t *)ui_combo_get_current_data(
    	    	    	    	    GTK_COMBO(sw->filenames_combo_));
    
    if (sw->populating_ || !sw->shown_ || f == 0/*stupid gtk2*/)
    	return;
    sw->set_filename(f->name(), f->minimal_name());
}


void
sourcewin_t::populate_filenames()
{
    list_iterator_t<cov_file_t> iter;
   
    populating_ = TRUE; /* suppress combo entry callback */
    ui_combo_clear(GTK_COMBO(filenames_combo_));    /* stupid glade2 */
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    {
    	cov_file_t *f = *iter;

    	ui_combo_add_data(GTK_COMBO(filenames_combo_), f->minimal_name(), f);
    }
    populating_ = FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_source_functions_entry_changed(GtkWidget *w, gpointer userdata)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    cov_function_t *fn = (cov_function_t *)ui_combo_get_current_data(
    	    	    	    	    	    GTK_COMBO(sw->functions_combo_));
    const cov_location_t *first;
    const cov_location_t *last;
    
    if (sw->populating_ || !sw->shown_ || fn == 0 /*stupid gtk2*/)
    	return;
    
    first = fn->get_first_location();
    last = fn->get_last_location();
#if DEBUG
    fprintf(stderr, "Function %s -> %s:%ld to %s:%ld\n",
    	    	    	fn->name(),
			first->filename, first->lineno,
			last->filename, last->lineno);
#endif
    
    /* Check for weirdness like functions spanning files */
    if (strcmp(first->filename, sw->filename_))
    {
    	fprintf(stderr, "WTF?  Wrong filename for first loc: %s vs %s\n",
	    	    	first->filename, sw->filename_.data());
	return;
    }
    if (strcmp(last->filename, sw->filename_))
    {
    	fprintf(stderr, "WTF?  Wrong filename for last loc: %s vs %s\n",
	    	    	last->filename, sw->filename_.data());
	return;
    }

    sw->ensure_visible(first->lineno);

    /* This only selects the span of the lines which contain executable code */		    
    sw->select_region(first->lineno, last->lineno);
}

void
sourcewin_t::populate_functions()
{
    GList *functions = 0;
    unsigned fnidx;
    cov_file_t *f;
    cov_function_t *fn;
    
    /* build an alphabetically sorted list of functions in the file */
    f = cov_file_t::find(filename_);
    assert(f != 0);
    for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
    {
    	fn = f->nth_function(fnidx);
	
	if (fn->is_suppressed() ||
	    fn->get_first_location() == 0)
	    continue;
	functions = g_list_prepend(functions, fn);
    }
    functions = g_list_sort(functions, cov_function_t::compare);
    
    /* now build the menu */

    ui_combo_clear(GTK_COMBO(functions_combo_));
    populating_ = TRUE; /* suppress combo entry callback */
    
    while (functions != 0)
    {
    	fn = (cov_function_t *)functions->data;
	
	ui_combo_add_data(GTK_COMBO(functions_combo_), fn->name(), fn);
	
	functions = g_list_remove_link(functions, functions);
    }
    populating_ = FALSE;
}

void
sourcewin_t::populate()
{
    populate_filenames();
    populate_functions();
    update();
    update_title_buttons();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Expand tabs in the line because GTK 1.2.7 through 1.2.9 can
 * only be trusted to expand *initial* tabs correctly, DAMMIT.
 */
static char *
fgets_tabexpand(char *buf, unsigned int maxlen, FILE *fp)
{
    int c;
    unsigned int len = 0, ns;
    static const char spaces[] = "        ";
    
    maxlen--;	/* allow for nul terminator */
    
    for (;;)
    {
    	if ((c = getc(fp)) == EOF)
	    return (len == 0 ? 0 : buf);
	    
	if (c == '\t')
	{
	    ns = 8-(len&7);
	    if (len+ns >= maxlen)
	    {
	    	ungetc(c, fp);
		return buf;
	    }
	    memcpy(buf+len, spaces, ns);
	    len += ns;
	    buf[len] = '\0';
	}
	else
	{
	    if (len+1 == maxlen)
	    {
	    	ungetc(c, fp);
		return buf;
	    }
	    buf[len++] = c;
	    buf[len] = '\0';
	    if (c == '\n')
	    	break;
	}
    }
    
    return buf;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *
pad(char *buf, unsigned int width, char c)
{
    char *p = buf;

    for ( ; *p && width > 0 ; p++, width--)
    	;
    for ( ; width > 0 ; width--)
    	*p++ = c;
    *p = '\0';
    
    return buf;
}

void
sourcewin_t::update()
{
    FILE *fp;
    unsigned long lineno;
    cov_file_t *f;
    cov_line_t *ln;
    int i, nstrs;
    gfloat scrollval;
    ui_text_tag *tag;
    const char *strs[NUM_COLS];
    char linenobuf[32];
    char blockbuf[32];
    char countbuf[32];
    char linebuf[1024];
    
    update_title_buttons();

    if ((f = cov_file_t::find(filename_)) == 0)
    	return;

    if ((fp = fopen(filename_, "r")) == 0)
    {
    	/* TODO: gui error report */
    	perror(filename_);
	return;
    }

    scrollval = ui_text_vscroll_sample(text_);
    ui_text_begin(text_);

    lineno = 0;
    while (fgets_tabexpand(linebuf, sizeof(linebuf), fp) != 0)
    {
    	++lineno;
    	ln = f->nth_line(lineno);
	
    	/* choose colours */
	tag = 0;
	if (GTK_CHECK_MENU_ITEM(colors_check_)->active)
	    tag = text_tags_[ln->status()];

    	/* generate strings */
	
	nstrs = 0;
	
	if (GTK_CHECK_MENU_ITEM(column_checks_[COL_LINE])->active)
	{
	    snprintf(linenobuf, sizeof(linenobuf), "%*lu ",
	    	      column_widths_[COL_LINE]-1, lineno);
	    strs[nstrs++] = linenobuf;
	}
	
	if (GTK_CHECK_MENU_ITEM(column_checks_[COL_BLOCK])->active)
	{
	    ln->format_blocks(blockbuf, column_widths_[COL_BLOCK]-1);
	    strs[nstrs++] = pad(blockbuf, column_widths_[COL_BLOCK], ' ');
	}
	
	if (GTK_CHECK_MENU_ITEM(column_checks_[COL_COUNT])->active)
	{
	    switch (ln->status())
	    {
	    case cov_line_t::COVERED:
	    case cov_line_t::PARTCOVERED:
		snprintf(countbuf, sizeof(countbuf), "%*llu",
		    	 column_widths_[COL_COUNT]-1, ln->count());
		break;
	    case cov_line_t::UNCOVERED:
		strncpy(countbuf, " ######", sizeof(countbuf));
		break;
	    case cov_line_t::UNINSTRUMENTED:
	    	countbuf[0] = '\0';
		break;
	    }
	    strs[nstrs++] = pad(countbuf, column_widths_[COL_COUNT], ' ');
    	}


	if (GTK_CHECK_MENU_ITEM(column_checks_[COL_SOURCE])->active)
	    strs[nstrs++] = linebuf;
	else
	    strs[nstrs++] = "\n";

    	for (i = 0 ; i < nstrs ; i++)
    	    ui_text_add(text_, tag, strs[i], -1);
    }
    
    fclose(fp);

    ui_text_end(text_);
    /* scroll back to the line we were at before futzing with the text */
    ui_text_vscroll_restore(text_, scrollval);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


void
sourcewin_t::update_title_buttons()
{
    GtkWidget *scrollw;
    int lpad, rpad, sbwidth;
    int i;
    
    /*
     * Need the window to be realized so that the scrollbar has
     * its final width for our calculations.
     */
    if (!GTK_WIDGET_REALIZED(window_))
    	gtk_widget_realize(window_);
    
    /*
     * Set the left and right padding labels to just the right values
     * to align the title buttons with the main text window body.
     */
    scrollw = text_->parent;
    lpad = rpad = GTK_CONTAINER(scrollw)->border_width;
    sbwidth = GTK_SCROLLED_WINDOW(scrollw)->vscrollbar->allocation.width + 
    	      GTK_SCROLLED_WINDOW_GET_CLASS(scrollw)->scrollbar_spacing;

    switch (GTK_SCROLLED_WINDOW(scrollw)->window_placement)
    {
    case GTK_CORNER_TOP_LEFT:
    case GTK_CORNER_BOTTOM_LEFT:
    	rpad += sbwidth;
	break;
    case GTK_CORNER_TOP_RIGHT:
    case GTK_CORNER_BOTTOM_RIGHT:
    	lpad += sbwidth;
	break;
    }
    
    gtk_widget_set_usize(left_pad_label_, lpad, /*height=whatever*/5);
    gtk_widget_set_usize(right_pad_label_, rpad, /*height=whatever*/5);

    /*
     * Size each title button to match the size of each column in
     * the text window.  Note the Source column takes up all the slack.
     */
    for (i = 0 ; i < NUM_COLS ; i++)
    {
    	if (column_widths_[i] > 0)
	    gtk_widget_set_usize(title_buttons_[i],
	    	    	         column_widths_[i] * font_width_,
				 /*height=whatever*/5);
    }

    /*
     * Show or hide the title buttons.
     */
    for (i = 0 ; i < NUM_COLS ; i++)
    {
	if (GTK_CHECK_MENU_ITEM(column_checks_[i])->active)
	    gtk_widget_show(title_buttons_[i]);
	else
	    gtk_widget_hide(title_buttons_[i]);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::set_filename(const char *filename, const char *display_fname)
{
    set_title((display_fname == 0 ? filename : display_fname));
    filename_ = filename;

    if (shown_)
    {
	populate_functions();
    	update();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::select_region(unsigned long startline, unsigned long endline)
{
#if DEBUG
    fprintf(stderr, "sourcewin_t::select_region: startline=%ld endline=%ld\n",
    	    	startline, endline);
#endif
    ui_text_select_lines(text_, startline, endline);
}

void
sourcewin_t::ensure_visible(unsigned long line)
{
    ui_text_ensure_visible(text_, line);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

sourcewin_t *
sourcewin_t::instance()
{
    sourcewin_t *sw = 0;
    
    if (prefs.reuse_srcwin)
    	sw = instances_.head();
    if (sw == 0)
    	sw = new sourcewin_t;

    return sw;
}


void
sourcewin_t::show_lines(
    const char *filename,
    unsigned long startline,
    unsigned long endline)
{
    sourcewin_t *sw = instance();
    estring fullname = cov_file_t::unminimise_name(filename);
    estring displayname = cov_file_t::minimise_name(fullname.data());

#if DEBUG
    fprintf(stderr, "sourcewin_t::show_lines(\"%s\", %lu, %lu) => \"%s\"\n",
    	    	filename, startline, endline, fullname.data());
#endif
    
    sw->set_filename(fullname.data(), displayname.data());
    sw->show();
    if (startline > 0)
    {
	sw->ensure_visible(startline);
	sw->select_region(startline, endline);
    }
}

void
sourcewin_t::show_function(const cov_function_t *fn)
{
    const cov_location_t *start, *end;

    start = fn->get_first_location();
    end = fn->get_last_location();

    if (start != 0 && end != 0)
	show_lines(start->filename, start->lineno, end->lineno);
}

void
sourcewin_t::show_file(const cov_file_t *f)
{
    show_lines(f->name(), 0, 0);
}

void
sourcewin_t::show_filename(const char *filename)
{
    show_lines(filename, 0, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


GLADE_CALLBACK void
on_source_column_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    
    sw->update();
}

GLADE_CALLBACK void
on_source_colors_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    
    sw->update();
}

GLADE_CALLBACK void
on_source_toolbar_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    
    if (GTK_CHECK_MENU_ITEM(w)->active)
    	gtk_widget_show(sw->toolbar_);
    else
    	gtk_widget_hide(sw->toolbar_);
}

GLADE_CALLBACK void
on_source_titles_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    
    if (GTK_CHECK_MENU_ITEM(w)->active)
    {
    	sw->update_title_buttons();
    	gtk_widget_show(sw->titles_hbox_);
    }
    else
    	gtk_widget_hide(sw->titles_hbox_);
}

GLADE_CALLBACK void
on_source_summarise_file_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    
    summarywin_t::show_file(cov_file_t::find(sw->filename_));
}

GLADE_CALLBACK void
on_source_summarise_function_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    cov_location_t loc;
    cov_line_t *ln;
    
    loc.filename = (char *)sw->filename_.data();
    ui_text_get_selected_lines(sw->text_, &loc.lineno, 0);

    if (loc.lineno == 0 ||
    	(ln = cov_line_t::find(&loc)) == 0 ||
	ln->blocks() == 0)
    	return;
    summarywin_t::show_function(((cov_block_t *)ln->blocks()->data)->function());
}

GLADE_CALLBACK void
on_source_summarise_range_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    unsigned long start, end;
    
    ui_text_get_selected_lines(sw->text_, &start, &end);
    if (start != 0)
	summarywin_t::show_lines(sw->filename_, start, end);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
sourcewin_t::save_with_annotations(const char *filename)
{
    int i;
    FILE *fp;
    char *contents;
    guint length;

    if ((fp = fopen(filename, "w")) == 0)
    {
    	perror(filename);
	return FALSE;
    }
    
    /* Generate header line */
    for (i = 0 ; i < NUM_COLS ; i++)
    {
	if (GTK_CHECK_MENU_ITEM(column_checks_[i])->active)
	{
	    if (column_widths_[i] > 0)
	    	fprintf(fp, "%-*s ", column_widths_[i]-1, column_names_[i]);
	    else
	    	fprintf(fp, "%s", column_names_[i]);
	}
    }
    fputc('\n', fp);

    /* Generate separator line */
    for (i = 0 ; i < NUM_COLS ; i++)
    {
	if (GTK_CHECK_MENU_ITEM(column_checks_[i])->active)
	{
	    int j, n = column_widths_[i];
	    if (n < 0)
	    	n = 20;
	    else
	    	n--;
	    for (j = 0 ; j < n ; j++)
	    	fputc('-', fp);
	    fputc(' ', fp);
	}
    }
    fputc('\n', fp);


    /*
     * Get the contents of the text window, including all visible columns
     */
    contents = ui_text_get_contents(text_);
    length = strlen(contents);

    /*
     * Write out the contents.
     */
    if (fwrite(contents, 1, length, fp) < length)
    {
    	perror("fwrite");
	g_free(contents);
	fclose(fp);
	return FALSE;
    }

    g_free(contents);
    fclose(fp);
    return TRUE;
}

GLADE_CALLBACK void
on_source_saveas_ok_button_clicked(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    const char *filename;

    filename = gtk_file_selection_get_filename(
    	    	    GTK_FILE_SELECTION(sw->saveas_dialog_));

    sw->save_with_annotations(filename);
    gtk_widget_hide(sw->saveas_dialog_);
}

GLADE_CALLBACK void
on_source_saveas_cancel_button_clicked(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);

    gtk_widget_hide(sw->saveas_dialog_);
}

GLADE_CALLBACK void
on_source_save_as_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    char *txt_filename;
    
    txt_filename = g_strconcat(sw->filename_.data(), ".cov.txt", (char *)0);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(sw->saveas_dialog_),
    	    	    	    	    txt_filename);
    g_free(txt_filename);
    
    gtk_widget_show(sw->saveas_dialog_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
