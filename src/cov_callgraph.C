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

#include "cov.H"

CVSID("$Id: cov_callgraph.C,v 1.3 2003-03-17 03:54:49 gnb Exp $");

GHashTable *cov_callnode_t::all_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callnode_t::cov_callnode_t(const char *nname)
{
    strassign(name, nname);
    
    g_hash_table_insert(all_, name, this);
}

cov_callnode_t::~cov_callnode_t()
{
#if 0
    listdelete(out_arcs, cov_callarc_t, delete);
    listclear(in_arcs);
    strdelete(name);
#else
    assert(0);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_callnode_t::init()
{
    all_ = g_hash_table_new(g_str_hash, g_str_equal);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callnode_t *
cov_callnode_t::find(const char *nname)
{
    return (cov_callnode_t *)g_hash_table_lookup(all_, nname);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    void (*func)(cov_callnode_t *, void *);
    void *userdata;
} cov_callnode_foreach_rec_t;

static void
cov_callnode_foreach_tramp(gpointer key, gpointer value, gpointer userdata)
{
    cov_callnode_t *cn = (cov_callnode_t *)value;
    cov_callnode_foreach_rec_t *rec = (cov_callnode_foreach_rec_t *)userdata;
    
    (*rec->func)(cn, rec->userdata);
}

void
cov_callnode_t::foreach(
    void (*func)(cov_callnode_t*, void *userdata),
    void *userdata)
{
    cov_callnode_foreach_rec_t rec;
    
    rec.func = func;
    rec.userdata = userdata;
    g_hash_table_foreach(all_, cov_callnode_foreach_tramp, &rec);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0
static gboolean
cov_callnode_t::delete_one(gpointer key, gpointer value, gpointer userdata)
{
    cov_callnode_t *cn = (cov_callnode_t *)value;
    
    delete cn;
    
    return TRUE;    /* please remove me */
}

static void
cov_callnode_t::delete_all(void)
{
    g_hash_table_foreach_remove(all_, delete_one, 0);
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callarc_t *
cov_callnode_t::find_arc_to(cov_callnode_t *to) const
{
    GList *iter;
    
    for (iter = out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *ca = (cov_callarc_t *)iter->data;
	
	if (ca->to == to)
	    return ca;
    }
    
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callarc_t::cov_callarc_t(cov_callnode_t *ffrom, cov_callnode_t *tto)
{
    from = ffrom;
    from->out_arcs = g_list_append(from->out_arcs, this);

    to = tto;
    to->in_arcs = g_list_append(to->in_arcs, this);
}

cov_callarc_t::~cov_callarc_t()
{
#if 0
#else
    assert(0);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_callarc_t::add_count(count_t ccount)
{
    count += ccount;
    to->count += ccount;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_add_callnodes(cov_file_t *f)
{
    unsigned int fnidx;
    cov_callnode_t *cn;
    
    for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
    {
    	cov_function_t *fn = f->nth_function(fnidx);
	
    	if (fn->is_suppressed())
	    continue;

	if ((cn = cov_callnode_t::find(fn->name())) == 0)
	{
	    cn = new cov_callnode_t(fn->name());
	    cn->function = fn;
	}
    }
}

void
cov_add_callarcs(cov_file_t *f)
{
    unsigned int fnidx;
    unsigned int bidx;
    list_iterator_t<cov_arc_t> aiter;
    cov_callnode_t *from;
    cov_callnode_t *to;
    cov_callarc_t *ca;
    
    for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
    {
    	cov_function_t *fn = f->nth_function(fnidx);
	
	if (fn->is_suppressed())
	    continue;

	from = cov_callnode_t::find(fn->name());
	assert(from != 0);
	
	for (bidx = 1 ; bidx < fn->num_blocks()-1 ; bidx++)
	{
    	    cov_block_t *b = fn->nth_block(bidx);
	
	    for (aiter = b->out_arc_iterator() ; aiter != (cov_arc_t *)0 ; ++aiter)
	    {
	    	cov_arc_t *a = *aiter;

    	    	if (!a->is_call() || a->name() == 0)
		    continue;
		    		
		if ((to = cov_callnode_t::find(a->name())) == 0)
		    to = new cov_callnode_t(a->name());
		    
		if ((ca = from->find_arc_to(to)) == 0)
		    ca = new cov_callarc_t(from, to);
		    
		ca->add_count(a->from()->count());
	    }
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
