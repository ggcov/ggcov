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

#include "callswin.H"
#include "sourcewin.H"
#include "filename.h"
#include "cov.h"
#include "estring.h"

CVSID("$Id: callswin.C,v 1.1 2002-12-15 15:53:23 gnb Exp $");

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
    	return ullcmp(a1->from->count, a2->from->count);
	
    case COL_ARC:
#define arcno(a)    (((a)->from->idx << 16)|(a)->idx)
    	return ulcmp(arcno(a1), arcno(a2));
#undef arcno
	
    case COL_TO:
    	return strcmp(safestr(a1->name), safestr(a2->name));
	
    case COL_FROM:
    	return strcmp(safestr(a1->from->function->name),
	    	      safestr(a2->from->function->name));
	
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

    estring_init(&label);
    for (iter = functions_ ; iter != 0 ; iter = iter->next)
    {
    	cov_function_t *fn = (cov_function_t *)iter->data;

    	estring_truncate(&label);
	estring_append_string(&label, fn->name);

    	/* see if we need to present some more scope to uniquify the name */
	if ((iter->next != 0 &&
	     !strcmp(((cov_function_t *)iter->next->data)->name, fn->name)) ||
	    (iter->prev != 0 &&
	     !strcmp(((cov_function_t *)iter->prev->data)->name, fn->name)))
	{
	    estring_append_string(&label, " (");
	    estring_append_string(&label, file_basename_c(fn->file->name));
	    estring_append_string(&label, ")");
	}
	
    	ui_combo_add_data(combo, label.data, fn);
    }
    estring_free(&label);
}

void
callswin_t::populate()
{
#if DEBUG
    fprintf(stderr, "callswin_t::populate\n");
#endif
    functions_ = cov_list_functions();
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
    GList *aiter;
    char *text[COL_NUM];
    char countbuf[32];
    char arcbuf[64];
    char linebuf[32];

    for (bidx = 0 ; bidx < cov_function_num_blocks(from_fn)-2 ; bidx++)
    {
    	cov_block_t *b = cov_function_nth_block(from_fn, bidx);

	for (aiter = b->out_arcs ; aiter != 0 ; aiter = aiter->next)
	{
	    cov_arc_t *a = (cov_arc_t *)aiter->data;
    	    
	    if (!a->fake)
	    	continue;
	    if (to_fn != 0 &&
	    	(a->name == 0 || strcmp(to_fn->name, a->name)))
	    	continue;
		
	    snprintf(countbuf, sizeof(countbuf), "%llu", a->from->count);
	    text[COL_COUNT] = countbuf;

	    snprintf(arcbuf, sizeof(arcbuf), "B%u A%u", b->idx, a->idx);
	    text[COL_ARC] = arcbuf;
	    
	    snprintf(linebuf, sizeof(linebuf), "%u",
	    	    cov_arc_get_from_location(a)->lineno);
	    text[COL_LINE] = linebuf;
	    
	    text[COL_FROM] = a->from->function->name;
	    if ((text[COL_TO] = a->name) == 0)
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
    estring_init(&title);
    switch ((from_fn == 0 ? 0 : 2)|(to_fn == 0 ? 0 : 1))
    {
    case 0:
	break;
    case 1:
    	estring_append_printf(&title, _("to %s"), to_fn->name);
	break;
    case 2:
    	estring_append_printf(&title, _("from %s"), from_fn->name);
	break;
    case 3:
    	estring_append_printf(&title, _("from %s to %s"),
	    	    	      from_fn->name, to_fn->name);
	break;
    }
    set_title(title.data);
    estring_free(&title);


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
    callswin_t *cw = callswin_t::from_widget(w);
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
	
	loc = cov_arc_get_from_location(a);
	sourcewin_t::show_lines(loc->filename, loc->lineno, loc->lineno);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
