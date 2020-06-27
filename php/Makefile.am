#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2005-2020 Greg Banks <gnb@fastmail.fm>
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

include $(srcdir)/phpdirs.am

SUBDIRS=    lib basic

LOCALTESTDIR=	/tmp/ggcov
localtest:
	$(MAKE) DESTDIR=$(LOCALTESTDIR) install

EXTRA_DIST= ggcov.conf.in
noinst_DATA= ggcov.conf
ggcov.conf: ggcov.conf.in
	sed \
	    -e 's|@phpincludedir\@|$(phpincludedir)|g' \
	    -e 's|@phpwwwdir\@|$(phpwwwdir)|g' \
	    -e 's|@ggcovtestdir\@|$(ggcovtestdir)|g' \
	    < $< > $@

if WEB
install-data-local:
	-mkdir -p $(DESTDIR)$(ggcovtestdir)
endif
