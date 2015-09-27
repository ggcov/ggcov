/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2005 Greg Banks <gnb@users.sourceforge.net>
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
#ifndef HAVE_LIBGCONF
#define HAVE_LIBGCONF 0
#endif

#if HAVE_LIBGCONF
#include <gconf/gconf-client.h>
#else
#include <libgnome/libgnome.h>
#endif
#include "confsection.H"
#include "estring.H"

CVSID("$Id: confsection.C,v 1.11 2010-05-09 05:37:14 gnb Exp $");

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

#if HAVE_LIBGCONF
static void
handle_error(const char *name, GError *e)
{
    if (e)
    {
	fprintf(stderr, "ERROR: confsection(%s): %s\n", name, e->message);
	g_error_free(e);
    }
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
confsection_t::make_key(const char *name) const
{
#if HAVE_LIBGCONF
    return g_strconcat("/apps/", filename, "/", secname_.data(), "/", name, (char *)0);
#else
    assert(strchr(name, '/') == 0);
    return g_strconcat(filename, "/", secname_.data(), "/", name, (char *)0);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
confsection_t::get_string(const char *name, const char *deflt)
{
    char *val;
    string_var key = make_key(name);

#if HAVE_LIBGCONF
    GError *e = 0;
    GConfValue *gcv = gconf_client_get(gconf_client_get_default(), key, &e);
    handle_error(secname_, e);
    if (gcv == 0)
    {
	val = (deflt == 0 ? 0 : g_strdup(deflt));
    }
    else
    {
	val = g_strdup(gconf_value_get_string(gcv));
	gconf_value_free(gcv);
    }
#else
    gboolean defaulted = FALSE;
    val = gnome_config_get_string_with_default(key.data(), &defaulted);
    if (defaulted)
	val = (deflt == 0 ? 0 : g_strdup(deflt));
#endif

    return val;
}

void
confsection_t::set_string(const char *name, const char *value)
{
    string_var key = make_key(name);

#if HAVE_LIBGCONF
    GError *e = 0;
    gconf_client_set_string(gconf_client_get_default(),
			    key.data(), value, &e);
    handle_error(secname_, e);
#else
    gnome_config_set_string(key.data(), value);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
confsection_t::get_enum(const char *name, const confenum_t *tbl, int deflt)
{
    string_var val = get_string(name, 0);

    if (val == (char *)0)
	return deflt;

    for ( ; tbl->string != 0 ; tbl++)
    {
	if (!strcasecmp(tbl->string, val))
	    return tbl->value;
    }

    if (isdigit(val.data()[0]))
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
    char buf[32];

    for ( ; tbl->string != 0 ; tbl++)
    {
	if (value == tbl->value)
	{
	    set_string(name, tbl->string);
	    return;
	}
    }

    snprintf(buf, sizeof(buf), "%d", value);
    set_string(name, buf);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
confsection_t::get_bool(const char *name, gboolean deflt)
{
    gboolean val;
    string_var key = make_key(name);

#if HAVE_LIBGCONF
    GError *e = 0;
    GConfValue *gcv = gconf_client_get(gconf_client_get_default(), key, &e);
    handle_error(secname_, e);
    if (gcv == 0)
    {
	val = deflt;
    }
    else
    {
	val = gconf_value_get_bool(gcv);
	gconf_value_free(gcv);
    }
#else
    gboolean defaulted = FALSE;
    val = gnome_config_get_bool_with_default(key.data(), &defaulted);
    if (defaulted)
	val = deflt;
#endif
    return val;
}

void
confsection_t::set_bool(const char *name, gboolean value)
{
    string_var key = make_key(name);

#if HAVE_LIBGCONF
    GError *e = 0;
    gconf_client_set_bool(gconf_client_get_default(),
			  key, value, &e);
#else
    gnome_config_set_bool(key.data(), value);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
confsection_t::get_int(const char *name, int deflt)
{
    int val;
    string_var key = make_key(name);

#if HAVE_LIBGCONF
    GError *e = 0;
    GConfValue *gcv = gconf_client_get(gconf_client_get_default(), key, &e);
    handle_error(secname_, e);
    if (gcv == 0)
    {
	val = deflt;
    }
    else
    {
	val = gconf_value_get_int(gcv);
	gconf_value_free(gcv);
    }
#else
    gboolean defaulted = FALSE;
    val = gnome_config_get_int_with_default(key.data(), &defaulted);
    if (defaulted)
	val = deflt;
#endif
    return val;
}

void
confsection_t::set_int(const char *name, int value)
{
    string_var key = make_key(name);

#if HAVE_LIBGCONF
    GError *e = 0;
    gconf_client_set_int(gconf_client_get_default(),
			 key, value, &e);
    handle_error(secname_, e);
#else
    gnome_config_set_int(key.data(), value);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

float
confsection_t::get_float(const char *name, float deflt)
{
    float val;
    string_var key = make_key(name);

#if HAVE_LIBGCONF
    GError *e = 0;
    GConfValue *gcv = gconf_client_get(gconf_client_get_default(), key, &e);
    handle_error(secname_, e);
    if (gcv == 0)
    {
	val = deflt;
    }
    else
    {
	val = gconf_value_get_float(gcv);
	gconf_value_free(gcv);
    }
#else
    gboolean defaulted = FALSE;
    val = gnome_config_get_float_with_default(key.data(), &defaulted);
    if (defaulted)
	val = deflt;
#endif
    return val;
}

void
confsection_t::set_float(const char *name, float value)
{
    string_var key = make_key(name);

#if HAVE_LIBGCONF
    GError *e = 0;
    gconf_client_set_float(gconf_client_get_default(), key, value, &e);
    handle_error(secname_, e);
#else
    gnome_config_set_float(key.data(), value);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
confsection_t::sync()
{
#if !HAVE_LIBGCONF
    gnome_config_sync_file((char *)filename);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
