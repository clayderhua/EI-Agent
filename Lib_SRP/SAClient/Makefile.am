ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../../lib/log/inc \
            -I../../Platform \
            -I../../Platform/Linux/ \
            -I../../Include \
            -I../../Lib_EI/WISECore \
            -I../../Library3rdParty/cjson/inc \
            -I../../lib/base64/inc \
            -I../../lib/des/inc \
            -Wall -fPIC \
            $(RMM3X)
lib_LTLIBRARIES = libSAClient.la
libSAClient_la_LIBADD = ../../lib/log/src/libLog.la \
                        ../../Platform/libWISEPlatform.la \
                        ../../Platform/libWISEUtil.la \
                        ../../Lib_EI/WISECore/libWISECore.la

libSAClient_la_SOURCES = topic.c eventqueue.c msgqueue.c smloader.c scparser.c SAClient.c
libSAClient_la_LDFLAGS = -L../../Library3rdParty/cjson/src/ -lcJSON \
                         -L../../lib/des/src/ -ldes \
                         -L../../lib/base64/src/ -lbase64 \
						 -Wl,-rpath,. \
                         -lpthread -ldl \
                         -release $$(cat ../Include/VERSION | tr -d '\n')
include_HEADERS = SAClient.h
# LDADD = -lstdc++ $(INTLLIBS)
