#!/bin/sh

NAME=`uname -s | tr 'A-Z' 'a-z'`
ARCH=`uname -m | tr 'A-Z' 'a-z'`
REV=`uname -r | tr 'A-Z' 'a-z'`
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
esac

case "$ARCH" in
i[3-9]86) ARCH=i386 ;;
IP[1-9]|IP[1-9][0-9]) ARCH=mips ;;
esac

REV=`echo "$REV" | sed -e 's|-.*||' -e 's|\.||g'`

echo ${ARCH}-${VENDOR}-${REV}-${NAME}

