dnl 
dnl This file is included along with aclocal.m4 when building configure.in
dnl
dnl ggcov - A GTK frontend for exploring gcov coverage data
dnl Copyright (c) 2003-2020 Greg Banks <gnb@fastmail.fm>
dnl 
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
dnl

dnl For gcc, ensure that the given flags are in $CFLAGS
AC_DEFUN([AC_GCC_ADD_CFLAGS],[
if test "x$GCC" = "xyes"; then
    AC_MSG_CHECKING([for additional gcc flags])
    CEXTRAWARNFLAGS=
    for flag in `cat <<EOF
$1
EOF
` _dummy
    do
    	test $flag = _dummy && continue
	case " $CFLAGS " in
	*[[\ \	]]$flag[[\ \	]]*) ;;
	*) CEXTRAWARNFLAGS="$CEXTRAWARNFLAGS $flag" ;;
	esac
    done
    CFLAGS="$CFLAGS $CEXTRAWARNFLAGS"
    AC_MSG_RESULT([$CEXTRAWARNFLAGS])
fi
])dnl

