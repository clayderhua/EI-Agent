AM_CPPFLAGS = -I../../Old_Platforms/Linux/ \
			  -I../../Include \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/log/inc \
			  -I../../Lib_SRP/SAConfig \
			  $(XML_CFLAGS) \
			  -Wall -fPIC 

lib_LTLIBRARIES = SoftwareMonitorHandler.la
SoftwareMonitorHandler_la_LIBADD = ../../lib/log/src/libLog.la
SoftwareMonitorHandler_la_SOURCES = Parser.c Parser.h software_config.c software_config.h SoftwareMonitorHandler.c SoftwareMonitorHandler.h SoftwareMonitorLog.h SUSIAgentWatch.h ../../Old_Platforms/Linux/platform.c
SoftwareMonitorHandler_la_LDFLAGS = $(XML_LIBS) \
									-L../../Library3rdParty/cjson/src/ -lcJSON \
									-lpthread -ldl -module \
									-release $$(cat ../../VERSION) \
									-Wl,-rpath,.
LDADD = $(top_srcdir)/Lib_SRP/SAConfig/libSAConfig.la $(INTLLIBS)
