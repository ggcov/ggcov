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

#ifndef _estring_H_
#define _estring_H_

#include "common.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifdef __GNUC__
#define _printf_fmt(fmtidx,firsta) \
    __attribute (( format(printf, fmtidx, firsta) ))
#else
#define _printf_fmt(fmtidx,firsta)
#endif

class estring
{
private:
    char *data_;
    unsigned int length_;	/* string length not including nul */
    unsigned int available_;	/* total bytes available, >= length+1 */

    void inline expand_by(unsigned int dl);
    void replace_vprintf(unsigned int start, unsigned int len,
			 const char *fmt, va_list args);

    
public:
    // default ctor
    estring()
     :  data_(0), length_(0), available_(0)
    {
    }
    // const initialisation ctor
    estring(const char *s)
    {
    	append_string(s);
    }
    // non-const initialisation ctor
    estring(char *s)
     :  data_(s)
    {
    	if (s == 0)
	{
	    length_ = available_ = 0;
	}
	else
	{
	    length_ = strlen(s);
	    available_ = length_+1;
	}
    }
    // dtor    
    ~estring()
    {
	if (data_ != 0)
	{
	    g_free(data_);
	    data_ = 0;
	    length_ = 0;
	    available_ = 0;
	}
    }

    const char *data() const
    {
    	return data_;
    }
    unsigned int length() const
    {
    	return length_;
    }
    
    char last() const
    {
    	return data_[length_-1];
    }
        
    char *take()
    {
    	char *d = data_;
	data_ = 0;
	length_ = 0;
	available_ = 0;
	return d;
    }
    
    

    void append_string(const char *str);
    void append_char(char c);
    void append_chars(const char *buf, unsigned int buflen);
    void append_printf(const char *fmt, ...) _printf_fmt(2,3);

    void replace_string(unsigned int start, unsigned int len,
    	    	    		const char *str);
    void replace_char(unsigned int start, unsigned int len,
    	    	    		char c);
    void replace_chars(unsigned int start, unsigned int len,
    	    	    		const char *buf, unsigned int buflen);
    void replace_printf(unsigned int start, unsigned int len,
    	    	    		const char *fmt, ...) _printf_fmt(4,5);
    void replace_all(const char *from, const char *to);

    void insert_string(unsigned int start, const char *str);
    void insert_char(unsigned int start, char c);
    void insert_chars(unsigned int start, const char *buf, int len);
    void insert_printf(unsigned int start, const char *fmt, ...)
    	    	    		_printf_fmt(3,4);

    void remove(unsigned int start, unsigned int len);

    void truncate();
    void truncate_to(unsigned int len);
};

#undef _printf_fmt

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _estring_H_ */
