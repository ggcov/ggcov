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

#ifndef _ggcov_json_H_
#define _ggcov_json_H_ 1

#include "common.h"
#include "estring.H"

class json_t
{
public:
    json_t();
    ~json_t();

    estring &data();

    void string(const char *value);
    void null() { string(0); }
    void ulong(unsigned long value);
    void begin_object() { push(OBJECT); }
    void end_object() { pop(OBJECT); }
    void begin_array() { push(ARRAY); }
    void end_array() { pop(ARRAY); }

    void string_field(const char *label, const char *value)
    {
	begin_field(label);
	string(value);
	end_field();
    }
    void null_field(const char *label)
    {
	begin_field(label);
	null();
	end_field();
    }
    void ulong_field(const char *label, unsigned long value)
    {
	begin_field(label);
	ulong(value);
	end_field();
    }

    void begin_object_field(const char *label)
    {
	begin_field(label);
	begin_object();
    }
    void end_object_field()
    {
	end_field();
	end_object();
    }
    void begin_array_field(const char *label)
    {
	begin_field(label);
	begin_array();
    }
    void end_array_field()
    {
	end_field();
	end_array();
    }

private:
    estring buf_;
    boolean ispretty_;
    boolean infield_;
    int depth_;
    enum {
	OBJECT=0,
	ARRAY=1,

	MAXDEPTH=16
    };
    struct {
	int type_;
	int nitems_;
    } stack_[MAXDEPTH];

    void escaped_string(const char *);
    void push(int type);
    void pop(int type);
    void begin_value();
    void end_value();
    void begin_field(const char *label);
    void end_field();
    int is_complete() const;
    void indent();
};

#endif /* _ggcov_json_H_ */
