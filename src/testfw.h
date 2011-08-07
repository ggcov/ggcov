/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2011 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef __GGCOV_TESTFW_H__
#define __GGCOV_TESTFW_H__ 1

#include <sys/types.h>
#include <stdarg.h>

extern int _testfw_verbose;

extern void __testfw_check(int pass, const char *file, int line, const char *fmt, ...);

#define check(expr) \
    __testfw_check((expr), __FILE__, __LINE__, "check(%s)", #expr)

#define check_str_equals(a, b) \
{ \
    const char *_a = (a); \
    const char *_b = (b); \
    const char *_sa = (_a) ? (_a) : ""; \
    const char *_sb = (_b) ? (_b) : ""; \
    __testfw_check(!strcmp(_sa, _sb), \
		   __FILE__, __LINE__, \
		   "check_str_equals(%s=\"%s\", %s=\"%s\")", \
		    #a, _sa, #b, _sb); \
}

struct testfn_t
{
    const char *name_;
    const char *suite_;
    void (*function_)(void);
    enum role_t { RSETUP, RTEARDOWN, RTEST } role_;
    testfn_t *next_;
    static testfn_t *head_, **tailp_;

    // c'tor
    testfn_t(const char *name,
	     const char *filename,
	     void (*function)(void),
	     role_t role)
     :  name_(name),
	suite_(filename),   // will be tweaked in testfw_init()
	function_(function),
	role_(role),
	next_(0)
    {
	*tailp_ = this;
	tailp_ = &next_;
    }

    void init();
    void uninit();
    void run();
};

#define _PASTE(a,b)	a##b

#define _TESTFN(rtype, name, role) \
    static rtype name(void) __attribute__((unused));\
    static testfn_t _PASTE(__testfn_,name) = testfn_t \
	(#name, __FILE__, (void(*)(void))name, role); \
    static rtype name(void)

#define TEST(name)  _TESTFN(void, name, testfn_t::RTEST)
#define SETUP	    _TESTFN(int, setup, testfn_t::RSETUP)
#define TEARDOWN    _TESTFN(int, teardown, testfn_t::RTEARDOWN)

extern void testfw_init(void);
extern void testfw_set_verbose(int);
extern void testfw_list(void);
extern int testfw_schedule(const char *arg);
extern void testfw_run(void);

#endif /* __GGCOV_TESTFW_H__ */
