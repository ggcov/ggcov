/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005-2020 Greg Banks <gnb@fastmail.fm>
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

#ifndef _ggcov_colors_H_
#define _ggcov_colors_H_ 1

#include "common.h"

/*
 * Default colours as RGB tuples.  These are the only colours
 * used for the web database and printing from tggcov, but just
 * the default colours for ggcov.
 */

#define COLOR_FG_COVERED        0x00, 0xc0, 0x00
#define COLOR_BG_COVERED        0x80, 0xd0, 0x80
#define COLOR_FG_PARTCOVERED    0xa0, 0xa0, 0x00
#define COLOR_BG_PARTCOVERED    0xd0, 0xd0, 0x80
#define COLOR_FG_UNCOVERED      0xc0, 0x00, 0x00
#define COLOR_BG_UNCOVERED      0xd0, 0x80, 0x80
#define COLOR_FG_UNINSTRUMENTED 0x00, 0x00, 0x00
#define COLOR_BG_UNINSTRUMENTED 0xa0, 0xa0, 0xa0
#define COLOR_FG_SUPPRESSED     0x00, 0x00, 0x80
#define COLOR_BG_SUPPRESSED     0x80, 0x80, 0xd0

#endif /* _ggcov_colors_H_ */
