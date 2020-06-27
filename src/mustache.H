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

#ifndef _ggcov_mustache_H_
#define _ggcov_mustache_H_ 1

#include "common.h"
#include "filename.h"
#include "string_var.H"
#include "yaml_generator.H"

namespace mustache
{

class template_t;

class environment_t
{
public:
    environment_t();
    ~environment_t();

    void set_template_directory(const char *dir) { template_dir_ = dir; }
    const char *get_template_directory() { return template_dir_.data(); }
    void set_output_directory(const char *dir) { output_dir_ = dir; }
    const char *get_output_directory() { return output_dir_.data(); }

    template_t *make_template(const char *name = 0, const char *output_name = 0);

private:
    string_var template_dir_;
    string_var output_dir_;

    friend class template_t;
};

class template_t
{
public:
    ~template_t();

    void set_template_name(const char *name)
    {
	template_path_ = file_make_absolute_to_dir(name, env_.template_dir_);
    }
    void set_output_name(const char *name)
    {
	output_path_ = file_make_absolute_to_dir(name, env_.output_dir_);
    }

    yaml_generator_t &begin_render();
    void end_render();

private:
    // only for environment_t
    template_t(environment_t &env, const char *name, const char *output_name);
    void cleanup();

    environment_t &env_;
    string_var template_path_;
    string_var output_path_;
    FILE *stdin_fp_;
    std::basic_filebuf<char> *stdin_sb_;
    std::ostream *stdin_stream_;
    yaml_generator_t *yaml_;

    friend class environment_t;
};

// end the namespace
}
#endif /* _ggcov_mustache_H_ */
