/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2010 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_cgi_H_
#define _ggcov_cgi_H_ 1

#include "common.h"
#include <cgicc/Cgicc.h>
#include "json.H"
#include "string_var.H"

class cgi_t
{
public:
    cgi_t();
    ~cgi_t();

    const char *get_variable(const char *name) const
    {
	return cgi_(name).c_str();
    }

    void set_reply(json_t &json)
    {
	set_replym(json.take(), "application/json");
    }
    void set_reply(estring &e, const char *type = "text/html")
    {
	set_replym(e.take(), type);
    }

    void error(const char *fmt, ...);

    void perror(const char *s)
    {
	error("%s: %s\n", s, strerror(errno));
    }

    void reply();

private:
    cgicc::Cgicc cgi_;
    boolean sent_;
    const char *content_type_;
    string_var reply_body_;

    void set_replym(char *body, const char *type);
};

#endif /* _ggcov_cgi_H_ */
