/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2015 Greg Banks <gnb@fastmail.fm>
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
#include "common.h"
#include "unique_ptr.H"
#include "testfw.H"

#define MAX_INSTANCES	16
struct foo_t
{
    static unsigned next_id;
    static bool allocated[MAX_INSTANCES];
    unsigned id;

    foo_t() : id(next_id++) { allocated[id] = true; }
    ~foo_t() { allocated[id] = false; }

    static unsigned num_allocated()
    {
	unsigned n = 0;
	unsigned i = 0;
	for (i = 0 ; i < MAX_INSTANCES ; i++)
	    n += !!allocated[i];
	return n;
    }
    static void reset() { next_id = 0; memset(allocated, 0, sizeof(allocated)); }
};

unsigned foo_t::next_id = 0;
bool foo_t::allocated[MAX_INSTANCES];

SETUP
{
    foo_t::reset();
    return 0;
}

TEST(empty)
{
    unique_ptr<foo_t> p;
    check_num_equals((bool)p, 0);
    check_null(p.get());
    check_null(p.release());
}

TEST(ctor)
{
    check_num_equals(foo_t::num_allocated(), 0);
    {
	unique_ptr<foo_t> p(new foo_t());
	check_num_equals(foo_t::num_allocated(), 1);
	check_num_equals((bool)p, 1);
	check_not_null(p.get());
    }
    check_num_equals(foo_t::next_id, 1);
    check_num_equals(foo_t::num_allocated(), 0);
}

TEST(release)
{
    check_num_equals(foo_t::num_allocated(), 0);
    {
	foo_t *f = new foo_t();
	check_num_equals(foo_t::num_allocated(), 1);
	unique_ptr<foo_t> p(f);
	check_num_equals((bool)p, 1);
	check_not_null(p.get());
	foo_t *g = p.release();
	check_not_null(g);
	check_null(p.get());
	check_null(p.release());
	check_num_equals(foo_t::num_allocated(), 1);
	delete g;
    }
    check_num_equals(foo_t::next_id, 1);
    check_num_equals(foo_t::num_allocated(), 0);
}

TEST(assign)
{
    check_num_equals(foo_t::num_allocated(), 0);
    {
	unique_ptr<foo_t> p;
	check_num_equals(foo_t::num_allocated(), 0);
	check_num_equals((bool)p, 0);
	check_null(p.get());
	p = new foo_t();
	check_num_equals(foo_t::num_allocated(), 1);
	check_num_equals((bool)p, 1);
	check_not_null(p.get());
    }
    check_num_equals(foo_t::next_id, 1);
    check_num_equals(foo_t::num_allocated(), 0);
}

TEST(assign_null)
{
    check_num_equals(foo_t::num_allocated(), 0);
    {
	unique_ptr<foo_t> p(new foo_t());
	check_num_equals(foo_t::num_allocated(), 1);
	check_num_equals((bool)p, 1);
	check_not_null(p.get());
	p = 0;
	check_num_equals(foo_t::num_allocated(), 0);
	check_num_equals((bool)p, 0);
	check_null(p.get());
    }
    check_num_equals(foo_t::next_id, 1);
    check_num_equals(foo_t::num_allocated(), 0);
}

TEST(get)
{
    unique_ptr<foo_t> p(new foo_t());
    check_not_null(p.get());
    check_num_equals(p.get()->id, 0);
    unique_ptr<foo_t> q(new foo_t());
    check_not_null(q.get());
    check_num_equals(q.get()->id, 1);
}

TEST(deref_star)
{
    unique_ptr<foo_t> p(new foo_t());
    check_not_null(*p);
    check_num_equals((*p)->id, 0);

    unique_ptr<foo_t> q(new foo_t());
    check_not_null(*q);
    check_num_equals((*q)->id, 1);
}

TEST(deref_ptr)
{
    unique_ptr<foo_t> p(new foo_t());
    check_not_null(*p);
    check_num_equals(p->id, 0);
    unique_ptr<foo_t> q(new foo_t());
    check_not_null(*q);
    check_num_equals(q->id, 1);
}

