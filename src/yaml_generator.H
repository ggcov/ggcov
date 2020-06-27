/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
#ifndef __ggcov_yaml_generator_H__
#define __ggcov_yaml_generator_H__ 1

#include "common.h"
#include "unicode.H"
#include <ostream>
#include <iomanip>
#include <string>
#include <vector>

class yaml_generator_t
{
public:
    yaml_generator_t(std::ostream &o) : o_(o), first_line_(true) { stack_.reserve(16); }
    ~yaml_generator_t() { stack_.clear(); }

    yaml_generator_t &key(const char *s);
    yaml_generator_t &key(const std::string &s) { return key(s.c_str()); }

    yaml_generator_t &value(const char *s)
    {
	begin_value();
	escape_utf8_string(s, o_);
	return *this;
    }
    yaml_generator_t &value(const std::string &s) { return value(s.c_str()); }
    yaml_generator_t &value(double v)
    {
	begin_value();
	o_ << std::fixed << std::setprecision(1) << v;
	return *this;
    }
    yaml_generator_t &value(uint64_t v)
    {
	begin_value();
	o_ << v;
	return *this;
    }
    yaml_generator_t &value(int v)
    {
	begin_value();
	o_ << v;
	return *this;
    }
    yaml_generator_t &value(unsigned int v)
    {
	begin_value();
	o_ << v;
	return *this;
    }
    yaml_generator_t &bool_value(bool v)
    {
	begin_value();
	o_ << (v ? "true" : "false");
	return *this;
    }

    yaml_generator_t &begin_sequence();
    yaml_generator_t &end_sequence();

    yaml_generator_t &begin_mapping();
    yaml_generator_t &end_mapping();

private:
    struct stack_t
    {
	enum type_t { MAPPING, SEQUENCE } type;
	unsigned int count;

	stack_t(type_t t) : type(t), count(0) {}
    };
    stack_t &top();
    void begin_value();
    void not_a_key();
    bool is_in_sequence() const
    {
	return (stack_.size() > 0 && stack_.back().type == stack_t::SEQUENCE);
    }
    bool is_in_mapping() const
    {
	return (stack_.size() > 0 && stack_.back().type == stack_t::MAPPING);
    }
    void line_break();

    std::ostream &o_;
    std::vector<stack_t> stack_;
    bool first_line_;
};

#endif /* __ggcov_yaml_generator_H__ */
