AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux/ \
              -I../../Include \
              $(XML_CFLAGS) \
              -Wall -fPIC \
              $(RMM3X)
lib_LTLIBRARIES = libSAConfig.la
libSAConfig_la_LIBADD = ../../Platform/libWISEPlatform.la \
                        ../../Platform/libWISEUtil.la
libSAConfig_la_SOURCES = configuration.c profile.c XMLBase.c
libSAConfig_la_LDFLAGS = $(XML_LIBS)  -Wl,-rpath,. -lpthread -ldl -release $$(cat ../Include/VERSION | tr -d '\n')
include_HEADERS = configuration.h profile.h XMLBase.h
LDADD = $(INTLLIBS)
