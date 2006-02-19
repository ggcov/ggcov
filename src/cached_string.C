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

#include "cached_string.H"

CVSID("$Id: cached_string.C,v 1.1 2006-02-19 04:25:32 gnb Exp $");

hashtable_t<const char, char> *cached_string::all_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
cached_string::get(const char *s)
{
    char *p = 0;

    if (s == 0)
	return 0;

    if (all_ == 0)
    	all_ = new hashtable_t<const char, char>;
    else
    	p = all_->lookup(s);

    if (p == 0)
    {
    	p = g_strdup(s);
	all_->insert(p, p);
    }

    return p;
}

const char *
cached_string::lookup(const char *s)
{
    if (all_ == 0 || s == 0)
    	return 0;
    return all_->lookup(s);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
