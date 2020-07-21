#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([NetInfoParser.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AM_PROG_AR
LT_INIT

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lHandlerKernel':
AC_CHECK_LIB([HandlerKernel], [main])
# FIXME: Replace `main' with a function in `-lLog':
AC_CHECK_LIB([Log], [main])
# FIXME: Replace `main' with a function in `-lReadINI':
AC_CHECK_LIB([ReadINI], [main])
# FIXME: Replace `main' with a function in `-lWISEPlatform':
AC_CHECK_LIB([WISEPlatform], [main])
# FIXME: Replace `main' with a function in `-lWISEUtil':
AC_CHECK_LIB([WISEUtil], [main])
# FIXME: Replace `main' with a function in `-lcJSON':
AC_CHECK_LIB([cJSON], [main])
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-lmsggen':
AC_CHECK_LIB([msggen], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset strerror strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
