#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

const char *me = "parent";

void
function_three(int x)
{
    printf("%s:    function_three\n", me);
}

void
function_two(int x)
{
    printf("%s:    function_two\n", me);
    if (--x)
	function_three(x);
}

void
function_one(int x)
{
    printf("%s:    function_one\n", me);
    if (--x)
	function_two(x);
}

void
do_stuff(const char *arg)
{
    int x;

    x = atoi(arg);
    function_one(x);
}

pid_t
start_child(const char *arg)
{
    pid_t pid;

    pid = fork();
    if (pid < 0)
    {
	perror("fork");
	exit(0);
    }
    else if (pid == 0)
    {
	/* child */
	me = "child";
	do_stuff(arg);
	exit(0);
    }
    else
    {
	/* parent */
	return pid;
    }
}

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
    
int
main(int argc, char **argv)
{
    pid_t pid;
    
    pid = start_child(argv[1]);
    do_stuff(argv[2]);
    wait_for_child(pid);

    return 0;
}
