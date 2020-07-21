ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../inc -I../../../lib/log/inc/ -I../../../Library3rdParty/minizip/ -Wall -fPIC
lib_LTLIBRARIES = libotaUnzip.la
libotaUnzip_la_SOURCES = MiniUnzipLib.c
libotaUnzip_la_LDFLAGS = -L../../../Library3rdParty/minizip/ \
                         -lminizip \
                         -Wl,-rpath,. \
                         -ldl -release $$(cat ../version.h | egrep "MAIN_VERSION|SUB_VERSION|BUILD_VERSION|SVN_REVISION" | awk 'BEGIN { ORS="." }; {print $$3}' | sed 's/\.$$//')
