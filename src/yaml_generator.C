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

#include "yaml_generator.H"

using namespace std;


yaml_generator_t &yaml_generator_t::key(const char *s)
{
    if (!is_in_mapping())
	fatal("key() called while not in an mapping");
    stack_t &top = stack_.back();
    if ((top.count & 0x1))
	fatal("key() called when expecting a value");
    line_break();
    o_ << s;
    o_ << ':';
    top.count++;
    return *this;
}

void yaml_generator_t::not_a_key()
{
    if (is_in_mapping() && !(top().count & 0x1))
	fatal("value, sequence, or mapping when expecting a key");
}

yaml_generator_t &yaml_generator_t::begin_sequence()
{
    not_a_key();
    if (stack_.size() > 0)
	top().count++;
    stack_.push_back(stack_t(stack_t::SEQUENCE));
    return *this;
}

yaml_generator_t &yaml_generator_t::end_sequence()
{
    if (!is_in_sequence())
	fatal("end_sequence() called while not in an sequence");
    if (top().count > 0 && stack_.size() == 1)
	o_ << '\n';
    stack_.pop_back();
    return *this;
}

yaml_generator_t &yaml_generator_t::begin_mapping()
{
    not_a_key();
    if (stack_.size() > 0)
	top().count++;
    stack_.push_back(stack_t(stack_t::MAPPING));
    return *this;
}

yaml_generator_t &yaml_generator_t::end_mapping()
{
    if (!is_in_mapping())
	fatal("end_mapping() called while not in an mapping");
    if ((top().count & 0x1))
	fatal("end_mapping() called while expecting value");

    if (top().count > 0 && stack_.size() == 1)
	o_ << '\n';
    stack_.pop_back();
    return *this;
}

yaml_generator_t::stack_t &yaml_generator_t::top()
{
    if (stack_.size() == 0)
	fatal("top() called without any begin_mapping() or begin_sequence()");
    return stack_.back();
}

void yaml_generator_t::begin_value()
{
    not_a_key();
    if (is_in_mapping())
	o_ << ' ';
    else
	line_break();
    top().count++;
}

void yaml_generator_t::line_break()
{
    static const char indent[] = "  ";
    static const char seq_indent[] = "- ";

    if (!first_line_)
	o_ << "\n";
    first_line_ = false;

    int vtop = stack_.size()-1;
    while (vtop >= 0 &&
	   stack_[vtop].type == stack_t::MAPPING &&
	   stack_[vtop].count == 0)
	vtop--;

    if (vtop >= 0 && stack_[vtop].type == stack_t::SEQUENCE)
    {
	for (int i = 0 ; i < vtop ; i++)
	    o_ << indent;
	o_ << seq_indent;
    }
    else
    {
	for (unsigned int i = 1 ; i < stack_.size() ; i++)
	    o_ << indent;
    }
}


