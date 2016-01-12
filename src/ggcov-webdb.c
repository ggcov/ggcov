/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005-2015 Greg Banks <gnb@users.sourceforge.net>
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

#include "common.h"
#include "cov_priv.H"
#include "filename.h"
#include "estring.H"
#include "tok.H"
#include "php_serializer.H"
#include "report.H"
#include "php_scenegen.H"
#include "callgraph_diagram.H"
#include "lego_diagram.H"
#include "argparse.H"
#include "logging.H"
#include <db.h>

#define V(major,minor,patch)    ((major)*10000+(minor)*1000+(patch))
#define DB_VERSION_CODE V(DB_VERSION_MAJOR,DB_VERSION_MINOR,DB_VERSION_PATCH)

/*
 * The transaction argument to DB->open was added in 4.1.25.
 * As ggcov doesn't use transactions this define allows us
 * to compile with older DB versions (some distros still
 * ship db3.3.x as the default DB version).
 */
#if DB_VERSION_CODE < V(4,1,25)
#define OPEN_TXN
#else
#define OPEN_TXN    (DB_TXN *)0,
#endif

char *argv0;

static hashtable_t<void, unsigned int> *file_index;
static hashtable_t<void, unsigned int> *function_index;
static hashtable_t<void, unsigned int> *callnode_index;
static list_t<cov_function_t> *all_functions;
static logging::logger_t &_log = logging::find_logger("web");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

inline DBT *dbt(char *data, unsigned int length)
{
#define MAX_DBTS    2
    static DBT bufs[MAX_DBTS];
    static int count = 0;
    DBT *d = &bufs[(count++) % MAX_DBTS];

    memset(d, 0, sizeof(*d));
    d->data = data;
    d->size = length;

    return d;
#undef MAX_DBTS
}

inline DBT *dbt(const estring &e)
{
    return dbt((char *)e.data(), e.length());
}

inline DBT *dbt(php_serializer_t &ser)
{
    return dbt(ser.data());
}

inline DBT *dbt(const char *s)
{
    return dbt((char *)s, (s == 0 ? 0 : strlen(s)));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static inline unsigned int
ftag(const cov_file_t *f)
{
    return *file_index->lookup((void *)f);
}

static inline unsigned int
fntag(const cov_function_t *fn)
{
    return *function_index->lookup((void *)fn);
}

static inline unsigned int
cntag(const cov_callnode_t *cn)
{
    return *callnode_index->lookup((void *)cn);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static inline const char *
get_tmpdir(void)
{
    char *v = getenv("TMPDIR");
    return (v == 0 ? "/tmp" : v);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


#ifdef __GNUC__
static int systemf(const char *fmt, ...) __attribute__(( format(printf,1,2) ));
#endif

static int
systemf(const char *fmt, ...)
{
    estring cmd;
    va_list args;

    va_start(args, fmt);
    cmd.append_vprintf(fmt, args);
    va_end(args);

    _log.debug("Running \"%s\"\n", cmd.data());
    return system(cmd.data());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
save_file_lines(DB *db, cov_file_t *f)
{
    php_serializer_t ser;
    unsigned int lineno;
    int ret;
    char blocks_buf[64];

    // PHP-serialise the line array
    ser.begin_array(f->num_lines());
    for (lineno = 1 ; lineno <= f->num_lines() ; lineno++)
    {
	cov_line_t *ln = f->nth_line(lineno);

	ser.integer(lineno);

	ser.begin_array(3);
	ser.next_key(); ser.integer(ln->status());
	ser.next_key(); ser.integer(ln->count());
	ln->format_blocks(blocks_buf, sizeof(blocks_buf)-1);
	ser.next_key(); ser.string(blocks_buf);
	ser.end_array();
    }
    ser.end_array();

    // Store the serialised line array
    string_var key = g_strdup_printf("FL%u", ftag(f));
    if ((ret = db->put(db, 0, dbt(key), dbt(ser), 0)))
    {
	db->err(db, ret, "save_filename_index");
	exit(1);
    }
}

static void
save_lines(DB *db)
{
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
	save_file_lines(db, *iter);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_filename_index(void)
{
    unsigned int n = 0;

    // Build the filename index
    file_index = new hashtable_t<void, unsigned int>;
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
	file_index->insert((void *)*iter, new unsigned int(++n));
}

static void
save_filename_index(DB *db)
{
    php_serializer_t ser;
    int ret;

    // PHP-serialise the filename index.
    ser.begin_array(file_index->size());
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	cov_file_t *f = *iter;
	ser.string(f->minimal_name());
	ser.integer(ftag(f));
    }
    ser.end_array();

    // Store the serialised filename index
    if ((ret = db->put(db, 0, dbt("FI"), dbt(ser), 0)))
    {
	db->err(db, ret, "save_filename_index");
	exit(1);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_global_function_index(void)
{
    unsigned int n = 0;

    all_functions = cov_list_all_functions();

    function_index = new hashtable_t<void, unsigned int>;
    for (list_iterator_t<cov_function_t> iter = all_functions->first() ; *iter ; ++iter)
	function_index->insert((void *)*iter, new unsigned int(++n));
}

static gboolean
unique_remove_one(const char *key, list_t<cov_function_t> *value, void *closure)
{
    value->remove_all();
    delete value;
    return TRUE;
}

static void
save_global_function_index(DB *db)
{
    php_serializer_t ser;
    hashtable_t<const char, list_t<cov_function_t> > *unique;
    list_t<const char> keys;
    int ret;

    // Uniquify the function names
    unique = new hashtable_t<const char, list_t<cov_function_t> >;
    for (list_iterator_t<cov_function_t> fniter = all_functions->first() ; *fniter ; ++fniter)
    {
	cov_function_t *fn = *fniter;
	if (fn->is_suppressed())
	    continue;
	list_t<cov_function_t> *list = unique->lookup(fn->name());
	if (list == 0)
	{
	    list = new list_t<cov_function_t>;
	    unique->insert(fn->name(), list);
	}
	list->append(fn);
    }

    // PHP-serialise the function index
    unique->keys(&keys);
    ser.begin_array(unique->size());
    for (list_iterator_t<const char> kiter = keys.first() ; *kiter ; ++kiter)
    {
	const char *fname = *kiter;
	list_t<cov_function_t> *list = unique->lookup(fname);

	ser.string(fname);
	ser.begin_array(list->length());
	for (list_iterator_t<cov_function_t> fniter = list->first() ; *fniter ; ++fniter)
	{
	    cov_function_t *fn = *fniter;
	    ser.string(fn->file()->minimal_name());
	    ser.integer(fntag(fn));
	}
	ser.end_array();
    }
    ser.end_array();

    // Store the serialised function list
    if ((ret = db->put(db, 0, dbt("UI"), dbt(ser), 0)))
    {
	db->err(db, ret, "save_global_function_index");
	exit(1);
    }

    unique->foreach_remove(unique_remove_one, 0);
    delete unique;
}


static void
save_global_function_list(DB *db)
{
    php_serializer_t ser;
    list_iterator_t<cov_function_t> iter;
    estring label;
    unsigned int n = 0;
    int ret;

    for (iter = all_functions->first() ; *iter ; ++iter)
    {
	cov_function_t *fn = *iter;

	if (fn->is_suppressed())
	    continue;
	n++;
    }

    // PHP-serialise the function list
    ser.begin_array(n);
    for (iter = all_functions->first() ; *iter ; ++iter)
    {
	cov_function_t *fn = *iter;

	if (fn->is_suppressed())
	    continue;

	label.truncate();
	label.append_string(fn->name());

	/* see if we need to present some more scope to uniquify the name */
	list_iterator_t<cov_function_t> next = iter.peek_next();
	list_iterator_t<cov_function_t> prev = iter.peek_prev();
	if ((*next && !strcmp((*next)->name(), fn->name())) ||
	    (*prev && !strcmp((*prev)->name(), fn->name())))
	{
	    label.append_string(" [");
	    label.append_string(fn->file()->minimal_name());
	    label.append_string("]");
	}

	ser.string(label);
	ser.string(fn->file()->minimal_name());
    }
    ser.end_array();

    // Store the serialised function list
    if ((ret = db->put(db, 0, dbt("UL"), dbt(ser), 0)))
    {
	db->err(db, ret, "save_global_function_list");
	exit(1);
    }
}

static void
save_file_function_indexes(DB *db)
{
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	cov_file_t *f = *iter;
	php_serializer_t ser;
	int ret;

	ser.begin_array(f->num_functions());
	for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
	{
	    cov_function_t *fn = *fnitr;

	    ser.string(fn->name());
	    ser.integer(fntag(fn));
	}
	ser.end_array();

	// Store the serialised function index
	string_var key = g_strdup_printf("FUI%u", ftag(f));
	if ((ret = db->put(db, 0, dbt(key), dbt(ser), 0)))
	{
	    db->err(db, ret, "save_file_function_indexes");
	    exit(1);
	}
    }
}

static inline void
serlineno(php_serializer_t *ser, const cov_location_t *loc)
{
    ser->integer(loc == 0 ? 0UL : loc->lineno);
}

static void
save_functions(DB *db)
{
    for (list_iterator_t<cov_function_t> iter = all_functions->first() ; *iter ; ++iter)
    {
	cov_function_t *fn = *iter;
	php_serializer_t ser;
	int ret;

	ser.begin_array(5);
	ser.next_key(); ser.string(fn->name());
	ser.next_key(); ser.integer(fn->status());
	ser.next_key(); ser.string(fn->file()->minimal_name());
	ser.next_key(); serlineno(&ser, fn->get_first_location());
	ser.next_key(); serlineno(&ser, fn->get_last_location());
	ser.end_array();

	// Store the function data
	string_var key = g_strdup_printf("U%u", fntag(fn));
	if ((ret = db->put(db, 0, dbt(key), dbt(ser), 0)))
	{
	    db->err(db, ret, "save_functions");
	    exit(1);
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
serialise_ulong_array(
    php_serializer_t *ser,
    unsigned int n,
    const unsigned long *p)
{
    unsigned int i;

    ser->begin_array(n);
    for (i = 0 ; i < n ; i++)
    {
	ser->next_key();
	ser->integer(p[i]);
    }
    ser->end_array();
}

static void
save_one_summary_f(DB *db, cov_scope_t *sc, const char *key)
{
    php_serializer_t ser;
    const cov_stats_t *stats = sc->get_stats();
    int ret;

    // PHP-serialise the scope's stats
    ser.begin_array(5);
    ser.next_key();
    serialise_ulong_array(&ser, cov::NUM_STATUS, stats->blocks_by_status());
    ser.next_key();
    serialise_ulong_array(&ser, cov::NUM_STATUS, stats->lines_by_status());
    ser.next_key();
    serialise_ulong_array(&ser, cov::NUM_STATUS, stats->functions_by_status());
    ser.next_key();
    serialise_ulong_array(&ser, cov::NUM_STATUS, stats->calls_by_status());
    ser.next_key();
    serialise_ulong_array(&ser, cov::NUM_STATUS, stats->branches_by_status());
    ser.end_array();

    if ((ret = db->put(db, 0, dbt(key), dbt(ser), 0)))
    {
	db->err(db, ret, "save_one_summary_f");
	exit(1);
    }
}

static void
save_summaries(DB *db)
{
    cov_scope_t *sc;

    // Save an overall scope object
    sc = new cov_overall_scope_t;
    save_one_summary_f(db, sc, "OS");
    delete sc;

    // Save a file scope object for each file
    for (list_iterator_t<cov_file_t> fiter = cov_file_t::first() ; *fiter ; ++fiter)
    {
	cov_file_t *f = *fiter;
	sc = new cov_file_scope_t(f);
	string_var key = g_strdup_printf("FS%u", ftag(f));
	save_one_summary_f(db, sc, key);
	delete sc;
    }

    // Save a function scope object for each function
    for (list_iterator_t<cov_function_t> fniter = all_functions->first() ; *fniter ; ++fniter)
    {
	cov_function_t *fn = *fniter;
	sc = new cov_function_scope_t(fn);
	string_var key = g_strdup_printf("US%u", fntag(fn));
	save_one_summary_f(db, sc, key);
	delete sc;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_callnode_index(void)
{
    unsigned int n = 0;

    callnode_index = new hashtable_t<void, unsigned int>;
    for (cov_callspace_iter_t csitr = cov_callgraph.first() ; *csitr ; ++csitr)
    {
	for (cov_callnode_iter_t cnitr = (*csitr)->first() ; *cnitr ; ++cnitr)
	    callnode_index->insert((void *)*cnitr, new unsigned int(++n));
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
save_callnode_index(DB *db)
{
    list_t<cov_callnode_t> all;
    php_serializer_t ser;
    int ret;

    // Sort the callnode index, for the node <select>
    for (cov_callspace_iter_t csitr = cov_callgraph.first() ; *csitr ; ++csitr)
    {
	for (cov_callnode_iter_t cnitr = (*csitr)->first() ; *cnitr ; ++cnitr)
	    all.append(*cnitr);
    }
    all.sort(cov_callnode_t::compare_by_name_and_file);

    // PHP-serialise the callnode index
    ser.begin_array(callnode_index->size());
    for (list_iterator_t<cov_callnode_t> iter = all.first() ; *iter ; ++iter)
    {
	cov_callnode_t *cn = *iter;

	ser.string(cn->name);
	ser.integer(cntag(cn));
    }
    ser.end_array();

    all.remove_all();

    // Save the callnode index
    if ((ret = db->put(db, 0, dbt("NI"), dbt(ser), 0)))
    {
	db->err(db, ret, "save_callnode_index");
	exit(1);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
serialize_arc_list(php_serializer_t *ser, list_t<cov_callarc_t> &arcs, bool out)
{
    list_t<cov_callarc_t> copy = arcs.copy();
    copy.sort(out ? cov_callarc_t::compare_by_count_and_to :
		    cov_callarc_t::compare_by_count_and_from);

    ser->begin_array(copy.length());
    for (list_iterator_t<cov_callarc_t> itr = copy.first() ; *itr ; ++itr)
    {
	cov_callarc_t *ca = *itr;
	cov_callnode_t *peer;

	ser->next_key();
	ser->begin_array(3);
	peer = (out ? ca->to : ca->from);
	ser->next_key(); ser->string(peer->name);
	ser->next_key(); ser->integer(cntag(peer));
	ser->next_key(); ser->integer(ca->count);
	ser->end_array();
    }
    copy.remove_all();
    ser->end_array();
}

static void
save_callnode(cov_callnode_t *cn, DB *db)
{
    php_serializer_t ser;
    int ret;

    ser.begin_array(4);
    ser.next_key(); ser.string(cn->name);
    ser.next_key(); ser.integer(cn->count);
    ser.next_key(); serialize_arc_list(&ser, cn->in_arcs, FALSE);
    ser.next_key(); serialize_arc_list(&ser, cn->out_arcs, TRUE);
    ser.end_array();

    // Save the callnode
    string_var key = g_strdup_printf("N%u", cntag(cn));
    if ((ret = db->put(db, 0, dbt(key), dbt(ser), 0)))
    {
	db->err(db, ret, "save_callnode");
	exit(1);
    }
}

static void
save_callgraph(DB *db)
{
    for (cov_callspace_iter_t csitr = cov_callgraph.first() ; *csitr ; ++csitr)
    {
	for (cov_callnode_iter_t cnitr = (*csitr)->first() ; *cnitr ; ++cnitr)
	    save_callnode(*cnitr, db);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
save_report_index(DB *db)
{
    const report_t *rep;
    unsigned int n;
    php_serializer_t ser;
    int ret;

    // PHP-serialise the report index
    for (rep = all_reports, n = 0 ; rep->name != 0 ; rep++, n++)
	;
    ser.begin_array(n);
    for (rep = all_reports, n = 0 ; rep->name != 0 ; rep++, n++)
    {
	ser.string(rep->name);
	ser.begin_array(2);
	ser.next_key(); ser.integer(n);
	ser.next_key(); ser.string(rep->label);
	ser.end_array();
    }
    ser.end_array();

    // Save the report index
    if ((ret = db->put(db, 0, dbt("RI"), dbt(ser), 0)))
    {
	db->err(db, ret, "save_report_index");
	exit(1);
    }
}

static void
save_reports(DB *db)
{
    const report_t *rep;
    unsigned int n;
    string_var tmp_filename;
    int fd;
    FILE *fp;
    int len;
    estring buffer;

    tmp_filename = g_strconcat(get_tmpdir(),
			       "/ggcov-web-reportXXXXXX",
			       (char *)0);
    if ((fd = mkstemp((char *)tmp_filename.data())) < 0)
    {
	_log.perror(tmp_filename);
	exit(1);
    }
    if ((fp = fdopen(fd, "r+")) == NULL)
    {
	_log.perror("fdopen");
	close(fd);
	exit(1);
    }

    for (rep = all_reports, n = 0 ; rep->name != 0 ; rep++, n++)
    {
	php_serializer_t ser;
	string_var key;
	int ret;

	// Rewind and truncate the temp file
	rewind(fp);
	if (ftruncate(fd, (off_t)0) < 0)
	{
	    _log.perror(tmp_filename);
	    exit(1);
	}

	// Run the report, capturing the output in the temp file
	(*rep->func)(fp, tmp_filename);
	fflush(fp);

	// Read the report output into buffer
	if ((len = fd_length(fd)) < 0)
	{
	    _log.perror(tmp_filename);
	    exit(1);
	}
	buffer.truncate_to(len);
	rewind(fp);
	int r = fread((char *)buffer.data(), 1, len, fp);
	if (r != len)
	{
	    _log.perror(tmp_filename);
	    exit(1);
	}

	// PHP-serialise the report data
	ser.stringl(buffer.data(), buffer.length());

	// Save the report data
	key = g_strdup_printf("R%u", n);
	if ((ret = db->put(db, 0, dbt(key), dbt(ser), 0)))
	{
	    db->err(db, ret, "save_reports");
	    exit(1);
	}
    }

    fclose(fp);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static ptrarray_t<diagram_t> *
diagrams(void)
{
    static ptrarray_t<diagram_t> *dd;

    if (dd == 0)
    {
	dd = new ptrarray_t<diagram_t>;
	dd->append(new callgraph_diagram_t());
	dd->append(new lego_diagram_t());
    }
    return dd;
}

static void
save_diagram_index(DB *db)
{
    unsigned int i;
    php_serializer_t ser;
    int ret;

    // PHP-serialise the diagram index
    ser.begin_array();
    for (i = 0 ; i < diagrams()->length() ; i++)
    {
	diagram_t *di = diagrams()->nth(i);

	ser.string(di->name());
	ser.begin_array(2);
	ser.next_key(); ser.integer(i);
	ser.next_key(); ser.string(di->title());
	ser.end_array();
    }
    ser.end_array();

    // Save the diagram index
    if ((ret = db->put(db, 0, dbt("GI"), dbt(ser), 0)))
    {
	db->err(db, ret, "save_diagram_index");
	exit(1);
    }
}

static void
save_diagrams(DB *db)
{
    unsigned int i;
    string_var key;
    int ret;

    for (i = 0 ; i < diagrams()->length() ; i++)
    {
	diagram_t *di = diagrams()->nth(i);

	php_scenegen_t *sg = new php_scenegen_t();

	di->prepare();
	di->render(sg);

	dbounds_t bounds;
	di->get_bounds(&bounds);
	sg->bounds(bounds.x1, bounds.y1,
		   (bounds.x2 - bounds.x1), (bounds.y2 - bounds.y1));

	// Save the diagram data
	key = g_strdup_printf("G%u", i);
	if ((ret = db->put(db, 0, dbt(key), dbt(sg->data()), 0)))
	{
	    db->err(db, ret, "save_diagrams");
	    exit(1);
	}

	delete sg;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
create_source_symlinks(const char *tempdir)
{
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
    {
	cov_file_t *f = *iter;
	string_var link = g_strconcat(tempdir, "/", f->minimal_name(), (char *)0);
	string_var tempmindir = file_dirname(link);
	file_build_tree(tempmindir, 0755);
	_log.debug("symlink %s -> %s\n", link.data(), f->name());
	if (symlink(f->name(), link) < 0)
	    _log.perror(f->name());
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * With the old GTK, we're forced to parse our own arguments the way
 * the library wants, with popt and in such a way that we can't use the
 * return from poptGetNextOpt() to implement multiple-valued options
 * (e.g. -o dir1 -o dir2).  This limits our ability to parse arguments
 * for both old and new GTK builds.  Worse, gtk2 doesn't depend on popt
 * at all, so some systems will have gtk2 and not popt, so we have to
 * parse arguments in a way which avoids potentially buggy duplicate
 * specification of options, i.e. we simulate popt in fakepopt.c!
 */

class webdb_params_t : public cov_project_params_t
{
public:
    webdb_params_t();
    ~webdb_params_t();

    ARGPARSE_STRING_PROPERTY(output_tarball);
    ARGPARSE_STRING_PROPERTY(dump_mode);

protected:
    void setup_parser(argparse::parser_t &parser)
    {
	cov_project_params_t::setup_parser(parser);
	parser.add_option('f', "output-file")
	      .description("name of the output (in .tgz format), or - for stdout")
	      .setter((argparse::arg_setter_t)&webdb_params_t::set_output_tarball);
	parser.add_option('\0', "dump")
	      .description("dump the entire database")
	      .setter((argparse::arg_setter_t)&webdb_params_t::set_dump_mode);
	parser.set_other_option_help("[OPTIONS] [executable|source|directory]...");
    }
};

webdb_params_t::webdb_params_t()
 :  output_tarball_("ggcov.webdb.tgz")
{
}

webdb_params_t::~webdb_params_t()
{
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

void
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

static int
create_database(const webdb_params_t &params)
{
    char *tempdir;
    char *webdb_file;
    DB *db;
    int ret;

    cov_read_files(params);

    tempdir = g_strdup_printf("%s/ggcov-web-%d.d", get_tmpdir(), (int)getpid());
    webdb_file = g_strconcat(tempdir, "/", "ggcov.webdb", (char*)0);

    systemf("/bin/rm -rf \"%s\"", tempdir);
    file_build_tree(tempdir, 0755);

    if ((ret = db_create(&db, 0, 0)))
    {
	_log.error("db_create(): %s\n", db_strerror(ret));
	exit(1);
    }

    ret = db->open(db, OPEN_TXN webdb_file, 0, DB_HASH, DB_CREATE, 0644);
    if (ret)
    {
	db->err(db, ret, "%s", webdb_file);
	exit(1);
    }

    build_filename_index();
    save_filename_index(db);
    save_lines(db);
    build_global_function_index();
    save_global_function_index(db);
    save_global_function_list(db);
    save_file_function_indexes(db);
    save_functions(db);
    save_summaries(db);
    build_callnode_index();
    save_callnode_index(db);
    save_callgraph(db);
    save_report_index(db);
    save_reports(db);
    save_diagram_index(db);
    save_diagrams(db);

    db->close(db, 0);

    create_source_symlinks(tempdir);

    const char *output_tarball = params.get_output_tarball();
    const char *output_tarball_abs = (!strcmp(output_tarball, "-") ? "-" :
				      file_make_absolute(output_tarball));
    systemf("cd \"%s\" ; tar -cvhzf \"%s\" *", tempdir, output_tarball_abs);
    systemf("/bin/rm -rf \"%s\"", tempdir);

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
dump_database(const webdb_params_t &params)
{
    const char *webdb_file;
    DB *db;
    DBC *dbc;
    DBT key, value;
    int ret;
    bool key_flag = false;
    bool value_flag = false;

    if (params.num_files() != 1)
    {
	_log.error("dump_database: must provide a .webdb filename\n");
	exit(1);
    }
    webdb_file = params.nth_file(0);

    const char *mode = params.get_dump_mode();
    for ( ; *mode ; mode++)
    {
	switch (*mode)
	{
	case 'k': key_flag = true; break;
	case 'v': value_flag = true; break;
	default:
	    _log.error("dump_database: argument to --dump must be a "
		       "combination of the characters 'k','v'\n");
	    exit(1);
	}
    }


    if ((ret = db_create(&db, 0, 0)))
    {
	_log.error("db_create(): %s\n", db_strerror(ret));
	exit(1);
    }

    if ((ret = db->open(db, OPEN_TXN webdb_file, 0, DB_HASH, DB_CREATE, 0644)))
    {
	db->err(db, ret, "%s", webdb_file);
	exit(1);
    }

    if ((ret = db->cursor(db, NULL, &dbc, 0)))
    {
	db->err(db, ret, "%s", webdb_file);
	exit(1);
    }

    for (;;)
    {
	memset(&key, 0, sizeof(key));
	memset(&value, 0, sizeof(value));

	ret = dbc->c_get(dbc, &key, &value, DB_NEXT);
	if (ret == DB_NOTFOUND)
	    break;
	if (ret)
	{
	    db->err(db, ret, "%s", webdb_file);
	    exit(1);
	}

	if (key_flag)
	    fwrite(key.data, key.size, 1, stdout);
	if (value_flag)
	{
	    if (key_flag)
		fputc('=', stdout);
	    fwrite(value.data, value.size, 1, stdout);
	}
	fputc('\n', stdout);
    }

    dbc->c_close(dbc);
    db->close(db, 0);

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    g_log_set_handler("GLib",
		      (GLogLevelFlags)(G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL),
		      log_func, /*user_data*/0);

    webdb_params_t params;
    argparse::parser_t parser(params);
    parser.parse(argc, argv);

    if (params.get_dump_mode() != NULL)
	return dump_database(params);
    else
	return create_database(params);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
