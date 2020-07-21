AM_CPPFLAGS = -I../../Platform \
			  -I../../Platform/Linux \
			  -I../../Include \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/log/inc \
			  -I../../lib/readini \
			  -I../../lib/messagegenerator \
			  -I../../lib/handlerkernel \
			  -DOLDFMT
lib_LTLIBRARIES = NetMonitorHandler.la
NetMonitorHandler_la_LIBADD = ../../Platform/libWISEPlatform.la \
							  ../../Platform/libWISEUtil.la \
							  ../../lib/log/src/libLog.la
NetMonitorHandler_la_SOURCES = NetInfoList.c  NetInfoParser.c  NetMonitor.c
NetMonitorHandler_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
							   -L../../lib/readini -lReadINI \
							   -L../../lib/handlerkernel -lHandlerKernel \
							   -L../../lib/messagegenerator -lmsggen \
							   -lpthread -ldl -module \
							   -release $$(cat ../../VERSION) 
							   -Wl,-rpath,. 
