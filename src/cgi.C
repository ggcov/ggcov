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

#include "cgi.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cgi_t::cgi_t()
 :  sent_(false)
{
}

cgi_t::~cgi_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cgi_t::set_replym(estring &e, const char *type)
{
    int len = e.length();
    body_.set(e.take(), len);
    content_type_ = type;
}

void
cgi_t::error(const char *fmt, ...)
{
    va_list args;
    estring msg("<html><head><title>CGI Fatal Error</title></head><body><p>");

    va_start(args, fmt);
    // TODO: html escape the string
    msg.append_vprintf(fmt, args);
    va_end(args);

    msg.append_string("</p></body></html>");

    set_reply(msg);
    reply();
}

void
cgi_t::reply()
{
    if (sent_)
	return;
    sent_ = true;

    if (!content_type_.data())
    {
	// sets content_type_ as a side effect
	error("No reply content type set");
    }

    printf("Content-type: %s\r\n", content_type_.data());
    printf("Content-length: %u\r\n", body_.length());
    printf("\r\n");
    if (body_.data())
	fwrite(body_.data(), 1, body_.length(), stdout);
    fflush(stdout);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
