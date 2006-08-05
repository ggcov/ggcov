#include <stdio.h>			    /* C(-) */
#include <stdlib.h>			    /* C(-) */
#include <unistd.h>			    /* C(-) */
#include <errno.h>			    /* C(-) */
#include <sys/wait.h>			    /* C(-) */
					    /* C(-) */
const char *me = "parent";		    /* C(-) */
					    /* C(-) */
void
function_three(int x)
{
    printf("%s:    function_three\n", me);  /* C(2) */
}
					    /* C(-) */
void
function_two(int x)
{
    printf("%s:    function_two\n", me);    /* C(3) */
    if (--x)				    /* C(3) */
	function_three(x);		    /* C(2) */
}
					    /* C(-) */
void
function_one(int x)
{
    printf("%s:    function_one\n", me);    /* C(4) */
    if (--x)				    /* C(4) */
	function_two(x);		    /* C(3) */
}
					    /* C(-) */
void
do_stuff(const char *arg)
{
    int x;

    x = atoi(arg);			    /* C(4) */
    function_one(x);			    /* C(4) */
}
					    /* C(-) */
pid_t
start_child(const char *arg)
{
    pid_t pid;

    pid = fork();			    /* C(2) */
    if (pid < 0)			    /* C(4) */
    {
	perror("fork");			    /* C(0) */
	exit(0);			    /* C(0) */
    }
    else if (pid == 0)			    /* C(4) */
    {
	/* child */
	me = "child";			    /* C(2) */
	do_stuff(arg);			    /* C(2) */
	exit(0);			    /* C(2) */
    }
    else
    {
	/* parent */
	return pid;			    /* C(2) */
    }
}
					    /* C(-) */
int
wait_for_child(pid_t pid)
{
    int status;
    pid_t r;

    for (;;)
    {
	fprintf(stderr, "%s:    waiting for child\n", me);
	if ((r = waitpid(pid, &status, 0)) < 0)
	{
	    if (errno == EINTR)
		continue;
	    perror("waitpid");
	    exit(1);
	}
	if (r != pid)
	{
	    fprintf(stderr, "Unexpected pid returned b waitpid(): %d expecting %d\n", (int)r, (int)pid);
	    continue;
	}
	if (WIFEXITED(status))
	{
	    fprintf(stderr, "%s:    child exited with status %d\n",
		    	me, WEXITSTATUS(status));
	    return WEXITSTATUS(status);
	}
	if (WIFSIGNALED(status))
	{
	    fprintf(stderr, "%s:    child died with signal %d\n",
		    	me, WTERMSIG(status));
	    return 128+WTERMSIG(status);
	}
    }
    /* NOTREACHED */
}
						/* C(-) */
int
main(int argc, char **argv)
{
    pid_t pid;
    
    pid = start_child(argv[1]);			/* C(2) */
    do_stuff(argv[2]);				/* C(2) */
    wait_for_child(pid);			/* C(2) */

    return 0;					/* C(2) */
}
