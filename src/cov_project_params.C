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

#include "cov_project_params.H"
#include "estring.H"
#include "logging.H"

static logging::logger_t &_log = logging::find_logger("dump");

cov_project_params_t::cov_project_params_t()
 :  recursive_(FALSE),
    solve_fuzzy_(FALSE),
    print_version_flag_(FALSE)
{
}

cov_project_params_t::~cov_project_params_t()
{
}

void
cov_project_params_t::setup_parser(argparse::parser_t &parser)
{
    argparse::params_t::setup_parser(parser);
    parser.add_option('r', "recursive")
	  .description("recursively scan directories for source")
	  .setter((argparse::noarg_setter_t)&cov_project_params_t::set_recursive);
    parser.add_option(0, "suppress-call")
	  .description("suppress blocks which call this function")
	  .setter((argparse::arg_setter_t)&cov_project_params_t::set_suppressed_calls);
    parser.add_option('X', "suppress-ifdef")
	  .description("suppress source which is conditional on this cpp define")
	  .setter((argparse::arg_setter_t)&cov_project_params_t::set_suppressed_ifdefs);
    parser.add_option('Y', "suppress-comment")
	  .description("suppress source on lines containing this comment")
	  .setter((argparse::arg_setter_t)&cov_project_params_t::set_suppressed_comment_lines);
    parser.add_option('Z', "suppress-comment-between")
	  .description("suppress source between lines containing these start and end comments")
	  .setter((argparse::arg_setter_t)&cov_project_params_t::set_suppressed_comment_ranges);
    parser.add_option('p', "gcda-prefix")
	  .description("directory underneath which to find .gcda files")
	  .setter((argparse::arg_setter_t)&cov_project_params_t::set_gcda_prefix);
    parser.add_option('o', "object-directory")
	  .description("directory in which to find .o,.gcno,.gcda files")
	  .setter((argparse::arg_setter_t)&cov_project_params_t::set_object_directory);
    parser.add_option('F', "solve-fuzzy")
	  .description("(silently ignored for compatibility)")
	  .setter((argparse::noarg_setter_t)&cov_project_params_t::set_solve_fuzzy);
    parser.add_option('D', "debug")
	  .description("enable ggcov debugging features")
	  .setter((argparse::arg_setter_t)&cov_project_params_t::set_debug_str);
    parser.add_option('v', "version")
	  .description("print version and exit")
	  .setter((argparse::noarg_setter_t)&cov_project_params_t::set_print_version_flag);
}

void
cov_project_params_t::post_args()
{
    if (debug_str_.data() != 0)
	logging::logger_t::debug_enable_loggers(debug_str_);

    if (print_version_flag_)
    {
	fputs(PACKAGE " version " VERSION "\n", stdout);
	exit(0);
    }

    if (_log.is_enabled(logging::DEBUG2))
    {
	string_var debug_loggers = logging::logger_t::describe_debug_enabled_loggers();

	_log.debug2("recursive=%d\n", recursive_);
	_log.debug2("suppressed_calls=%s\n", suppressed_calls_.data());
	_log.debug2("suppressed_ifdefs=%s\n", suppressed_ifdefs_.data());
	_log.debug2("suppressed_comment_lines=%s\n", suppressed_comment_lines_.data());
	_log.debug2("suppressed_comment_ranges=%s\n", suppressed_comment_ranges_.data());
	_log.debug2("debug = %s\n", debug_loggers.data());

	static const struct { const char *name; int value; } build_options[] =
	{
	    { "HAVE_LIBBFD",
	    #ifdef HAVE_LIBBFD
	    1
	    #endif
	    }, { "HAVE_LIBPOPT",
	    #ifdef HAVE_LIBPOPT
	    1
	    #endif
	    }, { "HAVE_LIBGCONF",
	    #ifdef HAVE_LIBGCONF
	    1
	    #endif
	    }, { "COV_I386",
	    #ifdef COV_I386
	    1
	    #endif
	    }, { "COV_AMD64",
	    #ifdef COV_AMD64
	    1
	    #endif
	    }, { "UI_DEBUG",
	    #ifdef UI_DEBUG
	    1
	    #endif
	    }, { "HAVE_GNOME_PROGRAM_INIT",
	    #ifdef HAVE_GNOME_PROGRAM_INIT
	    1
	    #endif
	    }, { "HAVE_GTK_TEXT_BUFFER_SELECT_RANGE",
	    #ifdef HAVE_GTK_TEXT_BUFFER_SELECT_RANGE
	    1
	    #endif
	    }, { "HAVE_G_HASH_TABLE_ITER",
	    #ifdef HAVE_G_HASH_TABLE_ITER
	    1
	    #endif
	    }, { NULL, 0 }
	};
	estring build_msg;
	for (int i = 0 ; build_options[i].name ; i++)
	    build_msg.append_printf(" %s%s", build_options[i].value ? "" : "!", build_options[i].name);
	_log.debug2("built with%s\n", build_msg.data());

	estring files_msg;
	for (file_iterator_t itr = file_begin() ; itr != file_end() ; ++itr)
	    files_msg.append_printf(" \"%s\"", itr->c_str());
	_log.debug2("files = {%s }\n", files_msg.data());
    }
}

