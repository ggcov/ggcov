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

#ifndef _estring_h_
#define _estring_h_

#include "common.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifdef __GNUC__
#define _printf_fmt(fmtidx,firsta) \
    __attribute (( format(printf, fmtidx, firsta) ))
#else
#define _printf_fmt(fmtidx,firsta)
#endif


typedef struct
{
    char *data;
    int length;		/* string length not including nul */
    int available;	/* total bytes available, >= length+1 */
} estring;

void estring_init(estring *e);

void estring_append_string(estring *e, const char *str);
void estring_append_char(estring *e, char c);
void estring_append_chars(estring *e, const char *buf, unsigned int buflen);
void estring_append_printf(estring *e, const char *fmt, ...) _printf_fmt(2,3);

void estring_replace_string(estring *e, unsigned int start, unsigned int len,
    	    	    	    const char *str);
void estring_replace_char(estring *e, unsigned int start, unsigned int len,
    	    	    	    char c);
void estring_replace_chars(estring *e, unsigned int start, unsigned int len,
    	    	    	    const char *buf, unsigned int buflen);
void estring_replace_printf(estring *e, unsigned int start, unsigned int len,
    	    	    	    const char *fmt, ...) _printf_fmt(4,5);

void estring_insert_string(estring *e, unsigned int start, const char *str);
void estring_insert_char(estring *e, unsigned int start, char c);
void estring_insert_chars(estring *e, unsigned int start, const char *buf, int len);
void estring_insert_printf(estring *e, unsigned int start, const char *fmt, ...)
    	    	    	    _printf_fmt(3,4);

void estring_remove(estring *e, unsigned int start, unsigned int len);

void estring_truncate(estring *e);
void estring_truncate_to(estring *e, unsigned int len);
void estring_free(estring *e);

#define ESTRING_STATIC_INIT \
	{ 0, 0, 0 }

#undef _printf_fmt

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _estring_h_ */
