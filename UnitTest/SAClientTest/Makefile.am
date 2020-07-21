AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Lib_SRP/SAClient/ -I$(top_srcdir)/Include/ -I$(top_srcdir)/Lib_Util/Log
bin_PROGRAMS = SAClientTest
SAClientTest_SOURCES = SAClientTest.c agentlog.h
SAClientTest_LDFLAGS = -L$(top_srcdir)/Lib_Util/Log -lLog -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Lib_SRP/SAClient -lSAClient -lpthread -ldl -Wl,-rpath,./ 
LDADD = $(INTLLIBS)
