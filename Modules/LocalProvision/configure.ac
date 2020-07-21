#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([src/LocalProvisionHandler.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AM_PROG_AR
LT_INIT

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [main])

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h limits.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime floor memset select strstr])

PKG_CHECK_MODULES(XML, libxml-2.0 >= 2.4)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tools/lpmcastsender/Makefile
                 tools/lptcpreceiver/Makefile
                 tools/lptcpsender/Makefile
                 tools/lpactivesender/Makefile])

AC_OUTPUT
