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

#include "countarray.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

countarray_t::countarray_t()
 :  length_(0),
    alloc_(0),
    blocks_(0)
{
    group_.first_ = ~0U;
    group_.last_ = ~0U;
}

countarray_t::~countarray_t()
{
    unsigned int bi;

    for (bi = 0 ; bi < alloc_/BLOCKSIZE ; bi++)
	free(blocks_[bi]);
    free(blocks_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
countarray_t::resize(unsigned int i)
{
    unsigned int maxbi = alloc_/BLOCKSIZE;
    unsigned int bi = i / BLOCKSIZE;

    if (bi >= maxbi) {
	blocks_ = (count_t**)gnb_xrealloc(blocks_,
				sizeof(count_t*) * (bi+1));
	memset((void *)(blocks_+maxbi), 0, (bi+1-maxbi) * sizeof(count_t*));
	blocks_[bi] = (count_t *)gnb_xmalloc(BLOCKSIZE * sizeof(count_t));
	// Initialise to COV_COUNT_INVALID
	memset(blocks_[bi], 0xff, BLOCKSIZE * sizeof(count_t));
	alloc_ = (bi+1)*BLOCKSIZE;
    }
}

count_t *
countarray_t::get_slot(unsigned int i) const
{
    if (i >= length_)
	return 0;
    unsigned int bi = i / BLOCKSIZE;
    if (!blocks_[bi])
	return 0;
    return &blocks_[bi][i % BLOCKSIZE];
}

count_t *
countarray_t::get_slot(unsigned int i)
{
    if (i >= length_) {
	resize(i);
	length_ = i+1;
    }
    unsigned int bi = i / BLOCKSIZE;
    assert(blocks_[bi]);
    return &blocks_[bi][i % BLOCKSIZE];
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
countarray_t::begin_group()
{
    group_.first_ = ~0U;
    group_.last_ = ~0U;
}

unsigned int
countarray_t::allocate()
{
    resize(length_);
    unsigned int i = length_++;

    if (group_.first_ == ~0U)
	group_.first_ = i;
    group_.last_ = i;

    return i;
}

void
countarray_t::end_group(group_t &g)
{
    g = group_;
    begin_group();
}

void
countarray_t::invalidate(const group_t &g)
{
    unsigned int firstbi = g.first_ / BLOCKSIZE;
    unsigned int lastbi = (g.last_+BLOCKSIZE-1) / BLOCKSIZE;
    unsigned int bi;
    unsigned int remain = (g.last_ - g.first_ + 1);
    unsigned int off = g.first_ % BLOCKSIZE;

    for (bi = firstbi ; bi <= lastbi ; bi++)
    {
	unsigned int n = MIN(BLOCKSIZE - off, remain);
	if (blocks_[bi])
	    memset(blocks_[bi]+off, 0xff, sizeof(count_t) * n);
	remain -= n;
	off = 0;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
