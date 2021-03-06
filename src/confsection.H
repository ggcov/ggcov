/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2020 Greg Banks <gnb@fastmail.fm>
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

#ifndef _ggcov_confsection_H_
#define _ggcov_confsection_H_ 1

#include "common.h"
#include "string_var.H"
#include "hashtable.H"

struct confenum_t
{
    const char *string;
    int value;
};

class confsection_t
{
public:
    static confsection_t *get(const char *secname);

    // returns a new string that must be freed
    char *get_string(const char *name, const char *deflt);
    void set_string(const char *name, const char *value);

    int get_enum(const char *name, const confenum_t *tbl, int deflt);
    void set_enum(const char *name, const confenum_t *tbl, int value);

    gboolean get_bool(const char *name, gboolean deflt);
    void set_bool(const char *name, gboolean value);

    int get_int(const char *name, int deflt);
    void set_int(const char *name, int value);

    float get_float(const char *name, float deflt);
    void set_float(const char *name, float value);

    static void sync();

private:
    confsection_t(const char *pref);
    ~confsection_t();

    char *make_key(const char *name) const;

    static hashtable_t<const char, confsection_t> *all_;

    string_var secname_;
};


#endif /* _ggcov_confsection_H_ */
