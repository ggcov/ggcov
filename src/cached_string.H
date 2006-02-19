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

#ifndef _cached_string_H_
#define _cached_string_H_

#include "common.h"
#include "hashtable.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
//
// Class cached_string, like string_var, is a smart pointer for
// handling storage of strings.  The difference is that cached_string
// uses a global hashtable of constant strings, in order to share
// storage for strings which would otherwise have many separate
// copies.
//

class cached_string
{
private:
    static const char *get(const char *);
    static const char *lookup(const char *);

    static hashtable_t<const char, char> *all_;
    const char *data_;
    
public:
    // default ctor
    cached_string()
     :  data_(0)
    {
    }
    // const initialisation ctor
    cached_string(const char *s)
    {
    	data_ = get(s);
    }
    // non-const initialisation ctor
    cached_string(char *s)
    {
    	data_ = get(s);
	g_free(s);
    }
    // copy c'tor
    cached_string(const cached_string &o)
    {
    	data_ = o.data_;
    }
    // dtor    
    ~cached_string()
    {
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
    	return data_ = get(s);
    }
    const char *operator=(char *s)
    {
    	data_ = get(s);
	g_free(s);
	return data_;
    }
    // equality operator
    int operator==(const char *s)
    {
    	return (data_ == lookup(s));
    }

    unsigned int length() const
    {
    	return (data_ == 0 ? 0 : strlen(data_));
    }

    // take() is pretty superfluous but it might
    // make converting string_var code easier.
    char *take()
    {
    	char *d = (data_ == 0 ? 0 : g_strdup(data_));
	data_ = 0;
	return d;
    }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _cached_string_H_ */
