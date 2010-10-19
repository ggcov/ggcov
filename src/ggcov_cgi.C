#include <stdio.h>
#include "cov.H"
#include "cgi.H"
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
query_listprojects(cgi_t &cgi, cov_project_t *proj)
{
    json_t json;
    json.begin_array();
    list_iterator_t<cov_project_t> iter;
    for (iter = cov_project_t::first() ; iter != (cov_project_t *)0 ; ++iter)
    {
	json.string((*iter)->name());
    }
    json.end_array();

    cgi.set_reply(json);
}

static void
query_listfiles(cgi_t &cgi, cov_project_t *proj)
{
    proj->pre_read();
    if (!proj->read_all_files())
    {
	cgi.error("Unable to read some filenames\n");
	return;
    }
    proj->post_read();

    json_t json;
    json.begin_array();
    list_iterator_t<cov_file_t> iter;
    for (iter = proj->get_files() ; iter != (cov_file_t *)0 ; ++iter)
    {
    	cov_file_t *f = *iter;

	json.begin_object();
	json.string_field("n", f->minimal_name());
	json.string_field("p", f->name());
	json.end_object();
    }
    json.end_array();

    cgi.set_reply(json);
}

static void
query_annotate(cgi_t &cgi, cov_project_t *proj)
{
    const char *fvar = cgi.get_variable("f");
    if (!fvar || !*fvar)
    {
	cgi.error("Missing filename\n");
	return;
    }
    proj->pre_read();
    if (!proj->read_source_file(fvar))
    {
	cgi.error("Failed to read file \"%s\"\n", fvar);
	return;
    }
    proj->post_read();
    string_var filename = proj->get_pathname(fvar);

    cov_file_t *f = cov_file_t::find(filename);
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
query_colorcss(cgi_t &cgi, cov_project_t *proj)
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
    void (*func)(cgi_t &, cov_project_t *);
    boolean needs_project;
} queries[] = {
    { "listprojects", query_listprojects, false },
    { "listfiles", query_listfiles, true },
    { "annotate", query_annotate, true },
    { "colorcss", query_colorcss, false },
    { 0, 0 }
};

int
main(int argc, char **argv)
{
    argv0 = argv[0];

    cgi_t cgi;

    const char *dbg = cgi.get_variable("debug");
    if (dbg && *dbg)
	debug_set(dbg);

    cov_init();
    new cov_project_t("hacky", HACKY_PROJDIR);
    cov_project_t *proj = cov_project_t::get(cgi.get_variable("p"));

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
	    if (queries[i].needs_project && !proj)
	    {
		cgi.error("Missing or bad project\n");
		return 0;
	    }

	    queries[i].func(cgi, proj);
	    cgi.reply();
	    return 0;
	}
    }

    cgi.error("Unknown query\n");
    return 0;
}
