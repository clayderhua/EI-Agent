AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux -I$(top_srcdir)/Lib_SRP/SAGeneralHandler/ -I$(top_srcdir)/Lib_SRP/SAHandlerLoader/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library3rdParty/curl-7.47.1/include $(XML_CFLAGS) -I$(top_srcdir)/Lib_Util/Log
bin_PROGRAMS = HandlerLoaderTest
HandlerLoaderTest_SOURCES = HandlerLoaderTest.c agentlog.h
HandlerLoaderTest_LDFLAGS = ${XML_LIBS} -L$(top_srcdir)/Lib_Util/Log -lLog -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Lib_SRP/SAGeneralHandler -lSAGeneralHandler -L$(top_srcdir)/Lib_SRP/SAHandlerLoader -lSAHandlerLoader -lpthread -ldl -Wl,-rpath,./ 
LDADD = $(INTLLIBS)
