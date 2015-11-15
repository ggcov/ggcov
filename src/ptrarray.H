/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ptrarray_H_
#define _ptrarray_H_ 1

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

template<class T> class ptrarray_t;

template<class T> class ptrarray_iterator_t
{
private:
    ptrarray_t<T> *array_;
    int i_;         /* needs to go to -1 for EOF in -- */

    ptrarray_iterator_t(ptrarray_t<T> *a, int i)
     :  array_(a),
	i_(i)
    {
    }

    gboolean valid()
    {
	return (array_ && i_ >= 0 && i_ <= (int)array_->length()-1);
    }

    friend class ptrarray_t<T>;

public:
    ptrarray_iterator_t()
     :  array_(0),
	i_(0)
    {
    }

    ~ptrarray_iterator_t()
    {
    }

    T *operator++()
    {
	T *item = 0;
	if (valid())
	{
	    item = (*array_)[i_];
	    i_++;
	}
	return item;
    }

    T *operator--()
    {
	T *item = 0;
	if (valid())
	{
	    item = (*array_)[i_];
	    i_--;
	}
	return item;
    }

    T *operator*()
    {
	return (valid() ? (*array_)[i_] : 0);
    }

    ptrarray_iterator_t<T> peek_next() const
    {
	return ptrarray_iterator_t<T>(array_, i_+1);
    }

    ptrarray_iterator_t<T> peek_prev() const
    {
	return ptrarray_iterator_t<T>(array_, i_-1);
    }

    void replace(T *item)
    {
	if (valid())
	    array_->set(i_, item);
    }

    T *remove()
    {
	T *item = 0;
	if (valid())
	{
	    item = (*array_)[i_];
	    array_->remove((unsigned int)i_);
	}
	return item;
    }
};

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
	g_ptr_array_add(this, (gpointer)p);
	return i;
    }

    gboolean remove(unsigned int i)
    {
	return g_ptr_array_remove_index(this, i);
    }
    gboolean remove(T *p)
    {
	return g_ptr_array_remove(this, (gpointer)p);
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
    T *head() const
    {
	return (T *)g_ptr_array_index(this, 0);
    }
    T *tail() const
    {
	return (T *)g_ptr_array_index(this, len-1);
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

    ptrarray_iterator_t<T>
    first() const
    {
	return ptrarray_iterator_t<T>((ptrarray_t<T> *)this, 0);
    }
    ptrarray_iterator_t<T>
    last() const
    {
	return ptrarray_iterator_t<T>((ptrarray_t<T> *)this, len-1);
    }
};

#endif /* _ptrarray_H_ */
