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

#ifndef _ggcov_cov_scope_H_
#define _ggcov_cov_scope_H_ 1

#include "common.h"
#include "cov_types.H"

/*
 * Class cov_scope_t reifies a scope to which a coverage
 * statistics summary can be calculated.
 */
class cov_scope_t
{
public:
    cov_scope_t();
    virtual ~cov_scope_t();

    const cov_stats_t *get_stats();
    virtual const char *describe() const = 0;
    cov::status_t status();

protected:
    virtual cov::status_t calc_stats(cov_stats_t *) = 0;
    void dirty();
    
private:
    gboolean dirty_;
    cov_stats_t stats_;
    cov::status_t status_;
};


class cov_overall_scope_t : public cov_scope_t
{
public:
    cov_overall_scope_t();
    ~cov_overall_scope_t();

    virtual const char *describe() const;
private:
    virtual cov::status_t calc_stats(cov_stats_t *);
};


class cov_file_t;
class cov_file_scope_t : public cov_scope_t
{
public:
    cov_file_scope_t(const char *filename);
    cov_file_scope_t(const cov_file_t *);
    ~cov_file_scope_t();

    virtual const char *describe() const;
private:
    virtual cov::status_t calc_stats(cov_stats_t *);

    const cov_file_t *file_;
};

/* TODO: directory or group of files */

class cov_function_t;
class cov_function_scope_t : public cov_scope_t
{
public:
    cov_function_scope_t(const cov_function_t *);
    ~cov_function_scope_t();

    const cov_function_t *function() const
    {
    	return function_;
    }
    
    virtual const char *describe() const;
private:
    virtual cov::status_t calc_stats(cov_stats_t *);

    const cov_function_t *function_;
};

class cov_range_scope_t : public cov_scope_t
{
public:
    cov_range_scope_t(const char *filename,
    	    	      unsigned long start, unsigned long end);
    cov_range_scope_t(const cov_file_t *,
    	    	      unsigned long start, unsigned long end);
    ~cov_range_scope_t();

    virtual const char *describe() const;
private:
    virtual cov::status_t calc_stats(cov_stats_t *);
    void set_description();

    const cov_file_t *file_;
    unsigned long start_;
    unsigned long end_;
    char *description_;
};

class cov_compound_scope_t : public cov_scope_t
{
public:
    cov_compound_scope_t();
    cov_compound_scope_t(list_t<cov_scope_t> *);
    ~cov_compound_scope_t();
    
    void add_child(cov_scope_t *);
    void remove_child(cov_scope_t *);

    virtual const char *describe() const;
private:
    virtual cov::status_t calc_stats(cov_stats_t *);

    list_t<cov_scope_t> children_;
};

#endif /* _ggcov_cov_scope_H_ */
