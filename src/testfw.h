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

#define check(expr) \
    testrunner_t::_check((expr), __FILE__, __LINE__, "check(%s)", #expr)

#define check_num_equals(a, b) \
{ \
    long long _a = (a); \
    long long _b = (b); \
    testrunner_t::_check(_a == _b, \
		   __FILE__, __LINE__, \
		   "check_num_equals(%s=%lld, %s=%lld)", \
		    #a, _a, #b, _b); \
}

#define check_null(p) \
{ \
    void *_p = (void *)(p); \
    testrunner_t::_check(_p == 0, \
		   __FILE__, __LINE__, \
		   "check_null(%s=%p)", \
		    #p, _p); \
}

#define check_not_null(p) \
{ \
    void *_p = (void *)(p); \
    testrunner_t::_check(_p != 0, \
		   __FILE__, __LINE__, \
		   "check_not_null(%s=%p)", \
		    #p, _p); \
}

#define check_ptr_equals(a, b) \
{ \
    void *_a = (void *)(a); \
    void *_b = (void *)(b); \
    testrunner_t::_check(_a == _b, \
		   __FILE__, __LINE__, \
		   "check_ptr_equals(%s=%p, %s=%p)", \
		    #a, _a, #b, _b); \
}

#define check_str_equals(a, b) \
{ \
    const char *_a = (a); \
    const char *_b = (b); \
    const char *_sa = (_a) ? (_a) : ""; \
    const char *_sb = (_b) ? (_b) : ""; \
    testrunner_t::_check(!strcmp(_sa, _sb), \
		   __FILE__, __LINE__, \
		   "check_str_equals(\n    %s=\"%s\",\n    %s=\"%s\")", \
		    #a, _sa, #b, _sb); \
}

class testrunner_t;

class testfn_t
{
public:
    enum role_t { RSETUP, RTEARDOWN, RTEST };

private:
    const char *name_;
    const char *filename_;
    char *suite_;
    void (*function_)(void);
    enum role_t role_;
    testfn_t *next_;
    static testfn_t *head_, **tailp_;
    testfn_t *before_;
    testfn_t *after_;

    void run();

    friend class testrunner_t;

public:

    // c'tor
    testfn_t(const char *name,
	     const char *filename,
	     void (*function)(void),
	     role_t role)
     :  name_(name),
	filename_(filename),
	suite_(0),
	function_(function),
	role_(role),
	next_(0),
	before_(0),
	after_(0)
    {
	*tailp_ = this;
	tailp_ = &next_;
    }
    // d'tor
    ~testfn_t()
    {
	free(suite_);
    }

    const char *suite();
    const char *name() { return name_; }
};

#define _PASTE(a,b)     a##b

#define _TESTFN(rtype, name, role) \
    static rtype name(void) __attribute__((unused));\
    static testfn_t _PASTE(__testfn_,name) = testfn_t \
	(#name, __FILE__, (void(*)(void))name, role); \
    static rtype name(void)

#define TEST(name)  _TESTFN(void, name, testfn_t::RTEST)
#define SETUP       _TESTFN(int, setup, testfn_t::RSETUP)
#define TEARDOWN    _TESTFN(int, teardown, testfn_t::RTEARDOWN)

class testrunner_t
{
private:
    int verbose_;
    bool forking_;
    testfn_t **scheduled_;
    unsigned int nscheduled_;
    testfn_t *running_;
    static testrunner_t *current_;
    unsigned int nrun_;
    unsigned int npass_;

    void schedule(testfn_t *fn);
    int schedule_matching(const char *suite, const char *name);
    void record_pass(testfn_t *fn);
    void record_fail(testfn_t *fn, const char *details);
    void run_test_in_child(testfn_t *fn);
    void run_test_directly(testfn_t *fn);
    void run_test(testfn_t *fn);

    friend class testfn_t;

public:
    testrunner_t();
    ~testrunner_t();

    static void _check(int pass, const char *file, int line, const char *fmt, ...);

    void set_verbose(int v);
    void set_forking(bool);
    void list();
    int schedule(const char *arg);
    int run();
    static testfn_t *running();
    static int verbose() { return current_ ? current_->verbose_ : 0; }
    static void _dmsg(const char *fn, const char *fmt, ...);
};

#define dmsg(...) \
    testrunner_t::_dmsg(__FUNCTION__, __VA_ARGS__)

#endif /* __GGCOV_TESTFW_H__ */
