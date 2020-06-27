/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>>
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

#ifndef _ggcov_unicode_H_
#define _ggcov_unicode_H_ 1

#include <ostream>

/*
 * Emit to stream `o` an escaped version of the UTF-8 encoded
 * string `s`.  The output string is surrounded by double-quotes
 * and will have the following escaped in a way suitable for
 * use in compliant JSON or YAML parsers: \, ", \n, \r, \t, other
 * control characters, and non-ASCII unicode characters.
 */
extern void escape_utf8_string(const char *s, std::ostream &o);

#endif /* _ggcov_unicode_H_ */
