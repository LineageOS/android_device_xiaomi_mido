# configure.ac -- Autoconf script for gps loc-net-iface
#
# Process this file with autoconf to produce a configure script

# Requires autoconf tool later than 2.61
AC_PREREQ(2.61)
# Initialize the gps lbs-core package version 1.0.0
AC_INIT([loc-net-iface],1.0.0)
# Does not strictly follow GNU Coding standards
AM_INIT_AUTOMAKE([foreign])
# Disables auto rebuilding of configure, Makefile.ins
AM_MAINTAINER_MODE
# Verifies the --srcdir is correct by checking for the path
AC_CONFIG_SRCDIR([Makefile.am])
# defines some macros variable to be included by source
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG

# Checks for libraries.
PKG_CHECK_MODULES([LOCPLA], [loc-pla])
AC_SUBST([LOCPLA_CFLAGS])
AC_SUBST([LOCPLA_LIBS])

PKG_CHECK_MODULES([GPSUTILS], [gps-utils])
AC_SUBST([GPSUTILS_CFLAGS])
AC_SUBST([GPSUTILS_LIBS])

PKG_CHECK_MODULES([LOCHAL], [loc-hal])
AC_SUBST([LOCHAL_CFLAGS])
AC_SUBST([LOCHAL_LIBS])

PKG_CHECK_MODULES([QMIF], [qmi-framework])
AC_SUBST([QMIF_CFLAGS])
AC_SUBST([QMIF_LIBS])

PKG_CHECK_MODULES([DATA], [data])
AC_SUBST([DATA_CFLAGS])
AC_SUBST([DATA_LIBS])

PKG_CHECK_MODULES([QMI], [qmi])
AC_SUBST([QMI_CFLAGS])
AC_SUBST([QMI_LIBS])

PKG_CHECK_MODULES([DSUTILS], [dsutils])
AC_SUBST([DSUTILS_CFLAGS])
AC_SUBST([DSUTILS_LIBS])

AC_ARG_WITH([glib],
      AC_HELP_STRING([--with-glib],
         [enable glib, building HLOS systems which use glib]))

if (test "x${with_glib}" = "xyes"); then
        AC_DEFINE(ENABLE_USEGLIB, 1, [Define if HLOS systems uses glib])
        PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 2.16, dummy=yes,
                                AC_MSG_ERROR(GThread >= 2.16 is required))
        PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16, dummy=yes,
                                AC_MSG_ERROR(GLib >= 2.16 is required))
        GLIB_CFLAGS="$GLIB_CFLAGS $GTHREAD_CFLAGS"
        GLIB_LIBS="$GLIB_LIBS $GTHREAD_LIBS"

        AC_SUBST(GLIB_CFLAGS)
        AC_SUBST(GLIB_LIBS)
fi

AM_CONDITIONAL(USE_GLIB, test "x${with_glib}" = "xyes")

AC_CONFIG_FILES([ \
        Makefile \
        loc-net-iface.pc
        ])

AC_OUTPUT
