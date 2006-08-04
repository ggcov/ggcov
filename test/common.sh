#!/bin/bash
#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2004-2005 Greg Banks <gnb@alphalink.com.au>
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
# $Id: common.sh,v 1.19 2006-08-04 13:00:42 gnb Exp $
#
# Common shell functions for all the test directories
#

DBFLAG=
[ x"$ENABLE_DB_FLAG" = x1 ] && DBFLAG="-db"

CC="gcc"
CWARNFLAGS="-Wall"
CCOVFLAGS="-g $DBFLAG -fprofile-arcs -ftest-coverage"
CDEFINES=

CXX="c++"
CXXWARNFLAGS="-Wall"
CXXCOVFLAGS="-g $DBFLAG -fprofile-arcs -ftest-coverage"
CXXDEFINES=

LDLIBS=
CXXLINK=no

TGGCOV_ANNOTATE_FORMAT=auto
TGGCOV_FLAGS=
SUBTEST=
srcdir=`/bin/pwd`
if [ -n "$RPLATFORM" ]; then
    PLATFORM="$RPLATFORM"
    CANNED=yes
else
    PLATFORM=`sh ../platform.sh`
    CANNED=no
fi

TMP1=/tmp/ggcov-test-$$a
TMP2=/tmp/ggcov-test-$$b
/bin/rm -f $TMP1 $TMP2
trap "/bin/rm -f $TMP1 $TMP2" 0 1 11 13 15

fatal ()
{
    echo "$0: FATAL: $*"
    exit 1
}

vdo ()
{
    echo "+ $@"
    eval "$@"
}

vncdo ()
{
    if [ $CANNED = yes ]; then
	echo "[skipping] $@"
    else
	echo "+ $@"
	eval "$@"
    fi
}

vcapdo ()
{
    local OUTFILE=$1
    shift

    echo "+ $@"
    eval "$@" > $OUTFILE 2>&1
}

vcmd ()
{
    echo "==$*"
}

CLEAN_FILES="*.o *.S *.exe *.bb *.bbg *.gcno *.da *.gcda *.bp *.out *.gcov *.tggcov *.filt"

init ()
{
    if [ $CANNED = yes ]; then
	vcmd "init[canned] $*"
    else
	vcmd "init $*"
	for f in $CLEAN_FILES ; do
	    [ -e $f ] && vdo /bin/rm -fr $f
	done
	vdo ls -AFC
    fi
    _DSTACK=`/bin/pwd`
    _ODSTACK=`/bin/pwd`
}

subdir_push ()
{
    local subdir="$1"

    if [ $CANNED = yes ]; then
	vcmd "subdir_push[canned] $subdir"
	[ -d "$subdir" ] || fatal "$subdir: No such directory"
    else
	vcmd "subdir_push $subdir"
	if [ ! -d "$subdir" ] ; then
	    vdo mkdir "$subdir"
	fi
    fi
    vdo cd "$subdir"
    _DSTACK=`/bin/pwd`":$_DSTACK"
}

subdir_pop ()
{
    local dir
    
    vcmd "subdir_pop"
    _DSTACK=`echo "$_DSTACK" | sed -e 's|[^:]*:||'`
    dir=`echo "$_DSTACK" | sed -e 's|:*||'`
    vdo cd $dir || fatal "$dir: No such directory"
}

compile_c ()
{
    local picflag=
    local cfile=
    
    while [ $# -gt 0 ] ; do
    	case "$1" in
	-pic) picflag="-fpic" ;;
	-*)
	    echo "compile_c: unknown flag \"$1\""
	    exit 1
	    ;;
	*)
	    if [ ! -z "$cfile" ]; then
    		echo "compile_c: too many cfiles given"
		exit 1
	    fi
	    cfile="$1"
	    ;;
	esac
	shift
    done
    
    if [ -z "$cfile" ]; then
    	echo "compile_c: no cfile given"
	exit 1
    fi

    vcmd "compile_c $cfile"
    vncdo $CC $CWARNFLAGS $CCOVFLAGS $CDEFINES $picflag -c $srcdir/$cfile || fatal "can't compile $srcdir/$cfile"
}

compile_cxx ()
{
    local picflag=
    local cfile=
    
    while [ $# -gt 0 ] ; do
    	case "$1" in
	-pic) picflag="-fpic" ;;
	-*)
	    echo "compile_cxx: unknown flag \"$1\""
	    exit 1
	    ;;
	*)
	    if [ ! -z "$cfile" ]; then
    		echo "compile_cxx: too many cfiles given"
		exit 1
	    fi
	    cfile="$1"
	    ;;
	esac
	shift
    done
    
    if [ -z "$cfile" ]; then
    	echo "compile_cxx: no cfile given"
	exit 1
    fi

    vcmd "compile_cxx $cfile"
    CXXLINK=yes
    vncdo $CXX $CXXWARNFLAGS $CXXCOVFLAGS $CXXDEFINES $picflag -c $srcdir/$cfile || fatal "can't compile $srcdir/$cfile"
}

link ()
{
    vcmd "link $*"
    local AOUT="$1.exe"
    shift

    case "$CXXLINK" in
    yes)
	vncdo $CXX $CXXCOVFLAGS -o "$AOUT" "$@" $LDLIBS || fatal "can't link $AOUT"
    	;;
    no)
	vncdo $CC $CCOVFLAGS -o "$AOUT" "$@" $LDLIBS || fatal "can't link $AOUT"
    	;;
    esac
}

_dso_filename ()
{
    echo "lib`basename $1`.so.1"
}

_dso_filename2 ()
{
    echo "lib`basename $1`.so"
}

link_shlib ()
{
    vcmd "link_shlib $*"
    local DSO=`_dso_filename "$1"`
    local DSO2=`_dso_filename2 "$1"`
    shift
    
    case "$CXXLINK" in
    yes)
	vncdo $CXX $CXXCOVFLAGS -shared -o "$DSO" "$@" || fatal "can't link $DSO"
    	;;
    no)
	vncdo $CC $CCOVFLAGS -shared -o "$DSO" "$@" || fatal "can't link $DSO"
    	;;
    esac
    ln -s "$DSO" "$DSO2"
}

add_shlib ()
{
    local absflag=no
    local dso
    local dir
    local file
    
    vcmd "add_shlib $*"
    while [ $# -gt 0 ] ; do
    	case "$1" in
	-abs) absflag=yes ;;
	-*)
	    echo "add_shlib: unknown flag \"$1\""
	    exit 1
	    ;;
	*)
	    if [ ! -z "$dso" ]; then
    		echo "add_shlib: too many dsos given"
		exit 1
	    fi
	    dso="`/bin/pwd`/$1"
	    ;;
	esac
	shift
    done
    
    if [ -z "$dso" ]; then
    	echo "add_shlib: no dso given"
	exit 1
    fi

    dir=`dirname $dso`
    file=`_dso_filename $dso`
    dso=`basename $dso`

    if [ $absflag = yes ] ; then
        LDLIBS="$LDLIBS -Wl,-rpath,$dir $dir/$file"
    else
        LDLIBS="$LDLIBS -Wl,-rpath,$dir -L$dir -l$dso"
    fi
}

run ()
{
    vcmd "run $*"
    local AOUT="./$1.exe"
    shift
    vncdo $AOUT "$@" || fatal "Can't run generated test program"
}

subtest ()
{
    vcmd "subtest $*"
    SUBTEST=".$1"
}

_gcov_file ()
{
    echo $1$SUBTEST.gcov
}

_tggcov_file ()
{
    echo $1$SUBTEST.tggcov
}

_subtest_file ()
{
    local base=${1%.*}
    local ext=${1##*.}
    echo $base$SUBTEST.$ext
}

_subtestize_files ()
{
    if [ -n "$SUBTEST" ] ; then
	for f in $* ; do
	    [ -f $f ] && vdo mv $f $(_subtest_file $f)
	done
    fi
}
 
run_gcov ()
{
    vcmd "run_gcov $*"
    local SRC="$1"
    if [ $CANNED = yes ]; then
	echo "[skipping] gcov -b -f $SRC"
	return
    fi

    if vcapdo $TMP1 gcov -b $SRC ; then
	cat $TMP1
	GCOV_FILES=$(sed -n \
	    -e 's|^Creating[ \t][ \t]*\([^ \t]*\.gcov\)\.$|\1|p' \
	    -e 's|^.*:creating[ \t][ \t]*.\([^ \t]*\.gcov\).$|\1|p' \
	    < $TMP1)
	[ -z "$GCOV_FILES" ] && fatal "no output files from gcov"
	_subtestize_files $GCOV_FILES
    else
	cat $TMP1
	fatal "gcov failed"
    fi
}

tggcov_annotate_format ()
{
    vcmd "tggcov_annotate_format $*"
    
    TGGCOV_ANNOTATE_FORMAT=$1
}

_tggcov_Nflag ()
{
    local SRC="$1"
    
    case "$TGGCOV_ANNOTATE_FORMAT" in
    new)
    	echo "-N"
	;;
    old)
    	;;
    auto)
        egrep '^[ \t]+(-|[0-9]+):[ \t]+[01]:' `_gcov_file $SRC` >/dev/null && echo "-N"
    	;;
    *)
    	fatal "unknown annotate format \"$TGGCOV_ANNOTATE_FORMAT\""
	;;
    esac
}


tggcov_flags ()
{
    vcmd "tggcov_flags $*"
    
    TGGCOV_FLAGS="$*"
}

run_tggcov ()
{
    vcmd "run_tggcov $*"
    local SRC="$1"
    local NFLAG=`_tggcov_Nflag $SRC`
    local pwd=$(/bin/pwd)

    if vcapdo $TMP1 ../../src/tggcov -a $NFLAG $TGGCOV_FLAGS $SRC ; then
	cat $TMP1
	TGGCOV_FILES=$(sed -n -e 's:^Writing[ \t][ \t]*'$pwd'/\([^ \t]*\.tggcov\)$:\1:p' < $TMP1)
	[ -z "$TGGCOV_FILES" ] && fatal "no output files from tggcov"
	_subtestize_files $TGGCOV_FILES
    else
	cat $TMP1
	fatal "tggcov failed"
    fi
}


_filter_spurious_counts ()
{
    local FILE="$1"

perl -e '
use strict;
my $in_decls = 0;
while (<STDIN>)
{
    next if m/^(call|branch|function)/;
    next if m/^        -:    0:/;
    my ($count, $lineno, $text) = m/^( *[#0-9-]*):( *[0-9]*):(.*)$/;
    my $zap = 0;
    if ($text =~ m/^\s*}/)
    {
	# ignore lonely closing brace
	$zap = 1;
    }
    elsif ($text =~ m/^{/)
    {
	# ignore lonely opening brace for a function
	$in_decls = 1;
	$zap = 1;
    }
    elsif ($text =~ m/^\s+{/)
    {
	# ignore all other lonely opening brace
	$zap = 1;
    }
    elsif ($text =~ m/^\s*for\s*\(.*;.*;.*\)\s*$/)
#    elsif ($text =~ m/^\s*for/)
    {
    	# entire for loop on a line...the gcov algorithm
	# changed historically and ggcov uses the old crappy
	# algorithm, so just ignore these lines
	$zap = 1;
    }
    elsif ($text =~ m/^\s*catch\s*\(.*\)\s*$/)
    {
	# catch statement; gcc 4.1 + gcov seem to account catch
	# statements differently than tggcov.  Our test code is
	# organised so that the catch block code is on sepearate
	# lines so the count for the catch statement itself is
	# redundant.
	# TODO: need to work out why in detail.
	# TODO: test the case where the catch statement and its
	#       block are all on the same line.
	$zap = 1;
    }
    elsif ($text =~ m/^\s*$/)
    {
	$in_decls = 0;
    }
    elsif ($in_decls && $text =~ m/^[^={}()]*$/)
    {
	# ignore lines after the opening brace for a function
	# until the first empty line, if they contain only
	# variable declarations.
	$zap = 1;
    }
    $count = "        ." if ($zap);
    print "$count:$lineno:$text\n";
}
' < $FILE

}

_compare_coverage ()
{
    echo "Filtering counts"
    _filter_spurious_counts "$1" > $1.filt
    _filter_spurious_counts "$2" > $2.filt
    vdo diff -u $1.filt $2.filt || fatal "$1.filt differs from $2.filt"
}

_filter_spurious_callgraph ()
{
    local FILE="$1"

perl -e '
use strict;
while (<STDIN>)
{
    chomp;
    s/block \d+/block NNN/;
    s/^base .*/base PPP/;
    print "$_\n";
}
' < $FILE

}

_compare_callgraph ()
{
    echo "Filtering callgraph"
    _filter_spurious_callgraph "$1" > $1.filt
    _filter_spurious_callgraph "$2" > $2.filt
    vdo diff -u $1.filt $2.filt || fatal "$1.filt differs from $2.filt"
}

compare_lines ()
{
    vcmd "compare_lines $*"
    local SRC="$1"
    local GCOV_FILE=$(_gcov_file $SRC)
    local TGGCOV_FILE=$(_tggcov_file $SRC)

    _compare_coverage "$GCOV_FILE" "$TGGCOV_FILE"
}

compare_file ()
{
    vcmd "compare_file $*"
    local SRC="$1"
    local EXPECTED_FILE=$SRC$SUBTEST.expected
    local TGGCOV_FILE=$(_tggcov_file $SRC)

    _compare_coverage "$EXPECTED_FILE" "$TGGCOV_FILE"
}

compare_callgraph ()
{
    vcmd "compare_callgraph"
    local SRC=callgraph
    local EXPECTED_FILE=$SRC$SUBTEST.expected
    local TGGCOV_FILE=$(_tggcov_file $SRC)

    _compare_callgraph "$EXPECTED_FILE" "$TGGCOV_FILE"
}
