lib_LTLIBRARIES = ServiceHandler.la
AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux \
              -I../../Include \
              -I../../Library3rdParty/cjson/inc \
              -I../../lib/log/inc \
              -I../../lib/datatransform \
			  -Wall -fPIC
ServiceHandler_la_SOURCES = LoadServiceSDK.c ServiceHandler.cpp
ServiceHandler_la_LIBADD = ../../lib/log/src/libLog.la \
						   ../../Platform/libWISEPlatform.la \
						   ../../Platform/libWISEUtil.la
ServiceHandler_la_LDFLAGS = -L../../Library3rdParty/cjson/src/ -lcJSON \
                            -L../../lib/datatransform -ldatatransform \
							-lpthread -ldl -module \
							-release $$(cat ../../VERSION) \
							-Wl,-rpath,.
