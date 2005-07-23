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

#include "cov.H"
#include "cov_suppression.H"

hashtable_t<const char, cov_suppression_t> *
    cov_suppression_t::all_[cov_suppression_t::NUM_TYPES];

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_suppression_t *
cov_suppression_t::add(const char *w, type_t t)
{
    cov_suppression_t *s = new cov_suppression_t(w, t);

    if (all_[t] == 0)
    	all_[t] = new hashtable_t<const char, cov_suppression_t>;
    all_[t]->insert(s->word_, s);

    return s;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_suppression_t *
cov_suppression_t::find(const char *w, type_t t)
{
    return (all_[t] == 0 ? 0 : all_[t]->lookup(w));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_suppression_t::foreach(
    type_t t,
    void (*func)(const char *, cov_suppression_t *, void *),
    void *closure)
{
    if (all_[t] != 0)
	all_[t]->foreach(func, closure);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

unsigned int
cov_suppression_t::count()
{
    unsigned int t;
    unsigned int n = 0;

    for (t = 0 ; t < NUM_TYPES ; t++)
	n += (all_[t] ? all_[t]->size() : 0);
    return n;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_suppress_ifdef(const char *variable)
{
    cov_suppression_t::add(variable, cov_suppression_t::IFDEF);
}

void
cov_suppress_lines_with_comment(const char *word)
{
    cov_suppression_t::add(word, cov_suppression_t::COMMENT_LINE);
}

void
cov_suppress_lines_between_comments(const char *startw, const char *endw)
{
    cov_suppression_t *s;
    
    s = cov_suppression_t::add(startw, cov_suppression_t::COMMENT_RANGE);
    s->word2_ = endw;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
