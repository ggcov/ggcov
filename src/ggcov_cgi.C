#include <stdio.h>
#include "cov.H"
#include "cgi.H"
#include "report.H"
#include "filename.h"
#include "colors.h"

const char *argv0;

static const char *short_status_names[cov::NUM_STATUS] =
{
    "CO",   /* COVERED */
    "PA",   /* PARTCOVERED */
    "UC",   /* UNCOVERED */
    "UI",   /* UNINSTRUMENTED */
    "SU"    /* SUPPRESSED */
};

static void
query_listreports(cgi_t &cgi)
{
    json_t json;
    json.begin_array();
    const report_t *rep;
    for (rep = all_reports ; rep->name != 0 ; rep++)
    {
	json.begin_object();
	json.string_field("n", rep->name);
	json.string_field("l", _(rep->label));
	json.end_object();
    }
    json.end_array();

    cgi.set_reply(json);
}

static void
query_report(cgi_t &cgi)
{
    const char *rvar = cgi.get_variable("r");
    if (!rvar || !*rvar)
    {
	cgi.error("Missing report name\n");
	return;
    }

    const report_t *rep;
    for (rep = all_reports ; rep->name != 0 ; rep++)
    {
	if (!strcmp(rep->name, rvar))
	    break;
    }
    if (!rep->name)
    {
	cgi.error("Bad report name\n");
	return;
    }

    estring text;
    if (!report_run(rep, text))
    {
	cgi.error("Report error\n");
	return;
    }

    cgi.set_reply(text, "text/plain");
}

static cov_file_t *
get_file(cgi_t &cgi)
{
    cov_project_t *proj = cov_project_t::current();

    const char *fvar = cgi.get_variable("f");
    if (!fvar || !*fvar)
    {
	cgi.error("Missing filename\n");
	return 0;
    }
    string_var filename = proj->get_pathname(fvar);

    cov_file_t *f = proj->find_file(fvar);
    if (!f)
    {
	cgi.error("Failed to read cov files...sorry\n");
	return 0;
    }
    return f;
}

static void encode_stats(json_t &json, const cov_stats_t *stats)
{
    static const char * const field[5] = { "bl", "li", "fn", "ca", "br" };
    const unsigned long *counters[5];

    if (!stats)
    {
    	static const cov_stats_t empty;
	stats = &empty;
    }

    counters[0] = stats->blocks_by_status();
    counters[1] = stats->lines_by_status();
    counters[2] = stats->functions_by_status();
    counters[3] = stats->calls_by_status();
    counters[4] = stats->branches_by_status();

    for (int i = 0 ; i < 5 ; i++)
    {
	json.begin_object_field(field[i]);
	for (int st = 0 ; st < cov::NUM_STATUS ; st++)
	{
	    if (st != cov::UNINSTRUMENTED)
		json.ulong_field(short_status_names[st], counters[i][st]);
	}
	json.end_object_field();
    }
}

static void
query_summary(cgi_t &cgi)
{
    const char *svar = cgi.get_variable("s");
    if (!svar || !*svar)
    {
	cgi.error("Missing scope name\n");
	return;
    }

    cov_scope_t *sc = 0;
    if (!strcmp(svar, "overall"))
    {
    	sc = new cov_overall_scope_t;
    }
    else if (!strcmp(svar, "filename"))
    {
	cov_file_t *f = get_file(cgi);
	if (!f)
	    return;
	sc = new cov_file_scope_t(f);
    }
#if 0
    else if (!strcmp(svar, "function"))
    {
    	assert(function_ != 0);
	sc = new cov_function_scope_t(function_);
    }
#endif
    else if (!strcmp(svar, "range"))
    {
	cov_file_t *f = get_file(cgi);
	if (!f)
	    return;
	const char *stvar = cgi.get_variable("st");
	const char *envar = cgi.get_variable("en");
	if (!stvar || !envar)
	{
	    cgi.error("Missing range parameters\n");
	    return;
	}
    	sc = new cov_range_scope_t(f, atoi(stvar), atoi(envar));
    }
    else
    {
	cgi.error("Bad scope name\n");
	return;
    }

    json_t json;
    json.begin_object();
    encode_stats(json, sc->get_stats());
    json.end_object();

    delete sc;

    cgi.set_reply(json);
}


static void
query_listprojects(cgi_t &cgi)
{
    json_t json;
    json.begin_array();
    list_iterator_t<cov_project_t> iter;
    for (iter = cov_project_t::first() ; iter != (cov_project_t *)0 ; ++iter)
    {
	cov_project_t *proj = *iter;

	json.begin_object();
	json.string_field("n", proj->name());
	json.ulong_field("m", proj->mtime());
	json.string_field("d", proj->description());
	json.end_object();
    }
    json.end_array();

    cgi.set_reply(json);
}

static void
query_listfiles(cgi_t &cgi)
{
    json_t json;
    json.begin_array();
    list_iterator_t<cov_file_t> iter;
    for (iter = cov_project_t::current()->first_file() ; iter != (cov_file_t *)0 ; ++iter)
    {
    	cov_file_t *f = *iter;

	json.begin_object();
	json.string_field("n", f->minimal_name());
	json.string_field("p", f->name());

	json.begin_object_field("s");
	cov_file_scope_t sc(f);
	encode_stats(json, sc.get_stats());
	json.end_object();

	json.end_object();
    }
    json.end_array();

    cgi.set_reply(json);
}

static void
query_listfunctions(cgi_t &cgi)
{
    const char *fvar = cgi.get_variable("f");
    if (!fvar || !*fvar)
    {
	cgi.error("Missing file name\n");
	return;
    }

    cov_file_t *f = get_file(cgi);
    if (!f)
	return;
    list_t<cov_function_t> functions;
    unsigned fnidx;
    cov_function_t *fn;

    /* build an alphabetically sorted list of functions in the file */
    for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
    {
	fn = f->nth_function(fnidx);

	if (fn->is_suppressed() ||
	    fn->get_first_location() == 0)
	    continue;
	functions.prepend(fn);
    }
    functions.sort(cov_function_t::compare);

    /* now emit the list as JSON */
    json_t json;
    json.begin_array();
    while ((fn = functions.remove_head()) != 0)
    {
	json.begin_object();
	json.string_field("n", fn->name());
	json.string_field("fl", fn->get_first_location()->describe());
	json.string_field("ll", fn->get_last_location()->describe());

	json.begin_object_field("s");
	cov_function_scope_t sc(fn);
	encode_stats(json, sc.get_stats());
	json.end_object();

	json.end_object();
    }
    json.end_array();

    cgi.set_reply(json);
}

static void
query_annotate(cgi_t &cgi)
{
    cov_project_t *proj = cov_project_t::current();

    const char *fvar = cgi.get_variable("f");
    if (!fvar || !*fvar)
    {
	cgi.error("Missing filename\n");
	return;
    }
    string_var filename = proj->get_pathname(fvar);

    cov_file_t *f = proj->find_file(fvar);
    if (!f)
    {
	cgi.error("Failed to read cov files...sorry\n");
	return;
    }

    FILE *fp;
    if ((fp = fopen(filename, "r")) == 0)
    {
    	/* TODO: proper error report */
    	cgi.perror(filename);
	return;
    }

    unsigned long lineno = 0;
    cov_line_t *ln;
    char linebuf[1024];
    json_t json;

    json.begin_array();
    while (fgets(linebuf, sizeof(linebuf), fp) != 0)
    {
    	++lineno;
    	ln = f->nth_line(lineno);

	json.begin_object();

	json.string_field("s", short_status_names[ln->status()]);
	switch (ln->status())
	{
	case cov::COVERED:
	case cov::PARTCOVERED:
	    json.ulong_field("c", ln->count());

	    json.begin_array_field("b");
	    for (const GList *itr = ln->blocks() ; itr ; itr = itr->next)
	    {
		cov_block_t *b = (cov_block_t *)itr->data;
		json.ulong(b->bindex());
	    }
	    json.end_array_field();
	    break;
	case cov::UNCOVERED:
	case cov::UNINSTRUMENTED:
	case cov::SUPPRESSED:
	    break;
	}

	json.string_field("t", linebuf);

	json.end_object();
    }
    json.end_array();

    fclose(fp);
    cgi.set_reply(json);
}

static void
query_colorcss(cgi_t &cgi)
{
    estring css;

#define PASTE(x,y)  x##y
#define ADD_COLOR(stat) \
    css.append_printf(".status%sf { color: #%02x%02x%02x; }\n", \
		      short_status_names[cov::stat], \
		      PASTE(COLOR_FG_,stat)); \
    css.append_printf(".status%sb { background-color: #%02x%02x%02x; }\n", \
		      short_status_names[cov::stat], \
		      PASTE(COLOR_BG_,stat))
    ADD_COLOR(COVERED);
    ADD_COLOR(PARTCOVERED);
    ADD_COLOR(UNCOVERED);
    ADD_COLOR(UNINSTRUMENTED);
    ADD_COLOR(SUPPRESSED);
#undef ADD_COLOR

    cgi.set_reply(css, "text/css");
}

static const struct
{
    const char *name;
    void (*func)(cgi_t &);
    boolean needs_project;
} queries[] = {
    { "listprojects", query_listprojects, false },
    { "listfiles", query_listfiles, true },
    { "listfunctions", query_listfunctions, true },
    { "annotate", query_annotate, true },
    { "colorcss", query_colorcss, false },
    { "listreports", query_listreports, true },
    { "report", query_report, true },
    { "summary", query_summary, true },
    { 0, 0 }
};

static gboolean
read_one_project(const char *filename, void *userdata)
{
    list_t<char> *bases = (list_t<char> *)userdata;

    if (file_is_directory(filename) == 0)
	bases->prepend(g_strdup(filename));

    return TRUE;
}

static void
read_projects(void)
{
    list_t<char> bases;
    char *base;

    file_apply_children(HACKY_PROJDIR, read_one_project, (void *)&bases);

    bases.sort(strcmp);
    while ((base = bases.remove_head()) != 0)
    {
	new cov_project_t(file_basename_c(base), base);
	g_free(base);
    }
}

static boolean
setup_project(cgi_t &cgi)
{
    cov_project_t *proj = cov_project_t::get(cgi.get_variable("p"));
    if (proj == 0)
    {
	cgi.error("Missing or bad project\n");
	return false;
    }

    proj->make_current();
    proj->pre_read();
    if (!proj->read_all_files())
    {
	cgi.error("Unable to read some filenames\n");
	return false;
    }
    proj->post_read();

    return true;
}

int
main(int argc, char **argv)
{
    argv0 = argv[0];

    cgi_t cgi;

    const char *dbg = cgi.get_variable("debug");
    if (dbg && *dbg)
	debug_set(dbg);

    cov_init();
    read_projects();

    const char *qvar = cgi.get_variable("q");
    if (qvar == 0 || *qvar == '\0')
    {
	cgi.error("Missing query\n");
	return 0;
    }

    for (int i = 0 ; queries[i].name ; i++)
    {
	if (!strcmp(qvar, queries[i].name))
	{
	    if (queries[i].needs_project && !setup_project(cgi))
		return 0;

	    queries[i].func(cgi);
	    cgi.reply();
	    return 0;
	}
    }

    cgi.error("Unknown query\n");
    return 0;
}
