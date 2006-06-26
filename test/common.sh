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
# $Id: common.sh,v 1.16 2006-06-26 02:55:39 gnb Exp $
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
O=o.$PLATFORM

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
    echo "$@"
    eval "$@"
}

vncdo ()
{
    if [ $CANNED = yes ]; then
	echo "[skipping] $@"
    else
	echo "$@"
	eval "$@"
    fi
}

vcapdo ()
{
    local OUTFILE=$1
    shift

    echo "$@"
    eval "$@" > $OUTFILE 2>&1
}

vcmd ()
{
    echo "==$*"
}

init ()
{
    if [ $CANNED = yes ]; then
	vcmd "init[canned] $*"
	[ -d $O ] || fatal "$O: No such directory"
    else
	vcmd "init $*"
	vdo /bin/rm -fr $O
	vdo mkdir $O
    fi
    vdo cd $O
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
    local AOUT="$1"
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

run_gcov ()
{
    local O=`/bin/pwd | sed -e "s|^$srcdir/||"`

    vcmd "run_gcov $*"
    if [ $CANNED = yes ]; then
	echo "[skipping] gcov -b -f -o $O $SRC"
	return
    fi
    local SRC="$1"
    (
        vdo cd $srcdir
	if vcapdo $TMP1 gcov -b -f -o $O $SRC ; then
	    cat $TMP1
	    GCOV_FILES=`sed -n \
		-e 's|^Creating[ \t][ \t]*\([^ \t]*\)\.gcov\.$|\1|p' \
		-e 's|^.*:creating[ \t][ \t]*.\([^ \t]*\)\.gcov.$|\1|p' \
		< $TMP1`
    	    [ -z "$GCOV_FILES" ] && fatal "no output files from gcov"
	    for f in $GCOV_FILES ; do
		vdo mv $f.gcov $O/`_gcov_file $f`
	    done
	else
	    cat $TMP1
    	    fatal "gcov failed"
	fi
    ) || exit 1
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
    local O=`/bin/pwd | sed -e "s|^$srcdir/||"`

    vcmd "run_tggcov $*"
    local SRC="$1"
    local NFLAG=`_tggcov_Nflag $SRC`

    (
        vdo cd $srcdir

	if vcapdo $TMP1 ../../src/tggcov -a $NFLAG $TGGCOV_FLAGS -o $O $SRC ; then
	    cat $TMP1
	    TGGCOV_FILES=`sed -n -e 's:^Writing[ \t][ \t]*'$srcdir'/\([^ \t]*\)\.tggcov$:\1:p' < $TMP1`
    	    [ -z "$TGGCOV_FILES" ] && fatal "no output files from tggcov"
	    for f in $TGGCOV_FILES ; do
		vdo mv $f.tggcov $O/`_tggcov_file $f`
	    done
	else
	    cat $TMP1
    	    fatal "tggcov failed"
	fi
    ) || exit 1
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
    _filter_spurious_counts "$1" > $TMP1
    _filter_spurious_counts "$2" > $TMP2
    echo "diff -u filter($1) filter($2)"
    diff -u $TMP1 $TMP2 || fatal "$4 line coverage differs from $3"
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
    _filter_spurious_callgraph "$1" > $TMP1
    _filter_spurious_callgraph "$2" > $TMP2
    echo "diff -u filter($1) filter($2)"
    diff -u $TMP1 $TMP2 || fatal "$4 line coverage differs from $3"
}

compare_lines ()
{
    vcmd "compare_lines $*"
    local SRC="$1"
    local GCOV_FILE=`_gcov_file $SRC`
    local TGGCOV_FILE=`_tggcov_file $SRC`

    _compare_coverage "$GCOV_FILE" "$TGGCOV_FILE" "gcov" "tggcov"
}

compare_file ()
{
    vcmd "compare_file $*"
    local SRC="$1"
    local EXPECTED_FILE="$srcdir/$SRC$SUBTEST.expected"
    local TGGCOV_FILE=`_tggcov_file $SRC`

    case " $TGGCOV_FLAGS " in
    *" -P "*)
	_compare_callgraph "$EXPECTED_FILE" "$TGGCOV_FILE" "expected" "tggcov"
	;;
    *)
	_compare_coverage "$EXPECTED_FILE" "$TGGCOV_FILE" "expected" "tggcov"
	;;
    esac
}
