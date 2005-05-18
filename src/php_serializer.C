/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@alphalink.com.au>
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

#include "php_serializer.H"

CVSID("$Id: php_serializer.C,v 1.1 2005-05-18 12:53:45 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

php_serializer_t::php_serializer_t()
{
    array_depth_ = 0;
}

php_serializer_t::~php_serializer_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
php_serializer_t::presize(unsigned int bytelength)
{
    // TODO
}

const estring &
php_serializer_t::data() const
{
    assert(array_depth_ == 0);
    return buf_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
php_serializer_t::array_element()
{
    if (array_depth_ > 0)
    {
	array_t *a = &arrays_[array_depth_-1];
	a->seen_++;
	assert(a->seen_ <= a->length_*2);
    }
}

void
php_serializer_t::begin_array(unsigned int length)
{
    array_element();
    assert(array_depth_ < MAX_ARRAY_DEPTH);
    array_t *a = &arrays_[array_depth_++];
    a->length_ = length;
    a->seen_ = 0;
    a->next_key_ = 0;
    buf_.append_printf("a:%u:{", length);
}

void
php_serializer_t::next_key()
{
    assert(array_depth_ > 0);
    array_t *a = &arrays_[array_depth_-1];
    integer(a->next_key_++);
}

void
php_serializer_t::end_array()
{
    assert(array_depth_ > 0);
    array_t *a = &arrays_[--array_depth_];
    assert(a->seen_ == a->length_ * 2);
    buf_.append_char('}');
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
php_serializer_t::integer(int i)
{
    array_element();
    buf_.append_printf("i:%d;", i);
}

void
php_serializer_t::string(const char *s)
{
    array_element();
    if (s == 0)
	s = "";
    buf_.append_printf("s:%d:\"%s\";", strlen(s), s);
}

void
php_serializer_t::stringl(const char *s, unsigned int l)
{
    array_element();
    if (s == 0)
	s = "";
    buf_.append_printf("s:%d:\"", l);
    buf_.append_chars(s, l);
    buf_.append_string("\";");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
