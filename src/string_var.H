/*
 * ggcov - A GTK frontend for exploring gcov coverage data
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

#ifndef _string_var_H_
#define _string_var_H_

#include "common.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
//
// Class string_var is a convenient way of holding a newly
// allocated string (as returned from various utility functions)
// and freeing the string when it's no longer needed (i.e.
// when the string_var goes out of scope).  String_var also
// has small storage requirements; it's just a pointer to
// the string.  If you want to do useful manipulations on
// strings such as expanding them and replacing characters
// in the middle, use class estring.
//
// The design of string_var is loosely based on OMG Corba's
// class of the same name.
//

class string_var
{
private:
    char *data_;
    
public:
    // default ctor
    string_var()
     :  data_(0)
    {
    }
    // const initialisation ctor
    string_var(const char *s)
    {
    	data_ = (s == 0 ? 0 : g_strdup(s));
    }
    // non-const initialisation ctor
    string_var(char *s)
     :  data_(s)
    {
    }
    // dtor    
    ~string_var()
    {
	if (data_ != 0)
	{
	    g_free(data_);
	    data_ = 0;
	}
    }

    const char *data() const
    {
    	return data_;
    }
    operator const char *() const
    {
    	return data_;
    }
    // assignment operators
    const char *operator=(const char *s)
    {
    	if (data_ != 0)
	    g_free(data_);
	data_ = g_strdup(s);
    }
    const char *operator=(char *s)
    {
    	if (data_ != 0)
	    g_free(data_);
	data_ = s;
    }
    // equality operator
    int operator==(const char *s)
    {
    	return !strcmp((data_ == 0 ? "" : data_),
	    	       (s == 0 ? "" : s));
    }

    unsigned int length() const
    {
    	return (data_ == 0 ? 0 : strlen(data_));
    }
    
    char *take()
    {
    	char *d = data_;
	data_ = 0;
	return d;
    }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _string_var_H_ */
