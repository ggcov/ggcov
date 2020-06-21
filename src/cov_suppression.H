/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2015 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_cov_suppression_H_
#define _ggcov_cov_suppression_H_ 1

#include "common.h"
#include "hashtable.H"
#include "list.H"
#include "string_var.H"

class cov_suppression_t
{
public:
    enum type_t
    {
	IFDEF,          /* lines inside #if word_ */
	COMMENT_LINE,   /* line with a comment containing word_ */
	COMMENT_RANGE,  /* lines between a comment containing word_
			 * and a comment containing word2_ */
	ARC_CALLS,      /* arc is a call to named function
                         * (requires CALLTREE_ENABLED) */
	BLOCK_CALLS,    /* block contains an arc which is a
			 * call to named function (requires
                         * CALLTREE_ENABLED) */
	FUNCTION,       /* named function */
	FILENAME,       /* named filename */
	MERGED,         /* all components are suppressed */
	UNSOLVABLE,     /* function tree cannot be solved */
	RELOC,          /* ELF reloc during call scanning */
	NUM_TYPES
    };

    cov_suppression_t(const char *w, type_t t, const char *o)
     : word_(w), type_(t), origin_(o) {}
    ~cov_suppression_t() {}

    type_t type() const
    {
	return type_;
    }
    const char *word() const
    {
	return word_;
    }

    const char *word2() const
    {
	return word2_;
    }
    void set_word2(const char *s)
    {
	word2_ = s;
    }

    const char *origin() const
    {
	return origin_;
    }
    void set_origin(const char *s)
    {
	origin_ = s;
    }

    const char *describe() const;

private:
    string_var word_;
    string_var word2_;
    enum type_t type_;
    string_var origin_;
};

class cov_suppression_set_t
{
public:
    cov_suppression_set_t();
    ~cov_suppression_set_t();

    void add(cov_suppression_t *);
    void remove(const cov_suppression_t *);

    const cov_suppression_t *find(const char *w, cov_suppression_t::type_t t) const;
    list_iterator_t<const cov_suppression_t> first(cov_suppression_t::type_t t)
    {
	return all_[t].first();
    }
    unsigned int count(cov_suppression_t::type_t t) const
    {
	return all_[t].length();
    }

    void init_builtins();

private:
    struct init_t
    {
	cov_suppression_t::type_t type;
	const char *word;
    };

    hashtable_t<const char, const cov_suppression_t> *by_word_[cov_suppression_t::NUM_TYPES];
    list_t<const cov_suppression_t> prefixed_[cov_suppression_t::NUM_TYPES];
    list_t<const cov_suppression_t> all_[cov_suppression_t::NUM_TYPES];
};


/*
 * Class cov_suppression_combiner_t reifys the algorithm
 * for choosing whether to push a suppression *upwards*
 * in the object hierarchy, i.e. whether to suppress a
 * function because all it's blocks are suppressed.
 */
class cov_suppression_combiner_t
{
public:
    cov_suppression_combiner_t(cov_suppression_set_t &sset)
     :  suppression_set_(sset),
	suppression_(0),
	num_unsuppressed_(0)
    {
    }

    void add(const cov_suppression_t *s)
    {
	if (!s)
	    num_unsuppressed_++;
	else if (!suppression_)
	    suppression_ = s;
	else if (suppression_ != s)
	    suppression_ = suppression_set_.find(0, cov_suppression_t::MERGED);
    }

    const cov_suppression_t *result() const
    {
	return (num_unsuppressed_ ? 0 : suppression_);
    }

private:
    const cov_suppression_set_t &suppression_set_;
    const cov_suppression_t *suppression_;
    unsigned int num_unsuppressed_;
};

#endif /* _ggcov_cov_suppression_H_ */
