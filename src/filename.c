/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

CVSID("$Id: filename.c,v 1.7 2003-05-11 00:28:20 gnb Exp $");

#ifndef __set_errno
#define __set_errno(v)	 errno = (v)
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

char *
file_make_absolute(const char *filename)
{
    estring abs;
    tok_t tok(filename, "/");
    const char *part;
    
    if (*filename == '/')
    {
    	abs.append_string("/");
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
	    char *p = strrchr(abs.data(), '/');
	    
	    if (p != abs.data())
	    	abs.truncate_to(p - abs.data());
	    continue;
	}
	if (abs.length() > 1)
	    abs.append_char('/');
	abs.append_string(part);
    }
    
    return abs.take();
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
/*END*/
