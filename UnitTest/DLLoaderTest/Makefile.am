AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux -I$(top_srcdir)/Lib_Util/Log
bin_PROGRAMS = DLLoader
DLLoader_SOURCES = DLLoader.c agentlog.h
DLLoader_LDFLAGS = -L$(top_srcdir)/Lib_Util/Log -lLog -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -ldl -Wl,-rpath,./
LDADD = $(INTLLIBS)
