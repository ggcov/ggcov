/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2020 Greg Banks <gnb@fastmail.fm>
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
 * ggcov-html is a libcov-based static HTML generator.
 */

#include "common.h"
#include "cov_priv.H"
#include "filename.h"
#include "estring.H"
#include "filerec.H"
#include "yaml_generator.H"
#include "mustache.H"
#include "flow_diagram.H"
#include "libgd_scenegen.H"
#include "unique_ptr.H"
#include "logging.H"

char *argv0;
mustache::environment_t menv;
static logging::logger_t &_log = logging::find_logger("ggcov-html");
static logging::logger_t &dump_log = logging::find_logger("dump");

class gghtml_params_t : public cov_project_params_t
{
public:
    gghtml_params_t(const char *argv0);
    ~gghtml_params_t();

    ARGPARSE_STRING_PROPERTY(output_directory);
    ARGPARSE_STRING_PROPERTY(template_directory);

public:
    void setup_parser(argparse::parser_t &parser)
    {
	cov_project_params_t::setup_parser(parser);
	estring o_desc;
	o_desc.append_printf("write HTML to the given directory, default \"%s\"",
			     output_directory_.data());
	estring t_desc;
	t_desc.append_printf("read HTML templates from the given directory, default \"%s\"",
			     template_directory_.data());
	parser.add_option('O', "output-directory")
	      .description(o_desc)
	      .setter((argparse::arg_setter_t)&gghtml_params_t::set_output_directory)
              .metavar("DIR");
	parser.add_option('t', "template-directory")
	      .description(t_desc)
	      .setter((argparse::arg_setter_t)&gghtml_params_t::set_template_directory)
              .metavar("DIR");
	parser.set_other_option_help("[OPTIONS] [executable|source|directory]...");
    }

    void post_args()
    {
	cov_project_params_t::post_args();
	if (!template_directory_.data())
	    template_directory_ = file_join2(data_directory_, "templates");
	if (dump_log.is_enabled(logging::DEBUG2))
	{
	    dump_log.debug2("output_directory=\"%s\"\n", output_directory_.data());
	    dump_log.debug2("template_directory=\"%s\"\n", template_directory_.data());
	    dump_log.debug2("data_directory=\"%s\"\n", data_directory_.data());
	}
    }
private:
    string_var data_directory_;
};

gghtml_params_t::gghtml_params_t(const char *argv0)
 :  output_directory_("html"),
    data_directory_(PKGDATADIR)
{
    /*
     * If we're being run from the source directory, fiddle the
     * data directory to point to ../lib instead of the configured
     * PKGDATADIR.  This means we can run ggcov-html before
     * installation without having to use the -t option.
     */
    estring dir = file_make_absolute(argv0);
    const char *p = strrchr(dir, '/');
    if (p != 0 &&
        (p -= 4) >= dir.data() &&
        !strncmp(p, "/src/", 5))
    {
        dir.truncate_to(p - dir);
        dir.append_string("/lib");
        if (!file_is_directory(dir))
        {
            _log.info("running from source directory, "
                      "so using %s to find HTML templates\n",
                      dir.data());
            data_directory_ = dir;
        }
    }
}

gghtml_params_t::~gghtml_params_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
generate_static_files(const gghtml_params_t &params)
{
    /* 
     * Read the static.files file which contains a list
     * of all the static files.  This avoids hard-coding
     * the static file names and later will allow us to
     * give the user the option to point at their own
     * template directory.
     */
    string_var filename = file_join2(params.get_template_directory(), "static.files");
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

	string_var filefrom = file_join2(params.get_template_directory(), p);
	string_var fileto = file_join2(params.get_output_directory(), p);
	_log.info("Installing static file %s to %s\n", filefrom.data(), fileto.data());
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

    yaml.key("blocks_executed").value((uint64_t)stats.blocks_executed());
    yaml.key("blocks_total").value((uint64_t)stats.blocks_total());
    yaml.key("blocks_fraction").value(stats.blocks_fraction());
    yaml.key("blocks_percent").value((unsigned)(100.0*stats.blocks_fraction()+0.5));
    yaml.key("blocks_sort_fraction").value(stats.blocks_sort_fraction());
    yaml.key("blocks_sort_percent").value((unsigned)(100.0*stats.blocks_sort_fraction()+0.5));
    cov_stats_t::format_row_labels(stats.blocks_by_status(), absbuf, pcbuf);
    yaml.key("blocks_abslabel").value(absbuf);
    yaml.key("blocks_pclabel").value(pcbuf);

    yaml.key("lines_executed").value((uint64_t)stats.lines_executed());
    yaml.key("lines_full").value((uint64_t)stats.lines_full());
    yaml.key("lines_partial").value((uint64_t)stats.lines_partial());
    yaml.key("lines_total").value((uint64_t)stats.lines_total());
    yaml.key("lines_fraction").value(stats.lines_fraction());
    yaml.key("lines_sort_fraction").value(stats.lines_sort_fraction());
    cov_stats_t::format_row_labels(stats.lines_by_status(), absbuf, pcbuf);
    yaml.key("lines_abslabel").value(absbuf);
    yaml.key("lines_pclabel").value(pcbuf);

    yaml.key("functions_executed").value((uint64_t)stats.functions_executed());
    yaml.key("functions_full").value((uint64_t)stats.functions_full());
    yaml.key("functions_partial").value((uint64_t)stats.functions_partial());
    yaml.key("functions_total").value((uint64_t)stats.functions_total());
    yaml.key("functions_fraction").value(stats.functions_fraction());
    yaml.key("functions_sort_fraction").value(stats.functions_sort_fraction());
    cov_stats_t::format_row_labels(stats.functions_by_status(), absbuf, pcbuf);
    yaml.key("functions_abslabel").value(absbuf);
    yaml.key("functions_pclabel").value(pcbuf);

    yaml.key("calls_executed").value((uint64_t)stats.calls_executed());
    yaml.key("calls_total").value((uint64_t)stats.calls_total());
    yaml.key("calls_fraction").value(stats.calls_fraction());
    yaml.key("calls_sort_fraction").value(stats.calls_sort_fraction());
    cov_stats_t::format_row_labels(stats.calls_by_status(), absbuf, pcbuf);
    yaml.key("calls_abslabel").value(absbuf);
    yaml.key("calls_pclabel").value(pcbuf);

    yaml.key("branches_executed").value((uint64_t)stats.branches_executed());
    yaml.key("branches_taken").value((uint64_t)stats.branches_taken());
    yaml.key("branches_total").value((uint64_t)stats.branches_total());
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
generate_index(const gghtml_params_t &params)
{
    unique_ptr<mustache::template_t> tmpl = menv.make_template("index.html");
    yaml_generator_t &yaml = tmpl->begin_render();

    cov_overall_scope_t scope;

    yaml.begin_mapping();
    yaml.key("subtitle").value("summary");
    yaml.key("stats"); generate_stats(yaml, *scope.get_stats());
    yaml.end_mapping();

    tmpl->end_render();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void generate_tree_node(file_rec_t *fr, yaml_generator_t &yaml, unsigned int depth)
{
    if (depth)
    {
	cov_file_t *f = fr->get_file();
	yaml.begin_mapping();
	yaml.key("name").value(file_basename_c(fr->get_name()));
	yaml.key("status").value(cov::short_name(fr->get_scope()->status()));
	yaml.key("is_file?").bool_value(fr->is_file());
	if (fr->is_file())
	{
	    string_var url = source_url(f);
	    yaml.key("url").value(url);
	    yaml.key("minimal_name").value(f->minimal_name());
	}
	yaml.key("indent").value(4*(depth-1));
	yaml.key("stats"); generate_stats(yaml, *fr->get_scope()->get_stats());
	yaml.end_mapping();
    }

    for (list_iterator_t<file_rec_t> friter = fr->first_child() ; *friter ; ++friter)
	generate_tree_node(*friter, yaml, depth+1);
}

static void
generate_source_tree(const gghtml_params_t &params)
{
    unique_ptr<mustache::template_t> tmpl = menv.make_template("tree.html");
    yaml_generator_t &yaml = tmpl->begin_render();

    unique_ptr<file_rec_t> tree = new file_rec_t("", 0);
    tree->add_descendents(cov_file_t::first());

    yaml.begin_mapping();
    yaml.key("subtitle").value("source tree");
    yaml.key("files").begin_sequence();
    generate_tree_node(tree.get(), yaml, 0U);
    yaml.end_sequence();
    yaml.end_mapping();

    tmpl->end_render();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct flow_t
{
    cov_function_t *function;
    unsigned int width, height;
    string_var url;
};

static void set_diagram_colors(diagram_t *di)
{
    // TODO: ugh, these are hardcoded to be the same as the ones in ggcov.css
    di->set_fg(cov::COVERED, 0x00c000);
    di->set_bg(cov::COVERED, 0x80d080);
    di->set_fg(cov::PARTCOVERED, 0xa0a000);
    di->set_bg(cov::PARTCOVERED, 0xd0d080);
    di->set_fg(cov::UNCOVERED, 0xc00000);
    di->set_bg(cov::UNCOVERED, 0xd08080);
    di->set_fg(cov::UNINSTRUMENTED, 0x000000);
    di->set_bg(cov::UNINSTRUMENTED, 0xa0a0a0);
    di->set_fg(cov::SUPPRESSED, 0x000080);
    di->set_bg(cov::SUPPRESSED, 0x8080d0);
}

static flow_t *generate_flow_diagram(const gghtml_params_t &params, cov_function_t *fn)
{
    unique_ptr<diagram_t> diag = new flow_diagram_t(fn);
    set_diagram_colors(diag.get());
    if (!diag->prepare())
        return 0;
    dbounds_t bounds;
    diag->get_bounds(&bounds);
    unsigned int width = (unsigned int)(4.0 * bounds.width() + 0.5);
    unsigned int height = (unsigned int)(4.0 * bounds.height() + 0.5);
    unique_ptr<libgd_scenegen_t> sg = new libgd_scenegen_t(width, height, bounds);
    diag->render(sg.get());

    static unsigned int tag = 1;
    string_var name = g_strdup_printf("flow%u.png", tag++);
    string_var path = file_join2(params.get_output_directory(), name);
    _log.info("Generating flow diagram %s for function %s file %s\n",
	    name.data(), fn->name(), fn->file()->minimal_name());
    sg->save(path);

    flow_t *flow = new flow_t;
    flow->function = fn;
    flow->width = width;
    flow->height = height;
    flow->url = name.take();
    return flow;
}

static hashtable_t<void, flow_t> *generate_flow_diagrams(const gghtml_params_t &params, cov_file_t *f)
{
    hashtable_t<void, flow_t> *flows = new hashtable_t<void, flow_t>;
    unsigned int i;
    for (i = 0 ; i < f->num_functions() ; i++)
    {
	cov_function_t *fn = f->nth_function(i);
        flow_t *flow = generate_flow_diagram(params, fn);
        if (flow)
            flows->insert((void*)fn, flow);
    }
    return flows;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
generate_annotated_source(const gghtml_params_t &params, cov_file_t *f,
			  hashtable_t<void, flow_t> *flows)
{
    cov_file_annotator_t annotator(f);
    if (!annotator.is_valid())
	return;

    string_var out_name = source_url(f);

    /*
     * We can't use annotator.function() because it's not true
     * for every line between the first line and the last line
     * of a function, so we need to run our own counter
     */
    unsigned int lines_left = 0;
    unique_ptr<mustache::template_t> tmpl = menv.make_template("source.html", out_name);
    yaml_generator_t &yaml = tmpl->begin_render();

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
	yaml.key("lineno").value((unsigned int)annotator.lineno());
	yaml.key("blocks").value(annotator.blocks_as_string());
	yaml.key("text").value(annotator.text());
        const char *stext = annotator.suppression_text();
        if (stext)
            yaml.key("suppression_text").value(stext);

        if (annotator.is_first_line_in_function())
        {
            cov_function_t *fn = annotator.function();
	    flow_t *flow = flows->lookup((void*)fn);
	    if (flow)
	    {
                lines_left = fn->get_num_lines();
		yaml.key("flow_diagram").begin_mapping();
		yaml.key("nlines").value(fn->get_num_lines());
		double sf = 0.445;
		yaml.key("width").value((unsigned int)(sf * flow->width + 0.5));
		yaml.key("height").value((unsigned int)(sf * flow->height + 0.5));
		yaml.key("url").value(flow->url);
		yaml.end_mapping();
	    }
	}
	if (!lines_left)
	    yaml.key("flow_filler").bool_value(true);
        if (lines_left)
            lines_left--;

	yaml.end_mapping();
    }
    yaml.end_sequence();
    yaml.end_mapping();

    tmpl->end_render();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
generate_functions(const gghtml_params_t &params)
{
    unique_ptr<mustache::template_t> tmpl = menv.make_template("functions.html");
    yaml_generator_t &yaml = tmpl->begin_render();

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
            if (!loc)
                continue;
	    string_var url = source_url(loc);
	    yaml.begin_mapping();
	    yaml.key("name").value(fn->name());
	    yaml.key("url").value(url);
	    yaml.key("status").value(cov::short_name(st));
	    yaml.key("filename").value(loc->filename);
	    yaml.key("lineno").value((unsigned int)loc->lineno);
	    yaml.key("stats"); generate_stats(yaml, stats);
	    yaml.end_mapping();
	}
    }
    yaml.end_sequence();
    yaml.end_mapping();

    tmpl->end_render();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
delete_one_flow(void *key, flow_t *value, void *closure)
{
    delete value;
    return TRUE;    /* remove from hashtable */
}

static int
generate_html(const gghtml_params_t &params)
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
    {
	hashtable_t<void, flow_t> *flows = generate_flow_diagrams(params, *iter);
	generate_annotated_source(params, *iter, flows);
	flows->foreach_remove(delete_one_flow, 0);
	delete flows;
    }
    generate_functions(params);
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static logging::level_t
log_level(GLogLevelFlags level)
{
    switch (level & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR: return logging::ERROR;
    case G_LOG_LEVEL_CRITICAL: return logging::FATAL;
    case G_LOG_LEVEL_WARNING: return logging::WARNING;
    case G_LOG_LEVEL_MESSAGE: return logging::INFO;
    case G_LOG_LEVEL_INFO: return logging::INFO;
    case G_LOG_LEVEL_DEBUG: return logging::DEBUG;
    default: return logging::INFO;
    }
}

static void
log_func(
    const char *domain,
    GLogLevelFlags level,
    const char *msg,
    gpointer user_data)
{
    _log.message(log_level(level), "%s:%s:%s\n",
	(domain == 0 ? PACKAGE : domain),
	msg);
    if (level & G_LOG_FLAG_FATAL)
	exit(1);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    g_log_set_handler("GLib",
		      (GLogLevelFlags)(G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL),
		      log_func, /*user_data*/0);
    argv0 = argv[0];
    gghtml_params_t params(argv[0]);
    argparse::default_parser_t parser(params);
    if (parser.parse(argc, argv) < 0)
    {
	exit(1);	/* error message emitted in parse_args() */
    }
    menv.set_template_directory(params.get_template_directory());
    menv.set_output_directory(params.get_output_directory());

    int r = cov_read_files(params);
    if (r < 0)
	exit(1);    /* error message in cov_read_files() */
    if (r == 0)
	exit(0);    /* error message in cov_read_files() */

    cov_dump();

    if (generate_html(params) < 0)
	return 1;   /* failure */
    return 0;
}
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
