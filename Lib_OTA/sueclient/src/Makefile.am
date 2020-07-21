ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../inc \
            -I../../../Platform/ \
            -I../../../lib/log/inc/ \
            -I../../../Library3rdParty/cjson/inc/ \
            -I../../../Library3rdParty/iniparser/inc/ \
            -I../../../lib/queuehelper/inc/ \
            -I../../../lib/listhelper/inc/ \
            -I../../sueclientcore/inc/ \
            -I../../zschedule/inc/ \
            -Wall -fPIC
lib_LTLIBRARIES = libsueClient.la
libsueClient_la_LIBADD = ../../../Library3rdParty/iniparser/src/libini.la \
                         ../../../lib/queuehelper/src/libqueueHelper.la \
                         ../../../lib/listhelper/src/liblistHelper.la \
                         ../../sueclientcore/src/libsueClientCore.la \
                         ../../zschedule/src/libzSchedule.la

libsueClient_la_SOURCES = ErrorDef.c MsgParser.c SUEClient.c SUESchedule.c
libsueClient_la_LDFLAGS = -Wl,-rpath,. \
                          -ldl -release $$(cat ../version.h | egrep "MAIN_VERSION|SUB_VERSION|BUILD_VERSION|SVN_REVISION" | awk 'BEGIN { ORS="." }; {print $$3}' | sed 's/\.$$//')
