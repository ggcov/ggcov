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
# $Id: Makefile.am,v 1.40 2004-11-20 09:13:38 gnb Exp $
#

SUBDIRS=	libiberty

noinst_LIBRARIES=	libcommon.a libcov.a

libcommon_a_SOURCES= \
		demangle.h demangle.c \
		common.h common.c \
		fakepopt.h fakepopt.c \
		estring.H estring.C string_var.H tok.H tok.C \
		filename.h filename.c \
		list.H \
		hashtable.H hashtable.C \
		ptrarray.H \
		mvc.h mvc.c \
		cpp_parser.H cpp_parser.C

libcov_a_SOURCES= \
		covio.H covio.C \
		cov_specific.H cov_specific.C \
		cov_bfd.H cov_bfd.C \
		cov_file.H cov_file.C \
		cov_line.H cov_line.C \
		cov_function.H cov_function.C \
		cov_block.H cov_block.C \
		cov_arc.H cov_arc.C \
		cov_callgraph.H cov_callgraph.C \
		cov_scope.H cov_scope.C \
		cov_types.H cov.H cov.C \
		report.H report.C
# This is a sad hack to work around the problem where some
# objects need to be unconditionally linked instead of pulled
# in from libcov.a on demand.
LIBCOV_STATIC_SOURCES= \
		cov_stab32.C cov_i386.C cov_dwarf2.C
		
if GUI
GUIPROG=	ggcov
BASEINCLUDES=	$(GGCOV_GUI_CFLAGS)
else
GUIPROG=	
BASEINCLUDES=	$(GGCOV_CLI_CFLAGS)
endif

bin_PROGRAMS=	$(GUIPROG) tggcov

ggcov_SOURCES=	uicommon.h ui.h ui.c uix.h uix.c \
		gnbstackedbar.h gnbstackedbar.c \
		confsection.H confsection.C \
		prefs.H prefs.C \
		window.H window.C \
		sourcewin.H sourcewin.C \
		summarywin.H summarywin.C \
		callswin.H callswin.C \
		callgraphwin.H callgraphwin.C \
		callgraph2win.H callgraph2win.C \
		functionswin.H functionswin.C \
		reportwin.H reportwin.C \
		fileswin.H fileswin.C \
		prefswin.H prefswin.C \
		$(LIBCOV_STATIC_SOURCES) \
		help.c ggcov.c

INCLUDES=	$(BASEINCLUDES) -DPKGDATADIR="\"$(pkgdatadir)\"" \
		-DPREFIX="\"$(prefix)\"" -DDATADIR="\"$(datadir)\"" \
		-DLIBDIR="\"$(libdir)\"" -DSYSCONFDIR="\"$(sysconfdir)\"" \
 		-DDEBUG=$(DEBUG) -DUI_DEBUG=$(UI_DEBUG)
ggcov_LDADD=	libcov.a libcommon.a $(GGCOV_GUI_LIBS)
CLI_LIBS=	libcov.a libcommon.a $(GGCOV_CLI_LIBS)

tggcov_SOURCES= tggcov.c $(LIBCOV_STATIC_SOURCES)
tggcov_LDADD=	$(CLI_LIBS)

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

EXTRA_CLEAN=		licence.c

noinst_PROGRAMS=	bbdump filetest mangletest \
			newbbgdump

bbdump_SOURCES= 	bbdump.c
bbdump_LDADD=		$(CLI_LIBS)

filetest_SOURCES=	filetest.c
filetest_LDADD= 	$(CLI_LIBS)

mangletest_SOURCES=	mangletest.c
mangletest_LDADD=	$(CLI_LIBS)

newbbgdump_SOURCES=	newbbgdump.c
newbbgdump_LDADD=	$(CLI_LIBS)
