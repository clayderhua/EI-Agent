AM_CPPFLAGS = -I../../Platform \
			  -I../../Platform/Linux \
			  -I../../Include \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/messagegenerator \
			  -I../../lib/handlerkernel \
			  -I../../lib/log/inc \
			  -I../../lib/readini \
			  -I../../Lib_SRP/SAConfig \
			  $(XML_CFLAGS) \
			  -DOLDFMT
			  -Wall -fPIC 
lib_LTLIBRARIES = ProcessMonitorHandler.la
ProcessMonitorHandler_la_LIBADD = ../../Platform/libWISEPlatform.la \
								  ../../Platform/libWISEUtil.la \
								  ../../lib/log/src/libLog.la \
								  ../../Lib_SRP/SAConfig/libSAConfig.la

ProcessMonitorHandler_la_SOURCES = process_config.c  ProcessList.c  ProcessMonitor.c  ProcessParser.c
ProcessMonitorHandler_la_LDFLAGS = $(XML_LIBS) \
								   -L../../Library3rdParty/cjson/src -lcJSON \
								   -L../../lib/readini -lReadINI \
								   -L../../lib/handlerkernel -lHandlerKernel \
								   -L../../lib/messagegenerator -lmsggen \
								   -lpthread -ldl -lm -module \
								   -release $$(cat ../../VERSION) \
								   -Wl,-rpath,. 
