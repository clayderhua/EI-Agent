ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = $(XML_CFLAGS) \
            -I../inc \
            -I../../../Platform \
            -I../../../Include/ \
            -I../../../lib/log/inc \
            -I../../../lib/mcast-socket/inc \
            -I../../../lib/simple-tcp-socket/inc \
            -I../../../lib/aes/inc \
            -I../../../Library3rdParty/cjson/inc \
            -Wall -fPIC
lib_LTLIBRARIES = LocalProvisionHandler.la
LocalProvisionHandler_la_LIBADD = ../../../lib/mcast-socket/src/libmcast.la \
                                  ../../../lib/simple-tcp-socket/src/libstcp.la \
								  ../../../lib/aes/src/libaes.la
LocalProvisionHandler_la_SOURCES = LocalProvisionHandler.c
LocalProvisionHandler_la_LDFLAGS = $(XML_LIBS) -ldl -module -Wl,-rpath,. \
				   -release $$(cat ../version.h | egrep "MAIN_VERSION|SUB_VERSION|BUILD_VERSION|SVN_REVISION" | awk 'BEGIN { ORS="." }; {print $$3}' | sed 's/\.$$//')
