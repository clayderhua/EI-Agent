AM_CPPFLAGS = -I../../Old_Platforms/Linux/ \
			  -I../../Include \
			  -I../../Library3rdParty/cjson/inc \
			  -I../../lib/log/inc \
			  -I../../lib/readini \
			  -I../../lib/susihelper \
			  -I../../lib/susiiot/include \
			  -Wall -fPIC 
lib_LTLIBRARIES = SUSIControlHandler.la
SUSIControlHandler_la_LIBADD = ../../lib/log/src/libLog.la
SUSIControlHandler_la_SOURCES = Parser.c ../../Old_Platforms/Linux/platform.c ../../Old_Platforms/Linux/common.c SUSIControlHandler.cpp
SUSIControlHandler_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
								-L../../lib/readini -lReadINI \
								-lpthread -ldl -module \
								-release $$(cat ../../VERSION) \
								-Wl,-rpath,. 
