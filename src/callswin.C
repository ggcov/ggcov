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

#include "callswin.H"
#include "sourcewin.H"
#include "filename.h"
#include "cov.H"
#include "estring.H"

CVSID("$Id: callswin.C,v 1.7 2003-03-17 03:54:49 gnb Exp $");

#define COL_FROM    0
#define COL_TO	    1
#define COL_LINE    2
#define COL_ARC     3
#define COL_COUNT   4
#define COL_NUM     5

static const char all_functions[] = N_("All Functions");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
callswin_compare(GtkCList *clist, const void *ptr1, const void *ptr2)
{
    cov_arc_t *a1 = (cov_arc_t *)((GtkCListRow *)ptr1)->data;
    cov_arc_t *a2 = (cov_arc_t *)((GtkCListRow *)ptr2)->data;

    switch (clist->sort_column)
    {
    case COL_COUNT:
    	return u64cmp(a1->from()->count(), a2->from()->count());
	
    case COL_ARC:
#define arcno(a)    (((a)->from()->bindex() << 16)|(a)->aindex())
    	return u32cmp(arcno(a1), arcno(a2));
#undef arcno
	
    case COL_TO:
    	return strcmp(safestr(a1->name()), safestr(a2->name()));
	
    case COL_FROM:
    	return strcmp(safestr(a1->from()->function()->name()),
	    	      safestr(a2->from()->function()->name()));
	
    default:
	return 0;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callswin_t::callswin_t()
{
    GladeXML *xml;
    
    /* load the interface & connect signals */
    xml = ui_load_tree("calls");
    
    set_window(glade_xml_get_widget(xml, "calls"));
    
    from_function_combo_ = glade_xml_get_widget(xml, "calls_from_function_combo");
    from_function_view_ = glade_xml_get_widget(xml, "calls_from_function_view");

    to_function_combo_ = glade_xml_get_widget(xml, "calls_to_function_combo");
    to_function_view_ = glade_xml_get_widget(xml, "calls_to_function_view");

    clist_ = glade_xml_get_widget(xml, "calls_clist");
    gtk_clist_column_titles_passive(GTK_CLIST(clist_));
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_FROM);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_TO);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_ARC);
    ui_clist_init_column_arrow(GTK_CLIST(clist_), COL_COUNT);
    gtk_clist_set_compare_func(GTK_CLIST(clist_), callswin_compare);
    ui_clist_set_sort_column(GTK_CLIST(clist_), COL_ARC);
    ui_clist_set_sort_type(GTK_CLIST(clist_), GTK_SORT_ASCENDING);
    gtk_clist_set_column_visibility(GTK_CLIST(clist_), COL_ARC, FALSE);

    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "calls_windows_dummy"));
}

callswin_t::~callswin_t()
{
    listclear(functions_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callswin_t::populate_function_combo(GtkCombo *combo)
{
    GList *iter;
    estring label;
    
    ui_combo_add_data(combo, _(all_functions), 0);

    for (iter = functions_ ; iter != 0 ; iter = iter->next)
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
}

void
callswin_t::populate()
{
#if DEBUG
    fprintf(stderr, "callswin_t::populate\n");
#endif
    functions_ = cov_function_t::list_all();
    populate_function_combo(GTK_COMBO(from_function_combo_));
    populate_function_combo(GTK_COMBO(to_function_combo_));
    update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callswin_t::update_for_func(cov_function_t *from_fn, cov_function_t *to_fn)
{
    unsigned int bidx;
    int row;
    list_iterator_t<cov_arc_t> aiter;
    const cov_location_t *loc;
    char *text[COL_NUM];
    char countbuf[32];
    char arcbuf[64];
    char linebuf[32];

    for (bidx = 0 ; bidx < from_fn->num_blocks()-2 ; bidx++)
    {
    	cov_block_t *b = from_fn->nth_block(bidx);

	for (aiter = b->out_arc_iterator() ; aiter != (cov_arc_t *)0 ; ++aiter)
	{
	    cov_arc_t *a = *aiter;
    	    
	    if (!a->is_call())
	    	continue;
	    if (to_fn != 0 &&
	    	(a->name() == 0 || strcmp(to_fn->name(), a->name())))
	    	continue;
		
	    snprintf(countbuf, sizeof(countbuf), GNB_U64_DFMT, a->from()->count());
	    text[COL_COUNT] = countbuf;

	    snprintf(arcbuf, sizeof(arcbuf), "B%u A%u", b->bindex(), a->aindex());
	    text[COL_ARC] = arcbuf;

    	    if ((loc = a->get_from_location()) == 0)
	    	strncpy(linebuf, "WTF??", sizeof(linebuf));
	    else
		snprintf(linebuf, sizeof(linebuf), "%lu", loc->lineno);
	    text[COL_LINE] = linebuf;
	    
	    text[COL_FROM] = (char *)a->from()->function()->name();
	    if ((text[COL_TO] = (char *)a->name()) == 0)
	    	text[COL_TO] = "(unknown)";
		
	    row = gtk_clist_append(GTK_CLIST(clist_), text);
	    gtk_clist_set_row_data(GTK_CLIST(clist_), row, a);
	}
    }
}

void
callswin_t::update()
{
    cov_function_t *from_fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	GTK_COMBO(from_function_combo_));
    cov_function_t *to_fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	GTK_COMBO(to_function_combo_));
    estring title;
    
#if DEBUG
    fprintf(stderr, "callswin_t::update\n");
#endif
    switch ((from_fn == 0 ? 0 : 2)|(to_fn == 0 ? 0 : 1))
    {
    case 0:
	break;
    case 1:
    	title.append_printf(_("to %s"), to_fn->name());
	break;
    case 2:
    	title.append_printf(_("from %s"), from_fn->name());
	break;
    case 3:
    	title.append_printf(_("from %s to %s"),
	    	    	      from_fn->name(), to_fn->name());
	break;
    }
    set_title(title.data());


    gtk_clist_freeze(GTK_CLIST(clist_));
    gtk_clist_clear(GTK_CLIST(clist_));

    if (from_fn != 0)
    {
    	update_for_func(from_fn, to_fn);
    }
    else
    {
    	/* all functions */
	GList *iter;
	
	for (iter = functions_ ; iter != 0 ; iter = iter->next)
	{
	    from_fn = (cov_function_t *)iter->data;
	    
	    update_for_func(from_fn, to_fn);
	}
    }
    
    gtk_clist_columns_autosize(GTK_CLIST(clist_));
    gtk_clist_thaw(GTK_CLIST(clist_));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_calls_close_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    assert(cw != 0);
    delete cw;
}

GLADE_CALLBACK void
on_calls_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_calls_call_from_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    gtk_clist_set_column_visibility(GTK_CLIST(cw->clist_), COL_FROM,
    	    	    	    	    GTK_CHECK_MENU_ITEM(w)->active);
}

GLADE_CALLBACK void
on_calls_call_to_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    gtk_clist_set_column_visibility(GTK_CLIST(cw->clist_), COL_TO,
    	    	    	    	    GTK_CHECK_MENU_ITEM(w)->active);
}

GLADE_CALLBACK void
on_calls_line_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    gtk_clist_set_column_visibility(GTK_CLIST(cw->clist_), COL_LINE,
    	    	    	    	    GTK_CHECK_MENU_ITEM(w)->active);
}

GLADE_CALLBACK void
on_calls_arc_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    gtk_clist_set_column_visibility(GTK_CLIST(cw->clist_), COL_ARC,
    	    	    	    	    GTK_CHECK_MENU_ITEM(w)->active);
}

GLADE_CALLBACK void
on_calls_count_check_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    
    gtk_clist_set_column_visibility(GTK_CLIST(cw->clist_), COL_COUNT,
    	    	    	    	    GTK_CHECK_MENU_ITEM(w)->active);
}

GLADE_CALLBACK void
on_calls_from_function_entry_changed(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);

    cw->update();
}

GLADE_CALLBACK void
on_calls_to_function_entry_changed(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);

    cw->update();
}

GLADE_CALLBACK void
on_calls_from_function_view_clicked(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    cov_function_t *fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->from_function_combo_));
    sourcewin_t::show_function(fn);
}

GLADE_CALLBACK void
on_calls_to_function_view_clicked(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_t::from_widget(w);
    cov_function_t *fn = (cov_function_t *)ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->to_function_combo_));
    sourcewin_t::show_function(fn);
}

GLADE_CALLBACK void
on_calls_clist_button_press_event(GtkWidget *w, GdkEvent *event, gpointer data)
{
    int row, col;
    cov_arc_t *a;
    const cov_location_t *loc;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(GTK_CLIST(w),
	    	    	    	     (int)event->button.x,
				     (int)event->button.y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "on_calls_clist_button_press_event: row=%d col=%d\n",
	    	    	row, col);
#endif
	a = (cov_arc_t *)gtk_clist_get_row_data(GTK_CLIST(w), row);
	
	loc = a->get_from_location();
	if (loc != 0)
	    sourcewin_t::show_lines(loc->filename, loc->lineno, loc->lineno);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
