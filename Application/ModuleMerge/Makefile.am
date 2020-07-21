AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux \
              -I../../Include \
              $(XML_CFLAGS) \
              -Wall -fPIC
bin_PROGRAMS = ModuleMerge
ModuleMerge_LDADD = ../../Platform/libWISEPlatform.la \
                    ../../Platform/libWISEUtil.la
ModuleMerge_SOURCES = moduleconfig.c ModuleMerge.cpp
ModuleMerge_LDFLAGS = $(XML_LIBS) -ldl \
                      -Wl,-rpath,./:./AgentService
LDADD = $(INTLLIBS)
