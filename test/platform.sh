#!/bin/sh
#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2004 Greg Banks <gnb@users.sourceforge.net>
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
# $Id: platform.sh,v 1.4 2010-05-09 05:37:15 gnb Exp $
#

tolower1 ()
{
    tr '[:upper:]' '[:lower:]'
}

tolower2 ()
{
    tr 'A-Z' 'a-z'
}

X=`echo FOo | tolower1`
if [ "$X" = "foo" ]; then
    tolower=tolower1
else
    X=`echo FOo | tolower2`
    if [ "$X" = "foo" ]; then
	tolower=tolower2
    else
	echo "platform.sh: no functional tr program" 1>&2
	exit 1
    fi
fi

NAME=`uname -s | $tolower`
ARCH=`uname -m | $tolower`
REV=`uname -r | $tolower`
VENDOR=unknown

get_linux_distro ()
{
    if [ -f /etc/redhat-release ]; then
	# Format for RH 7,8.9:
	# Red Hat Linux release 9 (Shrike)
	# Format for Fedora Core 1:
	# Fedora Core release 1 (Yarrow)
	sed \
    	    -e 's|.*Red[ \t]*Hat[ \t]\+Linux|rh|' \
	    -e 's|.*Fedora[ \t]\+Core|fc|' \
	    -e 's|release[ \t]\+\([^ \t]\+\).*|\1|g' \
	    < /etc/redhat-release
    else
    	echo unknown unknown
    fi
}

case "$NAME" in
linux)
    set `get_linux_distro`
    VENDOR=$1
    REV=$2
    ;;
irix64) NAME=irix ;;
sunos)
    case "$REV" in
    5.*)
	NAME=solaris
	REV=`echo $REV | sed -e s'/^5\.//'`
	;;
    esac
    VENDOR=sun
    ;;
esac

case "$ARCH" in
i[3-9]86) ARCH=i386 ;;
IP[1-9]|IP[1-9][0-9]) ARCH=mips ;;
sun4[cmu]) ARCH=sparc ;;
esac

REV=`echo "$REV" | sed -e 's|-.*||' -e 's|\.||g'`

echo ${ARCH}-${VENDOR}-${NAME}-${REV}

