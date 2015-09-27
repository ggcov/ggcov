/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2011 Greg Banks <gnb@users.sourceforge.net>
 * 
 *
 * TODO: attribution for decode-gcov.c
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
#include <sys/time.h>
#include <libxml/tree.h>
#include "cov.H"
#include "filename.h"
#include "estring.H"
#include "report.H"
#include "tok.H"

CVSID("$Id: report.C,v 1.4 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
print_summary(FILE *fp, const cov_stats_t *stats, const char *what)
{
    fprintf(fp, "Summary (%s)\n", what);
    fprintf(fp, "=======\n");
    fprintf(fp, "  %g%% blocks executed (%ld of %ld)\n",
	    (stats->blocks_executed() / (double) stats->blocks_total()) * 100.0,
	    stats->blocks_executed(),
	    stats->blocks_total());

    fprintf(fp, "     (%ld blocks suppressed)\n",
	    stats->blocks_suppressed());

    fprintf(fp, "  %g%% functions executed (%ld of %ld)\n",
	    (stats->functions_executed() / (double) stats->functions_total()) * 100.0,
	    stats->functions_executed(),
	    stats->functions_total());

    fprintf(fp, "  %g%% functions completely executed (%ld of %ld)\n",
	    (stats->functions_full() / (double) stats->functions_total()) * 100.0,
	    stats->functions_full(),
	    stats->functions_total());

    fprintf(fp, "     (%ld functions suppressed)\n",
	    stats->functions_suppressed());

    fprintf(fp, "  %g%% lines executed (%ld of %ld)\n",
	    (stats->lines_executed() / (double) stats->lines_total()) * 100.0,
	    stats->lines_executed(),
	    stats->lines_total());

    fprintf(fp, "  %g%% lines completely executed (%ld of %ld)\n",
	    (stats->lines_full() / (double) stats->lines_total()) * 100.0,
	    stats->lines_full(),
	    stats->lines_total());

    fprintf(fp, "     (%ld lines suppressed)\n",
	    stats->lines_suppressed());

    return 10;
}

static int
report_summary_all(FILE *fp, const char *)
{
    cov_scope_t *sc = new cov_overall_scope_t;
    int nlines = print_summary(fp, sc->get_stats(), "all files");
    delete sc;
    return nlines;
}

static int
report_summary_per_directory(FILE *fp, const char *)
{
    hashtable_t<char, cov_stats_t> *ht;
    cov_stats_t *st;
    unsigned int ndirs = 0;
    list_t<char> keys;
    char *key;
    int nlines = 0;
    
    ht = new hashtable_t<char, cov_stats_t>;
    
    for (list_iterator_t<cov_file_t> fiter = cov_file_t::first() ; *fiter ; ++fiter)
    {
    	dprintf1(D_REPORT, "report_summary_per_directory: [1] \"%s\"\n",
	    	(*fiter)->minimal_name());
		
	string_var dir = g_dirname((*fiter)->minimal_name());
	if ((st = ht->lookup((char *)dir.data())) == 0)
	{
	    st = new cov_stats_t;
    	    dprintf1(D_REPORT, "report_summary_per_directory: -> \"%s\"\n",
	    	    	dir.data());
	    ht->insert(dir.take(), st);
	    ndirs++;
	}

    	cov_file_scope_t fscope(*fiter);
	st->accumulate(fscope.get_stats());
    }
    
    ht->keys(&keys);

    while ((key = keys.remove_head()) != 0)
    {
    	dprintf1(D_REPORT, "report_summary_per_directory: [2] \"%s\"\n", key);

	st = ht->lookup(key);
	if (ndirs > 1)
	{
	    if (nlines)
	    	fputc('\n', fp);
	    nlines += print_summary(fp, st, key);
	}
	ht->remove(key);
	g_free(key);
	delete st;
    }
    
    delete ht;
    return nlines;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
report_untested_functions_per_file(FILE *fp, const char *)
{
    gboolean did_head1 = FALSE;
    int nlines = 0;
    
    for (list_iterator_t<cov_file_t> fiter = cov_file_t::first() ; *fiter ; ++fiter)
    {
    	cov_file_t *f = (*fiter);
	gboolean did_head2 = FALSE;

	for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
	{
	    cov_function_t *fn = *fnitr;

	    if (fn->status() != cov::UNCOVERED)
	    	continue;

    	    if (!did_head1)
	    {
	    	did_head1 = TRUE;
    		fprintf(fp, "Uncovered functions\n");
    		fprintf(fp, "===================\n");
		nlines += 2;
	    }
    	    if (!did_head2)
	    {
	    	did_head2 = TRUE;
    		fprintf(fp, "%s\n", f->minimal_name());
		nlines++;
	    }
	    fprintf(fp, "  %s\n", fn->name());
	    nlines++;
	}
    }
    
    return nlines;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static double
global_fraction_covered(void)
{
    cov_overall_scope_t sc;
    return sc.get_stats()->blocks_fraction();
}

static int
report_poorly_covered_functions_per_file(FILE *fp, const char *)
{
    double global_avg = global_fraction_covered();
    gboolean did_head1 = FALSE;
    int nlines = 0;

    for (list_iterator_t<cov_file_t> fiter = cov_file_t::first() ; *fiter ; ++fiter)
    {
    	cov_file_t *f = (*fiter);
	gboolean did_head2 = FALSE;

	for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
	{
	    const cov_function_t *fn = *fnitr;
    	    cov_function_scope_t fnscope(fn);
	    double fraction;
	    
	    if (fnscope.status() != cov::PARTCOVERED)
	    	continue;

	    fraction = fnscope.get_stats()->blocks_fraction();
	    if (fraction >= global_avg)
	    	continue;
		
    	    if (!did_head1)
	    {
	    	did_head1 = TRUE;
    		fprintf(fp, "Poorly covered functions\n");
    		fprintf(fp, "========================\n");
		nlines += 2;
	    }
	    if (!did_head2)
	    {
	    	did_head2 = TRUE;
    		fprintf(fp, "%s\n", f->minimal_name());
		nlines++;
	    }
	    fprintf(fp, "  %s (%g%%)\n", fn->name(), fraction);
	    nlines++;
	}
    }
    return nlines;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
report_incompletely_covered_functions_per_file(FILE *fp, const char *)
{
    gboolean did_head1 = FALSE;
    int nlines = 0;

    for (list_iterator_t<cov_file_t> fiter = cov_file_t::first() ; *fiter ; ++fiter)
    {
    	cov_file_t *f = (*fiter);
	gboolean did_head2 = FALSE;

	for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
	{
	    const cov_function_t *fn = *fnitr;
    	    cov_function_scope_t fnscope(fn);
	    const cov_stats_t *st = fnscope.get_stats();
	    
	    if (fnscope.status() != cov::PARTCOVERED)
	    	continue;

    	    if (!did_head1)
	    {
	    	did_head1 = TRUE;
    		fprintf(fp, "Incompletely covered functions\n");
    		fprintf(fp, "==============================\n");
		nlines += 2;
	    }
	    if (!did_head2)
	    {
	    	did_head2 = TRUE;
    		fprintf(fp, "%s\n", f->minimal_name());
		nlines++;
	    }
	    fprintf(fp, "  %s (%ld/%ld uncovered blocks)\n",
	    	    	fn->name(),
			(st->blocks_total() - st->blocks_executed()),
			st->blocks_total());
	    nlines++;
	}
    }
    return nlines;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class xml_node_t : private xmlNode
{
public:
    static xml_node_t *wrap(xmlNode *n) { return (xml_node_t *)n; }
    xmlNode *unwrap() { return (xmlNode *)this; }
    xml_node_t *new_child(const char *name, const char *content = 0)
    {
	return (xml_node_t *)xmlNewChild((xmlNode *)this,
					 0,
					 (const xmlChar *)name,
					 (const xmlChar *)content);
    }
    void add_prop(const char *name, const char *value)
    {
	xmlNewProp((xmlNode *)this, (const xmlChar *)name, (const xmlChar *)value);
    }
    void add_propf(const char *name, const char *fmt, ...)
	__attribute__((format(printf,3,4)))
    {
	estring e;
	va_list args;
	va_start(args, fmt);
	e.append_vprintf(fmt, args);
	va_end(args);
	xmlNewProp((xmlNode *)this, (const xmlChar *)name, (const xmlChar *)e.data());
    }
};


class cob_report_t
{
public:
    cob_report_t();
    ~cob_report_t();

    void add(cov_file_t *);
    void post_add();
    void emit(FILE *fp);

    struct package_t
    {
	string_var name_;
	list_t<cov_file_t> files_;
	cov_stats_t stats_;

	package_t(char *name)
	 :  name_(name)
	{
	}

	~package_t()
	{
	    files_.remove_all();
	}
    };
private:

    char *path(cov_file_t *f);
    void post_add_lines(xml_node_t *xparent, cov_file_t *f,
		        unsigned int first, unsigned int last);
    void setup_common();
    void setup_xdoc();
    void post_add_coverage_props(xml_node_t *, const cov_stats_t *, int);
    void post_add_method(xml_node_t *, cov_function_t *fn);
    void post_add_class(xml_node_t *, package_t *pkg, cov_file_t *f);
    void post_add_package(xml_node_t *, package_t *pkg);

    hashtable_t<const char, package_t> *packages_;
    const char *common_;
    unsigned int common_len_;
    cov_stats_t stats_;
    xmlDoc *xdoc_;
    xml_node_t *xroot_;
};

cob_report_t::cob_report_t()
{
    packages_ = new hashtable_t<const char, package_t>;

    setup_common();
    setup_xdoc();
}

void
cob_report_t::setup_common()
{
    common_ = cov_file_t::common_path();
    common_len_ = strlen(common_);

    /* double check that common_ starts and ends with a / and is at
     * least 3 chars long -- we rely on this */
    assert(common_ != 0);
    assert(common_len_ >= 3);
    assert(common_[0] == '/');
    assert(common_[common_len_-1] == '/');

    const char *p = common_ + common_len_ - 2;
    while (p >= common_ && *p != '/')
	--p;
    /* common_len_ includes a trailing / */
    common_len_ = p+1 - common_;
}

void
cob_report_t::setup_xdoc()
{
    xdoc_ = xmlNewDoc((const xmlChar *)XML_DEFAULT_VERSION);
    xmlCreateIntSubset(xdoc_,
		       (const xmlChar *)"coverage",
		       (const xmlChar *)"coverage",
		       (const xmlChar *)"http://cobertura.sourceforge.net/xml/coverage-04.dtd");
    xroot_ = xml_node_t::wrap(xmlNewDocNode(xdoc_, 0,
			      (const xmlChar *)"coverage", 0));
    xmlDocSetRootElement(xdoc_, xroot_->unwrap());

    string_var common = g_strndup(common_, common_len_-1);
    xroot_->new_child("sources")->new_child("source", common);

    /* Fake a Java timestamp which appears to be milliseconds
     * since the Unix epoch */
    struct timeval now;
    char buf[64];
    gettimeofday(&now, 0);
    snprintf(buf, sizeof(buf), "%lu%03u",
	    (unsigned long)now.tv_sec,
	    ((unsigned)now.tv_usec) / 1000);
    xroot_->add_prop("timestamp", buf);
    xroot_->add_prop("version", "1.9");
}

cob_report_t::~cob_report_t()
{
    xmlFreeDoc(xdoc_);
    delete packages_;
}

//
// Make and return a new string which is a partial pathname for the
// given file but starting with a common first directory rather than
// being minimal.  This ensures that the cobertura file sees a root
// <package> instead of trying to put <class>es in <coverage>.
//
char *
cob_report_t::path(cov_file_t *f)
{
    return g_strconcat(common_ + common_len_, f->minimal_name(), (char *)0);
}

void
cob_report_t::add(cov_file_t *f)
{
    if (f->minimal_name()[0] == '/')
	return;	    /* not common, probably in /usr/include */

    string_var fpath = path(f);
    string_var ppath = g_dirname(fpath);
    package_t *pkg = packages_->lookup(ppath);
    if (!pkg)
    {
	pkg = new package_t(ppath.take());
	packages_->insert(pkg->name_, pkg);
    }
    pkg->files_.append(f);
}

void
cob_report_t::post_add_lines(
    xml_node_t *xparent,
    cov_file_t *f,
    unsigned int first,
    unsigned int last)
{
    xml_node_t *xlines = xparent->new_child("lines");
    unsigned int lineno;
    for (lineno = first ; lineno <= last ; lineno++)
    {
	cov_line_t *ln = f->nth_line(lineno);
	if (ln->status() == cov::UNINSTRUMENTED ||
	    ln->status() == cov::SUPPRESSED)
	    continue;
	xml_node_t *xline = xlines->new_child("line");
	xline->add_propf("number", "%u", lineno);
	xline->add_propf("hits", "%llu", (unsigned long long)ln->count());
	xline->add_prop("branch", "false"); /* TODO */
    }
}

void
cob_report_t::post_add_coverage_props(
    xml_node_t *xnode,
    const cov_stats_t *stats,
    int level)
{
    xnode->add_propf("line-rate", "%f", stats->lines_fraction());
    xnode->add_propf("branch-rate", "%f", stats->branches_fraction());
    if (!level--)
	return;
    xnode->add_prop("complexity", "0.0"); /* TODO: WTF is this? */
    if (!level--)
	return;
    xnode->add_propf("lines-covered", "%lu", stats->lines_executed());
    xnode->add_propf("lines-valid", "%lu", stats->lines_total());
    xnode->add_propf("branches-covered", "%lu", stats->branches_executed());
    xnode->add_propf("branches-valid", "%lu", stats->branches_total());
}

void
cob_report_t::post_add_method(xml_node_t *xmethods, cov_function_t *fn)
{
    cov_function_scope_t scope(fn);
    if (scope.status() == cov::SUPPRESSED)
	return;
    xml_node_t *xmethod = xmethods->new_child("method");
    xmethod->add_prop("name", fn->name());
    /* TODO: do proper demangling of C++ names */
    xmethod->add_propf("signature", "void %s(void)", fn->name());
    post_add_coverage_props(xmethod, scope.get_stats(), 0);

    const cov_location_t *first = fn->get_first_location();
    const cov_location_t *last = fn->get_last_location();
    if (first &&
        last &&
	!strcmp(first->filename, last->filename) &&
	!strcmp(first->filename, fn->file()->name()))
	post_add_lines(xmethod, fn->file(), first->lineno, last->lineno);
}

void
cob_report_t::post_add_class(
    xml_node_t *xclasses,
    package_t *pkg,
    cov_file_t *f)
{
    xml_node_t *xclass = xclasses->new_child("class");

    string_var fpath = path(f);
    xclass->add_prop("filename", fpath);

    estring name = fpath.data();
    const char *ext = strrchr(name.data(), '.');
    if (ext)
	name.truncate_to(ext - name.data());
    name.replace_all("/", ".");
    xclass->add_prop("name", name);

    cov_file_scope_t scope(f);
    const cov_stats_t *stats = scope.get_stats();
    pkg->stats_.accumulate(stats);
    post_add_coverage_props(xclass, stats, 1);

    xml_node_t *xmethods = xclass->new_child("methods");
    for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
	post_add_method(xmethods, *fnitr);

    post_add_lines(xclass, f, 1, f->num_lines());
}

void
cob_report_t::post_add_package(xml_node_t *xpackages, package_t *pkg)
{
    xml_node_t *xpkg = xpackages->new_child("package");
    xpkg->add_prop("name", pkg->name_);
    xml_node_t *xclasses = xpkg->new_child("classes");

    for (list_iterator_t<cov_file_t> fiter = pkg->files_.first() ; *fiter ; ++fiter)
	post_add_class(xclasses, pkg, *fiter);

    post_add_coverage_props(xpkg, &pkg->stats_, 1);
    stats_.accumulate(&pkg->stats_);
}

void
cob_report_t::post_add()
{
    xml_node_t *xpackages = xroot_->new_child("packages");
    hashtable_iter_t<const char, package_t> hiter;
    for (hiter = packages_->first() ; *hiter ; ++hiter)
    {
	hiter.remove();
	post_add_package(xpackages, *hiter);
    }
    post_add_coverage_props(xroot_, &stats_, 2);
}

void
cob_report_t::emit(FILE *fp)
{
    xmlDocDump(fp, xdoc_);
}

static int
report_cobertura(FILE *fp, const char *filename)
{
    if (!filename)
    {
	fprintf(fp, "This report isn't really meant for human eyes.\n"
		    "Please try the Save As button\n");
	return 1;
    }

    cob_report_t report;

    for (list_iterator_t<cov_file_t> fiter = cov_file_t::first() ; *fiter ; ++fiter)
	report.add(*fiter);

    report.post_add();
    report.emit(fp);

    return 1;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const report_t all_reports[] = 
{
#define report(name, label, fname) { #name, label, report_##name, fname },
    report(summary_all,
	   N_("Summary, all files"), NULL)
    report(summary_per_directory,
	   N_("Summary, per directory"), NULL)
    report(untested_functions_per_file,
	   N_("Untested functions per file"), NULL)
    report(poorly_covered_functions_per_file,
	   N_("Poorly covered functions per file"), NULL)
    report(incompletely_covered_functions_per_file,
	   N_("Incompletely covered functions per file"), NULL)
    report(cobertura,
	   N_("Cobertura-compatible XML report"), "cobertura.xml")
#undef report
    {0, 0}
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
