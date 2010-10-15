/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2010 Greg Banks <gnb@users.sourceforge.net>
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

#include "json.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

json_t::json_t()
 :  ispretty_(debug_enabled(D_WEB)),
    infield_(false),
    depth_(0)
{
}

json_t::~json_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
json_t::string(const char *value)
{
    begin_value();
    if (!value)
    {
	buf_.append_string("null");
    }
    else
    {
	buf_.append_char('\'');
	for ( ; *value ; value++)
	{
	    /* TODO: do the proper JSON escaping rules in the RFC */
	    switch (*value)
	    {
	    case '"':
	    case '\'':
	    case '\\':
		buf_.append_char('\\');
		buf_.append_char(*value);
		break;
	    case '\r':
		buf_.append_string("\\r");
		break;
	    case '\n':
		buf_.append_string("\\n");
		break;
	    default:
		buf_.append_char(*value);
	    }
	}
	buf_.append_char('\'');
    }
    end_value();
}

void
json_t::ulong(unsigned long value)
{
    begin_value();
    buf_.append_printf("%lu", value);
    end_value();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *bounds[] = { "{}", "[]" };

void
json_t::push(int type)
{
    assert(depth_ < MAXDEPTH);
    begin_value();
    stack_[depth_].type_ = type;
    stack_[depth_].nitems_ = 0;
    depth_++;
    buf_.append_char(bounds[type][0]);
}

void
json_t::pop(int type)
{
    assert(depth_ > 0);
    end_value();
    depth_--;
    assert(stack_[depth_].type_ == type);
    if (ispretty_)
	indent();
    buf_.append_char(bounds[type][1]);
}

void
json_t::begin_value()
{
    if (!infield_)
    {
	if (depth_ > 0 && stack_[depth_-1].nitems_++)
	    buf_.append_char(',');
	if (ispretty_)
	    indent();
    }
    infield_ = false;
}

void
json_t::end_value()
{
    assert(!infield_);
}

void
json_t::begin_field(const char *label)
{
    assert(depth_ > 0);
    assert(!infield_);
    if (stack_[depth_-1].nitems_++)
	buf_.append_char(',');
    if (ispretty_)
	indent();
    buf_.append_printf("'%s':", label);
    infield_ = true;
}

void
json_t::end_field()
{
    assert(!infield_);
}

const char *
json_t::data() const
{
    // check that the serialised structure is complete
    assert(depth_ == 0);
    assert(!infield_);
    return buf_.data();
}

char *
json_t::take()
{
    // check that the serialised structure is complete
    assert(depth_ == 0);
    assert(!infield_);
    return buf_.take();
}

void
json_t::indent()
{
    buf_.append_char('\n');
    for (int i = 0 ; i < depth_ ; i++)
	buf_.append_string("  ");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
