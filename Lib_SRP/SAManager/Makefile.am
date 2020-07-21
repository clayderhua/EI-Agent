AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux/ \
              -I../../Include \
              -I../../Library3rdParty/cjson/inc \
              -I../../lib/log/inc \
              -I../SAConfig \
              $(XML_CFLAGS) \
              -Wall -fPIC \
              $(RMM3X)
lib_LTLIBRARIES = libSAManager.la
libSAManager_la_LIBADD = ../../lib/log/src/libLog.la \
                        ../../Lib_EI/WISECore/libWISECore.la \
                        ../SAClient/libSAClient.la
libSAManager_la_SOURCES = ghloader.c hlloader.c SAManager.c  pktparser.c topic.c keepalive.c ghloader.h hlloader.h SAManager.h  pktparser.h topic.h keepalive.h SAManagerLog.h SADataSync.c SADataSync.h reloader.c reloader.h
libSAManager_la_LDFLAGS = -L../../Library3rdParty/cjson/src/ -lcJSON \
                          -L../../lib/des/src/ -ldes \
                          -L../../lib/base64/src/ -lbase64 \
						  -Wl,-rpath,. \
                          -lpthread -ldl -release $$(cat ../Include/VERSION | tr -d '\n')
include_HEADERS = SAManager.h
