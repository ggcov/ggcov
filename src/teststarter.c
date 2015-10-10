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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <sys/wait.h>
#include <errno.h>
#include "filename.H"
#include "hashtable.H"
#include "tok.H"
#include "cov.H"
#include "testfw.h"
#include "teststarter.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void _dmsg(const char *fn, const char *fmt, ...)
{
    if (testrunner_t::verbose())
    {
	fprintf(stderr, "%s %d ", fn, getpid());
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
    }
}

#define dmsg(...) \
    _dmsg(__FUNCTION__, __VA_ARGS__)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char test_starter_t::default_exe[] = "foo";
const char test_starter_t::default_lang[] = "c";
string_var test_starter_t::base_dir = "..";

test_starter_t::test_starter_t()
{
    state_ = IDLE;
}

test_starter_t::test_starter_t(const char *builddir)
 :  builddir_(builddir)
{
}

test_starter_t::~test_starter_t()
{
}

test_starter_t::sourcefile_t &test_starter_t::add_sourcefile(const char *filename)
{
    sourcefile_t *sf = new sourcefile_t(filename);
    sourcefiles_.append(sf);
    return *sf;
}

test_starter_t::run_t &test_starter_t::add_run()
{
    run_t *run = new run_t();
    runs_.append(run);
    return *run;
}

static gboolean remove_one_linkline(const char *key, estring *value, void *closure)
{
    delete value;
    return TRUE;    /* please remove me */
}

void test_starter_t::write_file(const char *filename, const char *data, unsigned int len, mode_t mode)
{
    string_var path = g_strdup_printf("%s/%s", builddir_.data(), filename);
    FILE *fp = fopen(path, "w");
    if (!fp)
    {
	perror(path);
	return;
    }
    fchmod(fileno(fp), mode);
    while (len > 0)
    {
	int n = fwrite(data, 1, len, fp);
	if (n < 0)
	{
	    perror(path);
	    fclose(fp);
	    return;
	}
	data += n;
	len -= n;
    }
    fclose(fp);
}

void test_starter_t::write_file(const char *filename, const estring &s, mode_t mode)
{
    write_file(filename, s.data(), s.length(), mode);
}

void test_starter_t::write_file(const char *filename, const string_var &s, mode_t mode)
{
    write_file(filename, s.data(), s.length(), mode);
}

int test_starter_t::build()
{
    dmsg("Starting");

    if (!builddir_.data())
    {
	testfn_t *running = testrunner_t::running();
	builddir_ = g_strdup_printf("%s/tmp.%s.%s.d",
				    base_dir.data(),
				    running->suite(),
				    running->name());
	dmsg("Building builddir %s", builddir_.data());
	mkdir(builddir_, 0755);
    }

    estring buildscript(
"#!/usr/bin/env bash\n"
"\n"
". ../common.sh\n"
"\n"
"init \"generated build script\"\n"
"\n");

    hashtable_t<const char, estring> *linklines = new hashtable_t<const char, estring>;

    /* emit a main.c and build it without coverage */
    static const char main_c[] =
"#include <stdlib.h>\n"
"\n"
"extern int function_one(int arg);\n"
"\n"
"int main(int argc, char **argv)\n"
"{\n"
"    int i;\n"
"    for (i = 1 ; i < argc ; i++) function_one(atoi(argv[i]));\n"
"    return 0;\n"
"}\n"
"\n";
    write_file("main.c", main_c, sizeof(main_c)-1, 0644);
    buildscript.append_string("compile_c --no-coverage main.c\n");

    for (list_iterator_t<sourcefile_t> sfitr = sourcefiles_.first() ; *sfitr ; ++sfitr)
    {
	sourcefile_t *sf = *sfitr;

	/* write out the file contents */
	write_file(sf->filename(), sf->source(), 0644);

	/* update the buildscript with a "compile" command */
	buildscript.append_printf("compile_%s %s\n", sf->lang(), sf->filename());

	/* update the linklines hash for all exes this source belongs in */
	tok_t tok(sf->exes(), ",");
	const char *exe;
	while ((exe = tok.next()) != 0)
	{
	    estring *linkline = linklines->lookup(exe);
	    if (!linkline)
	    {
		linkline = new estring();
		linkline->append_printf("link %s main.o", exe);
		linklines->insert(exe, linkline);
	    }
	    string_var objfile = file_change_extension(sf->filename(), 0, ".o");
	    linkline->append_string(" ");
	    linkline->append_string(objfile);
	}
    }

    /* add the delayed linklines to the buildscript */
    for (hashtable_iter_t<const char, estring> llitr = linklines->first() ; *llitr ; ++llitr)
	buildscript.append_string((*llitr)->data());
    linklines->foreach_remove(remove_one_linkline, 0);
    delete linklines;

    /* add runs to the buildscript */
    buildscript.append_string(" \n");
    for (list_iterator_t<run_t> ritr = runs_.first() ; *ritr ; ++ritr)
    {
	run_t *r = *ritr;
	buildscript.append_printf("run %s %d\n", r->exe(), r->arg());
    }

    if (testrunner_t::verbose())
	buildscript.append_string("ls -AFC\n");

    /* the shell code complains if there's no test result */
    buildscript.append_string("silentpass\n");

    /* write out the build script */
    write_file("runtest", buildscript, 0755);

    estring cmd;
    cmd.append_printf("cd %s && ./runtest", builddir_.data());
    if (testrunner_t::verbose())
	cmd.append_string(" --no-log");
    dmsg("Running: %s", cmd.data());
    return system(cmd);
}

int test_starter_t::start()
{
    int r = build();
    if (r)
	return r;

    list_t<const char> files;
    for (list_iterator_t<char> itr = root_files_.first() ; *itr ; ++itr)
    {
	estring e;
	e.append_string(builddir_.data());
	e.append_string("/");
	e.append_string(*itr);
	files.append(e.take());
    }
    if (!files.head())
    {
	cov_set_recursive(TRUE);
	files.append(g_strdup(builddir_.data()));
    }
    cov_read_files(files);

    for (list_iterator_t<const char> citr = files.first() ; *citr ; ++citr)
	g_free((char *)*citr);

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
