AC_PREREQ([2.52])
m4_define([version_epoch], [0])
m4_define([version_major], [2])
m4_define([version_minor], [3])
m4_define([full_version],
    [version_epoch.version_major.version_minor])

AC_INIT([pidgin-hotkeys], [full_version], [email@ivanwong.info])
AC_CANONICAL_SYSTEM
	
AM_CONFIG_HEADER(hotkeys-config.h)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AM_MAINTAINER_MODE
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS")

AC_PROG_CC
AC_DISABLE_STATIC
AM_PROG_LIBTOOL
AC_PROG_INSTALL

win32=no
x11=no
case $host_os in
*mingw* | pw32* | cygwin*)
    win32=yes ;;
*)
    x11=yes ;;
esac
AM_CONDITIONAL(WITH_WIN32, test x"$win32" = "xyes")
AM_CONDITIONAL(WITH_X11, test x"$x11" = "xyes")

gtk_modules="gtk+-2.0 >= 2.4.0"
pidgin_modules="pidgin >= 2.0.0"
PKG_CHECK_MODULES(GTK, [ $gtk_modules ])
PKG_CHECK_MODULES(PIDGIN, [ $pidgin_modules ])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(PIDGIN_CFLAGS)
AC_SUBST(PIDGIN_LIBS)

AC_DEFINE_UNQUOTED(VERSION_EPOCH, version_epoch,
	[The Epoch of this release]
)
AC_DEFINE_UNQUOTED(VERSION_MAJOR, version_major,
	[The Major version number of this release]
)
AC_DEFINE_UNQUOTED(VERSION_MINOR, version_minor,
	[The Minor version number of this release]
)

if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -g3"
fi
AC_SUBST(CFLAGS)
	
AC_SUBST(ALL_LINGUAS,
"fr sv zh_TW de")

AC_SUBST(GETTEXT_PACKAGE, AC_PACKAGE_NAME)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
	[The package name, for gettext])
AM_GLIB_GNU_GETTEXT

AC_OUTPUT([
Makefile
po/Makefile.in
src/Makefile
])

