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
# $Id: Makefile.am,v 1.8 2003-07-16 15:15:45 gnb Exp $
#

desktopdir=	$(datadir)/gnome/apps/Development

GLADEFILES=	ggcov.glade ggcov-glade2.glade
BUILD_PIXMAPS=	icon.xpm
PIXMAPS=	logo.xpm

EXTRA_DIST=	$(GLADEFILES) $(PIXMAPS) $(BUILD_PIXMAPS) ggcov.desktop.in
pkgdata_DATA=	@GGCOV_GLADE_FILE@ $(PIXMAPS)
desktop_DATA=	ggcov.desktop

# Expansion of datadir cannot be performed at configure time because we need
# a full explicit pathname rather than a make/shell variable expression.
ggcov.desktop: ggcov.desktop.in
	sed -e 's|[@]datadir[@]|$(datadir)|g' \
		< ggcov.desktop.in > $@
