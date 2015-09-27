/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2011 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _hashtable_H_
#define _hashtable_H_ 1

#include "common.h"
#include "list.H"

/*
 * Wrapper for glib's GHashTable structure.
 *
 * Note that for memory efficiency, we inherit from GHashTable rather
 * than contain a pointer to it.  The problem is that GHashTable is
 * an opaque private structure, so you must never, ever ever allocate
 * hashtable_t as auto variables or struct member, in fact any way
 * except dynamically with new().
 */

// fool the compiler into believing we have enough information
// about GHashTable to be able to derive from it.
struct _GHashTable { int dummy_; };

template<class K> class hashtable_ops_t
{
public:
    static GHashFunc hash;
    /* for hash, returns 1 iff same */
    static GCompareFunc compare;
    /* for sorting keys, returns <0, 0, >0 like strcmp */
    static GCompareFunc sort_compare;
};

extern void gnb_hash_table_add_one_key(gpointer, gpointer, gpointer);

template<class K/*key*/, class T/*item*/> class hashtable_t;

template<class K/*key*/, class T/*item*/> class hashtable_iter_t
{
private:
#if HAVE_G_HASH_TABLE_ITER
    GHashTableIter hiter_;
#else
    GHashTable *table_;
    list_t<K> keys_;
    gboolean init_;
#endif
    gpointer key_, value_;
    gboolean valid_;

    hashtable_iter_t(GHashTable *ht)
     : key_(0),
       value_(0),
       valid_(TRUE)
    {
#if HAVE_G_HASH_TABLE_ITER
	g_hash_table_iter_init(&hiter_, ht);
	next();
#else
	table_ = ht;
	init_ = FALSE;
	/* initialise the list of keys later so we can
	 * return the iterator from ht::first() safely */
#endif
    }

public:
    hashtable_iter_t()
     : key_(0),
       value_(0),
       valid_(FALSE)
    {
#if HAVE_G_HASH_TABLE_ITER
	memset(&hiter_, 0, sizeof(hiter_));
#else
	table_ = 0;
	init_ = FALSE;
#endif
    }
    ~hashtable_iter_t()
    {
#if !HAVE_G_HASH_TABLE_ITER
	keys_.remove_all();
#endif
    }

    void delayed_init()
    {
#if !HAVE_G_HASH_TABLE_ITER
	if (!init_ && table_)
	{
	    ((hashtable_t<K, T>*)table_)->keys(&keys_);
	    init_ = TRUE;
	    next();
	}
#endif
    }

    T *operator++()
    {
	delayed_init();
	return next() ? (T *)value_ : 0;
    }
    T *operator*()
    {
	delayed_init();
	return (T *)value_;
    }

    gboolean next()
    {
	delayed_init();
#if HAVE_G_HASH_TABLE_ITER
	gboolean r = (valid_ && g_hash_table_iter_next(&hiter_, &key_, &value_));
	if (!r)
	{
	    key_ = value_ = 0;
	    valid_ = FALSE;
	}
#else
	if (valid_)
	{
	    key_ = (gpointer)keys_.remove_head();
	    if (key_)
	    {
		value_ = g_hash_table_lookup(table_, key_);
	    }
	    else
	    {
		value_ = 0;
		valid_ = FALSE;
	    }
	}
#endif
	return valid_;
    }
    gboolean next(K* &kt, T* &vt)
    {
	if (next())
	{
	    kt = (K *)key_;
	    vt = (T *)value_;
	    return TRUE;
	}
	return FALSE;
    }

    K *key()
    {
	delayed_init();
	return (K *)key_;
    }
    T *value()
    {
	delayed_init();
	return (T *)value_;
    }

    void remove()
    {
	delayed_init();
#if HAVE_G_HASH_TABLE_ITER
	if (valid_)
	    g_hash_table_iter_remove(&hiter_);
#else
	if (valid_)
	    g_hash_table_remove(table_, key_);
#endif
    }

    friend class hashtable_t<K, T>;
};

template<class K/*key*/, class T/*item*/> class hashtable_t : public GHashTable
{
public:
    /* ctor */
    hashtable_t()
    {
    }
    /* dtor */
    ~hashtable_t()
    {
    }

    void *operator new(size_t)
    {
	return g_hash_table_new(
		    hashtable_ops_t<K>::hash,
		    hashtable_ops_t<K>::compare);
    }

    void operator delete(void *x)
    {
	g_hash_table_destroy((GHashTable *)x);
    }

    void insert(K *key, T *value)
    {
	g_hash_table_insert(this, (gpointer)key, (gpointer)value);
    }

    void remove(K *key)
    {
	g_hash_table_remove(this, (gconstpointer)key);
    }

    T *lookup(K *key)
    {
	return (T *)g_hash_table_lookup(this, (gconstpointer)key);
    }

    gboolean lookup_extended(K *key, K *orig_key_ret, T **value_ret)
    {
	return g_hash_table_lookup_extended(this, (gconstpointer)key,
			(gpointer *)orig_key_ret, (gpointer *)value_ret);
    }

    void foreach(void (*func)(K*, T*, void *closure), void *closure)
    {
	g_hash_table_foreach(this, (GHFunc)*func, closure);
    }

    void foreach_remove(gboolean (*func)(K*, T*, void *closure), void *closure)
    {
	g_hash_table_foreach_remove(this, (GHRFunc)func, closure);
    }

    guint size() const
    {
	return g_hash_table_size((GHashTable*)this);
    }

    /* Builds a list of all the keys, sorted in increasing key order */
    void keys(list_t<K> *list) const
    {
	g_hash_table_foreach((GHashTable*)this, gnb_hash_table_add_one_key, list);
	list->sort((gint (*)(const K*, const K*))hashtable_ops_t<K>::sort_compare);
    }

    hashtable_iter_t<K, T> first() const
    {
	return hashtable_iter_t<K, T>((GHashTable *)this);
    }
};



#endif /* _hashtable_H_ */
