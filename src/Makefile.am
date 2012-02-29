#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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
# $Id: Makefile.am,v 1.74 2010-05-09 05:37:14 gnb Exp $
#

SUBDIRS=	libiberty libggcov

noinst_LIBRARIES=	libcommon.a libcov.a

libcommon_a_SOURCES= \
		demangle.h demangle.c \
		common.h common.c \
		fakepopt.h fakepopt.c \
		estring.H estring.C string_var.H tok.H tok.C \
		cached_string.H cached_string.C \
		filename.h filename.c \
		list.H \
		hashtable.H hashtable.C \
		ptrarray.H \
		mvc.h mvc.c \
		cpp_parser.H cpp_parser.C \
		scenegen.H scenegen.C \
		geometry.H

libcov_a_SOURCES= \
		covio.H covio.C \
		cov_specific.H cov_specific.C \
		cov_bfd.H cov_bfd.C \
		cov_file.H cov_file.C \
		cov_suppression.H cov_suppression.C \
		cov_line.H cov_line.C \
		cov_function.H cov_function.C \
		cov_block.H cov_block.C \
		cov_calliter.H cov_calliter.C \
		cov_arc.H cov_arc.C \
		cov_callgraph.H cov_callgraph.C \
		cov_scope.H cov_scope.C \
		cov_types.H cov.H cov.C \
		report.H report.C \
		diagram.H diagram.C colors.h \
		lego_diagram.H lego_diagram.C \
		callgraph_diagram.H callgraph_diagram.C \
		flow_diagram.H flow_diagram.C
# This is a sad hack to work around the problem where some
# objects need to be unconditionally linked instead of pulled
# in from libcov.a on demand.
LIBCOV_STATIC_SOURCES= \
		cov_stab32.C cov_i386.C cov_dwarf2.C cov_elf.C
		
if GUI
BASEINCLUDES=	$(GGCOV_GUI_CFLAGS)
else
BASEINCLUDES=	$(GGCOV_CLI_CFLAGS)
endif

bin_PROGRAMS=	$(GUIPROG) tggcov $(WEBPROG)
EXTRA_PROGRAMS= ggcov ggcov-webdb

gui_SOURCES=	uicommon.h ui.h ui.c uix.h uix.c \
		gnbstackedbar.h gnbstackedbar.c \
		confsection.H confsection.C \
		prefs.H prefs.C \
		window.H window.C \
		sourcewin.H sourcewin.C \
		summarywin.H summarywin.C \
		callswin.H callswin.C \
		callgraphwin.H callgraphwin.C \
		functionswin.H functionswin.C \
		reportwin.H reportwin.C \
		fileswin.H fileswin.C \
		prefswin.H prefswin.C \
		canvas_scenegen.H canvas_scenegen.C \
		canvas_function_popup.H canvas_function_popup.C \
		diagwin.H diagwin.C \
		utils.H utils.C \
		help.c ggcov.c
ggcov_SOURCES=	$(gui_SOURCES) $(LIBCOV_STATIC_SOURCES) glade_callbacks.c

INCLUDES=	$(BASEINCLUDES) -DPKGDATADIR="\"$(pkgdatadir)\"" \
		-DPREFIX="\"$(prefix)\"" -DDATADIR="\"$(datadir)\"" \
		-DLIBDIR="\"$(libdir)\"" -DSYSCONFDIR="\"$(sysconfdir)\"" \
 		-DDEBUG=$(DEBUG) -DUI_DEBUG=$(UI_DEBUG) \
		$(CPU_DEFINES) $(OS_DEFINES)
ggcov_LDADD=	libcov.a libcommon.a $(GGCOV_GUI_LIBS) $(BFD_LIBS)
CLI_LIBS=	libcov.a libcommon.a $(GGCOV_CLI_LIBS) $(BFD_LIBS)

tggcov_SOURCES= tggcov.c \
		check_scenegen.H check_scenegen.C \
		$(LIBCOV_STATIC_SOURCES)
tggcov_LDADD=	$(CLI_LIBS)

ggcov_webdb_SOURCES= \
		ggcov-webdb.c \
		php_serializer.H php_serializer.C \
		php_scenegen.H php_scenegen.C \
		$(LIBCOV_STATIC_SOURCES)
ggcov_webdb_CPPFLAGS=	$(DB_CPPFLAGS)
ggcov_webdb_LDADD=	$(CLI_LIBS) $(DB_LIBS)

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

# glade_callbacks.c is automatically generated to provide mappings
# of names to function pointers for functions marked with GLADE_CALLBACK,
# which are used as either signal handlers or custom widget create
# functions in the glade file.  This means we don't need to use the
# --export-dynamic linker flag anymore.
glade_callbacks.c: $(gui_SOURCES)
	perl gencallbacks.pl $(gui_SOURCES) > $@ || ($(RM) $@ ; exit 1)

EXTRA_DIST=		gencallbacks.pl

EXTRA_CLEAN=		licence.c glade_callbacks.c

noinst_PROGRAMS=	testrunner mangletest \
			popttest

testrunner_SOURCES=	testrunner.c testfw.c testfw.h \
			filetest.c \
			phptest.C php_serializer.C \
			estringtest.c \
			toktest.c \
			hashtabletest.c \
			listtest.c
testrunner_LDADD= 	$(CLI_LIBS)

mangletest_SOURCES=	mangletest.c
mangletest_LDADD=	$(CLI_LIBS)

popttest_SOURCES=	popttest.c
popttest_LDADD= 	$(GGCOV_CLI_LIBS)

# On Ubuntu, check that none of the installable programs
# are dynamically linked against libbfd.  See the comment
# in configure.in for why that would be bad.
install-exec-local:
	if grep Ubuntu /etc/issue > /dev/null 2>&1 ; then \
	    ok=yes ;\
	    for prog in $(bin_PROGRAMS) ; do \
		if ldd $$prog | grep libbfd >/dev/null ; then \
		    echo "error: $$prog is dynamically linked against libbfd" 1>&2 ;\
		    ok=no ;\
		fi ;\
	    done ;\
	    [ "$$ok" = yes ] || exit 1 ;\
	fi

