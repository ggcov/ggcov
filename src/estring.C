/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
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

#include "estring.H"
#include <stdarg.h>

CVSID("$Id: estring.C,v 1.2 2003-03-17 03:54:49 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void inline
estring::expand_by(unsigned int dl)
{
   if (length_ + dl + 1 > available_)
   {
   	available_ += MIN(1024, (length_ + dl + 1 - available_));
   	data_ = (data_ == 0 ?
	    	    g_new(char, available_) :
		    g_renew(char, data_, available_));
   }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
estring::append_string(const char *str)
{
    if (str != 0 && *str != '\0')
	append_chars(str, strlen(str));
}

void
estring::append_char(char c)
{
    expand_by(1);
    data_[length_++] = c;
    data_[length_] = '\0';
}

void
estring::append_chars(const char *buf, unsigned int buflen)
{
    expand_by(buflen);
    memcpy(&data_[length_], buf, buflen);
    length_ += buflen;
    data_[length_] = '\0';
}

void
estring::append_printf(const char *fmt, ...)
{
    va_list args;
    int len;

    /* ensure enough space exists for result, possibly too much */    
    va_start(args, fmt);
    len = g_printf_string_upper_bound(fmt, args);
    va_end(args);
    expand_by(len);

    /* format the string into the new space */    
    va_start(args, fmt);
    vsprintf(data_+length_, fmt, args);
    va_end(args);
    length_ += strlen(data_+length_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
estring::replace_string(
    unsigned int start,
    unsigned int len,
    const char *str)
{
    if (str == 0)
    	str = "";
    replace_chars(start, len, str, strlen(str));
}

void
estring::replace_char(
    unsigned int start,
    unsigned int len,
    char c)
{
    replace_chars(start, len, &c, 1);
}

void
estring::replace_chars(
    unsigned int start,
    unsigned int len,
    const char *buf,
    unsigned int buflen)
{
    unsigned int remain;
    
#if DEBUG > 40
    fputs("estring::replace_chars: replacing \"", stderr);
    if (data_ != 0)
	fwrite(data_+start, 1, len, stderr);
    fputs("\" -> \"", stderr);    
    if (buf != 0)
	fwrite(buf, 1, buflen, stderr);
    fputs("\"\n", stderr);    
#endif

    if (buflen > len)
    {
	expand_by(buflen-len);
    }

    if ((remain = length_ - (start+len)) > 0)
    {
    	/* have to move some chars at the end, up or down */
	memmove(data_+start+buflen, data_+start+len, remain);
    }
    
    /* insert new chars */
    if (buflen > 0)
	memmove(data_+start, (char*)buf, buflen);

    /* update length_ */
    if (buflen >= len)
	length_ += (buflen - len);
    else
	length_ -= (len - buflen);
    /* ensure there's a nul char at the right place */
    data_[length_] = '\0';
}

void
estring::replace_vprintf(
    unsigned int start,
    unsigned int len,
    const char *fmt, va_list args)
{
    char *str;

    str = g_strdup_printf(fmt, args);
    replace_chars(start, len, str, strlen(str));
    g_free(str);
}

void
estring::replace_printf(
    unsigned int start,
    unsigned int len,
    const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    replace_vprintf(start, len, fmt, args);
    va_end(args);
}

void
estring::replace_all(const char *from, const char *to)
{
    char *p;
    int i;

    if (to == 0)
    	to = "";
    i = 0;
    while ((p = strstr(data_+i, from)) != 0)
    {
    	i = (p - data_);
    	replace_string(i, strlen(from), to);
	i += strlen(to);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
estring::insert_string(unsigned int start, const char *str)
{
    replace_string(start, 0, str);
}

void
estring::insert_char(unsigned int start, char c)
{
    replace_char(start, 0, c);
}

void
estring::insert_chars(unsigned int start, const char *buf, int buflen)
{
    replace_chars(start, 0, buf, buflen);
}

void
estring::insert_printf(unsigned int start, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    replace_vprintf(start, 0, fmt, args);
    va_end(args);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
estring::remove(unsigned int start, unsigned int len)
{
    replace_chars(start, len, 0, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
estring::truncate()
{
    length_ = 0;
    if (data_ != 0)
	data_[0] = '\0';
}

void
estring::truncate_to(unsigned int len)
{
    if (len < length_)
    {
	length_ = len;
	if (data_ != 0)
	    data_[length_] = '\0';
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
