AM_CPPFLAGS = -I../../Platform \
			  -I../../Platform/Linux/ \
			  -I../../Include \
			  -I../../Lib_SRP/SAConfig \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/log/inc \
			  $(XML_CFLAGS) \
			  -Wall -fPIC
lib_LTLIBRARIES = PowerOnOffHandler.la
PowerOnOffHandler_la_LIBADD = ../../Platform/libWISEPlatform.la \
							  ../../Platform/libWISEUtil.la \
							  ../../lib/log/src/libLog.la \
							  ../../Lib_SRP/SAConfig/libSAConfig.la
PowerOnOffHandler_la_SOURCES = amtconfig.c amtconfig.h Parser.c Parser.h PowerOnOffHandler.c PowerOnOffHandler.h
PowerOnOffHandler_la_LDFLAGS = $(XML_LIBS) \
							   -L../../Library3rdParty/cjson/src/ -lcJSON \
							   -lpthread -ldl -module \
							   -release $$(cat ../../VERSION)
							   -Wl,-rpath,. 