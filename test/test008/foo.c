#include <stdio.h>
#include <stdlib.h>

char blocktrace[1024];
char *bp = blocktrace;

int
main(int argc, char **argv)
{
    int N;
    int i;
    
    printf("foo running\n");
    N = atoi(argv[1]);
    for (*bp++ = '1', i = 0 ; *bp++ = '2', i < N ; *bp++ = '3', i++) {
	printf("loop: %d\n", i);
	*bp++ = 'L';
    }

    printf("blocktrace[] = \"%s\"\n", blocktrace);
    
    return 0;
}
