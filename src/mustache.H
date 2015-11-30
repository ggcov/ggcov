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

#ifndef _ggcov_mustache_H_
#define _ggcov_mustache_H_ 1

#include "common.h"
#include "filename.h"
#include "string_var.h"
#include "yaml_generator.H"

class mustache_t
{
public:
    mustache_t(const char *name, const char *output_name);
    ~mustache_t();

    void set_template_name(const char *name)
    {
	template_path_ = file_make_absolute_to_dir(name, template_dir_);
    }
    void set_output_name(const char *name)
    {
	output_path_ = file_make_absolute_to_dir(name, output_dir_);
    }

    yaml_generator_t &begin_render();
    void end_render();

    static void set_template_directory(const char *dir) { template_dir_ = dir; }
    static const char *get_template_directory() { return template_dir_.data(); }
    static void set_output_directory(const char *dir) { output_dir_ = dir; }
    static const char *get_output_directory() { return output_dir_.data(); }

private:
    void cleanup();

    string_var template_path_;
    string_var output_path_;
    FILE *stdin_fp_;
    std::basic_filebuf<char> *stdin_sb_;
    std::ostream *stdin_stream_;
    yaml_generator_t *yaml_;

    static string_var template_dir_;
    static string_var output_dir_;
};


#endif /* _ggcov_mustache_H_ */
