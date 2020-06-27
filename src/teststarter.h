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

#ifndef __GGCOV_TESTSTARTER_H__
#define __GGCOV_TESTSTARTER_H__ 1

#include "estring.H"
#include "string_var.H"
#include "list.H"

class test_starter_t
{
public:
    test_starter_t();
    test_starter_t(const char *builddir);
    ~test_starter_t();

    static void set_base_dir(const char *dir);

    class sourcefile_t
    {
    public:
	sourcefile_t(const char *filename) : filename_(filename) {}
	~sourcefile_t() {}

	/* chaining mutators */
	sourcefile_t &source(const char *s) { source_ = s; return *this; }
	sourcefile_t &lang(const char *l) { lang_ = l; return *this; }
	sourcefile_t &exe(const char *e)
	{
	    if (exes_.length())
		exes_.append_char(',');
	    exes_.append_string(e);
	    return *this;
	}
	/* accessors */
	const char *filename() const { return filename_.data(); }
	const estring &source() const { return source_; }
	const char *lang() const { return lang_.data() ? lang_.data() : default_lang; }
	const char *exes() const { return exes_.data() ? exes_.data() : default_exe; }
    private:
	string_var filename_;
	estring source_;
	string_var lang_;
	estring exes_;
    };
    sourcefile_t &add_sourcefile(const char *filename);

    class run_t
    {
    public:
	run_t() {};
	~run_t() {};

	/* chaining mutators */
	run_t &exe(const char *e) { exe_ = e; return *this; }
	run_t &arg(int a) { arg_ = a; return *this; }

	/* accessors */
	const char *exe() const { return exe_.data() ? exe_.data() : default_exe; }
	int arg() const { return arg_; }
    private:
	string_var exe_;
	int arg_;
    };
    run_t &add_run();

    /* Add a filename which will be passed to cov_read_files().
     * Default is to read the directory recursively */
    test_starter_t &add_root_file(const char *file)
    {
	root_files_.append(g_strdup(file));
	return *this;
    }

    /* call this in your setup() after calling add_sourcefile() and
     * add_run() then return its return value. */
    int start();

private:
    void write_file(const char *filename, const char *data, unsigned int len, mode_t mode);
    void write_file(const char *filename, const estring &s, mode_t mode);
    void write_file(const char *filename, const string_var &s, mode_t mode);
    int build();
    static const char default_exe[];
    static const char default_lang[];
    static string_var base_dir;

    string_var builddir_;
    list_t<sourcefile_t> sourcefiles_;
    list_t<run_t> runs_;
    list_t<char> root_files_;
    enum { IDLE, INCHILD, INPARENT } state_;
};

#endif /* __GGCOV_TESTSTARTER_H__ */
