/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Derived from ggui code
 * Copyright (c) 2000-2003 Greg Banks <gnb@users.sourceforge.net>
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

#include "mvc.h"

typedef struct mvc_listener_s       mvc_listener_t;
typedef struct mvc_batch_s          mvc_batch_t;

struct mvc_listener_s
{
    mvc_listener_t *next;   /* chain for same object */
    unsigned int features;  /* interested only in these features */
    mvc_callback_t callback;
    void *closure;
};

struct mvc_batch_s
{
    unsigned int features;
};

static GHashTable *listeners;
static bool batching;
static GHashTable *batch;

static enum { NONE, LISTEN, UNLISTEN, CHANGED, UNBATCH } state = NONE;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mvc_listen(void *obj, unsigned int feat, mvc_callback_t proc, void *closure)
{
    mvc_listener_t *ml;

//    assert(state == NONE);
    state = LISTEN;

    ml = new(mvc_listener_t);
    ml->features = feat;
    ml->callback = proc;
    ml->closure = closure;

    if (listeners == 0)
    {
	listeners = g_hash_table_new(g_direct_hash, g_direct_equal);
	assert(listeners != 0);
    }
    else
    {
	mvc_listener_t *chain = (mvc_listener_t *)g_hash_table_lookup(
							listeners, obj);
	if (chain != 0)
	{
	    g_hash_table_remove(listeners, obj);
	    ml->next = chain;
	}
    }
    g_hash_table_insert(listeners, obj, ml);

    state = NONE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
mvc_do_unlisten(
    void *obj,
    bool all,
    unsigned int feat,
    mvc_callback_t proc,
    void *closure)
{
    mvc_listener_t *ml, *head, *next, *prev, *orig_head;

//    assert(state == NONE);
    state = UNLISTEN;

    orig_head = head = (mvc_listener_t *)g_hash_table_lookup(listeners, obj);
    prev = 0;

    for (ml = head ; ml != 0 ; prev = ml, ml = next)
    {
	next = ml->next;
	if (all ||
	    (ml->callback == proc &&
	     ml->closure == closure))
	{
	    ml->features &= ~feat;
	    if (all || ml->features == 0)
	    {
		/* remove from chain */
		if (prev == 0)
		    head = ml->next;
		else
		    prev->next = ml->next;
		g_free(ml);
	    }
	}
    }

    if (head != orig_head)
    {
	g_hash_table_remove(listeners, obj);
	g_hash_table_insert(listeners, obj, head);
    }

    state = NONE;
}

void
mvc_unlisten(void *obj, unsigned int feat, mvc_callback_t proc, void *closure)
{
    mvc_do_unlisten(obj, /*all*/false, feat, proc, closure);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
mvc_dispatch(void *obj, unsigned int features)
{
    mvc_listener_t *ml;

    if (listeners == 0)
	return;     /* no listeners on any object */

    for (ml = (mvc_listener_t *)g_hash_table_lookup(listeners, obj) ;
	 ml != 0 ;
	 ml = ml->next)
    {
	if ((ml->features & features))
	    (*ml->callback)(obj, features, ml->closure);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mvc_changed(void *obj, unsigned int features)
{
//    assert(state == NONE);
    state = CHANGED;

    if (batching)
    {
	mvc_batch_t *mb;

	if ((mb = (mvc_batch_t *)g_hash_table_lookup(batch, obj)) == 0)
	{
	    mb = new(mvc_batch_t);
	    mb->features = features;
	    g_hash_table_insert(batch, obj, mb);
	}
	else
	{
	    mb->features |= features;
	}
    }
    else
    {
	mvc_dispatch(obj, features);
    }

    state = NONE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mvc_deleted(void *obj)
{
    if (batching)
    {
	mvc_batch_t *mb;

	if ((mb = (mvc_batch_t *)g_hash_table_lookup(batch, obj)) != 0)
	{
	    fprintf(stderr, "mvc_deleted: object 0x%p deleted with pending 0x%x\n",
			obj, mb->features);
	    g_hash_table_remove(batch, obj);
	    g_free(mb);
	}
    }
    mvc_do_unlisten(obj, /*all*/true, 0, 0, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mvc_batch(void)
{
    if (!batching)
    {
	if (batch == 0)
	    batch = g_hash_table_new(g_direct_hash, g_direct_equal);
	batching = true;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
mvc_unbatch_1(gpointer key, gpointer value, gpointer user_data)
{
    void *object = (void *)key;
    mvc_batch_t *mb = (mvc_batch_t *)value;

    mvc_dispatch(object, mb->features);
    g_free(mb);
    return TRUE;    /* remove me */
}

void
mvc_unbatch(void)
{
//    assert(state == NONE);
    state = UNBATCH;

    assert(batching);
    batching = false;

    g_hash_table_foreach_remove(batch, mvc_unbatch_1, 0);

    state = NONE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
