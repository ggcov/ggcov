#!/bin/sh
#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2003 Greg Banks <gnb@alphalink.com.au>
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
# $Id: summarise-ifdefs.sh,v 1.3 2005-03-14 07:49:15 gnb Exp $
#
# Shell script to descend over a source tree and extract a summary
# of all the conditionally compiled code, organised by conditonal symbol.
# Remember, the goal is to *minimise* ifdefs globally.
#

FILES=
LOCALFLAG=no
GUARDFLAG=no

usage ()
{
    echo "Usage: $0 [-l] [file|dir...]"
    exit 1
}

while [ $# -gt 0 ]; do
    case "$1" in
    -l|--local) LOCALFLAG=yes ;;
    -g|--guards) GUARDFLAG=yes ;;
    -*) usage ;;
    *) FILES="$FILES $1" ;;
    esac
    shift
done

# echo "LOCALFLAG=$LOCALFLAG"
# echo "FILES=$FILES"
# exit 1

MAXDEPTH=
[ $LOCALFLAG = yes ] && MAXDEPTH="-maxdepth 1"

DOGUARD=0
[ $GUARDFLAG = yes ] && DOGUARD=1


#set -x
find $FILES $MAXDEPTH -type f -name '*.[chCH]' | while read file ; do
    echo "=$file"
    egrep '^[ \t]*#[ \t]*(if|ifdef|ifndef)[ \t]' $file |\
    	sed -e 's/^[ \t]*#[ \t]*[a-z]\+//g' \
	    -e 's|/\*.*\*/||g' \
	    -e 's/defined[ \t]*(\([^)]\+\))/\1/g' \
	    -e 's/\<[0-9]\+\>//g' \
	    -e 's/||//g' \
	    -e 's/&&//g' \
	    -e 's/>=//g' \
	    -e 's/>//g' \
	    -e 's/<=//g' \
	    -e 's/<//g' \
	    -e 's/==//g' \
	    -e 's/!=//g' \
	    -e 's/(//g' \
	    -e 's/)//g' \
	    -e 's/,//g' \
	    -e 's/!//g'
done | awk -v doguard=$DOGUARD '
/^=/ {
    fname = substr($1,2,length($1)-1);

    if (match(fname, "^.*/"))
    	gname = substr(fname, RLENGTH+1, length(fname)-RSTART-1);
    else
    	gname = fname;
    gsub("[^a-zA-Z]+", "_", gname);
#    printf "fname=\"%s\" gname=\"%s\"\n", fname, gname;

    next;
}
function is_guard(s) {
    if (match(s, "^[a-z_]*_*"gname"_*$")) return 1;
    if (match(s, "^[A-Z_]*_*"toupper(gname)"_*$")) return 1;
    return 0;
}
function add_ref(s) {
#    printf "add_ref(\"%s\")\n", s;
    if (!doguard && is_guard(s)) return;
    syms[s]++;
    refs[fname,s]++;
}
{
    for (i = 1 ; i <= NF ; i++) add_ref($i);
}
END {
    for (s in syms) {
    	printf "\n%s\n", s;
	cmd = "sort -nr +0 -1"
    	for (k in refs) {
	    split(k,a,SUBSEP);
#	    printf "k=\"%s\" a[1]=\"%s\" a[2]=\"%s\"\n", k, a[1], a[2];
	    if (a[2] != s) continue;
	    printf "%-5d   %s\n", refs[k], a[1] | cmd;
	}
	close(cmd);
    }
}
'

    
#     |cut -d: -f1|sort|uniq -c|sort -nr +0

