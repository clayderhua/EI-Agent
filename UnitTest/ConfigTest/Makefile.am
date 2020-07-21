AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux -I$(top_srcdir)/Lib_SRP/SAConfig/ -I$(top_srcdir)/Include $(XML_CFLAGS) -I$(top_srcdir)/Lib_Util/Log
bin_PROGRAMS = ConfigTest
ConfigTest_SOURCES = amtconfig.c amtconfig.h kvmconfig.c kvmconfig.h ConfigTest.c agentlog.h
ConfigTest_LDFLAGS = ${XML_LIBS} -L$(top_srcdir)/Lib_Util/Log -lLog -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Lib_SRP/SAConfig -lSAConfig -lpthread -ldl -Wl,-rpath,./ 
LDADD = $(INTLLIBS)
