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

#include "sourcewin.H"
#include "cov.H"
#include "estring.H"
#include "prefs.H"
#include "uix.h"

CVSID("$Id: sourcewin.C,v 1.5 2003-01-01 03:19:29 gnb Exp $");

gboolean sourcewin_t::initialised_ = FALSE;
GdkFont *sourcewin_t::font_;
int sourcewin_t::font_width_, sourcewin_t::font_height_;
list_t<sourcewin_t> sourcewin_t::instances_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::init(GtkWidget *w)
{
    if (initialised_)
    	return;
    initialised_ = TRUE;

    font_ = uix_fixed_width_font(gtk_widget_get_style(w)->font);
    font_height_ = uix_font_height(font_);
    font_width_ = uix_font_width(font_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


#define SOURCE_COLUMNS	    (8+8+80) 	/* number,count,source */
#define SOURCE_ROWS	    (35)
#define MAGIC_MARGINX	    14
#define MAGIC_MARGINY	    5

extern int screenshot_mode;

sourcewin_t::sourcewin_t()
{
    GladeXML *xml;
    
    offsets_by_line_ = g_array_new(/*zero_terminated*/TRUE,
				      /*clear*/TRUE,
				      sizeof(unsigned int));
    
    /* load the interface & connect signals */
    xml = ui_load_tree("source");
    
    set_window(glade_xml_get_widget(xml, "source"));

    text_ = glade_xml_get_widget(xml, "source_text");
    number_check_ = glade_xml_get_widget(xml, "source_number_check");
    block_check_ = glade_xml_get_widget(xml, "source_block_check");
    count_check_ = glade_xml_get_widget(xml, "source_count_check");
    source_check_ = glade_xml_get_widget(xml, "source_source_check");
    colors_check_ = glade_xml_get_widget(xml, "source_colors_check");
    filenames_menu_ = ui_get_dummy_menu(xml, "source_file_dummy");
    functions_menu_ = ui_get_dummy_menu(xml, "source_func_dummy");
    ui_register_windows_menu(ui_get_dummy_menu(xml, "source_windows_dummy"));
        
    init(window_);
    
    if (!screenshot_mode)
	gtk_widget_set_usize(text_,
    	    SOURCE_COLUMNS * font_width_ + MAGIC_MARGINX,
    	    SOURCE_ROWS * font_height_ + MAGIC_MARGINY);
	    
    instances_.append(this);
}

sourcewin_t::~sourcewin_t()
{
    strdelete(filename_);
    g_array_free(offsets_by_line_, /*free_segment*/TRUE);
    instances_.remove(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::on_source_filename_activate(GtkWidget *w, gpointer userdata)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    const char *filename = (const char *)userdata;
    estring fullname = cov_file_t::unminimise_name(filename);
    
    sw->set_filename(fullname.data(), filename);
}


void
sourcewin_t::populate_filenames()
{
    list_iterator_t<cov_file_t> iter;
   
    ui_delete_menu_items(filenames_menu_);
    
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    {
    	const char *filename = (*iter)->minimal_name();

	ui_menu_add_simple_item(filenames_menu_, filename,
	    	    on_source_filename_activate, (gpointer)filename);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Sadly, this godawful hack is necessary to delay the
 * select/scroll code until after the Functions menu has
 * popped down, there being some sort of display bug
 * in GTK 1.2.7.
 */
typedef struct
{
    sourcewin_t *sourcewin;
    cov_function_t *function;
} sourcewin_hacky_rec_t;

int
sourcewin_t::delayed_function_activate(gpointer userdata)
{
    sourcewin_hacky_rec_t *rec = (sourcewin_hacky_rec_t *)userdata;
    sourcewin_t *sw = rec->sourcewin;
    cov_function_t *fn = rec->function;
    const cov_location_t *first;
    const cov_location_t *last;

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
	    	    	first->filename, sw->filename_);
	g_free(rec);
	return FALSE;
    }
    if (strcmp(last->filename, sw->filename_))
    {
    	fprintf(stderr, "WTF?  Wrong filename for last loc: %s vs %s\n",
	    	    	last->filename, sw->filename_);
	g_free(rec);
	return FALSE;
    }

    sw->ensure_visible(first->lineno);

    /* This only selects the span of the lines which contain executable code */		    
    sw->select_region(first->lineno, last->lineno);

    delete rec;
    return FALSE;   /* and don't call me again! */
}

void
sourcewin_t::on_source_function_activate(GtkWidget *w, gpointer userdata)
{
    sourcewin_hacky_rec_t *rec;
    
    rec = new sourcewin_hacky_rec_t;
    rec->sourcewin = sourcewin_t::from_widget(w);
    rec->function = (cov_function_t *)userdata;

    gtk_idle_add(delayed_function_activate, rec);
}

void
sourcewin_t::populate_functions()
{
    GList *functions = 0;
    unsigned fnidx;
    cov_file_t *f;
    cov_function_t *fn;
    
    /* build an alphabetically sorted list of functions */
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

    ui_delete_menu_items(functions_menu_);
    
    while (functions != 0)
    {
    	fn = (cov_function_t *)functions->data;
	
	ui_menu_add_simple_item(functions_menu_, fn->name(),
	    	    on_source_function_activate, fn);
	
	functions = g_list_remove_link(functions, functions);
    }
}

void
sourcewin_t::populate()
{
    populate_filenames();
    populate_functions();
    update();
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
    unsigned int start = 0, end;
    
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
    GtkText *text = GTK_TEXT(text_);
    int i, nstrs;
    GdkColor *color;
    static GdkColor *scolors[4] =
    {
    	&prefs.covered_foreground, &prefs.partcovered_foreground,
	&prefs.uncovered_foreground, &prefs.uninstrumented_foreground
    };
    const char *strs[5];
    char linenobuf[32];
    char blockbuf[32];
    char countbuf[32];
    char linebuf[1024];
    
    if ((fp = fopen(filename_, "r")) == 0)
    {
    	/* TODO: gui error report */
    	perror(filename_);
	return;
    }


    gtk_text_freeze(text);
    gtk_editable_delete_text(GTK_EDITABLE(text), 0, -1);
    
    g_array_set_size(offsets_by_line_, 0);

    loc.filename = filename_;
    loc.lineno = 0;
    while (fgets_tabexpand(linebuf, sizeof(linebuf), fp) != 0)
    {
    	++loc.lineno;
	
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
	
	status = cov_get_count_by_location(&loc, &count);

    	/* choose colours */
	color = 0;
	if (GTK_CHECK_MENU_ITEM(colors_check_)->active)
	    color = scolors[status];

    	/* generate strings */
	
	nstrs = 0;
	
	if (GTK_CHECK_MENU_ITEM(number_check_)->active)
	{
	    snprintf(linenobuf, sizeof(linenobuf), "%7lu ", loc.lineno);
	    strs[nstrs++] = linenobuf;
	}
	
	if (GTK_CHECK_MENU_ITEM(block_check_)->active)
	{
	    format_blocks(blockbuf, 16, &loc);
	    strs[nstrs++] = blockbuf;
	}
	
	if (GTK_CHECK_MENU_ITEM(count_check_)->active)
	{
	    switch (status)
	    {
	    case CS_COVERED:
	    case CS_PARTCOVERED:
		snprintf(countbuf, sizeof(countbuf), "%7lu ", (unsigned long)count);
		strs[nstrs++] = countbuf;
		break;
	    case CS_UNCOVERED:
		strs[nstrs++] = " ###### ";
		break;
	    case CS_UNINSTRUMENTED:
		strs[nstrs++] = "        ";
		break;
	    }
    	}

	if (GTK_CHECK_MENU_ITEM(source_check_)->active)
	    strs[nstrs++] = linebuf;
	else
	    strs[nstrs++] = "\n";

    	for (i = 0 ; i < nstrs ; i++)
	{
    	    gtk_text_insert(text, font_,
	    		    color, /*back*/0,
			    strs[i], strlen(strs[i]));
    	}
    }
    
    max_lineno_ = loc.lineno;
    assert(offsets_by_line_->len == max_lineno_);
    
    fclose(fp);
    gtk_text_thaw(text);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::set_filename(const char *filename, const char *display_fname)
{
    set_title((display_fname == 0 ? filename : display_fname));
    strassign(filename_, filename);

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
}

/* This mostly works.  Not totally predictable but good enough for now */
void
sourcewin_t::ensure_visible(unsigned long line)
{
    GtkAdjustment *adj = GTK_TEXT(text_)->vadj;

    gtk_adjustment_set_value(adj,
	    	adj->upper * (double)line / (double)max_lineno_
		    - adj->page_size/2.0);
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

    show_lines(start->filename, start->lineno, end->lineno);
}

void
sourcewin_t::show_file(const char *filename)
{
    show_lines(filename, 0, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_source_close_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    
    assert(sw != 0);
    delete sw;
}

GLADE_CALLBACK void
on_source_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_source_count_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    
    sw->update();
}

GLADE_CALLBACK void
on_source_number_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    
    sw->update();
}

GLADE_CALLBACK void
on_source_block_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_t::from_widget(w);
    
    sw->update();
}

GLADE_CALLBACK void
on_source_source_check_activate(GtkWidget *w, gpointer data)
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
