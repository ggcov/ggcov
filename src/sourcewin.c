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

#include "sourcewin.h"
#include "cov.h"
#include "estring.h"

CVSID("$Id: sourcewin.c,v 1.1 2001-11-23 09:09:25 gnb Exp $");

extern GList *filenames;

static const char sourcewin_window_key[] = "sourcewin_key";
static gboolean sourcewin_initted = FALSE;
static GdkColor sourcewin_color0;	    /* colour for lines with count==0 */
static GdkColor sourcewin_color1;	    /* colour for lines with count>0 */

static void sourcewin_populate_filenames(sourcewin_t *sw);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
sourcewin_init(GtkWidget *w)
{
    GdkColormap *cmap;

    if (sourcewin_initted)
    	return;
    sourcewin_initted = TRUE;

    gtk_widget_realize(w);
    
    cmap = gtk_widget_get_colormap(w);

    gdk_color_parse("#c00000", &sourcewin_color0);
    gdk_colormap_alloc_color(cmap, &sourcewin_color0,
    	    	    	    	    /*writeable*/FALSE, /*best_match*/TRUE);

    gdk_color_parse("#00c000", &sourcewin_color1);
    gdk_colormap_alloc_color(cmap, &sourcewin_color1,
    	    	    	    	    /*writeable*/FALSE, /*best_match*/TRUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GtkWidget *
get_dummy_menu(GladeXML *xml, const char *name)
{
    GtkWidget *dummy;
    GtkWidget *menu;
    GtkWidget *tearoff;

    dummy = glade_xml_get_widget(xml, name);
    
    menu = dummy->parent;
    
    tearoff = gtk_tearoff_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), tearoff);
    gtk_widget_show(tearoff);
    
    return menu;
}

sourcewin_t *
sourcewin_new(void)
{
    sourcewin_t *sw;
    GladeXML *xml;
    
    sw = new(sourcewin_t);
    
    /* load the interface & connect signals */
    xml = ui_load_tree("source");
    
    sw->window = glade_xml_get_widget(xml, "source");
    ui_register_window(sw->window);
    sw->text = glade_xml_get_widget(xml, "source_text");
    sw->number_check = glade_xml_get_widget(xml, "source_number_check");
    sw->count_check = glade_xml_get_widget(xml, "source_count_check");
    sw->source_check = glade_xml_get_widget(xml, "source_source_check");
    sw->colors_check = glade_xml_get_widget(xml, "source_colors_check");
    sw->filenames_menu = get_dummy_menu(xml, "source_file_dummy");
    sw->functions_menu = get_dummy_menu(xml, "source_func_dummy");
    
    sw->title_string = g_strdup(GTK_WINDOW(sw->window)->title);
    gtk_object_set_data(GTK_OBJECT(sw->window), sourcewin_window_key, sw);
    
    sourcewin_init(sw->window);
    
    sourcewin_populate_filenames(sw);

    return sw;
}

void
sourcewin_delete(sourcewin_t *sw)
{
    /* JIC of strange gui stuff */
    if (sw->deleting)
    	return;
    sw->deleting = TRUE;
    
    gtk_widget_destroy(sw->window);
    strdelete(sw->filename);
    strdelete(sw->title_string);
    g_free(sw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static sourcewin_t *
sourcewin_from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : gtk_object_get_data(GTK_OBJECT(w), sourcewin_window_key));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
replace_all(estring *e, const char *from, const char *to)
{
    char *p;

    while ((p = strstr(e->data, from)) != 0)
    	estring_replace_string(e, (p - e->data), strlen(from), to);
}

/* Replace @FILE@ and @VERSION@ in the title string */
static void
sourcewin_set_title(sourcewin_t *sw, const char *filename)
{
    estring title;
    
    estring_init(&title);
    
    estring_append_string(&title, sw->title_string);
    replace_all(&title, "@FILE@", filename);
    replace_all(&title, "@VERSION@", VERSION);
    
    gtk_window_set_title(GTK_WINDOW(sw->window), title.data);
        
    estring_free(&title);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
on_source_filename_activate(GtkWidget *w, gpointer userdata)
{
    sourcewin_t *sw = sourcewin_from_widget(w);
    const char *filename = (const char *)userdata;
    
    sourcewin_set_filename(sw, filename);
}


static void
sourcewin_populate_filenames(sourcewin_t *sw)
{
    GtkWidget *butt;
    GList *iter;
    
    ui_delete_menu_items(sw->filenames_menu);
    
    for (iter = filenames ; iter != 0 ; iter = iter->next)
    {
    	const char *filename = (const char *)iter->data;
	
	butt = gtk_menu_item_new_with_label(filename);
	gtk_menu_append(GTK_MENU(sw->filenames_menu), butt);
	gtk_signal_connect(GTK_OBJECT(butt), "activate", 
    			   GTK_SIGNAL_FUNC(on_source_filename_activate),
			   (gpointer)filename);
	gtk_widget_show(butt);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
on_source_function_activate(GtkWidget *w, gpointer userdata)
{
    sourcewin_t *sw = sourcewin_from_widget(w);
    cov_function_t *fn = (cov_function_t *)userdata;
    cov_location_t *loc;
    GtkAdjustment *adj = GTK_TEXT(sw->text)->vadj;

    loc = cov_function_get_first_location(fn);
#if DEBUG
    fprintf(stderr, "Function %s -> %s:%d\n",
    	    	    	fn->name, loc->filename, loc->lineno);
#endif
    
    if (strcmp(loc->filename, sw->filename))
    {
    	fprintf(stderr, "WTF?  Wrong filename: %s vs %s\n",
	    	    	loc->filename, sw->filename);
	return;
    }
    
    /* This mostly works.  Not totally predictable but good enough for now */
    gtk_adjustment_set_value(adj,
	    	adj->upper * (float)loc->lineno / (float)sw->max_lineno
		    - adj->page_size/2.0);
}

static int
compare_functions(const void *a, const void *b)
{
    const cov_function_t *fa = (const cov_function_t *)a;
    const cov_function_t *fb = (const cov_function_t *)b;
    
    return strcmp(fa->name, fb->name);
}

static void
sourcewin_populate_functions(sourcewin_t *sw)
{
    GList *functions = 0;
    unsigned fnidx;
    cov_file_t *f;
    cov_function_t *fn;
    GtkWidget *butt;
    
    /* build an alphabetically sorted list of functions */
    f = cov_file_find(sw->filename);
    assert(f != 0);
    for (fnidx = 0 ; fnidx < f->functions->len ; fnidx++)
    {
    	fn = f->functions->pdata[fnidx];
	
	if (!strncmp(fn->name, "_GLOBAL_", 8))
	    continue;
	functions = g_list_prepend(functions, fn);
    }
    
    functions = g_list_sort(functions, compare_functions);
    
    /* now build the menu */

    ui_delete_menu_items(sw->functions_menu);
    
    while (functions != 0)
    {
    	fn = (cov_function_t *)functions->data;
	
	butt = gtk_menu_item_new_with_label(fn->name);
	gtk_menu_append(GTK_MENU(sw->functions_menu), butt);
	gtk_signal_connect(GTK_OBJECT(butt), "activate", 
    			   GTK_SIGNAL_FUNC(on_source_function_activate),
			   (gpointer)fn);
	gtk_widget_show(butt);
	
	functions = g_list_remove_link(functions, functions);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static count_t
block_list_total(const GList *list)
{
    count_t total = 0;
    
    for ( ; list != 0 ; list = list->next)
    	total += ((cov_block_t *)list->data)->count;
    return total;
}

static void
sourcewin_update_source(sourcewin_t *sw)
{
    FILE *fp;
    unsigned int lineno;
    count_t count;
    gboolean have_count;
    GtkText *text = GTK_TEXT(sw->text);
    int i, nstrs;
    GdkColor *color;
    char *strs[4];
    char linenobuf[32];
    char countbuf[32];
    char linebuf[1024];
    
    if ((fp = fopen(sw->filename, "r")) == 0)
    {
    	/* TODO: gui error report */
    	perror(sw->filename);
	return;
    }


    gtk_text_freeze(text);
    gtk_editable_delete_text(GTK_EDITABLE(text), 0, -1);
    
    sw->max_lineno = 0;

    lineno = 0;
    while (fgets(linebuf, sizeof(linebuf), fp) != 0)
    {
    	++lineno;
	
    	/* setup `count', `have_count' */
	{
	    const GList *blocks = cov_blocks_find_by_location(sw->filename, lineno);
	    have_count = FALSE;
	    if (blocks != 0)
	    {
		count = block_list_total(blocks);
		have_count = TRUE;
	    }
	}

    	/* choose colours */
	color = 0;
	if (GTK_CHECK_MENU_ITEM(sw->colors_check)->active && have_count)
	    color = (count ? &sourcewin_color1 : &sourcewin_color0);

    	/* generate strings */
	
	nstrs = 0;
	
	if (GTK_CHECK_MENU_ITEM(sw->number_check)->active)
	{
	    snprintf(linenobuf, sizeof(linenobuf), "%7u ", lineno);
	    strs[nstrs++] = linenobuf;
	}
	
	if (GTK_CHECK_MENU_ITEM(sw->count_check)->active)
	{
	    if (have_count)
	    {
		if (count)
		{
		    snprintf(countbuf, sizeof(countbuf), "%12lld    ", count);
		    strs[nstrs++] = countbuf;
		}
		else
		    strs[nstrs++] = "      ######    ";
	    }
	    else
		strs[nstrs++] = "                ";
    	}

	if (GTK_CHECK_MENU_ITEM(sw->source_check)->active)
	    strs[nstrs++] = linebuf;
	else
	    strs[nstrs++] = "\n";

    	for (i = 0 ; i < nstrs ; i++)
	{
    	    gtk_text_insert(text, /*font*/0,
	    		    color, /*back*/0,
			    strs[i], strlen(strs[i]));
    	}
    }
    
    sw->max_lineno = lineno;
    fclose(fp);
    gtk_text_thaw(text);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
sourcewin_set_filename(sourcewin_t *sw, const char *filename)
{
    sourcewin_set_title(sw, filename);
    strassign(sw->filename, filename);
    sourcewin_update_source(sw);
    sourcewin_populate_functions(sw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_source_close_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_from_widget(w);
    
    if (sw != 0)
	sourcewin_delete(sw);
}

GLADE_CALLBACK void
on_source_count_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_from_widget(w);
    
    sourcewin_update_source(sw);
}

GLADE_CALLBACK void
on_source_number_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_from_widget(w);
    
    sourcewin_update_source(sw);
}

GLADE_CALLBACK void
on_source_source_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_from_widget(w);
    
    sourcewin_update_source(sw);
}

GLADE_CALLBACK void
on_source_colors_check_activate(GtkWidget *w, gpointer data)
{
    sourcewin_t *sw = sourcewin_from_widget(w);
    
    sourcewin_update_source(sw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
