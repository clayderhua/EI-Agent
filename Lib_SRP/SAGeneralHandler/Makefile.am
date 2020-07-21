AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux/ \
              -I../../Include \
              -I../SAConfig \
              -I../../lib/log/inc \
              -I../../lib/ftphelper/inc \
              -I../../lib/md5/inc \
              -I../../Library3rdParty/cjson/inc \
              -I../../lib/messagegenerator \
              -Wall -fPIC \
              $(XML_CFLAGS)
lib_LTLIBRARIES = libSAGeneralHandler.la
libSAGeneralHandler_la_LIBADD = ../../Platform/libWISEPlatform.la \
                                ../../Platform/libWISEUtil.la \
                                ../../lib/log/src/libLog.la \
                                ../SAConfig/libSAConfig.la
libSAGeneralHandler_la_SOURCES = general_def.h generallog.h agentupdater.c agentupdater.h ghparser.c ghparser.h SAGeneralHandler.c SAGeneralHandler.h
libSAGeneralHandler_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
                                 -L../../lib/md5/src -lmd5 \
                                 -L../../lib/ftphelper/src/ -lftphelper \
                                 -L../../lib/messagegenerator/ -lmsggen \
                                 -L../../Library3rdParty/curl/lib/.libs -lcurl \
                                 -Wl,-rpath,. \
                                 -lpthread -ldl -release $$(cat ../Include/VERSION | tr -d '\n')
LDADD = $(INTLLIBS)
