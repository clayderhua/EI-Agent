AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux/ \
              -I../../Include \
              -I../../Library3rdParty/cjson/inc \
              -I../../lib/log/inc \
              -I../../lib/readini \
              -I../../Library3rdParty/sqlite/inc \
              -I../../lib/handlerkernel \
              -I../../lib/messagegenerator \
              -D__STDC_FORMAT_MACROS -Wall -fPIC
lib_LTLIBRARIES = libDataSync.la
libDataSync_la_SOURCES = DataSync.cpp DataSync.h
libDataSync_la_LIBADD = ../../Platform/libWISEPlatform.la \
                        ../../Platform/libWISEUtil.la \
                        ../../lib/log/src/libLog.la \
                        ../../Library3rdParty/sqlite/src/libsqlite3.la
libDataSync_la_LDFLAGS = -L../../Library3rdParty/cjson/src/ -lcJSON \
                         -L../../lib/readini/ -lReadINI \
                         -L../../lib/handlerkernel -lHandlerKernel \
                         -L../../lib/messagegenerator -lmsggen \
                         -lpthread -ldl \
                         -release $$(cat ../../VERSION) \
                         -Wl,-rpath,.
