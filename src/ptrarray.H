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

#ifndef _ptrarray_h_
#define _ptrarray_h_ 1

#include "common.h"

/*
 * Wrapper for glib's GPtrArray structure.
 *
 * Note that for memory efficiency, we inherit from GPtrArray rather
 * than contain a pointer to it.  The problem is that GPtrArray is
 * actually a public subset of a larger private structure, so you
 * must never, ever ever allocate ptrarray_t as auto variables or
 * struct member, in fact any way except dynamically with new().
 */

template<class T> class ptrarray_t : private GPtrArray
{
public:
    /* ctor */
    ptrarray_t()
    {
    }

    /* dtor */
    ~ptrarray_t()
    {
    }
    
    void * operator new(size_t)
    {
    	return g_ptr_array_new();
    }
    void operator delete(void *x)
    {
    	g_ptr_array_free((GPtrArray *)x, /*free_seg*/TRUE);
    }

    void resize(unsigned int n)
    {
    	g_ptr_array_set_size(this, n);
    }

    unsigned int append(T *p)
    {
	unsigned int i = len;
	g_ptr_array_add(this, p);
	return i;
    }

    gboolean remove(unsigned int i)
    {
	return g_ptr_array_remove_index(this, i);
    }
    gboolean remove(T *p)
    {
	return g_ptr_array_remove(this, p);
    }
    void set(unsigned i, T *p)
    {
    	if (i >= len)
    	    g_ptr_array_set_size(this, i+1);
	((T **)pdata)[i] = p;
    }

    T *nth(unsigned i) const
    {
    	assert(i < len);
	return (T *)g_ptr_array_index(this, i);
    }
    T *operator[](unsigned i) const
    {
	return (T *)g_ptr_array_index(this, i);
    }
    
    T * const *data() const
    {
	return (T * const *)(pdata);
    }
    unsigned int length() const
    {
    	return len;
    }

    void sort(int (*compare)(const T **, const T **))
    {
	qsort(pdata, len, sizeof(void*),
    	    	(int (*)(const void*, const void*))compare);
    }
};

#endif /* _ptrarray_h_ */
