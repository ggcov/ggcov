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

#ifndef _ggcov_cov_project_params_H_
#define _ggcov_cov_project_params_H_ 1

#include "argparse.H"

class cov_project_params_t : public argparse::params_t
{
public:
    cov_project_params_t();
    ~cov_project_params_t();

    ARGPARSE_BOOL_PROPERTY(recursive);
    ARGPARSE_STRING_PROPERTY(suppressed_calls);
    ARGPARSE_STRING_PROPERTY(suppressed_ifdefs);
    ARGPARSE_STRING_PROPERTY(suppressed_comment_lines);
    ARGPARSE_STRING_PROPERTY(suppressed_comment_ranges);
    ARGPARSE_STRING_PROPERTY(object_directory);
    ARGPARSE_BOOL_PROPERTY(solve_fuzzy);
    ARGPARSE_STRING_PROPERTY(gcda_prefix);
    ARGPARSE_STRING_PROPERTY(debug_str);
    ARGPARSE_BOOL_PROPERTY(print_version_flag);

protected:
    void setup_parser(argparse::parser_t &);
    void post_args();
};

#endif /* _ggcov_cov_project_params_H_ */
