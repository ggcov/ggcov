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

#include "callswin.h"
#include "sourcewin.h"
#include "cov.h"
#include "estring.h"

CVSID("$Id: callswin.c,v 1.1 2001-12-02 07:28:05 gnb Exp $");

#define COL_COUNT   0
#define COL_ARC     1
#define COL_NAME    2
#define COL_MAX     3

static const char callswin_window_key[] = "callswin_key";

static void callswin_populate(callswin_t*);
static void callswin_update(callswin_t*);


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
	
    case COL_NAME:
    	return strcmp(safestr(a1->name), safestr(a2->name));
	
    default:
	return 0;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callswin_t *
callswin_new(void)
{
    callswin_t *cw;
    GladeXML *xml;
    
    cw = new(callswin_t);

    /* load the interface & connect signals */
    xml = ui_load_tree("calls");
    
    cw->window = glade_xml_get_widget(xml, "calls");
    ui_register_window(cw->window);
    
    cw->function_combo = glade_xml_get_widget(xml, "calls_function_combo");
    cw->function_view = glade_xml_get_widget(xml, "calls_function_view");

    cw->clist = glade_xml_get_widget(xml, "calls_clist");
    gtk_clist_column_titles_passive(GTK_CLIST(cw->clist));
    ui_clist_init_column_arrow(GTK_CLIST(cw->clist), COL_COUNT);
    ui_clist_init_column_arrow(GTK_CLIST(cw->clist), COL_ARC);
    ui_clist_init_column_arrow(GTK_CLIST(cw->clist), COL_NAME);
    gtk_clist_set_compare_func(GTK_CLIST(cw->clist), callswin_compare);
    ui_clist_set_sort_column(GTK_CLIST(cw->clist), COL_ARC);
    ui_clist_set_sort_type(GTK_CLIST(cw->clist), GTK_SORT_ASCENDING);
    
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "calls_windows_dummy"));

    gtk_object_set_data(GTK_OBJECT(cw->window), callswin_window_key, cw);
    
    callswin_populate(cw);
    callswin_update(cw);
    gtk_widget_show(cw->window);
    
    return cw;
}

void
callswin_delete(callswin_t *cw)
{
    /* JIC of strange gui stuff */
    if (cw->deleting)
    	return;
    cw->deleting = TRUE;
    
    gtk_widget_destroy(cw->window);
    g_free(cw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static callswin_t *
callswin_from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : gtk_object_get_data(GTK_OBJECT(w), callswin_window_key));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* 
 * TODO: this needs to be common code, it's used in at least 2 places
 */
static int
compare_functions(const void *a, const void *b)
{
    const cov_function_t *fa = (const cov_function_t *)a;
    const cov_function_t *fb = (const cov_function_t *)b;
    int ret;
    
    ret = strcmp(fa->name, fb->name);
    if (ret == 0)
    	ret = strcmp(fa->file->name, fb->file->name);
    return ret;
}

static void
add_functions(cov_file_t *f, void *userdata)
{
    GList **listp = (GList **)userdata;
    unsigned fnidx;
    
    for (fnidx = 0 ; fnidx < f->functions->len ; fnidx++)
    {
    	cov_function_t *fn = cov_file_nth_function(f, fnidx);
	
	if (!strncmp(fn->name, "_GLOBAL_", 8))
	    continue;
	*listp = g_list_prepend(*listp, fn);
    }
}

static void
calls_populate_function_combo(GtkCombo *combo)
{
    GList *list = 0, *iter;
    estring label;
    
    cov_file_foreach(add_functions, &list);
    list = g_list_sort(list, compare_functions);
    
    estring_init(&label);
    for (iter = list ; iter != 0 ; iter = iter->next)
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
	    estring_append_string(&label, fn->file->name);
	    estring_append_string(&label, ")");
	}
	
    	ui_combo_add_data(combo, label.data, fn);
    }
    estring_free(&label);
    
    while (list != 0)
	list = g_list_remove_link(list, list);
}

static void
callswin_populate(callswin_t *cw)
{
#if DEBUG
    fprintf(stderr, "callswin_populate\n");
#endif
    
    calls_populate_function_combo(GTK_COMBO(cw->function_combo));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
callswin_update(callswin_t *cw)
{
    cov_function_t *fn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->function_combo));
    unsigned int bidx;
    int row;
    GList *aiter;
    char *text[COL_MAX];
    char countbuf[32];
    char arcbuf[64];
    
#if DEBUG
    fprintf(stderr, "callswin_update\n");
#endif
    ui_window_set_title(cw->window, fn->name);

    gtk_clist_freeze(GTK_CLIST(cw->clist));
    gtk_clist_clear(GTK_CLIST(cw->clist));
    
    for (bidx = 0 ; bidx < fn->blocks->len-2 ; bidx++)
    {
    	cov_block_t *b = cov_function_nth_block(fn, bidx);

	for (aiter = b->out_arcs ; aiter != 0 ; aiter = aiter->next)
	{
	    cov_arc_t *a = (cov_arc_t *)aiter->data;
    	    
	    if (!a->fake)
	    	continue;
		
	    snprintf(countbuf, sizeof(countbuf), "%llu", a->from->count);
	    text[COL_COUNT] = countbuf;

	    snprintf(arcbuf, sizeof(arcbuf), "B%u A%u", b->idx, a->idx);
	    text[COL_ARC] = arcbuf;
	    
	    if ((text[COL_NAME] = a->name) == 0)
	    	text[COL_NAME] = "(unknown)";
		
	    row = gtk_clist_append(GTK_CLIST(cw->clist), text);
	    gtk_clist_set_row_data(GTK_CLIST(cw->clist), row, a);
	}
    }
    
    gtk_clist_columns_autosize(GTK_CLIST(cw->clist));
    gtk_clist_thaw(GTK_CLIST(cw->clist));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: this needs to be common code too */
static void
callswin_show_source(
    const char *filename,
    unsigned long startline,
    unsigned long endline)
{
    sourcewin_t *srcw = sourcewin_new();
    
    sourcewin_set_filename(srcw, filename);
    if (startline > 0)
    {
	sourcewin_ensure_visible(srcw, startline);
	sourcewin_select_region(srcw, startline, endline);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_calls_close_activate(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_from_widget(w);
    
    if (cw != 0)
	callswin_delete(cw);
}

GLADE_CALLBACK void
on_calls_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

GLADE_CALLBACK void
on_calls_function_entry_changed(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_from_widget(w);

    callswin_update(cw);    
}

GLADE_CALLBACK void
on_calls_function_view_clicked(GtkWidget *w, gpointer data)
{
    callswin_t *cw = callswin_from_widget(w);
    cov_function_t *fn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->function_combo));
    const cov_location_t *start, *end;

    start = cov_function_get_first_location(fn);
    end = cov_function_get_last_location(fn);

    callswin_show_source(start->filename, start->lineno, end->lineno);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
