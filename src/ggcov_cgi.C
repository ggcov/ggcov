#include <stdio.h>
#include "cov.H"
#include "cgi.H"
#include "filename.h"

const char *argv0;

static const char *short_status_names[cov::NUM_STATUS] =
{
    "CO",   /* COVERED */
    "PA",   /* PARTCOVERED */
    "UC",   /* UNCOVERED */
    "UI",   /* UNINSTRUMENTED */
    "SU"    /* SUPPRESSED */
};

struct cov_project_t
{
    const char *name_;
    const char *basedir_;

    char *
    get_pathname(const char *filename) const
    {
	// TODO: ensure @filename is relative and does not escape from basedir
	return file_make_absolute_to_dir(filename, basedir_);
    }

    void
    read_file(const char *filename)
    {
	string_var path = get_pathname(filename);
	GList *files = g_list_append(NULL, (gpointer)path.data());
	cov_read_files(files);
	listclear(files);
    }

    void
    read_all_files()
    {
	/* Awful hack to set the global `recursive' flag */
	for (int i = 0 ; cov_popt_options[i].shortName ; i++)
	{
	    if (cov_popt_options[i].shortName == 'r')
	    {
		*(int *)cov_popt_options[i].arg = 1;
		break;
	    }
	}

	GList *files = g_list_append(NULL, (gpointer)basedir_);
	cov_read_files(files);
	listclear(files);
    }

    static cov_project_t *
    get(const char *name)
    {
	if (!name || !*name)
	    return 0;

	static cov_project_t projects[] = {
	    { "hacky", HACKY_PROJDIR },
	    { 0, 0 }
	};

	for (int i = 0 ; projects[i].name_ ; i++)
	{
	    if (!strcmp(projects[i].name_, name))
		return &projects[i];
	}
	return 0;
    }

    list_iterator_t<cov_file_t> get_files() const { return cov_file_t::first(); }
};

static void
query_listfiles(cgi_t &cgi, cov_project_t *proj)
{
    proj->read_all_files();

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
    proj->read_file(fvar);
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

static const struct
{
    const char *name;
    void (*func)(cgi_t &, cov_project_t *);
} queries[] = {
    { "listfiles", query_listfiles },
    { "annotate", query_annotate },
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

    cov_project_t *proj = cov_project_t::get(cgi.get_variable("p"));
    if (!proj)
    {
	cgi.error("Missing or bad project\n");
	return 0;
    }

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
	    queries[i].func(cgi, proj);
	    cgi.reply();
	    return 0;
	}
    }

    cgi.error("Unknown query\n");
    return 0;
}
