ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../../Platform/Linux/ \
            -I../../Include \
            -I../MQTTConnector \
            -Wall -fPIC \
            $(RMM3X)
lib_LTLIBRARIES = libWISECore.la
libWISECore_la_LIBADD = ../MQTTConnector/libMQTTConnector.la
libWISECore_la_SOURCES = liteparse.c liteparse.h WISECoreEx.c WISECore.c WISECore.h
libWISECore_la_LDFLAGS = -Wl,-rpath,. -lpthread -ldl -release $$(cat ../version.h | egrep "MAIN_VERSION|SUB_VERSION|BUILD_VERSION|SVN_REVISION" | awk 'BEGIN { ORS="." }; {print $$3}' | sed 's/\.$$//')
include_HEADERS = WISECore.h WISECoreEx.h
