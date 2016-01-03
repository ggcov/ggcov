/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2015 Greg Banks <gnb@users.sourceforge.net>
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

#include "logging.H"
#include "estring.H"
#include "tok.H"

namespace logging
{

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

FILE *logger_t::fp_ = stderr;
level_t logger_t::global_level_ = logging::INFO;
logger_t *logger_t::all_;

logger_t::logger_t(const char *name)
 :  name_(name),
    local_level_((level_t)(logging::_HIGHEST+1))
{
}

logger_t::~logger_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *level_names[] = { "DEBUG2", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL" };

void logger_t::dolog(level_t level, const char *fmt, va_list args)
{
    fprintf(fp_, "%s [%s] ", level_names[level], name_.data());
    vfprintf(fp_, fmt, args);
    fflush(fp_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

logger_t *logger_t::dofind(const char *name)
{
    for (logger_t *l = all_ ; l ; l = l->next_)
    {
	if (!strcmp(l->name_, name))
	    return l;
    }
    return 0;
}

logger_t &logger_t::find(const char *name)
{
    logger_t *l = dofind(name);
    if (!l)
    {
	l = new logger_t(name);
	l->next_ = all_;
	all_ = l;
    }
    return *l;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void logger_t::debug_enable_loggers(const char *str)
{
    tok_t tok(str, " \t\n\r,");
    const char *x;
    level_t level;
    bool verbose = false;

    while ((x = tok.next()) != 0)
    {
	if (*x == '+')
	{
	    level = logging::DEBUG;
	    x++;
	}
	else if (*x == '-')
	{
	    level = logging::INFO;
	    x++;
	}
	else
	    level = logging::DEBUG;

	if (!strcmp(x, "all"))
	{
	    global_level_ = level;
	}
	else if (!strcmp(x, "verbose"))
	{
	    verbose = true;
	}
	else
	{
	    logging::find_logger(x).set_level(level);
	}
    }

    if (verbose)
    {
	// upgrade all DEBUG to DEBUG2
	if (global_level_ == logging::DEBUG)
	    global_level_ = logging::DEBUG2;
	for (logger_t *l = all_ ; l ; l = l->next_)
	    if (l->local_level_ == logging::DEBUG)
		l->set_level(logging::DEBUG2);
    }
}

char *logger_t::describe_debug_enabled_loggers()
{
    estring buf;

    for (logger_t *l = all_ ; l ; l = l->next_)
    {
	if (l->is_enabled(logging::DEBUG))
	{
	    if (buf.length())
		buf.append_char(',');
	    buf.append_string(l->name_);
	}
    }

    return buf.take();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void logger_t::basic_config(level_t level, FILE *fp)
{
    if (!fp)
	fp = stderr;
    fp_ = fp;
    global_level_ = level;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

logger_t &find_logger(const char *name)
{
    return logger_t::find(name);
}

void basic_config(level_t level, FILE *fp)
{
    logger_t::basic_config(level, fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
// close the namespace
}
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
