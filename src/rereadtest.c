/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2011 Greg Banks <gnb@users.sourceforge.net>
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
#include <sys/poll.h>
#include "cov.H"
#include "filename.h"
#include "estring.H"

char *argv0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BLOCKS_WIDTH	8

static void
annotate_file(cov_file_t *f, const char *subtest)
{
    FILE *infp, *outfp;
    unsigned long lineno;
    cov_line_t *ln;
    char buf[1024];

    if ((infp = fopen(f->name(), "r")) == 0)
    {
    	perror(f->name());
	return;
    }

    string_var ggcov_filename;
    if (subtest)
	ggcov_filename = g_strconcat(f->name(), ".", subtest, ".tggcov", (char *)0);
    else
	ggcov_filename = g_strconcat(f->name(), ".tggcov", (char *)0);
    fprintf(stderr, "Writing %s\n", ggcov_filename.data());
    if ((outfp = fopen(ggcov_filename, "w")) == 0)
    {
    	perror(ggcov_filename);
	fclose(infp);
	return;
    }

    lineno = 0;
    while (fgets(buf, sizeof(buf), infp) != 0)
    {
    	++lineno;
	ln = f->nth_line(lineno);

	if (ln->status() != cov::UNINSTRUMENTED &&
	    ln->status() != cov::SUPPRESSED)
	{
	    if (ln->count())
		fprintf(outfp, "%9llu:%5lu:", ln->count(), lineno);
	    else
		fprintf(outfp, "    #####:%5lu:", lineno);
	}
	else
	    fprintf(outfp, "        -:%5lu:", lineno);
	fputs(buf, outfp);
    }

    fclose(infp);
    fclose(outfp);
}

static void
annotate(const char *subtest)
{
    list_iterator_t<cov_file_t> iter;

    for (iter = cov_project_t::current()->first_file() ; iter != (cov_file_t *)0 ; ++iter)
    	annotate_file(*iter, subtest);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    int r;

//     debug_set("cov");

    r = system("rm -f *.da *.gcda");
    r = system("./foo.exe 1");

    cov_project_t *proj = new cov_project_t("default", 0);
    cov_init();
    proj->pre_read();
    if (!proj->read_directory(".", TRUE))
	return FALSE;
    proj->post_read();
    annotate("1");

    sleep(2);	    /* enough time to ensure the mtime changes */
    r = system("./foo.exe 2");
    r = system("./foo.exe 23 423");

    proj->re_read_counts();
    annotate("2");

    return 0;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
