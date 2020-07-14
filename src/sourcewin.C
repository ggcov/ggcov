/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
#include "diagwin.H"
#include "flow_diagram.H"
#if HAVE_LIBGNOMEUI
#include "canvas_scenegen.H"
#endif
#include "cov.H"
#include "estring.H"
#include "prefs.H"
#include "confsection.H"
#include "logging.H"

#ifndef GTK_SCROLLED_WINDOW_GET_CLASS
#define GTK_SCROLLED_WINDOW_GET_CLASS(obj) \
	GTK_SCROLLED_WINDOW_CLASS(GTK_OBJECT_CLASS(GTK_OBJECT(obj)->klass))
#endif


list_t<sourcewin_t> sourcewin_t::instances_;
static logging::logger_t &_log = logging::find_logger("sourcewin");

/* column widths, in *characters* */
const int sourcewin_t::column_widths_[sourcewin_t::NUM_COLS] = { 6, 8, 16, 8, -1 };
const char *sourcewin_t::column_names_[sourcewin_t::NUM_COLS] = {
    "Flow", "Line", "Blocks", "Count", "Source"
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::on_vadjustment_value_changed(
    GtkAdjustment *adj,
    gpointer userdata)
{
    sourcewin_t *sw = (sourcewin_t *)userdata;

    sw->update_flows();
}

void
sourcewin_t::update_flow_window()
{
    GtkTextView *tv = GTK_TEXT_VIEW(text_);

    _log.debug("sourcewin_t::update_flow_window\n");

    if (!GTK_CHECK_MENU_ITEM(column_checks_[COL_FLOW])->active)
    {
	gtk_text_view_set_border_window_size(tv, GTK_TEXT_WINDOW_LEFT, 0);
	delete_flows();
	return;
    }

    // make the left child visible
    gtk_text_view_set_border_window_size(tv, GTK_TEXT_WINDOW_LEFT, flow_width_);

    if (GTK_WIDGET_REALIZED(text_))
    {
	// copy the text window's background to the left child
	GtkStyle *style = gtk_widget_get_style(text_);
	gdk_window_set_background(
		gtk_text_view_get_window(tv, GTK_TEXT_WINDOW_LEFT),
		&style->base[GTK_STATE_NORMAL]);
    }
}

/*
 * Get the first and last line number visible in the text window.
 * Returns FALSE if there are no numbers yet.
 */
gboolean
sourcewin_t::get_visible_lines(
    unsigned long *begin_linenop,
    unsigned long *end_linenop)
{
    GtkTextView *tv = GTK_TEXT_VIEW(text_);
    GdkRectangle rect;
    GtkTextIter begin_iter;
    GtkTextIter end_iter;

    gtk_text_view_get_visible_rect(tv, &rect);
    _log.debug("    visible rect={%u,%u,%u,%u}\n",
	       rect.x, rect.y, rect.width, rect.height);

    gtk_text_view_get_iter_at_location(tv, &begin_iter,
		    rect.x, rect.y);
    gtk_text_view_get_iter_at_location(tv, &end_iter,
		    rect.x, rect.y+rect.height);

    unsigned long begin_lineno = gtk_text_iter_get_line(&begin_iter);
    unsigned long end_lineno = gtk_text_iter_get_line(&end_iter);

    _log.debug("    begin_lineno=%lu end_lineno=%lu\n", begin_lineno, end_lineno);

    if (begin_lineno == 0 && end_lineno == 0)
	return FALSE;

    if (begin_linenop != 0)
	*begin_linenop = begin_lineno;
    if (end_linenop != 0)
	*end_linenop = end_lineno;

    return TRUE;
}

void
sourcewin_t::update_flows()
{
    _log.debug("scheduling sourcewin_t::do_update_flows() for later\n");
    retries_remaining_ = 10;
    g_idle_add_full(G_PRIORITY_LOW,
		    update_flows_tramp,
		    /*data*/(gpointer)this,
		    (GDestroyNotify)NULL);
}

gboolean
sourcewin_t::update_flows_tramp(gpointer user_data)
{
    sourcewin_t *sw = (sourcewin_t *)user_data;
    return sw->do_update_flows();
}

gboolean
sourcewin_t::do_update_flows()
{
#if HAVE_LIBGNOMEUI
    _log.debug("sourcewin_t::do_update_flows\n");
    GtkTextView *tv = GTK_TEXT_VIEW(text_);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(tv);

    if (!GTK_WIDGET_REALIZED(text_))
	return G_SOURCE_REMOVE;	    // no worries, will get called on realize
    if (!GTK_CHECK_MENU_ITEM(column_checks_[COL_FLOW])->active)
	return G_SOURCE_REMOVE;	    // nothing to do anyway

    cov_file_t *f;
    if ((f = cov_file_t::find(filename_)) == 0)
	return G_SOURCE_REMOVE;	    // nothing to do anyway

    unsigned long begin_lineno, end_lineno;
    if (!get_visible_lines(&begin_lineno, &end_lineno))
	return G_SOURCE_REMOVE;	    // nothing to do anyway

    flow_shown_++;
    cov_function_t *oldfn = 0, *fn;
    unsigned int lineno;
    if (begin_lineno < 1)
	begin_lineno = 1;
    if (end_lineno > f->num_lines())
	end_lineno = f->num_lines();
    for (lineno = begin_lineno ; lineno <= end_lineno ; lineno++)
    {
	cov_line_t *ln = f->nth_line(lineno);
	if (ln == 0 || (fn = ln->function()) == 0 || fn == oldfn)
	    continue;
	oldfn = fn;

	_log.debug("    line %u function %s\n", lineno, fn->name());

	flow_t *flow = 0;
	for (list_iterator_t<flow_t> fiter = flows_.first() ; *fiter ; ++fiter)
	{
	    if ((*fiter)->function_ == fn)
	    {
		flow = *fiter;
		break;
	    }
	}
	if (flow == 0)
	{
	    _log.debug("        creating new flow\n");
	    GtkTextIter first_iter, last_iter;

	    gtk_text_buffer_get_iter_at_line(buffer, &first_iter,
		    fn->get_first_location()->lineno);
	    gtk_text_buffer_get_iter_at_line(buffer, &last_iter,
		    fn->get_last_location()->lineno);

	    int first_y = -1, last_y = -1, height = -1;
	    gtk_text_view_get_line_yrange(tv, &first_iter, &first_y, &height);
	    gtk_text_view_get_line_yrange(tv, &last_iter, &last_y, &height);

	    _log.debug("        line geometry in pixels: first_y=%d last_y=%d height=%d\n",
		       first_y, last_y, height);
	    if (height == 0)
	    {
		if (retries_remaining_)
		{
		    _log.debug("        skipping flows and will retry\n");
		    retries_remaining_--;
		    return G_SOURCE_CONTINUE;
		}
		_log.warning("sourcewin_t::do_update_flows: too many retries, giving up\n");
		return G_SOURCE_REMOVE;
	    }

	    _log.debug("        flow geometry in pixels: y=%d height=%d\n",
		       first_y-height, last_y-first_y+height);
	    flow = create_flow(fn, first_y-height, last_y-first_y+height);
	    flows_.append(flow);
	    gtk_text_view_add_child_in_window(tv, flow->canvas_,
			    GTK_TEXT_WINDOW_LEFT, 0, 0);
	}
	int wx, wy;
	gtk_text_view_buffer_to_window_coords(tv, GTK_TEXT_WINDOW_LEFT,
		0, flow->bufy_, &wx, &wy);
	_log.debug("        moving to %d,%d\n", 0, wy);
	gtk_text_view_move_child(tv, flow->canvas_, 0, wy);
	gtk_widget_show(flow->canvas_);
	flow->shown_ = flow_shown_;
    }

    /* Hide flows not just shown */
    _log.debug("    hiding flows\n");
    for (list_iterator_t<flow_t> fiter = flows_.first() ; *fiter ; ++fiter)
    {
	flow_t *flow = *fiter;
	if (flow->shown_ < flow_shown_ &&
	    GTK_WIDGET_VISIBLE(flow->canvas_))
	{
	    _log.debug("        %s\n", flow->function_->name());
	    gtk_widget_hide(flow->canvas_);
	}
    }

    /* Recalculate the flow_width_ if necessary */
    if (flow_width_dirty_)
    {
	flow_width_ = column_widths_[COL_FLOW] * font_width_;
	_log.debug("    resizing window\n");
	for (list_iterator_t<flow_t> fiter = flows_.first() ; *fiter ; ++fiter)
	{
	    flow_t *flow = *fiter;
	    if (GTK_WIDGET_VISIBLE(flow->canvas_) && flow->width_ > flow_width_)
	    {
		flow_width_ = flow->width_;
		_log.debug("        %s -> %u\n",
			 flow->function_->name(), flow_width_);
	    }
	}
	flow_width_dirty_ = FALSE;
	update_flow_window();
	update_title_buttons();
    }

#endif
    return G_SOURCE_REMOVE;	    // all done, yay
}

#define GDK_TO_RGB(gdkcol) \
	RGB(((gdkcol)->red>>8), ((gdkcol)->green>>8), ((gdkcol)->blue>>8))

static void
set_diagram_colors(diagram_t *di)
{
    int i;

    for (i = 0 ; i < cov::NUM_STATUS ; i++)
    {
	di->set_fg((cov::status_t)i, GDK_TO_RGB(foregrounds_by_status[i]));
	di->set_bg((cov::status_t)i, GDK_TO_RGB(backgrounds_by_status[i]));
    }
}

sourcewin_t::flow_t *
sourcewin_t::create_flow(cov_function_t *fn, int y, int h)
{
#if HAVE_LIBGNOMEUI
    flow_t *flow = new flow_t;
    flow->function_ = fn;
    flow->bufy_ = y;

    _log.debug("        creating flow y=%d h=%d\n", y, h);

    flow->canvas_ = gnome_canvas_new();

    GtkStyle *style = gtk_widget_get_style(text_);
    gtk_widget_modify_bg(flow->canvas_, GTK_STATE_NORMAL,
			 &style->base[GTK_STATE_NORMAL]);

    _log.debug("bg[%d] = {%04x,%04x,%04x}\n",
	    GTK_STATE_NORMAL,
	    style->bg[GTK_STATE_NORMAL].red,
	    style->bg[GTK_STATE_NORMAL].green,
	    style->bg[GTK_STATE_NORMAL].blue);
    _log.debug("base[%d] = {%04x,%04x,%04x}\n",
	    GTK_STATE_NORMAL,
	    style->base[GTK_STATE_NORMAL].red,
	    style->base[GTK_STATE_NORMAL].green,
	    style->base[GTK_STATE_NORMAL].blue);

    diagram_t *diag = new flow_diagram_t(flow->function_);

    set_diagram_colors(diag);
    diag->prepare();

    scenegen_t *sg = new canvas_scenegen_t(GNOME_CANVAS(flow->canvas_));
    diag->render(sg);
    delete sg;

    /* setup the canvas to show the whole diagram, preserving aspect */
    dbounds_t bounds;
    diag->get_bounds(&bounds);
    double ppu = h / bounds.height();
    flow->width_ = (unsigned int)(bounds.width() * ppu + 0.5) + 2;

    gtk_widget_set_usize(flow->canvas_, flow->width_, h);
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(flow->canvas_), ppu);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(flow->canvas_),
				   bounds.x1, bounds.y1,
				   bounds.x2, bounds.y2);

    _log.debug("        canvas %g x %g units = %u x %u pixels\n",
		bounds.width(), bounds.height(), flow->width_, h);

    if (flow->width_ > flow_width_)
    {
	flow_width_dirty_ = TRUE;
	_log.debug("        flow width dirty\n");
    }

    return flow;
#else
    return (flow_t *)0;
#endif
}

void
sourcewin_t::delete_flows()
{
#if HAVE_LIBGNOMEUI
    flow_t *flow;

    while ((flow = flows_.remove_head()))
    {
	gtk_widget_hide(flow->canvas_);
	gtk_widget_unref(flow->canvas_);
	delete flow;
    }
    flow_width_dirty_ = TRUE;
#endif
}

/*
 * Poll and dispatch events (and in particular, the idle callbacks
 * that GtkTextView uses to update its data structures) until we
 * can do something with the GtkTextView that depends on that
 * state, like update the flows.
 */
void
sourcewin_t::wait_for_text_validation()
{
    gboolean vis = FALSE;
    int n = 5;

    while (g_main_context_iteration(NULL, FALSE))
    {
	_log.debug("wait_for_text_validation\n");

	if (!vis)
	    vis = get_visible_lines(0, 0);
	if (!vis)
	    continue;

	/*
	 * Need to run at least one more callback, otherwise
	 * we miscalculate the size of a flow which extends
	 * off the bottom of the visible window.
	 */
	if (n-- == 0)
	    break;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::setup_text()
{
    ui_text_setup(text_);
    font_width_ = ui_text_font_width(text_);

    text_tags_[cov::COVERED] =
	ui_text_create_tag(text_, "covered", &prefs.covered_foreground);

    text_tags_[cov::PARTCOVERED] =
	ui_text_create_tag(text_, "partcovered", &prefs.partcovered_foreground);

    text_tags_[cov::UNCOVERED] =
	ui_text_create_tag(text_, "uncovered", &prefs.uncovered_foreground);

    text_tags_[cov::UNINSTRUMENTED] =
	ui_text_create_tag(text_, "uninstrumented", &prefs.uninstrumented_foreground);

    text_tags_[cov::SUPPRESSED] =
	ui_text_create_tag(text_, "suppressed", &prefs.suppressed_foreground);

    /*
     * Setup the left child to show function flows.
     */
    GtkTextView *tv = GTK_TEXT_VIEW(text_);

    // hook up the function to update the flows on scrolling
    text_initialised_ = FALSE;
    flow_width_ = column_widths_[COL_FLOW] * font_width_;
    flow_width_dirty_ = FALSE;
    g_signal_connect_after(G_OBJECT(tv->vadjustment),
			   "value-changed",
			   G_CALLBACK(on_vadjustment_value_changed),
			   (gpointer)this);

    /*
     * Handle the end-user-action signal to grey out menu items
     * which work on the presence of a selection.
     */
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(tv);
    g_signal_connect_after(G_OBJECT(buffer),
			   "mark-set",
			   G_CALLBACK(on_buffer_mark_set),
			   (gpointer)this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


sourcewin_t::sourcewin_t()
{
    GladeXML *xml;
    GtkWidget *w;

    /* load the interface & connect signals */
    xml = ui_load_tree("source");

    set_window(glade_xml_get_widget(xml, "source"));

    text_ = glade_xml_get_widget(xml, "source_text");
    setup_text();
    column_checks_[COL_FLOW] = glade_xml_get_widget(xml, "source_flow_check");
    column_checks_[COL_LINE] = glade_xml_get_widget(xml, "source_line_check");
    column_checks_[COL_BLOCK] = glade_xml_get_widget(xml, "source_block_check");
    column_checks_[COL_COUNT] = glade_xml_get_widget(xml, "source_count_check");
    column_checks_[COL_SOURCE] = glade_xml_get_widget(xml, "source_source_check");
    colors_check_ = glade_xml_get_widget(xml, "source_colors_check");
    toolbar_check_ = glade_xml_get_widget(xml, "source_toolbar_check");
    titles_check_ = glade_xml_get_widget(xml, "source_titles_check");
    flow_diagram_item_ = glade_xml_get_widget(xml, "source_flow_diagram");
    summarise_file_item_ = glade_xml_get_widget(xml, "source_summarise_file");
    summarise_function_item_ = glade_xml_get_widget(xml, "source_summarise_function");
    summarise_range_item_ = glade_xml_get_widget(xml, "source_summarise_range");
    toolbar_ = glade_xml_get_widget(xml, "source_toolbar");
    w = glade_xml_get_widget(xml, "source_filenames_combo");
    filenames_combo_ = init(UI_COMBO(w), "/");
    w = glade_xml_get_widget(xml, "source_functions_combo");
    functions_combo_ = init(UI_COMBO(w));
    titles_hbox_ = glade_xml_get_widget(xml, "source_titles_hbox");
    left_pad_label_ = glade_xml_get_widget(xml, "source_left_pad_label");
    title_buttons_[COL_FLOW] = glade_xml_get_widget(xml, "source_col_flow_button");
    title_buttons_[COL_LINE] = glade_xml_get_widget(xml, "source_col_line_button");
    title_buttons_[COL_BLOCK] = glade_xml_get_widget(xml, "source_col_block_button");
    title_buttons_[COL_COUNT] = glade_xml_get_widget(xml, "source_col_count_button");
    title_buttons_[COL_SOURCE] = glade_xml_get_widget(xml, "source_col_source_button");
    right_pad_label_ = glade_xml_get_widget(xml, "source_right_pad_label");
    ui_register_windows_menu(ui_get_dummy_menu(xml, "source_windows_dummy"));

    /*
     * The View->Text Size->Increase menu item has an accelerator
     * which is nominally "Ctrl++", unfortunately what GTK sees on
     * many keyboards when the user tries that is "Ctrl+=" because
     * the + symbol is a shift-glyph on the = key and people don't
     * understand to press Shift.  So we hack around the problem
     * by registering a second accelerator for Ctrl+=.
     */
    w = glade_xml_get_widget(xml, "source_text_size_increase");
    const GSList *groups = gtk_accel_groups_from_object(G_OBJECT(window_));
    gtk_widget_add_accelerator(w, "activate",
			       GTK_ACCEL_GROUP(groups->data),
			       GDK_equal, GDK_CONTROL_MASK,
			       (GtkAccelFlags)0);

    xml = ui_load_tree("source_saveas");
    saveas_dialog_ = glade_xml_get_widget(xml, "source_saveas");
    attach(saveas_dialog_);

    instances_.append(this);
}

sourcewin_t::~sourcewin_t()
{
//     delete_flows();
    instances_.remove(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
sourcewin_t::on_filenames_entry_changed()
{
    cov_file_t *f = (cov_file_t *)get_active(filenames_combo_);
    if (populating_ || !shown_ || f == 0/*stupid gtk2*/)
	return;
    set_filename(f->name(), f->minimal_name());
}


void
sourcewin_t::populate_filenames()
{
    populating_ = TRUE; /* suppress combo entry callback */
    clear(filenames_combo_);
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	cov_file_t *f = *iter;
	add(filenames_combo_, f->minimal_name(), f);
    }
    done(filenames_combo_);
    populating_ = FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
sourcewin_t::on_functions_entry_changed()
{
    cov_function_t *fn = (cov_function_t *)get_active(functions_combo_);
    const cov_location_t *first;
    const cov_location_t *last;

    if (populating_ || !shown_ || fn == 0 /*stupid gtk2*/)
	return;

    first = fn->get_first_location();
    last = fn->get_last_location();
    _log.debug("Function %s -> %s:%ld to %s:%ld\n",
	       fn->name(),
	       first->filename, first->lineno,
	       last->filename, last->lineno);

    /* Check for weirdness like functions spanning files */
    if (strcmp(first->filename, filename_))
    {
	_log.error("Wrong filename for first loc: %s vs %s\n",
		   first->filename, filename_.data());
	return;
    }
    if (strcmp(last->filename, filename_))
    {
	_log.error("Wrong filename for last loc: %s vs %s\n",
		   last->filename, filename_.data());
	return;
    }

    ensure_visible(first->lineno);

    /* This only selects the span of the lines which contain executable code */
    select_region(first->lineno, last->lineno);
}

void
sourcewin_t::populate_functions()
{
    list_t<cov_function_t> functions;
    cov_file_t *f;
    cov_function_t *fn;

    /* build an alphabetically sorted list of functions in the file */
    f = cov_file_t::find(filename_);
    assert(f != 0);
    for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
    {
	fn = *fnitr;

	if (fn->is_suppressed() ||
	    fn->get_first_location() == 0)
	    continue;
	functions.prepend(fn);
    }
    functions.sort(cov_function_t::compare);

    /* now build the menu */

    populating_ = TRUE; /* suppress combo entry callback */
    clear(functions_combo_);
    while ((fn = functions.remove_head()) != 0)
	add(functions_combo_, fn->name(), fn);
    done(functions_combo_);
    populating_ = FALSE;
}

void
sourcewin_t::populate()
{
    populate_filenames();
    populate_functions();
    update();
    grey_items();
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

    maxlen--;   /* allow for nul terminator */

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
	    case cov::COVERED:
	    case cov::PARTCOVERED:
		snprintf(countbuf, sizeof(countbuf), "%*llu",
			 column_widths_[COL_COUNT]-1,
			 (unsigned long long)ln->count());
		break;
	    case cov::UNCOVERED:
		strncpy(countbuf, " ######", sizeof(countbuf));
		break;
	    case cov::UNINSTRUMENTED:
	    case cov::SUPPRESSED:
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
    wait_for_text_validation();

    delete_flows();
    update_flow_window();
    update_flows();
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
    gtk_widget_set_usize(title_buttons_[COL_FLOW],
			 flow_width_,
			 /*height=whatever*/5);
    for (i = COL_LINE ; i < NUM_COLS ; i++)
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
sourcewin_t::grey_items()
{
    unsigned long start = 0;
    ui_text_get_selected_lines(text_, &start, 0);
    gboolean selfn = !!selected_function();

    gtk_widget_set_sensitive(flow_diagram_item_, selfn);
    gtk_widget_set_sensitive(summarise_function_item_, selfn);
    gtk_widget_set_sensitive(summarise_range_item_, (start > 0));
    window_t::grey_items();
}

void
sourcewin_t::on_buffer_mark_set(GtkTextBuffer *buffer, GtkTextIter *iter,
				GtkTextMark *mark, gpointer closure)
{
    sourcewin_t *sw = (sourcewin_t *)closure;
    const char *nm = gtk_text_mark_get_name(mark);

    if (nm && (!strcmp(nm, "insert") || !strcmp(nm, "selection_bound")))
	sw->grey_items();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::set_filename(const char *filename, const char *display_fname)
{
    set_title((display_fname == 0 ? filename : display_fname));
    filename_ = filename;

    if (shown_)
    {
	delete_flows();
	populate_functions();
	update();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::select_region(unsigned long startline, unsigned long endline)
{
    _log.debug("sourcewin_t::select_region: startline=%ld endline=%ld\n",
		startline, endline);
    ui_text_select_lines(text_, startline, endline);
}

void
sourcewin_t::ensure_visible(unsigned long line)
{
    ui_text_ensure_visible(text_, line);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t *
sourcewin_t::selected_function() const
{
    cov_location_t loc;
    cov_line_t *ln;
    cov_function_t *fn;

    loc.filename = (char *)filename_.data();
    ui_text_get_selected_lines(text_, &loc.lineno, 0);

    if (loc.lineno == 0 ||
	(ln = cov_file_t::find_line(&loc)) == 0 ||
	(fn = ln->function()) == 0)
	return 0;
    return fn;
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

    _log.debug("sourcewin_t::show_lines(\"%s\", %lu, %lu) => \"%s\"\n",
	       filename, startline, endline, fullname.data());

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

void
sourcewin_t::apply_toggles()
{
    if (GTK_CHECK_MENU_ITEM(toolbar_check_)->active)
	gtk_widget_show(toolbar_);
    else
	gtk_widget_hide(toolbar_);

    if (GTK_CHECK_MENU_ITEM(titles_check_)->active)
    {
	update_title_buttons();
	gtk_widget_show(titles_hbox_);
    }
    else
	gtk_widget_hide(titles_hbox_);
}

void
sourcewin_t::load_state()
{
    populating_ = TRUE; /* suppress check menu item callback */
    for (unsigned int i = 0 ; i < NUM_COLS ; i++)
	load(GTK_CHECK_MENU_ITEM(column_checks_[i]));
    load(GTK_CHECK_MENU_ITEM(colors_check_));
    load(GTK_CHECK_MENU_ITEM(toolbar_check_));
    load(GTK_CHECK_MENU_ITEM(titles_check_));
    apply_toggles();
    populating_ = FALSE;
}

void
sourcewin_t::save_state()
{
    for (unsigned int i = 0 ; i < NUM_COLS ; i++)
	save(GTK_CHECK_MENU_ITEM(column_checks_[i]));
    save(GTK_CHECK_MENU_ITEM(colors_check_));
    save(GTK_CHECK_MENU_ITEM(toolbar_check_));
    save(GTK_CHECK_MENU_ITEM(titles_check_));
    confsection_t::sync();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


GLADE_CALLBACK void
sourcewin_t::on_column_check_activate()
{
    if (populating_)
	return;
    update();
    save_state();
}

GLADE_CALLBACK void
sourcewin_t::on_colors_check_activate()
{
    if (populating_)
	return;
    update();
    save_state();
}

GLADE_CALLBACK void
sourcewin_t::on_toolbar_check_activate()
{
    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
sourcewin_t::on_titles_check_activate()
{
    if (populating_)
	return;
    apply_toggles();
    save_state();
}

GLADE_CALLBACK void
sourcewin_t::on_summarise_file_activate()
{
    summarywin_t::show_file(cov_file_t::find(filename_));
}

GLADE_CALLBACK void
sourcewin_t::on_summarise_function_activate()
{
    cov_function_t *fn = selected_function();

    if (fn != 0)
	summarywin_t::show_function(fn);
}

GLADE_CALLBACK void
sourcewin_t::on_summarise_range_activate()
{
    unsigned long start, end;

    ui_text_get_selected_lines(text_, &start, &end);
    if (start != 0)
	summarywin_t::show_lines(filename_, start, end);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
sourcewin_t::on_flow_diagram_activate()
{
    cov_function_t *fn = selected_function();

    if (fn != 0)
    {
	diagwin_t *dw = new diagwin_t(new flow_diagram_t(fn));
	dw->show();
    }
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
    for (i = COL_LINE ; i < NUM_COLS ; i++)
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
    for (i = COL_LINE ; i < NUM_COLS ; i++)
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
sourcewin_t::on_saveas_ok_button_clicked()
{
    const char *filename = gtk_file_selection_get_filename(
				GTK_FILE_SELECTION(saveas_dialog_));
    save_with_annotations(filename);
    gtk_widget_hide(saveas_dialog_);
}

GLADE_CALLBACK void
sourcewin_t::on_saveas_cancel_button_clicked()
{
    gtk_widget_hide(saveas_dialog_);
}

GLADE_CALLBACK void
sourcewin_t::on_save_as_activate()
{
    string_var filename = g_strconcat(filename_.data(), ".cov.txt", (char *)0);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(saveas_dialog_), filename);
    gtk_widget_show(saveas_dialog_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_t::adjust_text_size(int dirn)
{
    ui_text_adjust_text_size(text_, dirn);
    font_width_ = ui_text_font_width(text_);
    delete_flows();
    update_flows();
    update_title_buttons();
}

GLADE_CALLBACK void
sourcewin_t::on_text_size_increase_activate()
{
    _log.debug("sourcewin_t::on_text_size_increase_activate\n");
    adjust_text_size(1);
}

GLADE_CALLBACK void
sourcewin_t::on_text_size_normal_activate()
{
    _log.debug("sourcewin_t::on_text_size_normal_activate\n");
    adjust_text_size(0);
}

GLADE_CALLBACK void
sourcewin_t::on_text_size_decrease_activate()
{
    _log.debug("sourcewin_t::on_text_size_decrease_activate\n");
    adjust_text_size(-1);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
