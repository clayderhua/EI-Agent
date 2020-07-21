#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])

AC_INIT([CAgentIoT], [VERSION])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AC_CONFIG_SRCDIR([Include/svnversion.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
#AM_PROG_AR
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Check STDLIB
AC_ARG_WITH(
	[stdlib],
	[AS_HELP_STRING([--with-stdlib=LIB],[specify the C++ standard library to use [default=system]])],
	[],[with_stdlib=system]
)
AS_IF([test "x$with_stdlib" != "xsystem"],
	[CXXFLAGS="$CXXFLAGS -stdlib=$with_stdlib"
	 LDFLAGS="$LDFLAGS -stdlib=$with_stdlib"]
)

#Flag for RMM3.X handshake protocol
AC_ARG_ENABLE(RMM3X,
	[AS_HELP_STRING([--enable-RMM3X], [Switch to RMM3.X handshake protocol])],
	[case "${enableval}" in
		yes | no ) WITH_RMM3X="${enableval}" ;;
		*) AC_MSG_ERROR(bad value ${enableval} for --enable-RMM3X) ;;
	esac],
	[WITH_RMM3X="no"]
)
AM_CONDITIONAL([WITH_RMM3X], [test "x$WITH_RMM3X" = "xyes"])
AM_COND_IF([WITH_RMM3X],
           [AC_SUBST([RMM3X],[-DRMM3X])])
		   
# Checks for libraries.
#### Check for libcurl
#LIBCURL_CHECK_CONFIG([yes],[],[],[AC_MSG_ERROR([libcurl development files required])])
#PKG_CHECK_MODULES(CURL, libcurl >= 7.35.0)

# Checks for header files.
AC_CHECK_HEADERS([netdb.h stddef.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostbyname memset socket strcasecmp strchr strncasecmp strstr])

# Checks for XML2 
PKG_CHECK_MODULES(XML, libxml-2.0 >= 2.4)

# Checks for header files.
AC_PATH_X
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([OS.h arpa/inet.h fcntl.h float.h inttypes.h limits.h mach/mach.h malloc.h memory.h netdb.h netinet/in.h sgtty.h stddef.h stdint.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/systeminfo.h sys/time.h sys/timeb.h syslog.h termio.h termios.h unistd.h utime.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([alarm clock_gettime dup2 fdatasync floor ftime ftruncate gethostbyaddr gethostbyname gethostname gethrtime getpagesize gettimeofday inet_ntoa isascii localtime_r memchr memmove memset mkdir pow select socket strcasecmp strchr strdup strerror strncasecmp strndup strpbrk strrchr strspn strstr strtol strtoul sysinfo uname utime])
AC_CONFIG_FILES([Makefile
		 SAClient/Makefile
		 SAConfig/Makefile
		 SAManager/Makefile
		 SAGeneralHandler/Makefile
		 SAHandlerLoader/Makefile
		])

AC_OUTPUT

