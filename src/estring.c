/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
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

#include "estring.h"
#include <stdarg.h>
#include <sys/stat.h>

CVSID("$Id: estring.c,v 1.1 2001-11-23 03:47:48 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


void
estring_init(estring *e)
{
    e->data = 0;
    e->length = 0;
    e->available = 0;
}

#define _estring_expand_by(e, dl) \
   if ((e)->length + (dl) + 1 > (e)->available) \
   { \
   	(e)->available += MIN(1024, ((e)->length + (dl) + 1 - (e)->available)); \
   	(e)->data = ((e)->data == 0 ? g_new(char, (e)->available) : g_renew(char, (e)->data, (e)->available)); \
   }

void
estring_append_string(estring *e, const char *str)
{
    if (str != 0 && *str != '\0')
	estring_append_chars(e, str, strlen(str));
}

void
estring_append_char(estring *e, char c)
{
    _estring_expand_by(e, 1);
    e->data[e->length++] = c;
    e->data[e->length] = '\0';
}

void
estring_append_chars(estring *e, const char *buf, int len)
{
    _estring_expand_by(e, len);
    memcpy(&e->data[e->length], buf, len);
    e->length += len;
    e->data[e->length] = '\0';
}

void
estring_append_printf(estring *e, const char *fmt, ...)
{
    va_list args;
    int len;

    /* ensure enough space exists for result, possibly too much */    
    va_start(args, fmt);
    len = g_printf_string_upper_bound(fmt, args);
    va_end(args);
    _estring_expand_by(e, len);

    /* format the string into the new space */    
    va_start(args, fmt);
    vsprintf(e->data+e->length, fmt, args);
    va_end(args);
    e->length += strlen(e->data+e->length);
}

void
estring_truncate(estring *e)
{
    e->length = 0;
    if (e->data != 0)
	e->data[0] = '\0';
}

void
estring_free(estring *e)
{
   if (e->data != 0)
   {
   	g_free(e->data);
	e->data = 0;
	e->length = 0;
	e->available = 0;
   }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
