#
# Common shell functions for all the test directories
#

CC="gcc"
CWARNFLAGS="-Wall"
CCOVFLAGS="-g -fprofile-arcs -ftest-coverage"
CDEFINES=

CXX="c++"
CXXWARNFLAGS="-Wall"
CXXCOVFLAGS="-g -fprofile-arcs -ftest-coverage"
CXXDEFINES=

LDCOVFLAGS="-g -fprofile-arcs -ftest-coverage"

TGGCOV_ANNOTATE_FORMAT=auto
TGGCOV_FLAGS=
SUBTEST=
srcdir=`/bin/pwd`
O=o.`../platform.sh`

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
    vcmd "init $*"
    vdo /bin/rm -fr $O
    vdo mkdir $O
    vdo cd $O
}

compile_c ()
{
    vcmd "compile_c $*"
    vdo $CC $CWARNFLAGS $CCOVFLAGS $CDEFINES -c $srcdir/$1 || fatal "can't compile $srcdir/$1"
}

compile_cxx ()
{
    vcmd "compile_cxx $*"
    vdo $CXX $CXXWARNFLAGS $CXXCOVFLAGS $CXXDEFINES -c $srcdir/$1 || fatal "can't compile $srcdir/$1"
}

link ()
{
    vcmd "link $*"
    local AOUT="$1"
    shift
    vdo $CC $LDCOVFLAGS -o "$AOUT" "$@" || fatal "can't link $AOUT"
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
    vcmd "run_gcov $*"
    local SRC="$1"
    (
        vdo cd $srcdir
	if vcapdo $TMP1 gcov -b -f -o $O $SRC ; then
	    cat $TMP1
	    GCOV_FILES=`sed -n -e 's:^Creating[ \t][ \t]*\([^ \t]*\)\.gcov\.$:\1:p' < $TMP1`
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

compare_lines ()
{
    vcmd "compare_lines $*"
    local SRC="$1"

    egrep -v '^(call|branch|        -:    0:)' `_gcov_file $SRC` > $TMP1
    cat `_tggcov_file $SRC` > $TMP2
    echo "diff -u filtered-gcov-output filtered-tggcov-output"
    diff -u $TMP1 $TMP2 || fatal "tggcov line coverage differs from gcov"
}

compare_file ()
{
    vcmd "compare_file $*"
    local SRC="$1"
    
    vdo diff -u "$srcdir/$SRC$SUBTEST.expected" `_tggcov_file $SRC`
}
