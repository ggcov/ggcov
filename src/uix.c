/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@users.sourceforge.net>
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

#include "uix.h"
#include <gdk/gdkx.h>	/* This is what we want to avoid in the main code */

CVSID("$Id: uix.c,v 1.8 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static XFontProp *
xfont_get_prop(Display *display, XFontStruct *xfont, const char *name)
{
    Atom atom;
    int i;
    
    if ((atom = XInternAtom(display, name, /*only_if_exists*/True)) == None)
    	return 0;

    for (i = 0 ; i < xfont->n_properties ; i++)
    {
    	XFontProp *fprop = &xfont->properties[i];
	if (fprop->name == atom)
	    return fprop;
    }
    return 0;
}


static char *
xfont_get_string_prop(Display *display, XFontStruct *xfont, const char *name)
{
    XFontProp *fprop;
    
    if ((fprop = xfont_get_prop(display, xfont, name)) == 0)
    	return 0;
    return XGetAtomName(display, fprop->card32);
}

static unsigned long
xfont_get_integer_prop(Display *display, XFontStruct *xfont, const char *name)
{
    XFontProp *fprop;
    
    if ((fprop = xfont_get_prop(display, xfont, name)) == 0)
    	return 0;
    return fprop->card32;
}

/* -B&H-Lucida-Medium-R-Normal-Sans-12-120-75-75-P-71-ISO8859-1 */
typedef struct
{
    char *family;
    char *weight;
    char *slant;
    unsigned long pixel_size;
    unsigned long resolution;
    char *spacing;
    char *charset_registry;
    char *charset_encoding;
} uix_font_desc_t;

static void
uix_font_desc_get(uix_font_desc_t *desc, Display *display, XFontStruct *xfont)
{
    desc->family = xfont_get_string_prop(display, xfont, "FAMILY_NAME");
    desc->weight = xfont_get_string_prop(display, xfont, "WEIGHT_NAME");
    desc->slant = xfont_get_string_prop(display, xfont, "SLANT");
    desc->pixel_size = xfont_get_integer_prop(display, xfont, "PIXEL_SIZE");
    desc->resolution = xfont_get_integer_prop(display, xfont, "RESOLUTION_X");
    desc->spacing = xfont_get_string_prop(display, xfont, "SPACING");
    desc->charset_registry = xfont_get_string_prop(display, xfont,
    	    	    	    	    	    	    "CHARSET_REGISTRY");
    desc->charset_encoding = xfont_get_string_prop(display, xfont,
    	    	    	    	    	    	    "CHARSET_ENCODING");
}

static void
uix_font_desc_free(uix_font_desc_t *desc)
{
    if (desc->family != 0) XFree(desc->family);
    if (desc->weight != 0) XFree(desc->weight);
    if (desc->slant != 0) XFree(desc->slant);
    if (desc->spacing != 0) XFree(desc->spacing);
    if (desc->charset_registry != 0) XFree(desc->charset_registry);
    if (desc->charset_encoding != 0) XFree(desc->charset_encoding);
}

#define safestr(s)  ((s) == 0 ? "" : (s))

static GdkFont *
uix_font_desc_load(const uix_font_desc_t *desc)
{
    GdkFont *font;
    char *fontname;
    static const char anything[] = "*";
    int n;
    const char *fields[16];
    char sizebuf[16];
    char resbuf[16];
    
    /* -B&H-Lucida-Medium-R-Normal-Sans-12-120-75-75-P-71-ISO8859-1 */
    n = 0;
    fields[n++] = "";	/* to get initial seperator */
    fields[n++] = /*foundry*/anything;
    fields[n++] = desc->family;
    fields[n++] = safestr(desc->weight);
    fields[n++] = safestr(desc->slant);
    fields[n++] = /*setwidth*/anything;
    fields[n++] = /*addstyle*/anything;
    snprintf(sizebuf, sizeof(sizebuf), "%ld", desc->pixel_size);
    fields[n++] = sizebuf;
    fields[n++] = /*pointsize*/anything;
    snprintf(resbuf, sizeof(resbuf), "%ld", desc->resolution);
    fields[n++] = resbuf;   /* resolutionx */
    fields[n++] = resbuf;   /* resolutiony */
    fields[n++] = desc->spacing;
    fields[n++] = /*avgwidth*/anything;
    fields[n++] = safestr(desc->charset_registry);
    fields[n++] = safestr(desc->charset_encoding);
    fields[n++] = 0;
    assert(n == sizeof(fields)/sizeof(fields[0]));
    
    fontname = g_strjoinv("-", (char **)fields);

    dprintf1(D_UICORE, "uix_font_desc_load: trying \"%s\"\n", fontname);

    font = gdk_font_load(fontname);
    
    g_free(fontname);
    
    return font;
}

static GdkFont *
uix_find_fixed(const uix_font_desc_t *origdesc)
{
    uix_font_desc_t tmpdesc;
    int i;
    GdkFont *newfont;
    static const char *families[] =
    	{
	    "lucidatypewriter",
	    "lucidasanstypewriter",
	    "courier",
	    "fixed",
	    0
	};

    tmpdesc = *origdesc;
    tmpdesc.spacing = (char *)"M";
    
    /* first try everything the same except the spacing -- we might get lucky */
    if ((newfont = uix_font_desc_load(&tmpdesc)) != 0)
    	return newfont;

    /* try the well known monospaced families */
    for (i = 0 ; families[i] != 0 ; i++)
    {
    	tmpdesc.family = (char *)families[i];
	if ((newfont = uix_font_desc_load(&tmpdesc)) != 0)
    	    return newfont;
    }

    /* set some fields to safe defaults and try the well-known families again */
    tmpdesc.weight = (char *)"Medium";
    tmpdesc.slant = (char *)"R";
    for (i = 0 ; families[i] != 0 ; i++)
    {
    	tmpdesc.family = (char *)families[i];
	if ((newfont = uix_font_desc_load(&tmpdesc)) != 0)
    	    return newfont;
    }
    
    return 0;
}


GdkFont *
uix_fixed_width_font(GdkFont *goldfont)
{
    Display *display = GDK_FONT_XDISPLAY(goldfont);
    XFontStruct *oldfont = (XFontStruct *)GDK_FONT_XFONT(goldfont);
    GdkFont *newfont = 0;
    uix_font_desc_t origdesc;

#if 0
    printf("font properties:\n");
    for (i = 0 ; i < oldfont->n_properties ; i++)
    {
    	XFontProp *fprop = &oldfont->properties[i];
	
	printf("name=%s value=%ld (%s)\n",
	    	XGetAtomName(display, fprop->name),
		fprop->card32,
	    	XGetAtomName(display, fprop->card32));
    }
#endif

    uix_font_desc_get(&origdesc, display, oldfont);
    
    if (strcasecmp(origdesc.spacing, "M"))
    	newfont = uix_find_fixed(&origdesc);
        
    uix_font_desc_free(&origdesc);
    return (newfont == 0 ? goldfont : newfont);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
uix_font_width(GdkFont *gfont)
{
    XFontStruct *xfont = (XFontStruct *)GDK_FONT_XFONT(gfont);
    return xfont->max_bounds.width;
}

int
uix_font_height(GdkFont *gfont)
{
    XFontStruct *xfont = (XFontStruct *)GDK_FONT_XFONT(gfont);
    return xfont->ascent + xfont->descent;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* Called when we receive some error events from the X server
 * (not things like BadFont from XLookupFont though) */
static int
x_error_handler(Display *display, XErrorEvent *event)
{
    char errmsg[1024];
    strncpy(errmsg, "<unknown>", sizeof(errmsg)-1);
    XGetErrorText(display, event->error_code, errmsg, sizeof(errmsg));
    fprintf(stderr, "ggcov: %s error from X server, resource %lu serial %lu request %u/%u\n",
	    errmsg, event->resourceid, event->serial,
	    event->request_code, event->minor_code);
    fflush(stderr);
    return 0;	/* return value igored */
}

/* Called when we lose connection to the X server */
static int
x_io_error_handler(Display *display)
{
    fprintf(stderr, "ggcov: lost connection to the X server!!\n");
    fflush(stderr);
    return 0;	/* return value igored */
    /* Xlib will exit() */
}

void
uix_log_init(void)
{
    XSetErrorHandler(x_error_handler);
    XSetIOErrorHandler(x_io_error_handler);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
