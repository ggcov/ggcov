dnl Process this file with autoconf to produce a configure script.
dnl You may need the --localdir option to get the included .m4's
dnl
dnl ggcov - A GTK frontend for exploring gcov coverage data
dnl Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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
dnl $Id: configure.in,v 1.79 2010-05-09 05:55:03 gnb Exp $
dnl

AC_INIT(ggcov, 0.9)
AC_CONFIG_SRCDIR(src/ggcov.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR(m4)

AM_MAINTAINER_MODE
RPM_SPEC_CONFIGURE_ARGS=

dnl Disable the GUI (useful for testing on headless platforms)
AC_MSG_CHECKING(whether to enable the GUI)
AC_ARG_ENABLE(gui,
[  --disable-gui           Do not build the GUI],[
case "${enableval}" in
yes|no)
    gui=${enableval}
    ;;
*)
    AC_MSG_ERROR(bad value ${enableval} for --enable-gui)
    ;;
esac
],[gui=yes])
AC_MSG_RESULT($gui)
AM_CONDITIONAL(GUI, test x$gui = xyes)
if test x$gui = xyes ; then
    GUIPROG=ggcov
else
    GUIPROG=
    RPM_SPEC_CONFIGURE_ARGS="$RPM_SPEC_CONFIGURE_ARGS --disable-gui"
fi
AC_SUBST(GUIPROG)

dnl Try to find the Berkeley DB in any of the odd places it might be hiding
DB_LIB_db42="db-4.2"
DB_CPPFLAGS_db42="-I/usr/include/db42"
DB_LDFLAGS_db42=""

DB_LIB_db41="db-4.1"
DB_CPPFLAGS_db41="-I/usr/include/db41"
DB_LDFLAGS_db41=""

DB_LIB_fedora="db-4"
DB_CPPFLAGS_fedora="-I/usr/include/libdb4"
DB_LDFLAGS_fedora=""

DB_LIB_plain="db"
DB_CPPFLAGS_plain=""
DB_LDFLAGS_plain=""

DB_FLAVORS="db42 db41 fedora plain"

found=no
web=yes
for flavor in $DB_FLAVORS ; do
    AC_MSG_NOTICE(Looking for Berkeley DB flavor $flavor)
    eval "lib=\$DB_LIB_$flavor"
    eval "ldflags=\$DB_LDFLAGS_$flavor"
    eval "cppflags=\$DB_CPPFLAGS_$flavor"

    oldCPPFLAGS="$CPPFLAGS"
    oldLIBS="$LIBS"
    CPPFLAGS="$cppflags $CPPFLAGS"
    found=yes
    dnl Defeat autoconf caching
    unset ac_cv_header_db_h
    AC_CHECK_HEADER(db.h,[],[found=no])
    dnl Defeat autoconf caching
    cachevar=`echo unset ac_cv_lib_${lib}_db_create | $as_tr_sh`
    eval "unset $cachevar"
    AC_CHECK_LIB($lib,db_create,[],[found=no],$ldflags)
    CPPFLAGS="$oldCPPFLAGS"
    LIBS="$oldLIBS"
    test $found = yes && break
done
if test $found = yes ; then
    DB_CPPFLAGS="$cppflags"
    DB_LIBS="-l$lib $ldflags"
    AC_SUBST(DB_CPPFLAGS)
    AC_SUBST(DB_LIBS)
else
    AC_MSG_NOTICE(Cannot find db.h, disabling web interface)
    web=no
fi

dnl Disable the web interface (useful if you haven't got Berkeley DB)
AC_MSG_CHECKING(whether to enable the WWW interface)
AC_ARG_ENABLE(web,
[  --disable-web           Do not build the WWW interface],[
case "${enableval}" in
no) web=no ;;
yes) ;;
*)
    AC_MSG_ERROR(bad value ${enableval} for --enable-web)
    ;;
esac
])
AC_MSG_RESULT($web)
AM_CONDITIONAL(WEB, test x$web = xyes)
if test x$web = xyes ; then
    WEBPROG=ggcov-webdb
else
    WEBPROG=
    RPM_SPEC_CONFIGURE_ARGS="$RPM_SPEC_CONFIGURE_ARGS --disable-web"
fi
AC_SUBST(WEBPROG)


dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
LT_INIT([shared disable-static])

dnl Stuff for gettext
dnl ALL_LINGUAS="la"
dnl AM_GNU_GETTEXT

dnl Check for va_copy, it might not exist or might be broken
dnl in some compilation environments.
AC_MSG_CHECKING([whether va_copy works])
AC_TRY_RUN([
#include <stdarg.h>
#include <string.h>
static int baz(va_list args)
{
	int x = 0;
	x |= strcmp(va_arg(args, const char *), "hello");
	x |= strcmp(va_arg(args, const char *), "world");
	return x;
}
static int bar(va_list args)
{
	int x = 0;
	va_list args2;
	va_copy(args2, args);
	x |= baz(args);
	x |= baz(args2);
	return x;
}
static int foo(int a, ...)
{
	int x;
	va_list args;
	va_start(args, a);
	x = bar(args);
	va_end(args);
	return x;
}
int main(int argc, char **argv)
{
	return foo(42, "hello", "world");
}
],[
    AC_DEFINE(HAVE_VA_COPY,1,[Whether the va_copy builtin works])
    AC_MSG_RESULT(yes)
],[
    AC_MSG_RESULT(no)
])

dnl Check whether -Wl,--as-needed works and add if so add it
dnl to $LIBS.  This flag reduces the number of silly libraries
dnl that pkg-config drags in.
AC_MSG_CHECKING([whether the linker flag --as-needed works])
have_asneeded=no
oldLDFLAGS="$LDFLAGS"
LDFLAGS="-Wl,--as-needed $LDFLAGS"
AC_TRY_LINK([],[return 0],[have_asneeded=yes])
if test $have_asneeded = no ; then
    LDFLAGS="$oldLDFLAGS"
fi
AC_MSG_RESULT($have_asneeded)

dnl Checks for libraries.
BFD_LIBS=
AC_ARG_WITH(bfd,
[  --without-bfd           Don't use BFD library (callgraphs will not work)],
[],[with_bfd=yes])
if test $with_bfd = yes ; then

dnl Try to link the bfd library (and it's buddy, -liberty) statically,
dnl falling back to dynamic linking if that fails.
dnl
dnl Why would we care?  Ubuntu, for reasons that doubtless make sense
dnl to them, include the binutils version number in the libbfd DSO name.
dnl Thus when we link ggcov against -lbfd, the name which is recorded
dnl in the ggcov executable is something like "libbfd-2.19.1.so".  When
dnl a user upgrades to a new version of binutils, the old DSO goes away
dnl and a new one appears, for example /usr/lib/libbfd-2.19.2.so.  Then
dnl the next time ggcov is run, it fails to start because the runtime
dnl linker cannot find the old DSO.  Note that this breaks ggcov every
dnl bump of binutils, not just when the libbfd ABI is incompatibly
dnl changed.  Compounding the problem, Ubuntu's packaging system detects
dnl and records a dependency on binutils but not the specific version,
dnl so there's not even a warning when the user upgrades binutils with
dnl ggcov installed.  Linking the bfd library statically should avoid
dnl all this unnecessary drama. -gnb 20110327

AC_MSG_CHECKING([for GNU bfd library, static])
have_libbfd=no
bfd_save_LIBS="$LIBS"
for dir in `$CC -print-search-dirs | sed -n -e 's|:| |g' -e 's|^libraries.*=||p'` ; do
    BFD_LIBS="$dir/libbfd.a $dir/libiberty.a"
    LIBS="$LIBS $BFD_LIBS"
    AC_LINK_IFELSE([AC_LANG_CALL([], [bfd_init])], [ have_libbfd=yes ])
    LIBS="$bfd_save_LIBS"
    test $have_libbfd = yes && break
done
AC_MSG_RESULT($have_libbfd)

if test $have_libbfd = no ; then
    AC_MSG_CHECKING([for GNU bfd library, any])
    bfd_save_LIBS="$LIBS"
    BFD_LIBS="-lbfd -liberty"
    LIBS="$LIBS $BFD_LIBS"
    AC_LINK_IFELSE([AC_LANG_CALL([], [bfd_init])], [ have_libbfd=yes ])
    LIBS="$bfd_save_LIBS"
    AC_MSG_RESULT($have_libbfd)
fi

if test $have_libbfd = yes ; then
    AC_DEFINE(HAVE_LIBBFD,1,[the GNU bfd library is available])

AC_MSG_CHECKING([whether bfd library needs dlopen])
AC_TRY_COMPILE([
#include <bfd.h>
],[
#if BFD_SUPPORTS_PLUGINS
    return 0;
#else
    fail to compile
#endif
],[
    BFD_LIBS="$BFD_LIBS -ldl"
    AC_MSG_RESULT(yes)
],[
    AC_MSG_RESULT(no)
])

AC_MSG_CHECKING([whether bfd library needs zlib])
AC_TRY_COMPILE([
#include <bfd.h>
],[
#if BFD_COMPRESS
    return 0;
#else
    fail to compile
#endif
],[
    BFD_LIBS="$BFD_LIBS -lz"
    AC_MSG_RESULT(yes)
],[
    AC_MSG_RESULT(no)
])

else
    BFD_LIBS=
fi

dnl Sometime before binutils 2.15.92.0.2, the field _raw_size
dnl in the asection struct was renamed to rawsize.
AC_MSG_CHECKING([name of raw size field in struct bfd_section])
AC_TRY_COMPILE([
#include <bfd.h>
],[
    return (int)(&((asection *)0)->rawsize);
],[
    AC_DEFINE(HAVE_BFD_ASECTION_RAWSIZE,1,[Whether the raw size field in struct bfd_section is called rawsize])
    AC_MSG_RESULT(rawsize)
],[
    AC_MSG_RESULT(_raw_size)
])
fi
AC_SUBST(BFD_LIBS)

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(malloc.h sys/ioctl.h sys/time.h unistd.h memory.h)
AC_CHECK_HEADERS(signal.h sys/filio.h stdint.h stdbool.h elf.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T


dnl A bunch of warning options for gcc
AC_GCC_ADD_CFLAGS([-Wall -Wchar-subscripts 
    	    	   -Wpointer-arith -Wsign-compare
		   -pedantic -Wno-long-long -Wno-variadic-macros])

AC_GCC_ADD_CFLAGS([-fno-rtti -fno-exceptions -fno-implement-inlines])
AC_GCC_ADD_CFLAGS([-fmessage-length=0 -fdiagnostics-show-location=every-line -fno-diagnostics-show-caret])

ifdef([PKG_PROG_PKG_CONFIG],[
PKG_PROG_PKG_CONFIG
])

#
# GUI libraries: Glade, GTK and Gnome.
#
have_gconf=no
libpopt=
if test x$gui = xyes ; then
    # gui enabled
    PKG_CHECK_MODULES(GGCOV_GUI, libglade-2.0 >= 2.0.0 libgnomeui-2.0 >= 2.0.0,
    	    	      [gtk2=yes],[gtk2=no])
    if test $gtk2 = yes ; then
	PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.4.0,have_gconf=yes)
	PKG_CHECK_MODULES(GGCOV_CLI, glib-2.0 >= 2.0.0 libxml-2.0 >= 2.0.0 gdlib >= 2.0.0)

	dnl we don't need --export-dynamic anymore, remove it
	dnl so the executable will be smaller
	GGCOV_GUI_LIBS=`echo "$GGCOV_GUI_LIBS" | sed -e 's/-Wl,--export-dynamic//g'`
	GCONF_LIBS=`echo "$GCONF_LIBS" | sed -e 's/-Wl,--export-dynamic//g'`

	dnl some modern distros don't make X11 symbols visible
	dnl by default when you link against GTK2 so for them
	dnl we need to link against libX11 explicitly.
	GGCOV_GUI_LIBS="$GGCOV_GUI_LIBS -lX11"

	AC_CHECK_LIB(gtk-x11-2.0, gtk_text_buffer_select_range,
		  AC_DEFINE(HAVE_GTK_TEXT_BUFFER_SELECT_RANGE,1,[
     * The function gtk_text_buffer_select_range may or may
     * not be present in the gtk2 libraries...we have to work
     * around it if not.
    ]),,
		  [$GGCOV_GUI_LIBS])
	GGCOV_GLADE_FILE=ggcov-glade2.glade
	# gnome2 doesn't need popt, so it may or may not be present
	AC_CHECK_LIB(popt,poptGetContext,
		     [ libpopt=" -lpopt" ],,
		     $GGCOV_GUI_CFLAGS $GGCOV_GUI_LIBS)
	GGCOV_GUI_LIBS="$GGCOV_GUI_LIBS$libpopt"

    	gprog=no
	AC_CHECK_LIB(gnome,gnome_program_init,[gprog=yes],,[$GGCOV_GUI_LIBS])
	if test $gprog = no ; then
	    AC_CHECK_LIB(gnome-2,gnome_program_init,[gprog=yes],,[$GGCOV_GUI_LIBS])
	fi
	if test $gprog = yes ; then
	      AC_DEFINE(HAVE_GNOME_PROGRAM_INIT,1,[
    Whether to use the GnomeProgram feature to initialise GNOME.
    ])
	fi
    else
	AC_MSG_ERROR([ggcov requires the GNOME libraries, unless configured with --disable-gui])
    fi
    AC_SUBST(GGCOV_GUI_CFLAGS)
    AC_SUBST(GGCOV_GUI_LIBS)
    AC_SUBST(GGCOV_GLADE_FILE)
else
    # gui disabled
    PKG_CHECK_MODULES(GGCOV_CLI, glib-2.0 >= 2.0.0 libxml-2.0 >= 2.0.0 gdlib >= 2.0.0, [glib2=yes],[glib2=no])
    if test $glib2 = no ; then
	AC_MSG_ERROR([ggcov requires the glib library])
    fi
    # we're headless, so we have no idea whether libpopt is present
    AC_CHECK_LIB(popt,poptGetContext,
		 [ libpopt=" -lpopt" ],,
		 $GGCOV_CLI_CFLAGS $GGCOV_CLI_LIBS)
fi
GGCOV_CLI_LIBS="$GGCOV_CLI_LIBS$libpopt"
AC_SUBST(GGCOV_CLI_CFLAGS)
AC_SUBST(GGCOV_CLI_LIBS)

if test $have_gconf = yes ; then
    GCONF=
    AC_DEFINE(HAVE_LIBGCONF,1,[
    	Whether the GConf library for storing application preferences is available.])
else
    GCONF="#"
fi
AM_CONDITIONAL(GCONF, test $have_gconf = yes)
AC_SUBST(GCONF)

AC_CHECK_LIB(glib-2.0, g_hash_table_iter_init,
	     AC_DEFINE(HAVE_G_HASH_TABLE_ITER,1,[
 * An iterator for GHashTables may or may not be present
 * in the glib library...we have to work around it if not.
]),,
	      [$GGCOV_CLI_LIBS])
    
if test "x$libpopt" != x ; then
    AC_DEFINE(HAVE_LIBPOPT,,[Define to 1 if the popt library is present])
fi

dnl Compile C and C++ with the same (C++) compiler.
CXXFLAGS="$CFLAGS"
CC="$CXX"

AC_CANONICAL_HOST

AC_MSG_CHECKING(for cpu-specific defines)
case "$host_cpu" in
i?86|i86pc)
    AC_DEFINE(COV_I386,1,[Building on i386 platform])
    AC_MSG_RESULT([COV_I386])
    ;;
x86_64)
    AC_DEFINE(COV_I386,1,[Building on i386 platform])
    AC_DEFINE(COV_AMD64,1,[Building on amd64 platform])
    AC_MSG_RESULT([COV_AMD64 COV_I386])
    ;;
*)
    AC_MSG_RESULT([])
    ;;
esac

AC_MSG_CHECKING(for OS-specific defines)
is_linux=no
is_redhat=no
case "$host_os" in
solaris*)
    AC_DEFINE(ELF_TARGET_ALL,1,[Something magical for Solaris])
    AC_MSG_RESULT([-DELF_TARGET_ALL])
    ;;
linux*)
    is_linux=yes
    AC_MSG_RESULT([])
    ;;
*)
    AC_MSG_RESULT([])
    ;;
esac

if test x$is_linux = xyes ; then
    AC_MSG_CHECKING(for Linux distro-specific defines)
    dnl Are we on RedHat or similar, e.g. CentOS?
    if test -f /etc/redhat-release ; then
	is_redhat=yes
    fi
fi
AM_CONDITIONAL(REDHAT, test x$is_redhat = xyes)

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL
AC_FUNC_WAIT3
AC_CHECK_FUNCS(putenv regcomp strchr)
AC_CHECK_FUNCS(sincos)

AC_SUBST(RPM_SPEC_CONFIGURE_ARGS)

dnl AC_SUBST(ALL_LINGUAS)
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_OUTPUT([
    Makefile
    src/Makefile
    src/libiberty/Makefile
    src/libggcov/Makefile
    scripts/Makefile
    ui/Makefile
    doc/Makefile
    php/Makefile
    php/lib/Makefile
    php/basic/Makefile
    lib/Makefile
    lib/templates/Makefile
    ggcov.spec
    version.m4
    test/Makefile
])
dnl intl/Makefile po/Makefile.in
