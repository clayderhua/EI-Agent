ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux/ \
              -I../../lib/advtools/AdvCC/inc \
              -I../../lib/advtools/AdvCompression/inc \
              -I../../lib/advtools/AdvJSON/inc \
              -Wall -fPIC \
              $(RMM3X)

AM_CFLAGS = -I../../Platform \
            -I../../Platform/Linux/ \
            -I../../Include \
            -I../Include \
            -I../../lib/advtools/AdvJSON/inc \
            -I../../lib/advtools/AdvCC/inc \
            -I../../lib/advtools/AdvCompression/inc \
            -Wall -fPIC \
            $(RMM3X)
lib_LTLIBRARIES = libMosquittoCarrier.la
libMosquittoCarrier_la_LIBADD = ../../lib/advtools/AdvCC/src/libAdvCC.la \
                                ../../lib/advtools/AdvCompression/src/libAdvCompression.la
libMosquittoCarrier_la_SOURCES = CompressionTunnel.cpp ExternalTranslator.c MosquittoCarrierEx.c MosquittoCarrier.c topic.c topic.h
libMosquittoCarrier_la_LDFLAGS = -lmosquitto \
                                 -Wl,-rpath,. \
                                 -lpthread -ldl \
                                 -release $$(cat ../version.h | egrep "MAIN_VERSION|SUB_VERSION|BUILD_VERSION|SVN_REVISION" | awk 'BEGIN { ORS="." }; {print $$3}' | sed 's/\.$$//')
include_HEADERS = WiseCarrier_MQTT.h WiseCarrierEx_MQTT.h ExternalTranslator.h

