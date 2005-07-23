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

#ifndef _ggcov_cov_suppression_H_
#define _ggcov_cov_suppression_H_ 1

#include "common.h"
#include "hashtable.H"
#include "string_var.H"

struct cov_suppression_t
{
    enum type_t
    {
	IFDEF,		/* lines inside #if word_ */
	COMMENT_LINE,	/* line with a comment containing word_ */
	COMMENT_RANGE,	/* lines between a comment containing word_
	     	         * and a comment containing word2_ */
	NUM_TYPES
    };

    string_var word_;
    string_var word2_;
    enum type_t type_;

    cov_suppression_t(const char *w, type_t t)
     : word_(w), type_(t)
    {
    }

    static cov_suppression_t *add(const char *w, type_t t);
    static cov_suppression_t *find(const char *w, type_t t);
    static void foreach(type_t t,
			void (*f)(const char *, cov_suppression_t *, void *),
		        void *closure);
    static unsigned int count();

    static hashtable_t<const char, cov_suppression_t> *all_[NUM_TYPES];
};


#endif /* _ggcov_cov_suppression_H_ */
