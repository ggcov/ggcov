#
# Common shell functions for all the test directories
#

CC="gcc"
CWARNFLAGS="-Wall"
CCOVFLAGS="-g -fprofile-arcs -ftest-coverage"
TGGCOV_FORMAT=
srcdir=`/bin/pwd`
O=o.`../platform.sh`

TMP1=/tmp/ggcov-test-$$a
TMP2=/tmp/ggcov-test-$$b
/bin/rm -f $TMP1 $TMP2
trap "/bin/rm -f $TMP1 $TMP2" 0 1 11 13 15

fatal ()
{
    set +x
    echo "$0: FATAL: $*"
    exit 1
}

init ()
{
    /bin/rm -fr $O
    mkdir $O
    cd $O
}

compile_c ()
{
    $CC $CWARNFLAGS $CCOVFLAGS -c $srcdir/$1
}

link ()
{
    local AOUT="$1"
    shift
    $CC $CCOVFLAGS -o "$AOUT" "$@"
}

run_gcov ()
{
    local SRC="$1"
    (
        cd ..
	gcov -b -f -o $O $SRC
	mv $SRC.gcov $O
    )
}

run_tggcov ()
{
    local SRC="$1"

    if egrep '^[ \t]+(-|[0-9]+):[ \t+][01]:' $SRC.gcov 2>/dev/null ; then
    	TGGCOV_FORMAT="-N"
    fi

    (
        cd ..
	../../src/tggcov -a $TGGCOV_FORMAT -o $O $SRC
	mv $SRC.tggcov $O
    )
}

compare_lines ()
{
    local SRC="$1"
    egrep -v '^(call|branch|        -:    0:)' $SRC.gcov > $TMP1
    diff -u $TMP1 $SRC.tggcov || fatal "tggcov line coverage differs from gcov"
}

set -x
