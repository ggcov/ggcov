#include "common.h"
#include "cov.h"
#include "filename.h"
#include "estring.h"
#include <dirent.h>


char *argv0;
int verbose;
GList *files;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
read_gcov_directory(const char *dirname)
{
    DIR *dir;
    struct dirent *de;
    estring child;
    const char *ext;
    int successes = 0;
    
    if ((dir = opendir(dirname)) == 0)
    {
    	perror(dirname);
    	return FALSE;
    }
    
    estring_init(&child);
    while ((de = readdir(dir)) != 0)
    {
    	if (!strcmp(de->d_name, ".") || 
	    !strcmp(de->d_name, ".."))
	    continue;
	    
	estring_truncate(&child);
	if (strcmp(dirname, "."))
	{
	    estring_append_string(&child, dirname);
	    if (child.data[child.length-1] != '/')
		estring_append_char(&child, '/');
	}
	estring_append_string(&child, de->d_name);
	
    	if (file_is_regular(child.data) == 0 &&
	    (ext = file_extension_c(child.data)) != 0 &&
	    !strcmp(ext, ".c"))
	    successes += cov_handle_c_file(child.data);
    }
    
    estring_free(&child);
    closedir(dir);
    return (successes > 0);
}


static void
read_gcov_files(void)
{
    GList *iter;
    
    if (files == 0)
    {
    	if (!read_gcov_directory("."))
	    exit(1);
    }
    else
    {
	for (iter = files ; iter != 0 ; iter = iter->next)
	{
	    const char *cfilename = (const char *)iter->data;
	    
	    if (file_is_directory(cfilename) == 0)
    		read_gcov_directory(cfilename);
	    else if (!cov_handle_c_file(cfilename))
		exit(1);
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static count_t
block_list_total(const GList *list)
{
    count_t total = 0;
    
    for ( ; list != 0 ; list = list->next)
    	total += ((cov_block_t *)list->data)->count;
    return total;
}

static void
annotate_file(cov_file_t *f, void *userdata)
{
    const char *cfilename = f->name;
    FILE *infp, *outfp;
    unsigned lineno;
    const GList *blocks;
    char *ggcov_filename;
    char buf[1024];
    
    if ((infp = fopen(cfilename, "r")) == 0)
    {
    	perror(cfilename);
	return;
    }

    ggcov_filename = g_strconcat(cfilename, ".ggcov", 0);
    fprintf(stderr, "Writing %s\n", ggcov_filename);
    if ((outfp = fopen(ggcov_filename, "w")) == 0)
    {
    	perror(ggcov_filename);
	g_free(ggcov_filename);
	fclose(infp);
	return;
    }
    g_free(ggcov_filename);
    
    
    lineno = 0;
    while (fgets(buf, sizeof(buf), infp) != 0)
    {
    	++lineno;
	
	blocks = cov_blocks_find_by_location(cfilename, lineno);
	if (blocks != 0)
	{
	    count_t total = block_list_total(blocks);
	    if (total)
		fprintf(outfp, "%12lld    ", total);
	    else
		fputs("      ######    ", outfp);
	}
	else
	    fputs("\t\t", outfp);
	fputs(buf, outfp);
    }
    
    fclose(infp);
    fclose(outfp);
}

static void
annotate(void)
{
    cov_file_foreach(annotate_file, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG

static void
dump_arc(cov_arc_t *a)
{
    fprintf(stderr, "                    ARC {\n");
    fprintf(stderr, "                        FROM=%s:%u\n",
    	    	    	    	a->from->function->name,
				a->from->idx);
    fprintf(stderr, "                        TO=%s:%u\n",
    	    	    	    	a->to->function->name,
				a->to->idx);
    fprintf(stderr, "                        COUNT=%lld\n", a->count);
    fprintf(stderr, "                        ON_TREE=%s\n", boolstr(a->on_tree));
    fprintf(stderr, "                        FAKE=%s\n", boolstr(a->fake));
    fprintf(stderr, "                        FALL_THROUGH=%s\n", boolstr(a->fall_through));
    fprintf(stderr, "                    }\n");
}

static void
dump_block(cov_block_t *b)
{
    GList *iter;
    
    fprintf(stderr, "            BLOCK {\n");
    fprintf(stderr, "                IDX=%s:%u\n", b->function->name, b->idx);
    fprintf(stderr, "                COUNT=%lld\n",b->count);

    fprintf(stderr, "                OUT_ARCS {\n");
    for (iter = b->out_arcs ; iter != 0 ; iter = iter->next)
    	dump_arc((cov_arc_t *)iter->data);
    fprintf(stderr, "                }\n");

    fprintf(stderr, "                IN_ARCS {\n");
    for (iter = b->in_arcs ; iter != 0 ; iter = iter->next)
    	dump_arc((cov_arc_t *)iter->data);
    fprintf(stderr, "                }\n");
    
    fprintf(stderr, "                LOCATIONS {\n");
    for (iter = b->locations ; iter != 0 ; iter = iter->next)
    {
    	cov_location_t *loc = (cov_location_t *)iter->data;
	fprintf(stderr, "                    %s:%d\n", loc->filename, loc->lineno);
    }
    fprintf(stderr, "                }\n");
    fprintf(stderr, "            }\n");
}

static void
dump_function(cov_function_t *fn)
{
    int i;
    
    fprintf(stderr, "        FUNCTION {\n");
    fprintf(stderr, "            NAME=\"%s\"\n", fn->name);
    for (i = 0 ; i < fn->blocks->len ; i++)
    	dump_block((cov_block_t *)g_ptr_array_index(fn->blocks, i));
    fprintf(stderr, "    }\n");
}

static void
dump_file(cov_file_t *f, void *userdata)
{
    int i;
    
    fprintf(stderr, "FILE {\n");
    fprintf(stderr, "    NAME=\"%s\"\n", f->name);
    for (i = 0 ; i < f->functions->len ; i++)
    	dump_function((cov_function_t *)g_ptr_array_index(f->functions, i));
    fprintf(stderr, "}\n");
}

static void
summarise(void)
{
    cov_file_foreach(dump_file, 0);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char usage_str[] = 
"Usage: ggcov [directory...]\n"
" options are:\n"
"--help             print this message and exit\n"
"--version          print version and exit\n"
"--verbose          print more messages\n"
;

static void
usage(int ec)
{
    fputs(usage_str, stderr);
    fflush(stderr); /* JIC */
    
    exit(ec);
}

static void
usagef(int ec, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    fprintf(stderr, "%s: ", argv0);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    usage(ec);
}

static void
parse_args(int argc, char **argv)
{
    int i;
    
    argv0 = argv[0];
    
    for (i = 1 ; i < argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    if (!strcmp(argv[i], "--version"))
	    {
	    	printf("%s version %s\n", PACKAGE, VERSION);
	    	exit(0);
	    }
	    else if (!strcmp(argv[i], "--verbose"))
	    {
	    	verbose++;
	    }
	    else if (!strcmp(argv[i], "--help"))
	    {
	    	usage(0);
	    }
	    else
	    {
	    	usagef(1, "unknown option \"%s\"\n", argv[i]);
	    }
	}
	else
	{
	    files = g_list_append(files, argv[i]);
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    parse_args(argc, argv);
    read_gcov_files();

#if DEBUG
    summarise();
#endif
    annotate();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
