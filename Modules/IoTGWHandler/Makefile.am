AM_CPPFLAGS = -I../../Old_Platforms/Linux/ \
              -I../../Include \
              -I../../Library3rdParty/cjson/inc \
              -I../../lib/log/inc \
              -I../../lib/datatransform \
              -I../../lib/advtools/AdvJSON/inc \
              -Wall -fPIC
lib_LTLIBRARIES = IoTSensorHandler.la
IoTSensorHandler_la_LIBADD = ../../lib/log/src/libLog.la
IoTSensorHandler_la_SOURCES = AdvAPIMuxServer.c ArrayLib.c BaseLib.c BasicFun_Tool.c BasicSocketFunc.c HashTable.c IoTGWFunction.c IoTGWHandler.c LoadSNManagerAPI.c LoadWAPI.c \
                 SnGwParser.c SnMJSONParser.c ../../Old_Platforms/Linux/platform.c ../../Old_Platforms/Linux/common.c
IoTSensorHandler_la_LDFLAGS = -L../../Library3rdParty/cjson/src/ -lcJSON \
                              -L../../lib/datatransform -ldatatransform \
                              -lpthread -ldl -module \
                              -release $$(cat ../../VERSION) \
                              -Wl,-rpath,.

                  
