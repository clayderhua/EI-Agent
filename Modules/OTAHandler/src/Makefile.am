ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../inc \
            -I../../../Include/ \
            -I../../../lib/log/inc \
            -I../../../Library3rdParty/cjson/inc \
            -I../../../Library3rdParty/iniparser/inc \
            -I../../../Lib_OTA/sueclient/inc \
            -I../../../Lib_OTA/sueclientcore/inc \
            -I../../../Platform \
			-DOTA_TAGS_CONFIG=$$(echo $${OTA_TAGS}) \
            -Wall -fPIC
AM_CXXFLAGS = -I../inc -Wall -fPIC
lib_LTLIBRARIES = OTAHandler.la
OTAHandler_la_LIBADD = ../../../Lib_OTA/sueclient/src/libsueClient.la \
                       ../../../Library3rdParty/iniparser/src/libini.la
OTAHandler_la_SOURCES = OTAHandler.c
OTAHandler_la_LDFLAGS = -Wl,-rpath,. -lpthread -ldl -module \
						-lm \
						-release $$(cat ../version.h | egrep "MAIN_VERSION|SUB_VERSION|BUILD_VERSION|SVN_REVISION" | awk 'BEGIN { ORS="." }; {print $$3}' | sed 's/\.$$//')
