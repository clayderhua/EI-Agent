#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([parser_prtt.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AM_PROG_AR
LT_INIT

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lBase64':
AC_CHECK_LIB([Base64], [main])
# FIXME: Replace `main' with a function in `-lDES':
AC_CHECK_LIB([DES], [main])
# FIXME: Replace `main' with a function in `-lLog':
AC_CHECK_LIB([Log], [main])
# FIXME: Replace `main' with a function in `-lcJSON':
AC_CHECK_LIB([cJSON], [main])
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-lftphelper':
AC_CHECK_LIB([ftphelper], [main])
# FIXME: Replace `main' with a function in `-lmd5':
AC_CHECK_LIB([md5], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_CHECK_FUNCS([localtime_r memset strerror strrchr strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
