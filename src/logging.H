/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2015 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_logging_H_
#define _ggcov_logging_H_ 1

#include "common.h"
#include "string_var.H"

// global logging class based loosely on Python's "logging" module

namespace logging
{

enum level_t
{
    DEBUG2,
    _LOWEST = DEBUG2,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL,
    _HIGHEST = FATAL
};

#ifdef __GNUC__
#define __logger_expect(expr, val)      __builtin_expect(expr, val)
#else
#define __logger_expect(expr, val)      (expr)
#endif

class logger_t
{
public:
    bool is_enabled(level_t l) const { return (l >= local_level_ || l >= global_level_); }
    void set_level(level_t l) { local_level_ = l; }
#define LOGME(lev) \
    { \
	if (__logger_expect(is_enabled(lev), 0)) \
	{ \
	    va_list args; \
	    va_start(args, fmt); \
	    dolog(lev, fmt, args); \
	    va_end(args); \
	} \
    }
    void debug2(const char *fmt, ...) LOGME(logging::DEBUG2)
    void debug(const char *fmt, ...) LOGME(logging::DEBUG)
    void info(const char *fmt, ...) LOGME(logging::INFO)
    void warning(const char *fmt, ...) LOGME(logging::WARNING)
    void error(const char *fmt, ...) LOGME(logging::ERROR)
    void fatal(const char *fmt, ...) LOGME(logging::FATAL)
    void message(level_t level, const char *fmt, ...) LOGME(level)
    void vmessage(level_t level, const char *fmt, va_list args)
    {
	if (__logger_expect(is_enabled(level), 0))
	    dolog(level, fmt, args);
    }
#undef LOGME

    void perror(const char *filename) { error("%s: %s\n", filename, strerror(errno)); }

    static logger_t &find(const char *name);
    static void basic_config(level_t level, FILE *fp = 0);
    static void debug_enable_loggers(const char *str);
    static char *describe_debug_enabled_loggers();

private:
    logger_t(const char *);
    ~logger_t();

    static logger_t *dofind(const char *name);
    void dolog(level_t level, const char *fmt, va_list args);

    static level_t global_level_;
    static FILE *fp_;
    static logger_t *all_;
    logger_t *next_;
    string_var name_;
    level_t local_level_;
};

logger_t &find_logger(const char *name);
void basic_config(level_t level, FILE *fp = 0);

// close the namespace
}
#endif /* _ggcov_logging_H_ */
