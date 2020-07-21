ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../../Platform/Linux/ \
            -I../../Include \
            -I../MosquittoCarrier \
            -Wall -fPIC \
            $(RMM3X)
              
lib_LTLIBRARIES = libMQTTConnector.la
libMQTTConnector_la_LIBADD = ../MosquittoCarrier/libMosquittoCarrier.la
libMQTTConnector_la_SOURCES = MQTTConnectorEx.c MQTTConnector.c MQTTConnector.h
libMQTTConnector_la_LDFLAGS = -Wl,-rpath,. -ldl -release $$(cat ../version.h | egrep "MAIN_VERSION|SUB_VERSION|BUILD_VERSION|SVN_REVISION" | awk 'BEGIN { ORS="." }; {print $$3}' | sed 's/\.$$//')
include_HEADERS = WISEConnector.h WISEConnectorEx.h
