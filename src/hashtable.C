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

CVSID("$Id: hashtable.C,v 1.2 2004-02-22 10:59:20 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
gnb_hash_table_add_one_key(gpointer key, gpointer value, gpointer closure)
{
    list_t<void> *list = (list_t<void> *)closure;
    
    list->prepend(key);
}

static int
string_compare(gconstpointer v1, gconstpointer v2)
{
    return strcmp((const char *)v1, (const char *)v2);
}

static int
direct_compare(gconstpointer v1, gconstpointer v2)
{
    if ((unsigned long)v1 < (unsigned long)v2)
    	return -1;
    else if ((unsigned long)v1 > (unsigned long)v2)
    	return 1;
    else
    	return 0;
}

GHashFunc hashtable_ops_t<char>::hash = g_str_hash;
GCompareFunc hashtable_ops_t<char>::compare = g_str_equal;
GCompareFunc hashtable_ops_t<char>::sort_compare = string_compare;

GHashFunc hashtable_ops_t<const char>::hash = g_str_hash;
GCompareFunc hashtable_ops_t<const char>::compare = g_str_equal;
GCompareFunc hashtable_ops_t<const char>::sort_compare = string_compare;

GHashFunc hashtable_ops_t<void>::hash = g_direct_hash;
GCompareFunc hashtable_ops_t<void>::compare = g_direct_equal;
GCompareFunc hashtable_ops_t<void>::sort_compare = direct_compare;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
