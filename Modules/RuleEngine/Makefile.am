AM_CPPFLAGS = -I../../Platform/ \
              -I../../Platform/Linux/ \
              -I../../Include \
              -I../../Library3rdParty/cjson/inc \
              -I../../lib/log/inc \
              -I../../lib/handlerkernel \
              -I../../lib/messagegenerator \
              -Wall -fPIC
lib_LTLIBRARIES = libRuleEngine.la
libRuleEngine_la_LIBADD = ../../lib/log/src/libLog.la \
                          ../../Platform/libWISEPlatform.la \
                          ../../Platform/libWISEUtil.la
libRuleEngine_la_SOURCES = LoadServiceSDK.c  RuleEngine.cpp  SenHub.c LoadWAPI.c RuleParser.cpp ServiceHandle.c ActionTrigger.c actionqueue.c ThresholdCheck.cpp
libRuleEngine_la_LDFLAGS = -L../../Library3rdParty/cjson/src -lcJSON \
                           -L../../lib/handlerkernel -lHandlerKernel \
                           -L../../lib/messagegenerator -lmsggen \
                           -Wl,-rpath,. \
                           -lpthread -ldl -release $$(cat VERSION | tr -d '\n')
