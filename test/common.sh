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
    PLATFORM=`../platform.sh`
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
}

compile_c ()
{
    vcmd "compile_c $*"
    vncdo $CC $CWARNFLAGS $CCOVFLAGS $CDEFINES -c $srcdir/$1 || fatal "can't compile $srcdir/$1"
}

compile_cxx ()
{
    vcmd "compile_cxx $*"
    CXXLINK=yes
    vncdo $CXX $CXXWARNFLAGS $CXXCOVFLAGS $CXXDEFINES -c $srcdir/$1 || fatal "can't compile $srcdir/$1"
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
	vncdo $CC $CCOVFLAGS -o "$AOUT" "$@" $LDLIB || fatal "can't link $AOUT"
    	;;
    esac
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
    if [ $CANNED = yes ]; then
	echo "[skipping] gcov -b -f -o $O $SRC"
	return
    fi
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
    local GCOV_FILE=`_gcov_file $SRC`
    local TGGCOV_FILE=`_tggcov_file $SRC`

    egrep -v '^(call|branch|        -:    0:)' $GCOV_FILE > $TMP1
    echo "diff -u filter($GCOV_FILE) filter($TGGCOV_FILE)"
    diff -u $TMP1 $TGGCOV_FILE || fatal "tggcov line coverage differs from gcov"
}


_filter_spurious_counts ()
{
    local FILE="$1"
    
    sed -e 's|^ *[#0-9-]*\(: *[0-9]*:[ \t]*[{}]\)$|        .\1|' $FILE
}

compare_file ()
{
    vcmd "compare_file $*"
    local SRC="$1"
    local EXPECTED_FILE="$srcdir/$SRC$SUBTEST.expected"
    local TGGCOV_FILE=`_tggcov_file $SRC`

    _filter_spurious_counts "$EXPECTED_FILE" > $TMP1
    _filter_spurious_counts "$TGGCOV_FILE" > $TMP2
    
    echo "diff -u filter($EXPECTED_FILE) filter($TGGCOV_FILE)"
    diff -u $TMP1 $TMP2
}
