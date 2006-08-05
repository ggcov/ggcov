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
# $Id: common.sh,v 1.26 2006-08-04 14:12:02 gnb Exp $
#
# Common shell functions for all the test directories
#

top_builddir=../..
top_srcdir=${top_srcdir:-../..}
builddir=.
srcdir=${srcdir:-.}

_VALGRIND=
VALGRIND="valgrind --tool=memcheck --num-callers=16 --leak-check=yes"

CC="gcc"
CWARNFLAGS="-Wall"
CCOVFLAGS="-g -fprofile-arcs -ftest-coverage"
CDEFINES=

CXX="c++"
CXXWARNFLAGS="-Wall"
CXXCOVFLAGS="-g -fprofile-arcs -ftest-coverage"
CXXDEFINES=

LDLIBS=
CXXLINK=no

TGGCOV_ANNOTATE_FORMAT=auto
TGGCOV_FLAGS=
TEST=$(cd $(dirname $0) ; basename $(pwd))
SUBTEST=
#
# `RESULT' tracks the running result for the whole test.  Values are:
#   ""	    no `pass' or `fail' calls, indeterminate
#   PASS    at least one `pass' and no `fail' calls
#   FAIL    at least one `fail' call
#   ERROR   something went wrong with the test, exiting immediately
#
RESULT=
LOG=log
if [ -n "$RPLATFORM" ]; then
    PLATFORM="$RPLATFORM"
    CANNED=yes
else
    PLATFORM=$(sh $top_srcdir/test/platform.sh)
    CANNED=no
fi

TMP1=/tmp/ggcov-test-$$a
/bin/rm -f $TMP1
trap "/bin/rm -f $TMP1 ; _result ERROR signal caught ; return 1" 1 11 13 15
trap "/bin/rm -f $TMP1 ; _resonexit" 0

_resmsg ()
{
    local res="$1"
    shift
    echo "$res: ($TEST$SUBTEST) $*"
    [ -n "$LOG" ] && echo "$res: ($TEST$SUBTEST) $*" 1>&3
}

_result ()
{
    local res="$1"
    shift

    # log any result which isn't a PASS and its message
    if [ $res != PASS ]; then
	_resmsg $res $*
    fi

    # update $RESULT
    case "$RESULT:$res" in
    :*) RESULT=$res ;;	# first result of any kind, just take it
    *:PASS) ;;		# PASS doesn't change any other result
    FAIL:ERROR) ;;	# ERRORs after a FAIL are probably flow-on
    *) RESULT=$res ;;	# otherwise, take the new result
    esac
}

_resonexit ()
{
    case "$RESULT" in
    "") _resmsg ERROR "no test code exists here" ;;
    PASS)
	SUBTEST=
	_resmsg PASS $DESCRIPTION
	;;
    esac
    [ $RESULT = PASS ] || exit 1;
}

pass ()
{
    _result PASS
}

fail ()
{
    _result FAIL $*
}

fatal ()
{
    _result ERROR $*
    [ $RESULT = ERROR ] && exit 1
}

while [ $# -gt 0 ]; do
    case "$1" in
    -D)
	TGGCOV_FLAGS="$TGGCOV_FLAGS $1 $2"
	shift
	;;
    --debug=*)
	TGGCOV_FLAGS="$TGGCOV_FLAGS $1"
	;;
    --no-log)
	LOG=
	;;
    --valgrind)
	_VALGRIND="$VALGRIND"
	;;
    *)
	echo "Unknown option: $1" 1>&2
	exit 1
	;;
    esac
    shift
done

if [ $CANNED = yes ]; then
    exec 3>/dev/null
elif [ -n "$LOG" ]; then
    [ -f $LOG ] && mv -f $LOG $LOG.old
    # save old stdout as fd 3, then redirect stdout and stderr to log
    exec 3>&1 >$LOG 2>&1
fi

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
    DESCRIPTION="$*"
    _DSTACK=$(/bin/pwd)
    _DDOWN=
    _DDOWNSTACK=
    _DUP=
    _DUPSTACK=
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
	    vdo mkdir -p "$subdir"
	fi
    fi
    vdo cd "$subdir"
    _DDOWN="${_DDOWN:+$_DDOWN/}$subdir"
    _DDOWNSTACK="$_DDOWN:$_DDOWNSTACK"
    _DUP=$(echo $subdir | sed -e 's:[^/]\+:..:g')/$_DUP
    _DSTACK=$(/bin/pwd)":$_DSTACK"
    _DUPSTACK="$_DUP:$_DUPSTACK"
}

subdir_pop ()
{
    local dir

    vcmd "subdir_pop"
    _DSTACK=$(echo "$_DSTACK" | sed -e 's|[^:]*:||')
    dir=$(echo "$_DSTACK" | sed -e 's|:*||')
    _DUPSTACK=$(echo "$_DUPSTACK" | sed -e 's|[^:]*:||')
    _DUP=$(echo "$_DUPSTACK" | sed -e 's|:*||')
    _DDOWNSTACK=$(echo "$_DDOWNSTACK" | sed -e 's|[^:]*:||')
    _DDOWN=$(echo "$_DDOWNSTACK" | sed -e 's|:*||')
    vdo cd $dir || fatal "$dir: No such directory"
}

# Return a path to a file in $srcdir.  Used where the
# pathname doesn't matter, e.g. *.expected files
_srcfile ()
{
    case "$srcdir" in
    .) echo $_DUP$1 ;;
    /*) echo "$srcdir/$1" ;;
    *) echo $_DUP$srcdir/$1 ;;
    esac
}

# Ensure the given files exist at the given paths relative to
# the current directory, if necessary linking them from $srcdir
# Used where the pathname matters, e.g. C source files.
need_files ()
{
    if [ $CANNED == yes ]; then
	vcmd "need_files[canned] $*"
    else
	vcmd "need_files $*"
	if [ $srcdir != "." -o -n "$_DDOWN" ]; then
	    for path in $* ; do
		local d=$(dirname $path)
		local f=$(basename $path)
		if [ ! -d $d ]; then
		    vdo mkdir -p $d || fatal "Can't build directory $d"
		fi
		if [ ! -e $path ]; then
		    vdo ln $(_srcfile ${_DDOWN:+x}$f) $path || fatal "Can't link source file to $path"
		fi
	    done
	fi
    fi
}


compile_c ()
{
    local cfile=
    local cflags="$CWARNFLAGS $CCOVFLAGS $CDEFINES"

    vcmd "compile_c $*"

    while [ $# -gt 0 ] ; do
    	case "$1" in
	-d*|-f*|-m*|-I*|-D*) cflags="$cflags $1" ;;
	-*) fatal "compile_c: unknown flag \"$1\"" ;;
	*)
	    [ -n "$cfile" ] && fatal "compile_c: too many cfiles given"
	    cfile="$1"
	    ;;
	esac
	shift
    done

    [ -z "$cfile" ] && fatal "compile_c: no cfile given"

    vncdo $CC $cflags -c $cfile || fatal "can't compile $cfile"
}

compile_cxx ()
{
    local cfile=
    local cflags="$CXXWARNFLAGS $CXXCOVFLAGS $CXXDEFINES"

    vcmd "compile_cxx $*"

    while [ $# -gt 0 ] ; do
    	case "$1" in
	-d*|-f*|-m*|-I*|-D*) cflags="$cflags $1" ;;
	-*) fatal "compile_cxx: unknown flag \"$1\"" ;;
	*)
	    [ -n "$cfile" ] && fatal "compile_cxx: too many cfiles given"
	    cfile="$1"
	    ;;
	esac
	shift
    done

    [ -z "$cfile" ] && fatal "compile_cxx: no cfile given"

    CXXLINK=yes
    vncdo $CXX $cflags -c $cfile || fatal "can't compile $cfile"
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
    if [ -n "$LDLIBS" ]; then
	vdo ldd $AOUT
	vdo readelf --dynamic $AOUT
    fi
}

_dso_filename ()
{
    echo "lib$(basename $1).so.1"
}

_dso_filename2 ()
{
    echo "lib$(basename $1).so"
}

link_shlib ()
{
    vcmd "link_shlib $*"
    local DSO=$(_dso_filename "$1")
    local DSO2=$(_dso_filename2 "$1")
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
	-*) fatal "add_shlib: unknown flag \"$1\"" ;;
	*)
	    [ -n "$dso" ] && fatal "add_shlib: too many dsos given"
	    dso="$(/bin/pwd)/$1"
	    ;;
	esac
	shift
    done

    [ -z "$dso" ] && fatal "add_shlib: no dso given"

    dir=$(dirname $dso)
    file=$(_dso_filename $dso)
    dso=$(basename $dso)

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
    vncdo $AOUT "$@" || fail "Can't run generated test program"
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
	echo "[skipping] gcov -b $SRC"
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

_tggcov_Nflag ()
{
    local gcovfile=$(_gcov_file $1)
    if [ -f $gcovfile ]; then
	egrep '^[ \t]+(-|[0-9]+):[ \t]+[01]:' $gcovfile >/dev/null && echo "-N"
    fi
}

run_tggcov ()
{
    vcmd "run_tggcov $*"
    local SRC=
    local mode="-a"
    local flags="$TGGCOV_FLAGS"
    local nflag=
    local pwd=$(/bin/pwd)

    while [ $# -gt 0 ] ; do
    	case "$1" in
	-a|-G) mode="$1" ;;
	-N) nflag="$1" ;;
	-P) flags="$flags $1" ;;
	-D|-X|-Y|-Z) flags="$flags $1 $2" ; shift ;;
	-*) fatal "run_tggcov: unknown option \"$1\"" ;;
	*)
	    SRC="$SRC $1"
	    ;;
	esac
	shift
    done

    [ -z "$SRC" ] && fatal "run_tggcov: no source files given"
    [ -z "$nflag" ] && nflag=$(_tggcov_Nflag $SRC)
    if vcapdo $TMP1 $_VALGRIND $top_builddir/${_DUP}src/tggcov $mode $nflag $flags $SRC ; then
	cat $TMP1
	if [ x$mode = "x-a" ] ; then
	    TGGCOV_FILES=$(sed -n -e 's:^Writing[ \t][ \t]*'$pwd'/\([^ \t]*\.tggcov\)$:\1:p' < $TMP1)
	    [ -z "$TGGCOV_FILES" ] && fail "no output files from tggcov"
	    _subtestize_files $TGGCOV_FILES
	fi
	pass
    else
	cat $TMP1
	fail "tggcov failed"
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
    vdo diff -u $1.filt $2.filt && pass || fail "$1.filt differs from $2.filt"
}

_compare_callgraph ()
{
    echo "Filtering callgraph"
    perl $top_srcdir/test/filter-callgraph.pl "$1" > $1.filt
    perl $top_srcdir/test/filter-callgraph.pl "$2" > $2.filt
    vdo diff -u $1.filt $2.filt && pass || fail "$1.filt differs from $2.filt"
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
    local EXPECTED_FILE=$(_srcfile $SRC$SUBTEST.expected)
    local TGGCOV_FILE=$(_tggcov_file $SRC)

    _compare_coverage "$EXPECTED_FILE" "$TGGCOV_FILE"
}

compare_callgraph ()
{
    vcmd "compare_callgraph"
    local SRC=callgraph
    local EXPECTED_FILE=$(_srcfile $SRC$SUBTEST.expected)
    local TGGCOV_FILE=$(_tggcov_file $SRC)

    _compare_callgraph "$EXPECTED_FILE" "$TGGCOV_FILE"
}
