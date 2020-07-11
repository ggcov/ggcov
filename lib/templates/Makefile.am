#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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

PAGE_TEMPLATES=	index.html source.html tree.html functions.html
STATIC_FILES=	ggcov.css ncXL7RzcB_120x120.png
FRAGMENTS=	foot.html head.html

EXTRA_DIST=	$(PAGE_TEMPLATES:.html=.html.in) $(STATIC_FILES) $(FRAGMENTS)

templatesdir=	$(pkgdatadir)/templates
templates_DATA=	$(PAGE_TEMPLATES) $(STATIC_FILES) static.files

$(PAGE_TEMPLATES): %: %.in head.html foot.html
	cat head.html $< foot.html > $@

static.files:
	( \
	    echo "# List of static files" ;\
	    for f in $(STATIC_FILES) ; do \
		echo "$$f" ;\
	    done ;\
	) > $@

CLEANFILES=	$(PAGE_TEMPLATES)
