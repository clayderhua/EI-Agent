AM_CPPFLAGS = -I../../Platform/ \
              -I../../Platform/Linux/ \
              -I../../Include \
              -I../../lib/messagegenerator/ \
              -I../../lib/handlerkernel \
              -I../../Library3rdParty/cjson/inc \
              -I../../lib/log/inc/ \
              -I../../lib/sqflashhelper \
              -I../../lib/readini \
              -IPMQLib \
              -Wall -fPIC

lib_LTLIBRARIES = HDDPMQ.la
HDDPMQ_la_SOURCES = PMQLib/PMQAlert.c PMQLib/PMQPredict.c DiskPMQInfo.c GenMessage.c HDDPMQ.c iniNode.c
HDDPMQ_la_LIBADD = ../../Platform/libWISEPlatform.la \
                   ../../Platform/libWISEUtil.la \
                   ../../lib/log/src/libLog.la
HDDPMQ_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
                    -L../../lib/susihelper -lsusihelper \
                    -L../../lib/sqflashhelper -lsqflashhelper \
                    -L../../lib/readini  -lReadINI \
                    -L../../lib/handlerkernel -lHandlerKernel \
                    -L../../lib/messagegenerator -lmsggen \
                    -lpthread -ldl -module \
                    -release $$(cat ../../VERSION) \
                    -Wl,-rpath,.
