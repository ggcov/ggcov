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
#include "uix.h"

CVSID("$Id: sourcewin.C,v 1.14 2003-06-08 06:34:07 gnb Exp $");

gboolean sourcewin_t::initialised_ = FALSE;
#if GTK2
PangoFontDescription *sourcewin_t::font_;
#else
GdkFont *sourcewin_t::font_;
int sourcewin_t::font_width_, sourcewin_t::font_height_;
#endif
list_t<sourcewin_t> sourcewin_t::instances_;

/* column widths, in *characters* */
const int sourcewin_t::column_widths_[sourcewin_t::NUM_COLS] = { 7, 16, 7, -1 };
const char *sourcewin_t::column_names_[sourcewin_t::NUM_COLS] = {
    "Line", "Blocks", "Count", "Source"
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::init(GtkWidget *w)
{
    if (initialised_)
    	return;
    initialised_ = TRUE;

#if GTK2
    font_ = pango_font_description_from_string("monospace");
#else
    font_ = uix_fixed_width_font(gtk_widget_get_style(w)->font);
    font_height_ = uix_font_height(font_);
    font_width_ = uix_font_width(font_);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if GTK2
void
sourcewin_t::initialise_tags()
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_));
    
    text_tags_[CS_COVERED] = gtk_text_buffer_create_tag(buffer, "covered",
    	"foreground-gdk",   	&prefs.covered_foreground,
	(char *)0);
    
    text_tags_[CS_PARTCOVERED] = gtk_text_buffer_create_tag(buffer, "partcovered",
    	"foreground-gdk",   	&prefs.partcovered_foreground,
	(char *)0);
    
    text_tags_[CS_UNCOVERED] = gtk_text_buffer_create_tag(buffer, "uncovered",
    	"foreground-gdk",   	&prefs.uncovered_foreground,
	(char *)0);
    
    text_tags_[CS_UNINSTRUMENTED] = gtk_text_buffer_create_tag(buffer, "uninstrumented",
    	"foreground-gdk",   	&prefs.uninstrumented_foreground,
	(char *)0);
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


#define SOURCE_COLUMNS	    (8+8+80) 	/* number,count,source */
#define SOURCE_ROWS	    (35)
#define MAGIC_MARGINX	    14
#define MAGIC_MARGINY	    5

extern int screenshot_mode;

sourcewin_t::sourcewin_t()
{
    GladeXML *xml;
    
#if !GTK2
    offsets_by_line_ = g_array_new(/*zero_terminated*/TRUE,
				      /*clear*/TRUE,
				      sizeof(unsigned int));
#endif
    
    /* load the interface & connect signals */
    xml = ui_load_tree("source");
    
    set_window(glade_xml_get_widget(xml, "source"));

#if GTK2
    text_view_ = glade_xml_get_widget(xml, "source_text");
    initialise_tags();
#else
    text_ = glade_xml_get_widget(xml, "source_text");
#endif
    column_checks_[COL_LINE] = glade_xml_get_widget(xml, "source_line_check");
    column_checks_[COL_BLOCK] = glade_xml_get_widget(xml, "source_block_check");
    column_checks_[COL_COUNT] = glade_xml_get_widget(xml, "source_count_check");
    column_checks_[COL_SOURCE] = glade_xml_get_widget(xml, "source_source_check");
    colors_check_ = glade_xml_get_widget(xml, "source_colors_check");
    toolbar_ = glade_xml_get_widget(xml, "source_toolbar");
    filenames_combo_ = glade_xml_get_widget(xml, "source_filenames_combo");
    functions_combo_ = glade_xml_get_widget(xml, "source_functions_combo");
#if !GTK2
    titles_hbox_ = glade_xml_get_widget(xml, "source_titles_hbox");
    left_pad_label_ = glade_xml_get_widget(xml, "source_left_pad_label");
    title_buttons_[COL_LINE] = glade_xml_get_widget(xml, "source_col_line_button");
    title_buttons_[COL_BLOCK] = glade_xml_get_widget(xml, "source_col_block_button");
    title_buttons_[COL_COUNT] = glade_xml_get_widget(xml, "source_col_count_button");
    title_buttons_[COL_SOURCE] = glade_xml_get_widget(xml, "source_col_source_button");
    right_pad_label_ = glade_xml_get_widget(xml, "source_right_pad_label");
#endif
    ui_register_windows_menu(ui_get_dummy_menu(xml, "source_windows_dummy"));
        
    init(window_);
    
#if GTK2
    gtk_widget_modify_font(text_view_, font_);
#else
    if (!screenshot_mode)
	gtk_widget_set_usize(text_,
    	    SOURCE_COLUMNS * font_width_ + MAGIC_MARGINX,
    	    SOURCE_ROWS * font_height_ + MAGIC_MARGINY);
#endif

    xml = ui_load_tree("source_saveas");
    saveas_dialog_ = glade_xml_get_widget(xml, "source_saveas");
    attach(saveas_dialog_);

    instances_.append(this);
}

sourcewin_t::~sourcewin_t()
{
#if !GTK2
    g_array_free(offsets_by_line_, /*free_segment*/TRUE);
#endif
    instances_.remove(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_source_filenames_entry_changed(GtkWidget *w, gpointer userdata)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    cov_file_t *f = (cov_file_t *)ui_combo_get_current_data(
    	    	    	    	    GTK_COMBO(sw->filenames_combo_));
    
    if (sw->populating_)
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
    
    if (sw->populating_)
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
	
	if (fn->is_suppressed())
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

static void
format_blocks(char *buf, unsigned int width, const cov_location_t *loc)
{
    const GList *blocks;
    unsigned int len;
    unsigned int start = 0, end = 0;
    
    for (blocks = cov_block_t::find_by_location(loc) ; 
    	 blocks != 0 && width > 0 ;
	 blocks = blocks->next)
    {
    	cov_block_t *b = (cov_block_t *)blocks->data;
	
	assert(b->bindex() != 0);
	if (start > 0 && b->bindex() == end+1)
	{
	    end++;
	    continue;
	}
	if (start == 0)
	{
	    start = end = b->bindex();
	    continue;
	}
	
    	snprintf(buf, width, "%u-%u,", start, end);
	len = strlen(buf);
	buf += len;
	width -= len;
	start = end = b->bindex();
    }
    
    if (width > 0 && start > 0)
    {
	if (start == end)
    	    snprintf(buf, width, "%u", start);
	else
    	    snprintf(buf, width, "%u-%u", start, end);
	len = strlen(buf);
	buf += len;
	width -= len;
    }
    
    for ( ; width > 0 ; width--)
    	*buf++ = ' ';
    *buf = '\0';
}

void
sourcewin_t::update()
{
    FILE *fp;
    cov_location_t loc;
    count_t count;
    cov_status_t status;
    int i, nstrs;
#if GTK2
    GtkTextView *text_view = GTK_TEXT_VIEW(text_view_);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextTag *tag;
#else
    GtkText *text = GTK_TEXT(text_);
    GdkColor *color;
    static GdkColor *scolors[CS_NUM_STATUS] =
    {
    	&prefs.covered_foreground, &prefs.partcovered_foreground,
	&prefs.uncovered_foreground, &prefs.uninstrumented_foreground
    };
#endif
    const char *strs[NUM_COLS];
    char linenobuf[32];
    char blockbuf[32];
    char countbuf[32];
    char linebuf[1024];
    
    update_title_buttons();

    if ((fp = fopen(filename_, "r")) == 0)
    {
    	/* TODO: gui error report */
    	perror(filename_);
	return;
    }

#if GTK2
    gtk_text_buffer_set_text(buffer, "", -1);
#else
    gtk_text_freeze(text);
    gtk_editable_delete_text(GTK_EDITABLE(text), 0, -1);
    
    g_array_set_size(offsets_by_line_, 0);
#endif

    loc.filename = (char *)filename_.data();
    loc.lineno = 0;
    while (fgets_tabexpand(linebuf, sizeof(linebuf), fp) != 0)
    {
    	++loc.lineno;
	
#if !GTK2
	/*
	 * Stash the offset of this (file) line number.
	 *
	 * Felching text widget does this internally, wish we could
	 * get a hold of that...TODO: not if we start inserting
	 * phantom lines, then the correspondence between file lineno
	 * and text widget lineno goes away.
	 */
	{
	    unsigned int offset = gtk_text_get_length(text);
	    g_array_append_val(offsets_by_line_, offset);
#if DEBUG > 10
	    fprintf(stderr, "line=%d offset=%d\n", lineno, offset);
#endif
	}
#endif
	
	status = cov_get_count_by_location(&loc, &count);

#if GTK2
    	/* choose colours */
	tag = 0;
	if (GTK_CHECK_MENU_ITEM(colors_check_)->active)
	    tag = text_tags_[status];
#else
	color = 0;
	if (GTK_CHECK_MENU_ITEM(colors_check_)->active)
	    color = scolors[status];
#endif

    	/* generate strings */
	
	nstrs = 0;
	
	if (GTK_CHECK_MENU_ITEM(column_checks_[COL_LINE])->active)
	{
	    snprintf(linenobuf, sizeof(linenobuf), "%*lu ",
	    	      column_widths_[COL_LINE], loc.lineno);
	    strs[nstrs++] = linenobuf;
	}
	
	if (GTK_CHECK_MENU_ITEM(column_checks_[COL_BLOCK])->active)
	{
	    format_blocks(blockbuf, column_widths_[COL_BLOCK], &loc);
	    strs[nstrs++] = blockbuf;
	}
	
	if (GTK_CHECK_MENU_ITEM(column_checks_[COL_COUNT])->active)
	{
	    switch (status)
	    {
	    case CS_COVERED:
	    case CS_PARTCOVERED:
		snprintf(countbuf, sizeof(countbuf), "%*lu ",
		    	 column_widths_[COL_COUNT], (unsigned long)count);
		break;
	    case CS_UNCOVERED:
		strncpy(countbuf, " ###### ", sizeof(countbuf));
		break;
	    case CS_UNINSTRUMENTED:
		strncpy(countbuf, "        ", sizeof(countbuf));
		break;
	    }
	    strs[nstrs++] = countbuf;
    	}


	if (GTK_CHECK_MENU_ITEM(column_checks_[COL_SOURCE])->active)
	    strs[nstrs++] = linebuf;
	else
	    strs[nstrs++] = "\n";

    	for (i = 0 ; i < nstrs ; i++)
	{
#if GTK2
    	    GtkTextIter end;

    	    gtk_text_buffer_get_end_iter(buffer, &end);
    	    gtk_text_buffer_insert_with_tags(buffer, &end, strs[i], -1,
	    	    	    	    	     tag, (char*)0);
#else
    	    gtk_text_insert(text, font_,
	    		    color, /*back*/0,
			    strs[i], strlen(strs[i]));
#endif
    	}
    }
    
    max_lineno_ = loc.lineno;
#if !GTK2
    assert(offsets_by_line_->len == max_lineno_);
#endif
    
    fclose(fp);
#if !GTK2
    gtk_text_thaw(text);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


void
sourcewin_t::update_title_buttons()
{
    /* TODO: port the title button feature to GTK2 */
#if !GTK2
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
    	      GTK_SCROLLED_WINDOW_CLASS(GTK_OBJECT(scrollw)->klass)->scrollbar_spacing;
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
#endif
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
#if GTK2
#if GTK_CHECK_VERSION(2,0,3)
    GtkTextIter start, end;
    GtkTextBuffer *buffer;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_));
    
    gtk_text_buffer_get_iter_at_line(buffer, &start, startline);
    gtk_text_buffer_get_iter_at_line(buffer, &end, endline);

    /* this function appeared sometime after 2.0.2 */
    gtk_text_buffer_select_range(buffer, &start, &end);
#endif /* 2.0.0 */
#else
    int endoff;
    
    assert(offsets_by_line_->len > 0);

    if (startline < 1)
    	startline = 1;
    if (endline < 1)
    	endline = startline;
    if (startline > offsets_by_line_->len)
    	startline = offsets_by_line_->len;
    if (endline > offsets_by_line_->len)
    	endline = offsets_by_line_->len;
    if (startline > endline)
    	return;
	
    assert(startline >= 1);
    assert(startline <= offsets_by_line_->len);
    assert(endline >= 1);
    assert(endline <= offsets_by_line_->len);
    
    /* set endoff to the first location after the last line to be selected */
    if (endline == offsets_by_line_->len)
	endoff = -1;
    else
	endoff = g_array_index(offsets_by_line_, unsigned int, endline)-1;

    gtk_editable_select_region(GTK_EDITABLE(text_),
    	    g_array_index(offsets_by_line_, unsigned int, startline-1),
    	    endoff);
#endif
}

void
sourcewin_t::ensure_visible(unsigned long line)
{
#if GTK2
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_));
    
    gtk_text_buffer_get_iter_at_line(buffer, &iter, line);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view_),
                                 &iter,
                                 0.0, FALSE, 0.0, 0.0);
#else
    /* This mostly works.  Not totally predictable but good enough for now */
    GtkAdjustment *adj = GTK_TEXT(text_)->vadj;

    gtk_adjustment_set_value(adj,
	    	adj->upper * (double)line / (double)max_lineno_
		    - adj->page_size/2.0);
#endif
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

#if !GTK2
unsigned long
sourcewin_t::offset_to_lineno(unsigned int offset)
{
    unsigned int top, bottom;
    
    if (offset == 0)
    	return 0;

    top = offsets_by_line_->len-1;
    bottom = 0;
    
    fprintf(stderr, "offset_to_lineno: { offset=%u top=%u bottom=%u\n",
    	    offset, top, bottom);

    while (top - bottom > 1)
    {
    	unsigned int mid = (top + bottom)/2;
	unsigned int midoff = g_array_index(offsets_by_line_, unsigned int, mid);
	
    	fprintf(stderr, "offset_to_lineno:     top=%d bottom=%d mid=%d midoff=%u\n",
	    	    	 top, bottom, mid, midoff);

	if (midoff == offset)
	    top = bottom = mid;
    	else if (midoff < offset)
	    bottom = mid;
	else
	    top = mid;
    }

    fprintf(stderr, "offset_to_lineno: offset=%u line=%u }\n",
    	    offset, bottom);

    return (unsigned long)bottom+1;
}
#endif

void
sourcewin_t::get_selected_lines(unsigned long *startp, unsigned long *endp)
{
#if GTK2
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_));
    GtkTextIter start_iter, end_iter;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
    {
	if (startp != 0)
	    *startp = 0;
	if (endp != 0)
	    *endp = 0;
    }
    else
    {
	if (startp != 0)
	    *startp = gtk_text_iter_get_line(&start_iter);
	if (endp != 0)
	    *endp = gtk_text_iter_get_line(&end_iter);
    }
#else
    if (GTK_EDITABLE(text_)->selection_start_pos == 0 &&
    	GTK_EDITABLE(text_)->selection_end_pos == 0)
    {
	if (startp != 0)
	    *startp = 0;
	if (endp != 0)
	    *endp = 0;
    }
    else
    {
	if (startp != 0)
	    *startp = offset_to_lineno(GTK_EDITABLE(text_)->selection_start_pos);
	if (endp != 0)
	    *endp = offset_to_lineno(GTK_EDITABLE(text_)->selection_end_pos-1);
    }
#endif
}


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

#if !GTK2
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
#endif

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
    const GList *blocks;
    
    loc.filename = (char *)sw->filename_.data();
    sw->get_selected_lines(&loc.lineno, 0);

    if (loc.lineno != 0 ||
    	(blocks = cov_block_t::find_by_location(&loc)) == 0)
    	return;
    summarywin_t::show_function(((cov_block_t *)blocks->data)->function());
}

GLADE_CALLBACK void
on_source_summarise_range_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    unsigned long start, end;
    
    sw->get_selected_lines(&start, &end);
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
#if GTK2
    {
	GtkTextBuffer *buffer;
    	GtkTextIter start, end;
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view_));
    	gtk_text_buffer_get_bounds(buffer, &start, &end);
    	contents = gtk_text_buffer_get_text(buffer, &start, &end,
                                           /*include_hidden_chars*/FALSE);
    }
#else
    contents = gtk_editable_get_chars(GTK_EDITABLE(text_), 0, -1);
#endif
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
