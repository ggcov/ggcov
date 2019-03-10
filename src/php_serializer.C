/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@users.sourceforge.net>
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

php_serializer_t::php_serializer_t()
{
    array_depth_ = 0;
    needs_deflation_ = FALSE;
}

php_serializer_t::~php_serializer_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const estring &
php_serializer_t::data()
{
    assert(array_depth_ == 0);
    if (needs_deflation_)
    {
	/*
	 * Scan through the buffer, deflating inflated array lengths.
	 */
	char *in, *out;
	enum {SCAN,DEFLATE,STRING} state = SCAN;
	int ndeflated = 0;

	for (in = out = (char *)buf_.data() ; *in ; in++)
	{
	    switch (state)
	    {
	    case SCAN:
		if (*in == ':')
		{
		    state = DEFLATE;
		    ndeflated = 0;
		}
		else if (*in == '"')
		    state = STRING;
		*out++ = *in;
		break;
	    case DEFLATE:
		if (*in != '0')
		{
		    state = SCAN;
		    if (!isdigit(*in) && ndeflated > 0)
			*out++ = '0';
		    *out++ = *in;
		}
		else
		{
		    ndeflated++;
		}
		break;
	    case STRING:
		if (*in == '"')
		    state = SCAN;
		*out++ = *in;
		break;
	    }
	}

	buf_.truncate_to(out - buf_.data());
	needs_deflation_ = FALSE;
    }
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
	if (a->count_offset_ == 0)
	{
	    assert(a->seen_ <= a->length_*2);
	}
    }
}

#define INFLATED_LENGTH 10
#define INFLATED_FORMAT "%010u"

void
php_serializer_t::_begin_array(unsigned int length, gboolean known)
{
    const char *fmt;

    array_element();
    assert(array_depth_ < MAX_ARRAY_DEPTH);
    array_t *a = &arrays_[array_depth_++];
    a->length_ = length;
    a->seen_ = 0;
    a->next_key_ = 0;
    if (known)
    {
	a->count_offset_ = 0;
	fmt = "a:%u:{";
    }
    else
    {
	/*
	 * Unknown length.  Format a wide field of zeros and remember where
	 * it's stored so we can overwrite it later with the correct value.
	 */
	needs_deflation_ = TRUE;
	a->count_offset_ = buf_.length()+2;
	fmt = "a:" INFLATED_FORMAT ":{";
    }
    buf_.append_printf(fmt, length);
}

void
php_serializer_t::begin_array(unsigned int length)
{
    _begin_array(length, TRUE);
}

void
php_serializer_t::begin_array()
{
    _begin_array(0, FALSE);
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
    if (a->count_offset_ == 0)
    {
	/* length was provided in begin_array() call so check it here */
	assert(a->seen_ == a->length_ * 2);
    }
    else
    {
	/* check that we get balanced key/value pairs */
	assert(a->seen_ % 2 == 0);
	/* go back and format the actual length */
	char bb[INFLATED_LENGTH+1];
	snprintf(bb, sizeof(bb), INFLATED_FORMAT, a->seen_/2);
	memcpy((char *)buf_.data() + a->count_offset_, bb, INFLATED_LENGTH);
    }
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
    buf_.append_printf("s:%lu:\"%s\";", strlen(s), s);
}

void
php_serializer_t::stringl(const char *s, unsigned int l)
{
    array_element();
    if (s == 0)
	s = "";
    buf_.append_printf("s:%u:\"", l);
    buf_.append_chars(s, l);
    buf_.append_string("\";");
}

void
php_serializer_t::floating(double x)
{
    array_element();
    buf_.append_printf("d:%f;", x);
}

void
php_serializer_t::null()
{
    array_element();
    buf_.append_printf("N;");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
