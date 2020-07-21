AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux -I$(top_srcdir)/Library/SAGatherInfo/ -I$(top_srcdir)/Include $(XML_CFLAGS) -I$(top_srcdir)/Lib_Util/Log
bin_PROGRAMS = GatherInfoTest
GatherInfoTest_SOURCES = GatherInfoTest.c agentlog.h
GatherInfoTest_LDFLAGS = ${XML_LIBS} -L$(top_srcdir)/Lib_Util/Log -lLog -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Library/SAGatherInfo -lSAGatherInfo -lpthread -ldl -Wl,-rpath,./ 
LDADD = $(INTLLIBS)
