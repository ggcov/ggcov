#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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
# $Id: Makefile.am,v 1.24 2003-06-03 14:46:49 gnb Exp $
#

bin_PROGRAMS=	ggcov

# libiberty/demangle.h is from the GNU libiberty library,
# for some reason the RedHat distro doesn't install it.

ggcov_SOURCES=	libiberty/demangle.h demangle.h demangle.c \
		common.h common.c \
		estring.H estring.C string_var.H tok.H tok.C \
		filename.h filename.c \
		list.H \
		hashtable.H hashtable.C \
		covio.h covio.c \
		cov_file.H cov_file.C \
		cov_function.H cov_function.C \
		cov_block.H cov_block.C \
		cov_arc.H cov_arc.C \
		cov_callgraph.H cov_callgraph.C \
		cov_scope.H cov_scope.C \
		cov_types.H cov.H cov.C \
		uicommon.h ui.h ui.c uix.h uix.c \
		gnbprogressbar.h gnbprogressbar.c \
		confsection.H confsection.C \
		prefs.H prefs.C \
		window.H window.C \
		sourcewin.H sourcewin.C \
		summarywin.H summarywin.C \
		callswin.H callswin.C \
		callgraphwin.H callgraphwin.C \
		callgraph2win.H callgraph2win.C \
		functionswin.H functionswin.C \
		fileswin.H fileswin.C \
		prefswin.H prefswin.C \
		help.c ggcov.c
INCLUDES=	$(GGCOV_CFLAGS) -DPKGDATADIR="\"$(pkgdatadir)\"" \
 		-DDEBUG=$(DEBUG) -DUI_DEBUG=$(UI_DEBUG)
ggcov_LDADD=	$(GGCOV_LIBS)


# Note this is an automake conditional *not* a gmake conditional
if DEBUG
DEBUG=1
else
DEBUG=0
endif

UI_DEBUG=0

# licence.c contains the GPL text in C syntax. This
# avoids having to install the COPYING file just to
# provide the Licence window.
help.o: licence.c
licence.c: $(top_srcdir)/COPYING
	sed \
	    -e 's///g' \
	    -e 's/\\/\\\\/g' \
	    -e 's/"/\\"/g' \
	    -e 's/^\(.*\)$$/"\1\\n"/' \
	    < $(top_srcdir)/COPYING > $@

EXTRA_CLEAN=	licence.c

noinst_PROGRAMS=	bbdump filetest mangletest

bbdump_SOURCES= bbdump.c covio.c estring.C
bbdump_LDADD=	$(GGCOV_LIBS)

filetest_SOURCES= filetest.c filename.c estring.C tok.C
filetest_LDADD=	$(GGCOV_LIBS)

mangletest_SOURCES= mangletest.c demangle.c estring.C
mangletest_LDADD=	$(GGCOV_LIBS)

