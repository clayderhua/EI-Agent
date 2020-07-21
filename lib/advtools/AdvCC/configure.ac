#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Advantech Concurrent Container Library], [19.1.8.10], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_PREFIX_DEFAULT([/usr/])
AC_CONFIG_FILES([advCC.pc])
AC_CONFIG_SRCDIR([inc/AdvCC.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR
LT_INIT

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memchr memset])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
