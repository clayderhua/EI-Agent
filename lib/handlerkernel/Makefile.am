AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux/ \
              -I../../lib/messagegenerator \
              -I../../lib/log/inc \
              -I../../Library3rdParty/cjson/inc \
              -I../../Include \
              -Wall -fPIC
lib_LIBRARIES = libHandlerKernel.a
libHandlerKernel_a_SOURCES = HandlerKernelEx.c HandlerKernel.c HandlerParser.c HandlerThreshold.c triggerqueue.c
#include_HEADERS = HandlerKernel.h HandlerKernelEx.h
#AM_LDFLAGS = -ldl
