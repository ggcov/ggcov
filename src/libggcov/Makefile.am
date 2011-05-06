#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2001-2011 Greg Banks <gnb@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

pkglib_LTLIBRARIES= libggcov.la

libggcov_la_SOURCES=	intercept.c
libggcov_la_LIBADD=	-ldl

bin_SCRIPTS = ggcov-run

# Can't do this at configure time, as @libdir@ is defined in
# terms of ${exec_prefix} which we don't have.  We need a full
# expansion, and the easiest way to do that is at make time.
ggcov-run:  ggcov-run.in
	sed -e 's|@pkglibdir@|$(pkglibdir)|g' <$< >$@

CLEANFILES=	ggcov-run
EXTRA_DIST=	ggcov-run.in
