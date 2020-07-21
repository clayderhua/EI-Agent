ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../inc \
            -I../../../lib/log/inc/ \
            -I../../../lib/listhelper/inc/ \
            -I../../../Platform/ \
            -I../../../lib/xmlhelperlib/inc/ \
            -I../../filetransferlib/inc/ \
            -I../../../lib/md5/inc/ \
            -I../../../lib/base64/inc/ \
            -I../../../lib/des/inc/ \
            -I../../../Library3rdParty/sqlite/inc/ \
            -I../../../Library3rdParty/iniparser/inc/ \
            -I../../miniunziplib/inc/ \
            -Wall -fPIC
lib_LTLIBRARIES = libsueClientCore.la
libsueClientCore_la_LIBADD = ../../../lib/xmlhelperlib/src/libxmlHelper.la \
                             ../../../lib/listhelper/src/liblistHelper.la \
                             ../../filetransferlib/src/libfileTransfer.la \
                             ../../../Library3rdParty/sqlite/src/libsqlite3.la \
                             ../../../Library3rdParty/iniparser/src/libini.la \
                             ../../miniunziplib/src/libotaUnzip.la
libsueClientCore_la_SOURCES = ErrorDef.c cp_fun.c PackageLogParser.c PkgParams.c  SUECDBAccess.c  SUEClientCore.c
libsueClientCore_la_LDFLAGS = -L../../../lib/base64/src/ -lbase64 \
                              -L../../../lib/md5/src/ -lmd5 \
                              -L../../../lib/des/src/ -ldes \
							  -Wl,-rpath,. \
                              -ldl -release $$(cat ../version.h | egrep "MAIN_VERSION|SUB_VERSION|BUILD_VERSION|SVN_REVISION" | awk 'BEGIN { ORS="." }; {print $$3}' | sed 's/\.$$//')
