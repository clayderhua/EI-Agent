#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([src/cJSON.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AM_PROG_AR
LT_INIT

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([float.h limits.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([floor memset pow strchr])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
