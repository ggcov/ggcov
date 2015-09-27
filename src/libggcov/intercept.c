#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dlfcn.h>

/*
 * Sadly, as this is a standalone library designed to be dynamically
 * linked into 3rd party programs, we cannot use any of the ggcov debug
 * or memory infrastructure and have to replicate pale shadows of it.
 */
#undef DEBUG

static void _fatal(const char *msg)
    __attribute__((noreturn));

static void
_fatal(const char *msg)
{
    fprintf(stderr, "libggcov: cannot get real open() "
		    "call address, exiting\n");
    fflush(stderr);
    /* we were probably called from an onexit handler,
     * so avoid recursing by using _exit() not exit() */
    _exit(1);
}

static void *
_xmalloc(size_t s)
{
    void *p = malloc(s);
    if (!p)
	_fatal("libggcov: out of memory\n");
    return p;
}

#if DEBUG
static const struct
{
    const char *name;
    int mask;
    int value;
}
flagdesc[] =
{
    { "O_RDONLY", O_ACCMODE, O_RDONLY },
    { "O_WRONLY", O_ACCMODE, O_WRONLY },
    { "O_RDWR", O_ACCMODE, O_RDWR },
    { "O_CREAT", O_CREAT, O_CREAT },
    { "O_EXCL", O_EXCL, O_EXCL },
#ifdef O_NOCTTY
    { "O_NOCTTY", O_NOCTTY, O_NOCTTY },
#endif
    { "O_TRUNC", O_TRUNC, O_TRUNC },
    { "O_APPEND", O_APPEND, O_APPEND },
    { "O_NONBLOCK", O_NONBLOCK, O_NONBLOCK },
#ifdef O_NDELAY
    { "O_NDELAY", O_NDELAY, O_NDELAY },
#endif
    { "O_SYNC", O_SYNC, O_SYNC },
#ifdef O_FSYNC
    { "O_FSYNC", O_FSYNC, O_FSYNC },
#endif
#ifdef O_ASYNC
    { "O_ASYNC", O_ASYNC, O_ASYNC },
#endif
#ifdef O_DIRECTORY
    { "O_DIRECTORY", O_DIRECTORY, O_DIRECTORY },
#endif
#ifdef O_NOFOLLOW
    { "O_NOFOLLOW", O_NOFOLLOW, O_NOFOLLOW },
#endif
#ifdef O_CLOEXEC
    { "O_CLOEXEC", O_CLOEXEC, O_CLOEXEC },
#endif
#ifdef O_DIRECT
    { "O_DIRECT", O_DIRECT, O_DIRECT },
#endif
#ifdef O_NOATIME
    { "O_NOATIME", O_NOATIME, O_NOATIME },
#endif
#ifdef O_DSYNC
    { "O_DSYNC", O_DSYNC, O_DSYNC },
#endif
#ifdef O_RSYNC
    { "O_RSYNC", O_RSYNC, O_RSYNC },
#endif
#ifdef O_LARGEFILE
    { "O_LARGEFILE", O_LARGEFILE, O_LARGEFILE },
#endif
    { 0, 0, 0 },
};

static char *
describe_flags(int flags, char *buf, size_t maxlen)
{
    char *p = buf;
    char *end = buf + maxlen - 1;
    int len;
    int i;

    for (i = 0 ; flags && p < end && flagdesc[i].name ; i++)
    {
	if ((flags & flagdesc[i].mask) == flagdesc[i].value)
	{
	    flags &= ~flagdesc[i].mask;
	    len = strlen(flagdesc[i].name);
	    if (p + len + 1 >= end)
		break;
	    if (p > buf)
		*p++ = '|';
	    strcpy(p, flagdesc[i].name);
	    p += len;
	}
    }
    if (p < end && (flags || p == buf))
    {
	if (p > buf)
	    *p++ = '|';
	sprintf(buf, "0x%x", flags);
    }

    return buf;
}
#endif

/* Linux-specific tweak to call the real open() */
static int
real_open(const char *filename, int flags, int mode)
{
    static int (*func)(const char *, int, int) = 0;

    if (!func)
    {
	func = (int (*)(const char *, int, int))dlsym(RTLD_NEXT, "open");
	if (!func)
	    _fatal("libggcov: cannot get real open() "
		   "call address, exiting\n");
    }
    return func(filename, flags, mode);
}

/* Note: argument not const so we can cheat and write
 * into the path buffer */

static int
mkpath(char *path, mode_t mode)
{
    char *p;
    struct stat sb;

    for (p = path ; ; *p++ = '/')
    {
	while (*p == '/')
	    p++;            /* skip any leading / */
	while (*p && *p != '/')
	    p++;            /* skip the path component */
	if (!*p)
	    return 0;       /* last component is filename */
	*p = '\0';

#if DEBUG
	fprintf(stderr, "--> mkpath: \"%s\"\n", path);
#endif

	if (stat(path, &sb) == 0)
	{
	    /* already exists and we can stat() it */
	    if (S_ISDIR(sb.st_mode))
		continue;   /* already a directory */
	    /* not a directory...fail */
	    errno = ENOTDIR;
	    return -1;
	}
	else if (errno != ENOENT)
	    return -1;

#if DEBUG
	fprintf(stderr, "-->   mkdir\n");
#endif
	if (mkdir(path, mode) < 0)
	{
	    if (errno == EEXIST)
		continue;
	    return -1;
	}
    }
}

int
open(const char *filename, int flags, ...)
{
    va_list args;
    mode_t mode = 0;
    int fd;
    int len;
    char *freeme = 0;
    int r;
    mode_t old_umask = 0, new_umask = 0022;

    if ((flags & O_CREAT))
    {
	va_start(args, flags);
	mode = va_arg(args, mode_t);
	va_end(args);
    }

    /* Possibly redirect the filename */
    len = strlen(filename);
    if (*filename == '/' &&
	len > 5 &&
	!strcmp(filename+len-5, ".gcda"))
    {
	static const char *prefix;
	static int prefix_len = -1;

	if (prefix_len < 0)
	{
	    prefix = getenv("_GGCOV_GCDA_PREFIX");
	    prefix_len = (prefix ? strlen(prefix) : 0);
	    if (prefix)
		fprintf(stderr, "libggcov: look for .gcda files under %s\n",
			prefix);
	}
	if (prefix)
	{
	    freeme = (char *)_xmalloc(prefix_len + len + 1);
	    strcpy(freeme, prefix);
	    strcat(freeme, filename);
	    filename = freeme;
	    if ((mode & O_ACCMODE) != O_RDONLY)
	    {
		/* note: ensure other others have read permission
		 * to both the directories and files we create */
		r = mkpath(freeme, 0755);
		if (r < 0)
		{
		    perror(freeme);
		    return r;
		}
		mode |= 0444;
		old_umask = umask(new_umask);
	    }
	}
    }

    /* Call the real libc open() call */
    fd = real_open(filename, flags, mode);

#if DEBUG
    {
	char flagbuf[256];

	fprintf(stderr, "----> open(\"%s\", %s, %o) = %d\n",
		filename,
		describe_flags(flags, flagbuf, sizeof(flagbuf)),
		mode, fd);
    }
#endif

    if (old_umask != new_umask)
	umask(old_umask);
    free(freeme);
    return fd;
}
