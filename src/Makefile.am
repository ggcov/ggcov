#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
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
# $Id: Makefile.am,v 1.7 2001-11-30 01:05:26 gnb Exp $
#

bin_PROGRAMS=	ggcov

ggcov_SOURCES=	common.h common.c \
		estring.h estring.c \
		filename.h filename.c \
		covio.h covio.c \
		cov.h cov.c \
		ui.h ui.c uix.h uix.c \
		gnbprogressbar.h gnbprogressbar.c \
		sourcewin.h sourcewin.c \
		summarywin.h summarywin.c \
		functionswin.h functionswin.c \
		fileswin.h fileswin.c \
		ggcov.c
INCLUDES=	$(LIBGLADE_CFLAGS)
ggcov_LDADD=	$(LIBGLADE_LIBS)
CPPFLAGS=	-DPKGDATADIR="\"$(pkgdatadir)\"" \
		-DDEBUG=$(DEBUG) -DUI_DEBUG=$(UI_DEBUG)

# Note this is an automake conditional *not* a gmake conditional
if DEBUG
DEBUG=1
else
DEBUG=0
endif

UI_DEBUG=0
