AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux/ \
              -I../../Include \
              -I../../lib/advcarehelper \
              -I../../lib/susihelper \
              $(XML_CFLAGS) \
              -Wall -fPIC
lib_LTLIBRARIES = libSAGatherInfo.la
libSAGatherInfo_la_SOURCES = SAGatherInfo.c SAGatherInfo.h
libSAGatherInfo_la_LIBADD = ../../Platform/libWISEPlatform.la \
                            ../../Platform/libWISEUtil.la
libSAGatherInfo_la_LDFLAGS = -L../../lib/susihelper -lsusihelper \
                             -L../../lib/advcarehelper -ladvcarehelper \
                             $(XML_LIBS) \
                             -lpthread -ldl \
                             -release $$(cat ../../VERSION) \
                             -Wl,-rpath,.
