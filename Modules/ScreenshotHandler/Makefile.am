AM_CPPFLAGS = -I../../Platform/ \
			  -I../../Platform/Linux \
			  -I../../Include \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/log/inc \
			  -I../../lib/base64/inc \
			  -Wall -fPIC
lib_LTLIBRARIES = ScreenshotHandler.la
ScreenshotHandler_la_LIBADD = ../../Platform/libWISEPlatform.la \
							  ../../Platform/libWISEUtil.la \
							  ../../lib/log/src/libLog.la

ScreenshotHandler_la_SOURCES = Parser.c ScreenshotHandler.c
ScreenshotHandler_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
							   -L../../lib/base64/src -lbase64 \
							   -lpthread -ldl -lm -module \
							   -release $$(cat ../../VERSION) \
							   -Wl,-rpath,. 
