#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Advantech Compression Library], [19.1.8.10], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_PREFIX_DEFAULT([/usr/])
AC_CONFIG_FILES([advCompression.pc])
AC_CONFIG_SRCDIR([inc/AdvCompression.h])
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
AC_CHECK_HEADERS([limits.h stddef.h unistd.h zlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
