/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
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

#ifndef _cant_filename_h_
#define _cant_filename_h_ 1

#include "common.h"

typedef gboolean (*file_apply_proc_t)(const char *filename, void *userdata);

const char *file_basename_c(const char *filename);
const char *file_extension_c(const char *filename);
char *file_change_extension(const char *filename, const char *oldext,
    	    	    	    const char *newext);
char *file_dirname(const char *filename);
int file_mode(const char *filename);
FILE *file_open_mode(const char *filename, const char *rw, mode_t mode);
char *file_make_absolute(const char *filename);
int file_exists(const char *filename);
int file_is_regular(const char *filename);
int file_build_tree(const char *dirname, mode_t mode);	/* make sequence of directories */
mode_t file_mode_from_string(const char *str, mode_t base, mode_t deflt);
int file_apply_children(const char *filename, file_apply_proc_t, void *userdata);
int file_is_directory(const char *filename);

#endif /* _cant_filename_h_ */
