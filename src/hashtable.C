/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
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

#include "hashtable.H"

CVSID("$Id: hashtable.C,v 1.1 2003-06-01 09:30:24 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GHashFunc hashtable_ops_t<char*>::hash = g_str_hash;
GCompareFunc hashtable_ops_t<char*>::compare = g_str_equal;

GHashFunc hashtable_ops_t<const char*>::hash = g_str_hash;
GCompareFunc hashtable_ops_t<const char*>::compare = g_str_equal;

GHashFunc hashtable_ops_t<void*>::hash = g_direct_hash;
GCompareFunc hashtable_ops_t<void*>::compare = g_direct_equal;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
