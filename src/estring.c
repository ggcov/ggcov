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

CVSID("$Id: estring.c,v 1.2 2001-11-23 09:07:13 gnb Exp $");

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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

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
estring_append_chars(estring *e, const char *buf, unsigned int buflen)
{
    _estring_expand_by(e, buflen);
    memcpy(&e->data[e->length], buf, buflen);
    e->length += buflen;
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
estring_replace_string(
    estring *e,
    unsigned int start,
    unsigned int len,
    const char *str)
{
    if (str == 0)
    	str = "";
    estring_replace_chars(e, start, len, str, strlen(str));
}

void
estring_replace_char(
    estring *e,
    unsigned int start,
    unsigned int len,
    char c)
{
    estring_replace_chars(e, start, len, &c, 1);
}

void
estring_replace_chars(
    estring *e,
    unsigned int start,
    unsigned int len,
    const char *buf,
    unsigned int buflen)
{
    unsigned int remain;
    
#if DEBUG > 40
    fputs("estring_replace_chars: replacing \"", stderr);
    if (e->data != 0)
	fwrite(e->data+start, 1, len, stderr);
    fputs("\" -> \"", stderr);    
    if (buf != 0)
	fwrite(buf, 1, buflen, stderr);
    fputs("\"\n", stderr);    
#endif

    if (buflen > len)
    {
	_estring_expand_by(e, buflen-len);
    }

    if ((remain = e->length - (start+len)) > 0)
    {
    	/* have to move some chars at the end, up or down */
	memmove(e->data+start+buflen, e->data+start+len, remain);
    }
    
    /* insert new chars */
    if (buflen > 0)
	memmove(e->data+start, (char*)buf, buflen);

    /* update e->length */
    if (buflen >= len)
	e->length += (buflen - len);
    else
	e->length -= (len - buflen);
    /* ensure there's a nul char at the right place */
    e->data[e->length] = '\0';
}

static void
estring_replace_vprintf(
    estring *e,
    unsigned int start,
    unsigned int len,
    const char *fmt, va_list args)
{
    char *str;

    str = g_strdup_printf(fmt, args);
    estring_replace_chars(e, start, len, str, strlen(str));
    g_free(str);
}

void
estring_replace_printf(
    estring *e,
    unsigned int start,
    unsigned int len,
    const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    estring_replace_vprintf(e, start, len, fmt, args);
    va_end(args);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
estring_insert_string(estring *e, unsigned int start, const char *str)
{
    estring_replace_string(e, start, 0, str);
}

void
estring_insert_char(estring *e, unsigned int start, char c)
{
    estring_replace_char(e, start, 0, c);
}

void
estring_insert_chars(estring *e, unsigned int start, const char *buf, int buflen)
{
    estring_replace_chars(e, start, 0, buf, buflen);
}

void
estring_insert_printf(estring *e, unsigned int start, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    estring_replace_vprintf(e, start, 0, fmt, args);
    va_end(args);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
estring_truncate(estring *e)
{
    e->length = 0;
    if (e->data != 0)
	e->data[0] = '\0';
}

void
estring_truncate_to(estring *e, unsigned int len)
{
    if (len < e->length)
    {
	e->length = len;
	if (e->data != 0)
	    e->data[e->length] = '\0';
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

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
