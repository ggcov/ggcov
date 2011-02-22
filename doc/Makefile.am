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
# $Id: Makefile.am,v 1.7 2010-05-09 05:37:14 gnb Exp $
#

if GUI
GUI_MANS=	ggcov.1
endif
if WEB
WEB_MANS=	ggcov-webdb.1
endif

man_MANS=	$(GUI_MANS) $(WEB_MANS) \
		git-history-coverage.1
EXTRA_DIST=	bbg-file-format.txt dbus-decode-gcov.txt \
		dwarf-file-format.txt HOWTO.web ggcov.1 \
		ggcov-webdb.1
