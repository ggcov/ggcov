/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
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

#include "estring.H"
#include <stdarg.h>

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define AVAIL_ROUND         256

void inline
estring::expand_by(unsigned int dl)
{
   if (length_ + dl + 1 > available_)
   {
	available_ = ((length_ + dl + 1) + AVAIL_ROUND-1) & ~(AVAIL_ROUND-1);
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
estring::append_vprintf(const char *fmt, va_list args)
{
    va_list args2;
    int len;

#if HAVE_VA_COPY
    va_copy(args2, args);
#else
    args2 = args;
#endif

    /* ensure enough space exists for result, possibly too much */
    len = g_printf_string_upper_bound(fmt, args);
    expand_by(len);

    /* format the string into the new space */
    vsprintf(data_+length_, fmt, args2);
    length_ += strlen(data_+length_);
}

void
estring::append_printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    append_vprintf(fmt, args);
    va_end(args);
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
estring::insert_vprintf(unsigned int start, const char *fmt, va_list args)
{
    replace_vprintf(start, 0, fmt, args);
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
    else if (len > length_)
    {
	expand_by(len - length_);
	memset(data_+length_, 0, len - length_);
	length_ = len;
	data_[length_] = '\0';
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

//static inline gboolean is_crnl(int c)   { return (c == '\n' || c == '\r'); }

void
estring::chomp()
{
    while (length_ > 0 && isspace(data_[length_-1]))
	data_[--length_] = '\0';
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
estring::find_char(int c) const
{
    const char *p = (data_ ? strchr(data_, c) : 0);
    return (p ? (p - data_) : -1);
}

int
estring::find_string(const char *s) const
{
    const char *p = (data_ ? strstr(data_, s) : 0);
    return (p ? (p - data_) : -1);
}

int
estring::find_last_char(int c) const
{
    const char *p = (data_ ? strrchr(data_, c) : 0);
    return (p ? (p - data_) : -1);
}

int
estring::find_last_string(const char *s) const
{
    /* libc doesn't have a strrstr(), we have to do this stupidly */
    if (!data_)
	return -1;
    int slen = strlen(s);
    const char *p;
    for (p = data_+length_-1 ; p >= data_ ; --p)
    {
	if (*p == *s && !strncmp(p, s, slen))
	    return (p - data_);
    }
    return -1;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
