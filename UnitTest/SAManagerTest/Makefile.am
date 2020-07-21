AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Include/ -I$(top_srcdir)/Lib_Util/Log -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Lib_SRP/SAManager/
bin_PROGRAMS = SAManagerTest
SAManagerTest_SOURCES = SAManagerTest.c agentlog.h
SAManagerTest_LDFLAGS = -L$(top_srcdir)/Lib_Util/Log -lLog -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Lib_SRP/SAManager -lSAManager -lpthread -ldl -Wl,-rpath,./ 
LDADD = $(INTLLIBS)
