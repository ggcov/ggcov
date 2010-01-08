#include "common.h"
#undef HAVE_LIBPOPT
#include "fakepopt.h"
#include "fakepopt.c"

static int boolopt = 0;
static int spoolopt = 0;
const char *stringopt = "before";
const char *ropeopt = "BEFORE";
GList *files;

static const struct poptOption included_options[] =
{
    {
    	"rope",	    	    	    	    	/* longname */
	'R',  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&ropeopt,     	    	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"he's ropable",	    	    	    	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"spool",	    	    	    	/* longname */
	'S',  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	&spoolopt,     	    	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"spoolin' spoolin' spoolin'",	    	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    POPT_TABLEEND
};

static const struct poptOption options[] =
{
    {
    	"string",	    	    	    	/* longname */
	's',  	    	    	    	    	/* shortname */
	POPT_ARG_STRING,  	    	    	/* argInfo */
	&stringopt,     	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"how long is a piece of string",    	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"bool",	    	    	    	    	/* longname */
	'b',  	    	    	    	    	/* shortname */
	POPT_ARG_NONE,  	    	    	/* argInfo */
	&boolopt,     	    	    	    	/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	"to be or not to be",	    	    	/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    {
    	"whatever",				/* longname */
	'?',  	    	    	    	    	/* shortname */
	POPT_ARG_INCLUDE_TABLE,			/* argInfo */
	(void *)included_options,		/* arg */
	0,  	    	    	    	    	/* val 0=don't return */
	0,					/* descrip */
	0	    	    	    	    	/* argDescrip */
    },
    POPT_TABLEEND
};

int
main(int argc, char **argv)
{
    int rc;
    poptContext con;
    const char *file;
    GList *iter;
    const char *argv0;

    argv0 = strrchr(argv[0], '/');
    if (argv0 == NULL)
	argv0 = argv[0];
    else
	argv0++;

    con = poptGetContext("popttest", argc, (const char**)argv, options, 0);
    poptSetOtherOptionHelp(con,
    	    	           "[OPTIONS] [executable|source|directory]...");

    while ((rc = poptGetNextOpt(con)) > 0)
    	;
    if (rc < -1)
    {
    	fprintf(stderr, "%s:%s at or near %s\n",
	    argv0,
	    poptStrerror(rc),
	    poptBadOption(con, POPT_BADOPTION_NOALIAS));
    	exit(0);
    }
    
    while ((file = poptGetArg(con)) != 0)
	files = g_list_append(files, (gpointer)file);
	
    poptFreeContext(con);

    printf("boolopt=%d\n", boolopt);
    printf("spoolopt=%d\n", spoolopt);
    printf("stringopt=\"%s\"\n", stringopt);
    printf("ropeopt=\"%s\"\n", ropeopt);
    printf("files=");
    for (iter = files ; iter != 0 ; iter = iter->next)
    	printf(" \"%s\"", (const char *)iter->data);
    printf("\n");
    return 0;
}
