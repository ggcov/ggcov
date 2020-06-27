/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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

#include "filename.h"
#include "estring.H"
#include "string_var.H"
#include "tok.H"
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#ifndef __set_errno
#define __set_errno(v)   errno = (v)
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
file_dirname(const char *filename)
{
    const char *base;

    if ((base = file_basename_c(filename)) == filename)
	return g_strdup(".");
    return g_strndup(filename, (base - filename - 1));
}

const char *
file_basename_c(const char *filename)
{
    const char *base;

    return ((base = strrchr(filename, '/')) == 0 ? filename : ++base);
}

const char *
file_extension_c(const char *filename)
{
    const char *base = file_basename_c(filename);

    return strrchr(base, '.');
}

char *
file_change_extension(
    const char *filename,
    const char *oldext,
    const char *newext)
{
    estring e;
    int oldlen;

    if (oldext == 0)
    {
	if ((oldext = file_extension_c(filename)) == 0)
	    return 0;
    }

    e.append_string(filename);
    oldlen = strlen(oldext);
    if (!strcmp(e.data()+e.length()-oldlen, oldext))
    {
	e.truncate_to(e.length()-oldlen);
	e.append_string(newext);
    }

    return e.take();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
fd_length(int fd)
{
    struct stat sb;

    if (fstat(fd, &sb) < 0)
	return -1;
    return sb.st_size;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Returns an int instead of a mode_t because we can fail and
 * return -1 but mode_t is unsigned and doesn't allow for -1.
 */
int
file_mode(const char *filename)
{
    struct stat sb;

    if (stat(filename, &sb) < 0)
	return -1;

    return (sb.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

FILE *
file_open_mode(const char *filename, const char *rw, mode_t mode)
{
    int fd;
    FILE *fp;
    int flags;

    if (rw[0] == 'r')
	flags = O_RDONLY;
    else
	flags = O_WRONLY|O_CREAT;

    if ((fd = open(filename, flags, mode)) < 0)
	return 0;

    if ((fp = fdopen(fd, rw)) == 0)
    {
	int e = errno;
	close(fd);
	__set_errno(e);
	return 0;
    }

    return fp;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Return a new string which is a normalised absolute filename.
 */

static gboolean
is_path_tail(const char *path, const char *file)
{
    if (strlen(file) > strlen(path))
	return FALSE;
    const char *tail = path + strlen(path) - strlen(file);
    return (!strcmp(tail, file) && (tail == path || tail[-1] == '/'));
}

static const char *
file_make_absolute_to(
    const char *filename,
    const char *absfile,
    gboolean isdir)
{
    static estring abs;
    tok_t tok(filename, "/");
    const char *part;

    abs.truncate();
    if (*filename == '/')
    {
	abs.append_string("/");
    }
    else if (absfile != 0)
    {
	if (isdir)
	{
	    abs.append_string(absfile);
	}
	else
	{
	    if (is_path_tail(absfile, filename))
	    {
		abs.truncate();
		abs.append_string(absfile);
		return abs.data();
	    }
	    const char *t = strrchr(absfile, '/');
	    assert(t != 0);
	    abs.append_chars(absfile, t-absfile);
	}
    }
    else
    {
	string_var curr = g_get_current_dir();
	abs.append_string(curr);
    }

    while ((part = tok.next()) != 0)
    {
	if (!strcmp(part, "."))
	{
	    continue;
	}
	if (!strcmp(part, ".."))
	{
	    const char *p = strrchr(abs.data(), '/');

	    if (p != abs.data())
		abs.truncate_to(p - abs.data());
	    continue;
	}
	if (abs.length() > 1)
	    abs.append_char('/');
	abs.append_string(part);
    }

    return abs.data();
}

const char *
file_make_absolute(const char *filename)
{
    return file_make_absolute_to(filename, 0, FALSE);
}

const char *
file_make_absolute_to_file(const char *filename, const char *absfile)
{
    return file_make_absolute_to(filename, absfile, FALSE);
}

const char *
file_make_absolute_to_dir(const char *filename, const char *absdir)
{
    return file_make_absolute_to(filename, absdir, TRUE);
}

/*
 * Normalise an absolute or relative path.  Remove "."
 * path components, collapse ".." path components where
 * possible.  Returns a new string.
 */
char *
file_normalise(const char *filename)
{
    estring up;
    estring down;
    tok_t tok(filename, "/");
    const char *part;
    gboolean is_abs = (*filename == '/');

    if (is_abs)
	up.append_char('/');

    while ((part = tok.next()) != 0)
    {
	if (!strcmp(part, "."))
	{
	    continue;
	}
	if (!strcmp(part, ".."))
	{
	    int i = down.find_last_char('/');

	    if (i >= 0)
		down.truncate_to(i);
	    else if (down.length())
		down.truncate_to(0);
	    else if (!is_abs)
		up.append_string("../");
	    continue;
	}
	if (down.length() > 1)
	    down.append_char('/');
	down.append_string(part);
    }

    if (!down.length() && !up.length())
	return g_strdup(".");

    up.append_string(down);
    return up.take();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
file_exists(const char *filename)
{
    struct stat sb;

    if (stat(filename, &sb) < 0)
	return (errno == ENOENT ? -1 : 0);
    return 0;
}

int
file_is_directory(const char *filename)
{
    struct stat sb;

    if (stat(filename, &sb) < 0)
	return -1;

    if (!S_ISDIR(sb.st_mode))
    {
	__set_errno(ENOTDIR);
	return -1;
    }

    return 0;
}

int
file_is_regular(const char *filename)
{
    struct stat sb;

    if (stat(filename, &sb) < 0)
	return -1;

    if (!S_ISREG(sb.st_mode))
    {
	__set_errno(EISDIR);
	return -1;
    }

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
file_build_tree(const char *dirname, mode_t mode)
{
    char *p, *dir = g_strdup(dirname);
    int ret = 0;
    char oldc;

    /* skip leading /s */
    for (p = dir ; *p && *p == '/' ; p++)
	;

    /* check and make each directory part in turn */
    for (;;)
    {
	/* skip p to next / */
	for ( ; *p && *p != '/' ; p++)
	    ;

	oldc = *p;
	*p = '\0';

	if (file_exists(dir) < 0)
	{
	    if (mkdir(dir, mode) < 0)
	    {
		ret = -1;
		break;
	    }
	}

	if (file_is_directory(dir) < 0)
	{
	    ret = -1;
	    break;
	}

	if (!oldc)
	    break;
	*p = '/';

	/* skip possible multiple / */
	for ( ; *p && *p == '/' ; p++)
	    ;
	if (!*p)
	    break;
    }

    g_free(dir);
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

mode_t
file_mode_from_string(const char *str, mode_t base, mode_t deflt)
{
    if (str == 0 || *str == '\0')
	return deflt;

    if (str[0] >= '0' && str[0] <= '7')
	return strtol(str, 0, 8);

    fprintf(stderr, "TODO: can't handle mode strings properly\n");
    return base;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
file_apply_children(
    const char *filename,
    file_apply_proc_t function,
    void *userdata)
{
    DIR *dir;
    struct dirent *de;
    estring child;
    int ret = 1;

    if ((dir = opendir(filename)) == 0)
	return -1;

    while ((de = readdir(dir)) != 0)
    {
	if (!strcmp(de->d_name, ".") ||
	    !strcmp(de->d_name, ".."))
	    continue;

	/* TODO: truncate_to() */
	child.truncate();
	child.append_string(filename);
	child.append_char('/');
	child.append_string(de->d_name);

	if (!(*function)(child.data(), userdata))
	{
	    ret = 0;
	    break;
	}
    }

    closedir(dir);
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
file_join2(const char *part1, const char *part2)
{
    return file_join(part1, part2, (const char *)0);
}

char *
file_join(const char *part, ...)
{
    estring path(part);
    va_list args;
    va_start(args, part);
    while ((part = va_arg(args, const char*)) != 0)
    {
	while (path.last() == '/')
	    path.truncate_to(path.length()-1);
	path.append_char('/');
	while (*part == '/')
	    part++;
	path.append_string(part);
    }
    va_end(args);
    return path.take();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
file_copy(const char *filefrom, const char *fileto)
{
    int fdfrom = -1;
    int fdto = -1;
    int nfrom;
    struct stat statfrom;
    int r = 0;
    char *tmp_fileto = 0;
    char buf[4096];

    tmp_fileto = g_strconcat(fileto, ".NEW", (const char*)0);

    if ((fdfrom = open(filefrom, O_RDONLY)) < 0)
    {
	perror(filefrom);
	goto error;
    }
    if (fstat(fdfrom, &statfrom) < 0)
    {
	perror(filefrom);
	goto error;
    }
    if ((fdto = open(tmp_fileto, O_WRONLY|O_CREAT|O_TRUNC, statfrom.st_mode)) < 0)
    {
	perror(tmp_fileto);
	goto error;
    }

    while ((nfrom = read(fdfrom, buf, sizeof(buf))) > 0)
    {
	int offto = 0;
	while (nfrom > 0)
	{
	    int nto = write(fdto, buf+offto, nfrom);
	    if (nto < 0)
	    {
		perror(tmp_fileto);
		goto error;
	    }
	    offto += nto;
	    nfrom -= nto;
	}
    }
    if (nfrom < 0)
    {
	perror(filefrom);
	goto error;
    }

    /* success */
    if (rename(tmp_fileto, fileto) < 0)
    {
	perror(fileto);
	goto error;
    }

out:
    if (fdfrom >= 0 && close(fdfrom) < 0)
    {
	perror(filefrom);
	r = -1;
    }
    if (fdto >= 0 && close(fdto) < 0)
    {
	perror(fileto);	/* write errors over NFS can be reported here */
	r = -1;
    }
    free(tmp_fileto);
    return r;
error:
    r = -1;
    goto out;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *file_temp_directory(const char *prefix)
{
    char *path;
    const char *tmpdir;

    tmpdir = getenv("TMPDIR");
    if (tmpdir == 0)
	tmpdir = "/tmp";

    path = g_strconcat(tmpdir, "/", prefix, "-XXXXXX", (char*)0);
    if (path == 0)
	return 0;

    if (mkdtemp(path) == 0)
    {
	/* failed to create directory, see errno */
	int e = errno;
	g_free(path);
	errno = e;
	return 0;
    }

    return path;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
