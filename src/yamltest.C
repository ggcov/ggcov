/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2015 Greg Banks <gnb@fastmail.fm>
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
#include "common.h"
#include "yaml_generator.H"
#include <assert.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include "testfw.H"

#define expect(_e) \
    { \
	std::string _s = buf.str(); \
	const char *actual = _s.c_str(); \
	static const char expected[] = _e; \
	check_str_equals(actual, expected); \
    }

TEST(empty_sequence)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    yaml.end_sequence();

    expect("");
}

TEST(one_int_sequence)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    yaml.value(42);
    yaml.end_sequence();

    expect("- 42\n");
}

TEST(several_int_sequence)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    yaml.value(1);
    yaml.value(42);
    yaml.value(1337);
    yaml.end_sequence();

    expect(
	"- 1\n"
	"- 42\n"
	"- 1337\n");
}

TEST(one_string_sequence)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    yaml.value("hello");
    yaml.end_sequence();

    expect(
	"- \"hello\"\n");
}

TEST(several_string_sequence)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    yaml.value("is");
    yaml.value("it");
    yaml.value("me");
    yaml.value("you're");
    yaml.value("looking");
    yaml.value("for");
    yaml.end_sequence();

    expect(
	"- \"is\"\n"
	"- \"it\"\n"
	"- \"me\"\n"
	"- \"you're\"\n"
	"- \"looking\"\n"
	"- \"for\"\n");
}

TEST(empty_mapping)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_mapping();
    yaml.end_mapping();

    expect("");
}

TEST(one_int_mapping)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_mapping();
    yaml.key("answer");
    yaml.value(42);
    yaml.end_mapping();

    expect(
	"answer: 42\n");
}

TEST(several_int_mapping)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_mapping();
    yaml.key("width");
    yaml.value(23);
    yaml.key("height");
    yaml.value(378);
    yaml.key("depth");
    yaml.value(3);
    yaml.end_mapping();

    expect(
	"width: 23\n"
	"height: 378\n"
	"depth: 3\n");
}

TEST(sequence_in_sequence)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    yaml.value(1);
    yaml.begin_sequence();
    yaml.value(42);
    yaml.value(1337);
    yaml.end_sequence();
    yaml.value(3);
    yaml.end_sequence();

    expect(
	"- 1\n"
	"  - 42\n"
	"  - 1337\n"
	"- 3\n");
}


TEST(mapping_in_sequence)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    yaml.begin_mapping();
    yaml.key("answer");
    yaml.value(42);
    yaml.key("value");
    yaml.value(1337);
    yaml.end_mapping();
    yaml.end_sequence();

    expect(
	"- answer: 42\n"
	"  value: 1337\n");
}

TEST(mapping_in_bigger_sequence)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    yaml.value("freegan");
    yaml.begin_mapping();
    yaml.key("answer");
    yaml.value(42);
    yaml.key("value");
    yaml.value(1337);
    yaml.end_mapping();
    yaml.value("vinyl");
    yaml.end_sequence();

    expect(
	"- \"freegan\"\n"
	"- answer: 42\n"
	"  value: 1337\n"
	"- \"vinyl\"\n");
}

TEST(mapping_in_mapping)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_mapping();
    yaml.key("values");
    yaml.begin_mapping();
    yaml.key("length");
    yaml.value(23);
    yaml.key("width");
    yaml.value(45);
    yaml.end_mapping();
    yaml.end_mapping();

    expect(
	"values:\n"
	"  length: 23\n"
	"  width: 45\n");
}

TEST(sequence_in_mapping)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_mapping();
    yaml.key("values");
    yaml.begin_sequence();
    yaml.value(23);
    yaml.value(45);
    yaml.end_sequence();
    yaml.end_mapping();

    expect(
	"values:\n"
	"  - 23\n"
	"  - 45\n");
}

TEST(sequence_in_bigger_mapping)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_mapping();
    yaml.key("mustache");
    yaml.value("seitan");
    yaml.key("values");
    yaml.begin_sequence();
    yaml.value(23);
    yaml.value(45);
    yaml.end_sequence();
    yaml.key("shoreditch");
    yaml.value("dreamcatcher");
    yaml.end_mapping();

    expect(
	"mustache: \"seitan\"\n"
	"values:\n"
	"  - 23\n"
	"  - 45\n"
	"shoreditch: \"dreamcatcher\"\n");
}

TEST(deeper_mappings)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_mapping();
    yaml.key("fixie");
    yaml.begin_mapping();
    yaml.key("mustache");
    yaml.begin_mapping();
    yaml.key("dreamcatcher");
    yaml.begin_mapping();
    yaml.key("leggings");
    yaml.begin_mapping();
    yaml.key("brooklyn");
    yaml.value("shoreditch");
    yaml.end_mapping();
    yaml.end_mapping();
    yaml.end_mapping();
    yaml.end_mapping();
    yaml.end_mapping();

    expect(
	"fixie:\n"
	"  mustache:\n"
	"    dreamcatcher:\n"
	"      leggings:\n"
	"        brooklyn: \"shoreditch\"\n");
}

TEST(utf8_basic_plane)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    // 3 characters: 'I', U+2764 Heavy Black Heart, 'U'
    yaml.value("I\xe2\x9d\xa4U");
    yaml.end_sequence();

    expect("- \"I\\u2764U\"\n");
}

TEST(utf8_supplementary_plane)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    // 3 characters: 'X', U+1201F Cuneiform Sign Ak Times Shita Plus Gish, 'Y'
    yaml.value("X\xf0\x92\x80\x9fY");
    yaml.end_sequence();

    expect("- \"X\\uD808\\uDC1FY\"\n");
}

TEST(utf8_broken1)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    // broken UTF-8 string
    // 3 bytes: 'X', bogus UTF-8 trailing byte, 'Y'
    yaml.value("X\x80Y");
    yaml.end_sequence();

    expect("- \"X\\uFFFDY\"\n");
}

TEST(utf8_broken2)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    // broken UTF-8 string
    // 4 bytes: 'X', bogus UTF-8 trailing byte,
    // another bogus UTF-8 trailing byte, 'Y'
    yaml.value("X\x80\xbfY");
    yaml.end_sequence();

    expect("- \"X\\uFFFD\\uFFFDY\"\n");
}

TEST(xfloat)
{
    std::ostringstream buf;
    yaml_generator_t yaml(buf);

    yaml.begin_sequence();
    yaml.value(1.0);
    yaml.value(1000.0);
    yaml.value(1000000.0);
    yaml.value(1000000000.0);
    yaml.value(1000000000000.0);
    yaml.end_sequence();

    expect(
	"- 1.0\n"
	"- 1000.0\n"
	"- 1000000.0\n"
	"- 1000000000.0\n"
	"- 1000000000000.0\n");
}

#undef expect
