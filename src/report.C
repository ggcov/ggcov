/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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
#include "cov.H"
#include "filename.h"
#include "estring.H"
#include "report.H"

CVSID("$Id: report.C,v 1.2 2004-03-08 10:00:09 gnb Exp $");

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
report_summary_all(FILE *fp)
{
    cov_scope_t *sc = new cov_overall_scope_t;
    int nlines = print_summary(fp, sc->get_stats(), "all files");
    delete sc;
    return nlines;
}

static int
report_summary_per_directory(FILE *fp)
{
    hashtable_t<char, cov_stats_t> *ht;
    cov_stats_t *st;
    list_iterator_t<cov_file_t> fiter;
    unsigned int ndirs = 0;
    list_t<char> keys;
    char *key;
    int nlines = 0;
    
    ht = new hashtable_t<char, cov_stats_t>;
    
    for (fiter = cov_file_t::first() ; fiter != (cov_file_t *)0 ; ++fiter)
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
report_untested_functions_per_file(FILE *fp)
{
    list_iterator_t<cov_file_t> fiter;
    gboolean did_head1 = FALSE;
    int nlines = 0;
    
    for (fiter = cov_file_t::first() ; fiter != (cov_file_t *)0 ; ++fiter)
    {
    	cov_file_t *f = (*fiter);
	unsigned int i;
	gboolean did_head2 = FALSE;
	
    	for (i = 0 ; i < f->num_functions() ; i++)
	{
	    cov_function_t *fn = f->nth_function(i);
	    
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
report_poorly_covered_functions_per_file(FILE *fp)
{
    list_iterator_t<cov_file_t> fiter;
    double global_avg = global_fraction_covered();
    gboolean did_head1 = FALSE;
    int nlines = 0;

    for (fiter = cov_file_t::first() ; fiter != (cov_file_t *)0 ; ++fiter)
    {
    	cov_file_t *f = (*fiter);
	unsigned int i;
	gboolean did_head2 = FALSE;
	
    	for (i = 0 ; i < f->num_functions() ; i++)
	{
	    const cov_function_t *fn = f->nth_function(i);
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
report_incompletely_covered_functions_per_file(FILE *fp)
{
    list_iterator_t<cov_file_t> fiter;
    gboolean did_head1 = FALSE;
    int nlines = 0;

    for (fiter = cov_file_t::first() ; fiter != (cov_file_t *)0 ; ++fiter)
    {
    	cov_file_t *f = (*fiter);
	unsigned int i;
	gboolean did_head2 = FALSE;
	
    	for (i = 0 ; i < f->num_functions() ; i++)
	{
	    const cov_function_t *fn = f->nth_function(i);
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

const report_t all_reports[] = 
{
#define report(name, label) { #name, label, report_##name },
    report(summary_all,
    	   N_("Summary, all files"))
    report(summary_per_directory,
    	   N_("Summary, per directory"))
    report(untested_functions_per_file,
    	   N_("Untested functions per file"))
    report(poorly_covered_functions_per_file,
    	   N_("Poorly covered functions per file"))
    report(incompletely_covered_functions_per_file,
    	   N_("Incompletely covered functions per file"))
#undef report
    {0, 0}
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
