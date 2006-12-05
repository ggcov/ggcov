#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2005 Greg Banks <gnb@alphalink.com.au>
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
# $Id: Makefile.am,v 1.12 2006-12-04 13:02:24 gnb Exp $
#

include $(srcdir)/../phpdirs.am

LIB_PHP=		basic.php 

PAGE_PHP=		basic.css ../../ui/ggcov64.gif \
			tests.php summary.php covbar.php files.php \
			reports.php functions.php calls.php \
			callbutterfly.php source.php diagram.php \
			drender.php dviewport.php \
			pan-ul.gif pan-u.gif pan-ur.gif \
			pan-l.gif pan-c.gif pan-r.gif \
			pan-dl.gif pan-d.gif pan-dr.gif \
			zoom-in.gif zoom-out.gif show-all.gif \
			favicon.ico valid-html40.gif

if WEB
myincludedir=		$(phpincludedir)/ggcov/basic
myinclude_DATA=		$(LIB_PHP)
phpwww_DATA=		$(PAGE_PHP)
endif

EXTRA_DIST=		$(LIB_PHP) $(PAGE_PHP)
