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
#include "mustache.H"
#include "testfw.h"

#define expect(_e) \
    { \
	const char *actual = buf.str().c_str(); \
	static const char expected[] = _e; \
	check_str_equals(actual, expected); \
    }

mustache::environment_t *menv;

SETUP
{
    string_var tmpldir = file_temp_directory("mustache-test-templates");
    dmsg("tmpldir=\"%s\"", tmpldir.data());
    string_var outdir = file_temp_directory("mustache-test-output");
    dmsg("outdir=\"%s\"", outdir.data());
    menv = new mustache::environment_t();
    menv->set_template_directory(tmpldir);
    menv->set_output_directory(outdir);
    return 0;
}

TEARDOWN
{
    delete menv;
    return 0;
}

static void generate_template(const char *name, const char *text)
{
    string_var path = g_strconcat(menv->get_template_directory(), "/", name, (char *)0);
    dmsg("path=\"%s\"", path.data());
    FILE *fp = fopen(path, "w");
    check_not_null(fp);
    fputs(text, fp);
    fclose(fp);
}

static void check_output(const char *name, const char *expected)
{
    string_var path = g_strconcat(menv->get_output_directory(), "/", name, (char *)0);
    char actual[4096];

    dmsg("path=\"%s\"", path.data());
    FILE *fp = fopen(path, "r");
    if (fp == 0)
	perror(path);
    check_not_null(fp);
    check_not_null(fgets(actual, sizeof(actual), fp));
    fclose(fp);

    check_str_equals(actual, expected);
}

TEST(basic)
{
    generate_template("basic.txt", "Hello {{name}}!\n");
    mustache::template_t *tmpl = menv->make_template("basic.txt");
    yaml_generator_t &yaml = tmpl->begin_render();
    yaml.begin_mapping();
    yaml.key("name").value("Fred");
    yaml.end_mapping();
    tmpl->end_render();
    check_output("basic.txt", "Hello Fred!\n");
    delete tmpl;
}

