#!/bin/sh

GGCOVSRC=`dirname $0`
GGCOVSRC=`cd $GGCOVSRC ; /bin/pwd`

TD1=/tmp
TD2=$TD1/test.$USER
TD3=$TD2/dir3
TD4=$TD3/dir4

mkdir -p $TD4
trap "cd $GGCOVSRC ; /bin/rm -r $TD2" 0 1 11 13 15
cd $TD4 || exit 1

$GGCOVSRC/filetest make_absolute /foo/bar /foo/bar || exit 1
$GGCOVSRC/filetest make_absolute /foo /foo || exit 1
$GGCOVSRC/filetest make_absolute / / || exit 1
$GGCOVSRC/filetest make_absolute foo $TD4/foo || exit 1
$GGCOVSRC/filetest make_absolute foo/bar $TD4/foo/bar || exit 1
$GGCOVSRC/filetest make_absolute . $TD4 || exit 1
$GGCOVSRC/filetest make_absolute ./foo $TD4/foo || exit 1
$GGCOVSRC/filetest make_absolute ./foo/bar $TD4/foo/bar || exit 1
$GGCOVSRC/filetest make_absolute ./foo/./bar $TD4/foo/bar || exit 1
$GGCOVSRC/filetest make_absolute ./././foo/./bar  $TD4/foo/bar || exit 1
$GGCOVSRC/filetest make_absolute .. $TD3 || exit 1
$GGCOVSRC/filetest make_absolute ../foo $TD3/foo || exit 1
$GGCOVSRC/filetest make_absolute ../foo/bar $TD3/foo/bar || exit 1
$GGCOVSRC/filetest make_absolute ../../foo $TD2/foo || exit 1
$GGCOVSRC/filetest make_absolute ../../../foo/bar $TD1/foo/bar || exit 1
$GGCOVSRC/filetest make_absolute ./../.././../foo/bar $TD1/foo/bar || exit 1

