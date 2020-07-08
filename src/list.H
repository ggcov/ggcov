/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 * Copyright (c) 2002-2020 Greg Banks <gnb@fastmail.fm>
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

#ifndef _list_H_
#define _list_H_ 1

#include "common.h"

/*
 * Wrapper for glib's GList structure.
 *
 * Unlike hashtable_t and strarray_t, list_t can be allocated
 * as auto variables or struct members.  It's basically a head
 * and tail pointer made typesafe.
 */

template<class T> class list_t;

template<class T> class list_iterator_t
{
private:
    GList *elem_;

    list_iterator_t(GList *elem)
     :  elem_(elem)
    {
    }

    friend class list_t<T>;

public:
    list_iterator_t()
     :  elem_(0)
    {
    }

    T *operator++()
    {
	T *item = 0;
	if (elem_ != 0)
	{
	    item = (T *)elem_->data;
	    elem_ = elem_->next;
	}
	return item;
    }

    T *operator--()
    {
	T *item = 0;
	if (elem_ != 0)
	{
	    item = (T *)elem_->data;
	    elem_ = elem_->prev;
	}
	return item;
    }

    T *operator*()
    {
	return (elem_ == 0 ? 0 : (T *)elem_->data);
    }

    list_iterator_t<T> peek_next() const
    {
	return list_iterator_t<T>(elem_ == 0 ? 0 : elem_->next);
    }
    list_iterator_t<T> peek_prev() const
    {
	return list_iterator_t<T>(elem_ == 0 ? 0 : elem_->prev);
    }

    void replace(T *item)
    {
	elem_->data = (gpointer)item;
    }

    void splice_after(list_t<T> *list)
    {
	GList *last;

	if (list == 0 || list->head_ == 0)
	    return;

	last = g_list_last(list->head_);   // TODO: be smarter

	if (elem_->next != 0)
	    elem_->next->prev = last;
	last->next = elem_->next;

	elem_->next = list->head_;
	list->head_->prev = elem_;

	list->head_ = list->tail_ = 0;
    }


    int operator==(T *item) const
    {
	return (item == (elem_ == 0 ? 0 : (T *)elem_->data));
    }
    int operator!=(T *item) const
    {
	return (item != (elem_ == 0 ? 0 : (T *)elem_->data));
    }
};

template<class T> class list_t
{
private:
    GList *head_;
    GList *tail_;

    friend class list_iterator_t<T>;

    /* ctor */
    list_t(GList *gl)
     :  head_(gl),
	tail_(0)
    {
    }

public:
    /* ctor */
    list_t()
     :  head_(0),
	tail_(0)
    {
    }
    /* dtor */
    ~list_t()
    {
#if 0
	// User must clear out list first
	assert(head_ == 0);
	assert(tail_ == 0);
#endif
    }

    void append(T *item)
    {
	if (tail_ == 0)
	    tail_ = g_list_last(head_);
	GList *elem = g_list_append(tail_, (gpointer)item);
	if (tail_ == 0)
	    head_ = tail_ = elem;
	else
	    tail_ = tail_->next;
    }

    void prepend(T *item)
    {
	head_ = g_list_prepend(head_, (gpointer)item);
	if (tail_ == 0)
	    tail_ = head_;
    }

    void insert_sorted(T *item, gint (*compare)(const T*, const T*))
    {
	head_ = g_list_insert_sorted(head_, (gpointer)item, (GCompareFunc)compare);
	// update tail_ pointer only if its no longer valid
	if (tail_ == 0 || tail_->next != 0)
	    tail_ = g_list_last(head_);
    }

    void sort(gint (*compare)(const T*, const T*))
    {
	head_ = g_list_sort(head_, (GCompareFunc)compare);
	// update tail_ pointer only if its no longer valid
	if (tail_ == 0 || tail_->next != 0)
	    tail_ = g_list_last(head_);
    }

    void remove(T *item)
    {
	head_ = g_list_remove(head_, item);
	// invalidate tail
	tail_ = 0;
    }

    T *remove(list_iterator_t<T> itr)
    {
	T *item = (T *)itr.elem_->data;
	if (tail_ != 0 && itr.elem_ == tail_)
	    tail_ = tail_->prev;
	else
	    tail_ = 0;  // invalidate tail
	head_ = g_list_delete_link(head_, itr.elem_);
	return item;
    }

    void remove_all()
    {
	while (head_ != 0)
	    head_ = g_list_delete_link(head_, head_);
	tail_ = 0;
    }

    T *remove_head()
    {
	T *item = 0;
	if (head_ != 0)
	{
	    item = (T *)head_->data;
	    if (head_ == tail_)
		tail_ = 0;
	    head_ = g_list_delete_link(head_, head_);
	}
	return item;
    }

    void apply_remove(void (*dtor)(T *))
    {
	while (head_ != 0)
	{
	    T *item = (T *)head_->data;
	    head_ = g_list_delete_link(head_, head_);
	    (*dtor)(item);
	}
	tail_ = 0;
    }

    void delete_all()
    {
	while (head_ != 0)
	{
	    delete (T *)head_->data;
	    head_ = g_list_delete_link(head_, head_);
	}
	tail_ = 0;
    }

    void foreach(void (*func)(T*, void *closure), void *closure)
    {
	GList *iter;
	for (iter = head_ ; iter != 0 ; iter = iter->next)
	    (*func)((T *)iter->data, closure);
    }

    void foreach_remove(gboolean (*func)(T*, void *closure), void *closure)
    {
	GList *iter, *next;
	for (iter = head_ ; iter != 0 ; iter = next)
	{
	    next = iter->next;
	    if ((*func)((T *)iter->data, closure))
		remove(list_iterator_t<T>(iter));
	}
    }

    void concat(list_t<T> *list)
    {
	// TODO: be smarter
	head_ = g_list_concat(head_, list->head_);
	tail_ = list->tail_;
	list->head_ = list->tail_ = 0;
    }

    list_t<T> copy() const
    {
	return list_t<T>(g_list_copy(head_));
    }

    void take(list_t<T> *list)
    {
	head_ = list->head_;
	tail_ = list->tail_;
	list->head_ = list->tail_ = 0;
    }

    T *head() const
    {
	return (head_ == 0 ? 0 : (T *)head_->data);
    }

    T *tail() const
    {
	if (tail_ == 0)
	    ((list_t<T> *)this)->tail_ = g_list_last(head_);
	return (tail_ == 0 ? 0 : (T *)tail_->data);
    }

    guint length() const
    {
	return g_list_length(head_);
    }

    // iteration interface
    list_iterator_t<T> first() const
    {
	return list_iterator_t<T>(head_);
    }

    list_iterator_t<T> last() const
    {
	if (tail_ == 0)
	    ((list_t<T> *)this)->tail_ = g_list_last(head_);
	return list_iterator_t<T>(tail_);
    }

};

extern char *join(const char *sep, const list_t<char> &list);

#endif /* _list_H_ */
