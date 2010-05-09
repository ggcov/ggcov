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

#ifndef _php_serializer_H_
#define _php_serializer_H_ 1

#include "common.h"
#include "estring.H"

/*
 * A class to wrap construction of strings which can be
 * as deserialized PHP objects.
 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class php_serializer_t
{
public:
    /* ctor */
    php_serializer_t();
    /* dtor */
    ~php_serializer_t();

    const estring &data();

    void begin_array(unsigned int length);
    void begin_array();
    void next_key();
    void end_array();

    void integer(int);
    void string(const char *);
    void stringl(const char *, unsigned int length);
    void floating(double);
    void null();

private:
    void array_element();
    void _begin_array(unsigned int length, gboolean known);

    estring buf_;
    enum _constants
    {
	MAX_ARRAY_DEPTH=16
    };

    struct array_t
    {
	unsigned int length_;
	unsigned int seen_;
	int next_key_;
	unsigned int count_offset_;
    };
    
    array_t arrays_[MAX_ARRAY_DEPTH];
    unsigned int array_depth_;
    gboolean needs_deflation_;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* _php_serializer_H_ */
