#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2005 Greg Banks <gnb@users.sourceforge.net>
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
# $Id: Makefile.am,v 1.12 2010-05-09 05:37:15 gnb Exp $
#

EXTRA_DIST=	common.sh platform.sh runtest list-manifest.sh \
		filter-callgraph.pl filter-coverage.pl \
		filter-expected.pl filter-status.pl

TESTS=		   runtest
TESTS_ENVIRONMENT= sh

dist-hook:
		@for file in `sh list-manifest.sh` ; do \
			mkdir -p `dirname $(distdir)/$$file` ;\
			cp $(srcdir)/$$file $(distdir)/$$file ;\
		done
