/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2020 Greg Banks <gnb@fastmail.fm>
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

#ifndef _ggcov_uicommon_h_
#define _ggcov_uicommon_h_ 1

#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(1,99,0)
#define GTK2 1
#else
#define GTK2 0
#endif

#if GTK_CHECK_VERSION(2,99,0)
#define GTK3 1
#else
#define GTK3 0
#endif

#ifdef __cplusplus
#define GLADE_CALLBACK extern "C"
#else
#define GLADE_CALLBACK
#endif

#endif /* _ggcov_uicommon_h_ */
