/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2015 Greg Banks <gnb@fastmail.fm>
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

/*
 * lggcov is a libcov-based static HTML generator.
 */

#include "common.h"
#include "cov_priv.H"
#include "filename.h"
#include "estring.H"
#include "tok.H"
#include "yaml_generator.H"
#include "mustache.H"

char *argv0;
string_var data_dir;
string_var templates_dir;

class lggcov_params_t : public cov_project_params_t
{
public:
    lggcov_params_t();
    ~lggcov_params_t();

    ARGPARSE_STRING_PROPERTY(output_directory);

    static const char default_output_directory[];

public:
    void setup_parser(argparse::parser_t &parser)
    {
	cov_project_params_t::setup_parser(parser);
	estring o_desc;
	o_desc.append_printf("write HTML to the given directory, default \"%s\"",
			     default_output_directory);
	parser.add_option('o', "output-directory")
	      .description(o_desc)
	      .setter((argparse::arg_setter_t)&lggcov_params_t::set_output_directory);
	parser.set_other_option_help("[OPTIONS] [executable|source|directory]...");
    }

    void post_args()
    {
	cov_project_params_t::post_args();
	if (debug_enabled(D_DUMP|D_VERBOSE))
	{
	    duprintf1("output_directory=\"%s\"\n", output_directory_.data());
	}
    }
};

const char lggcov_params_t::default_output_directory[] = "html";

lggcov_params_t::lggcov_params_t()
 :  output_directory_(default_output_directory)
{
}

lggcov_params_t::~lggcov_params_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
generate_static_files(const lggcov_params_t &params)
{
    /* 
     * Read the static.files file which contains a list
     * of all the static files.  This avoids hard-coding
     * the static file names and later will allow us to
     * give the user the option to point at their own
     * template directory.
     */
    string_var filename = file_join2(templates_dir, "static.files");
    FILE *fp = 0;
    char *p;
    int r = 0;
    char buf[1024];

    if ((fp = fopen(filename, "r")) == 0)
    {
	perror(filename);
	return -1;
    }

    while ((fgets(buf, sizeof(buf), fp)) != 0)
    {
	if (buf[0] == '#')
	    continue;	    /* ignore comments */

	/* trim trailing whitespace */
	p = buf+strlen(buf)-1;
	while (p > buf && isspace(*p))
	    *p-- = '\0';

	/* trim leading whitespace */
	p = buf;
	while (*p && isspace(*p))
	    p++;

	if (!*p)
	    continue;	    /* ignore empty lines */

	string_var filefrom = file_join2(templates_dir, p);
	string_var fileto = file_join2(params.get_output_directory(), p);
	fprintf(stderr, "Installing static file %s to %s\n", filefrom.data(), fileto.data());
	if (file_copy(filefrom, fileto) < 0)
	{
	    r = -1;
	    goto out;
	}
    }

out:
    if (fp) fclose(fp);
    return r;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *
source_url(const cov_file_t *f, unsigned long lineno = 0)
{
    estring nm(f->minimal_name());
    nm.replace_all("/", "_");
    nm.append_string(".html");
    if (lineno)
	nm.append_printf("#L%lu", lineno);
    return nm.take();
}

static char *
source_url(const cov_location_t *loc)
{
    cov_file_t *f = cov_file_t::find(loc->filename);
    return f ? source_url(f, loc->lineno) : 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
generate_stats(yaml_generator_t &yaml, const cov_stats_t &stats)
{
    estring absbuf, pcbuf;

    yaml.begin_mapping();

    yaml.key("blocks_executed").value(stats.blocks_executed());
    yaml.key("blocks_total").value(stats.blocks_total());
    yaml.key("blocks_fraction").value(stats.blocks_fraction());
    yaml.key("blocks_sort_fraction").value(stats.blocks_sort_fraction());
    cov_stats_t::format_row_labels(stats.blocks_by_status(), absbuf, pcbuf);
    yaml.key("blocks_abslabel").value(absbuf);
    yaml.key("blocks_pclabel").value(pcbuf);

    yaml.key("lines_executed").value(stats.lines_executed());
    yaml.key("lines_full").value(stats.lines_full());
    yaml.key("lines_partial").value(stats.lines_partial());
    yaml.key("lines_total").value(stats.lines_total());
    yaml.key("lines_fraction").value(stats.lines_fraction());
    yaml.key("lines_sort_fraction").value(stats.lines_sort_fraction());
    cov_stats_t::format_row_labels(stats.lines_by_status(), absbuf, pcbuf);
    yaml.key("lines_abslabel").value(absbuf);
    yaml.key("lines_pclabel").value(pcbuf);

    yaml.key("functions_executed").value(stats.functions_executed());
    yaml.key("functions_full").value(stats.functions_full());
    yaml.key("functions_partial").value(stats.functions_partial());
    yaml.key("functions_total").value(stats.functions_total());
    yaml.key("functions_fraction").value(stats.functions_fraction());
    yaml.key("functions_sort_fraction").value(stats.functions_sort_fraction());
    cov_stats_t::format_row_labels(stats.functions_by_status(), absbuf, pcbuf);
    yaml.key("functions_abslabel").value(absbuf);
    yaml.key("functions_pclabel").value(pcbuf);

    yaml.key("calls_executed").value(stats.calls_executed());
    yaml.key("calls_total").value(stats.calls_total());
    yaml.key("calls_fraction").value(stats.calls_fraction());
    yaml.key("calls_sort_fraction").value(stats.calls_sort_fraction());
    cov_stats_t::format_row_labels(stats.calls_by_status(), absbuf, pcbuf);
    yaml.key("calls_abslabel").value(absbuf);
    yaml.key("calls_pclabel").value(pcbuf);

    yaml.key("branches_executed").value(stats.branches_executed());
    yaml.key("branches_taken").value(stats.branches_taken());
    yaml.key("branches_total").value(stats.branches_total());
    yaml.key("branches_fraction").value(stats.branches_fraction());
    yaml.key("branches_sort_fraction").value(stats.branches_sort_fraction());
    cov_stats_t::format_row_labels(stats.branches_by_status(), absbuf, pcbuf);
    yaml.key("branches_abslabel").value(absbuf);
    yaml.key("branches_pclabel").value(pcbuf);

    yaml.key("status_by_blocks").value(cov::short_name(stats.status_by_blocks()));
    yaml.key("status_by_lines").value(cov::short_name(stats.status_by_lines()));
    yaml.end_mapping();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
generate_index(const lggcov_params_t &params)
{
    mustache_t tmpl("index.html", "index.html");
    yaml_generator_t &yaml = tmpl.begin_render();

    cov_overall_scope_t scope;

    yaml.begin_mapping();
    yaml.key("subtitle").value("summary");
    yaml.key("stats"); generate_stats(yaml, *scope.get_stats());
    yaml.end_mapping();

    tmpl.end_render();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
generate_source_tree(const lggcov_params_t &params)
{
    mustache_t tmpl("tree.html", "tree.html");
    yaml_generator_t &yaml = tmpl.begin_render();

    yaml.begin_mapping();
    yaml.key("subtitle").value("source tree");
    yaml.key("files").begin_sequence();
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	const cov_file_t *f = *iter;
	cov_file_scope_t scope(f);
	cov::status_t st = scope.status();
	if (st == cov::SUPPRESSED || st == cov::UNINSTRUMENTED)
	    continue;
	string_var url = source_url(f);
	yaml.begin_mapping();
	yaml.key("minimal_name").value(f->minimal_name());
	yaml.key("status").value(cov::short_name(st));
	yaml.key("url").value(url);
	yaml.key("stats"); generate_stats(yaml, *scope.get_stats());
	yaml.end_mapping();
    }
    yaml.end_sequence();
    yaml.end_mapping();

    tmpl.end_render();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
generate_annotated_source(const lggcov_params_t &params, cov_file_t *f)
{
    cov_file_annotator_t annotator(f);
    if (!annotator.is_valid())
	return;

    string_var out_name = source_url(f);

    mustache_t tmpl("source.html", out_name);
    yaml_generator_t &yaml = tmpl.begin_render();

    yaml.begin_mapping();
    string_var subtitle = g_strdup_printf("source %s", f->minimal_name());
    yaml.key("subtitle").value(subtitle);
    yaml.key("filename").value(f->name());
    yaml.key("minimal_name").value(f->minimal_name());
    yaml.key("lines").begin_sequence();
    while (annotator.next())
    {
	yaml.begin_mapping();
	yaml.key("status_short").value(cov::short_name(annotator.status()));
	yaml.key("status_long").value(cov::long_name(annotator.status()));
	yaml.key("count").value(annotator.count());
	yaml.key("count_if_instrumented").value(annotator.count_as_string());
	yaml.key("lineno").value(annotator.lineno());
	yaml.key("blocks").value(annotator.blocks_as_string());
	yaml.key("text").value(annotator.text());
	yaml.end_mapping();
    }
    yaml.end_sequence();
    yaml.end_mapping();

    tmpl.end_render();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
generate_functions(const lggcov_params_t &params)
{
    mustache_t tmpl("functions.html", "functions.html");
    yaml_generator_t &yaml = tmpl.begin_render();

    yaml.begin_mapping();
    yaml.key("subtitle").value("functions");
    yaml.key("functions").begin_sequence();
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	for (ptrarray_iterator_t<cov_function_t> fnitr = (*iter)->functions().first() ; *fnitr ; ++fnitr)
	{
	    cov_function_t *fn = *fnitr;
	    cov_stats_t stats;
	    cov::status_t st = fn->calc_stats(&stats);
	    if (st == cov::SUPPRESSED || st == cov::UNINSTRUMENTED)
		continue;
	    const cov_location_t *loc = fn->get_first_location();
	    string_var url = source_url(loc);
	    yaml.begin_mapping();
	    yaml.key("name").value(fn->name());
	    yaml.key("url").value(url);
	    yaml.key("status").value(cov::short_name(st));
	    yaml.key("filename").value(loc->filename);
	    yaml.key("lineno").value(loc->lineno);
	    yaml.key("stats"); generate_stats(yaml, stats);
	    yaml.end_mapping();
	}
    }
    yaml.end_sequence();
    yaml.end_mapping();

    tmpl.end_render();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
generate_html(const lggcov_params_t &params)
{
    int r = file_build_tree(params.get_output_directory(), 0777);
    if (r < 0)
    {
	perror(params.get_output_directory());
	exit(1);
    }

    if (generate_static_files(params) < 0)
	return -1;
    generate_index(params);
    generate_source_tree(params);
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
	generate_annotated_source(params, *iter);
    generate_functions(params);
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#define DEBUG_GLIB 1
#if DEBUG_GLIB

static const char *
log_level_to_str(GLogLevelFlags level)
{
    static char buf[32];

    switch (level & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR: return "ERROR";
    case G_LOG_LEVEL_CRITICAL: return "CRITICAL";
    case G_LOG_LEVEL_WARNING: return "WARNING";
    case G_LOG_LEVEL_MESSAGE: return "MESSAGE";
    case G_LOG_LEVEL_INFO: return "INFO";
    case G_LOG_LEVEL_DEBUG: return "DEBUG";
    default:
	snprintf(buf, sizeof(buf), "%d", level);
	return buf;
    }
}

void
log_func(
    const char *domain,
    GLogLevelFlags level,
    const char *msg,
    gpointer user_data)
{
    fprintf(stderr, "%s:%s:%s\n",
	(domain == 0 ? PACKAGE : domain),
	log_level_to_str(level),
	msg);
    if (level & G_LOG_FLAG_FATAL)
	exit(1);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
#if DEBUG_GLIB
    g_log_set_handler("GLib",
		      (GLogLevelFlags)(G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL),
		      log_func, /*user_data*/0);
#endif
    argv0 = argv[0];
    lggcov_params_t params;
    argparse::parser_t parser(params);
    if (parser.parse(argc, argv) < 0)
    {
	exit(1);	/* error message emitted in parse_args() */
    }
    data_dir = file_make_absolute_to_file("../lib", argv[0]);
    templates_dir = file_join2(data_dir, "templates");
    mustache_t::set_template_directory(templates_dir);
    mustache_t::set_output_directory(params.get_output_directory());

    int r = cov_read_files(params);
    if (r < 0)
	exit(1);    /* error message in cov_read_files() */
    if (r == 0)
	exit(0);    /* error message in cov_read_files() */

    cov_dump(stderr);

    if (generate_html(params) < 0)
	return 1;   /* failure */
    return 0;
}
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
