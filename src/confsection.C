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

#include <libgnome/libgnome.h>
#include "confsection.H"
#include "estring.H"

CVSID("$Id: confsection.C,v 1.7 2004-02-22 10:56:20 gnb Exp $");

hashtable_t<const char, confsection_t> *confsection_t::all_;
static const char filename[] = "ggcov";

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

confsection_t::confsection_t(const char *secname)
 :  secname_(secname)
{
    if (all_ == 0)
    	all_ = new hashtable_t<const char, confsection_t>;
    all_->insert(secname_, this);
}

confsection_t::~confsection_t()
{
#if 0
#else
    assert(0);
#endif
}

confsection_t *
confsection_t::get(const char *name)
{
    confsection_t *cs;
    
    if (all_ == 0 ||
    	(cs = all_->lookup(name)) == 0)
    	cs = new confsection_t(name);
    return cs;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
confsection_t::make_key(const char *name) const
{
    assert(strchr(name, '/') == 0);
    return g_strconcat(filename, "/", secname_.data(), "/", name, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
confsection_t::get_string(const char *name, const char *deflt)
{
    gboolean defaulted = FALSE;
    const char *val;
    estring key = make_key(name);
    
    val = gnome_config_get_string_with_default(key.data(), &defaulted);
    defaulted = TRUE;

    if (defaulted)
    	val = deflt;
    return val;
}

void
confsection_t::set_string(const char *name, const char *value)
{
    estring key = make_key(name);

    gnome_config_set_string(key.data(), value);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
confsection_t::get_enum(const char *name, const confenum_t *tbl, int deflt)
{
    gboolean defaulted = FALSE;
    const char *val;
    estring key = make_key(name);
    
    val = gnome_config_get_string_with_default(key.data(), &defaulted);
    if (defaulted)
    	return deflt;
    
    for ( ; tbl->string != 0 ; tbl++)
    {
    	if (!strcasecmp(tbl->string, val))
	    return tbl->value;
    }
    
    if (isdigit(val[0]))
    {
	char *end = 0;
	int ival = (int)strtol(val, &end, 0);
	if (end != 0 && end != val && *end == '\0')
	    return ival;
    }
    return deflt;
}

void
confsection_t::set_enum(const char *name, const confenum_t *tbl, int value)
{
    estring key = make_key(name);
    char buf[32];

    for ( ; tbl->string != 0 ; tbl++)
    {
    	if (value == tbl->value)
	{
	    gnome_config_set_string(key.data(), tbl->string);
	    return;
	}
    }
    
    snprintf(buf, sizeof(buf), "%d", value);
    gnome_config_set_string(key.data(), buf);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
confsection_t::get_bool(const char *name, gboolean deflt)
{
    gboolean defaulted = FALSE;
    gboolean val;
    estring key = make_key(name);
    
    val = gnome_config_get_bool_with_default(key.data(), &defaulted);
    if (defaulted)
    	val = deflt;
    return val;
}

void
confsection_t::set_bool(const char *name, gboolean value)
{
    estring key = make_key(name);

    gnome_config_set_bool(key.data(), value);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
confsection_t::get_int(const char *name, int deflt)
{
    gboolean defaulted = FALSE;
    int val;
    estring key = make_key(name);
    
    val = gnome_config_get_int_with_default(key.data(), &defaulted);
    if (defaulted)
    	val = deflt;
    return val;
}

void
confsection_t::set_int(const char *name, int value)
{
    estring key = make_key(name);

    gnome_config_set_int(key.data(), value);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

float
confsection_t::get_float(const char *name, float deflt)
{
    gboolean defaulted = FALSE;
    float val;
    estring key = make_key(name);
    
    val = gnome_config_get_float_with_default(key.data(), &defaulted);
    if (defaulted)
    	val = deflt;
    return val;
}

void
confsection_t::set_float(const char *name, float value)
{
    estring key = make_key(name);

    gnome_config_set_float(key.data(), value);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
confsection_t::sync()
{
    gnome_config_sync_file((char *)filename);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
