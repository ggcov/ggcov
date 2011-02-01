/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2011 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _countarray_H_
#define _countarray_H_ 1

#include "cov_types.H"

class countarray_t
{
public:
    /* ctor */
    countarray_t();

    /* dtor */
    ~countarray_t();
    
    void resize(unsigned int n);

    unsigned int append(count_t c)
    {
	*get_slot(length_) = c;
	return length_-1;
    }

// This isn't necessary
//     gboolean remove(unsigned int i);

    void set(unsigned i, count_t c)
    {
	*get_slot(i) = c;
    }

    count_t nth(unsigned i) const
    {
	count_t *ss = get_slot(i);
	return (ss ? *ss : COV_COUNT_INVALID);
    }
    count_t operator[](unsigned i) const
    {
	count_t *ss = get_slot(i);
	return (ss ? *ss : COV_COUNT_INVALID);
    }
    
//  This isn't feasible
//     T * const *data() const

    unsigned int length() const
    {
	return length_;
    }

    unsigned int next_slot();

private:
    count_t *get_slot(unsigned int i) const;
    count_t *get_slot(unsigned int i);

    enum { BLOCKSIZE = 2048 };
    unsigned int length_;	// number of slots valid
    unsigned int alloc_;	// largest slot allocated+1
    count_t **blocks_;
};

#endif /* _countarray_H_ */
