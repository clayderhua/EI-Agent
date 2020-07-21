AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux/ \
              -I../../Include \
              -I../../lib/log/inc \
              -I../SAConfig \
              -Wall -fPIC \
              $(XML_CFLAGS)
lib_LTLIBRARIES = libSAHandlerLoader.la
libSAHandlerLoader_la_LIBADD = ../../Platform/libWISEPlatform.la \
                                ../../Platform/libWISEUtil.la \
                                ../../lib/log/src/libLog.la \
                                ../SAConfig/libSAConfig.la
libSAHandlerLoader_la_SOURCES = loaderlog.h moduleconfig.c moduleconfig.h SALoader.c SALoader.h
libSAHandlerLoader_la_LDFLAGS = $(XML_LIBS) -Wl,-rpath,. -lpthread -ldl -release $$(cat ../Include/VERSION | tr -d '\n')
include_HEADERS = SALoader.h
LDADD = $(INTLLIBS)
