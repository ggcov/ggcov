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
 :  sent_(false),
    content_type_(0)
{
}

cgi_t::~cgi_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cgi_t::set_replym(const char *type, char *body)
{
    content_type_ = type;
    reply_body_ = body;
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

    set_replym("text/html", msg.take());
    reply();
}

void
cgi_t::reply()
{
    if (sent_)
	return;
    sent_ = true;

    if (content_type_ == 0)
    {
	// sets content_type_ as a side effect
	error("No reply content type set");
    }

    printf("Content-type: %s\r\n", content_type_);
    printf("\r\n");
    if (reply_body_.data())
	fputs(reply_body_.data(), stdout);
    fflush(stdout);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
