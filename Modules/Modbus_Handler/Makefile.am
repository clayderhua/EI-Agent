AM_CPPFLAGS = -I../../Platform \
			  -I../../Platform/Linux/ \
			  -I../../Include \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/log/inc \
			  -I../../lib/readini \
			  -I../../lib/messagegenerator/ \
			  -I../../lib/handlerkernel \
			  -I../../Library3rdParty/libmodbus/src \
			  -Wall -fPIC 
lib_LTLIBRARIES = Modbus_Handler.la
Modbus_Handler_la_SOURCES = Modbus_Handler.cpp Modbus_Handler.h Modbus_HandlerLog.h Modbus_Parser.c Modbus_Parser.h
Modbus_Handler_la_LIBADD = ../../Platform/libWISEPlatform.la \
						   ../../Platform/libWISEUtil.la \
						   ../../lib/log/src/libLog.la \
						   ../../Library3rdParty/libmodbus/src/libmodbus.la
Modbus_Handler_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
							-L../../lib/readini  -lReadINI \
							-L../../lib/handlerkernel -lHandlerKernel \
							-L../../lib/messagegenerator -lmsggen \
							-lpthread -ldl -module \
							-release $$(cat ../../VERSION) \
							-Wl,-rpath,.
LDADD = $(INTLLIBS)
