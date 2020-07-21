AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/ReadINI -I$(top_srcdir)/Library/MessageGenerator -I$(top_srcdir)/Library/HandlerKernel -I$(top_srcdir)/Library/WISECore
bin_PROGRAMS = WISECoreTest
WISECoreTest_SOURCES = WISECoreTest.c
WISECoreTest_LDFLAGS = -L$(top_srcdir)/Library/HandlerKernel -lHandlerKernel -L$(top_srcdir)/Library/Log -lLog -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Library/cJSON -lcJSON -L$(top_srcdir)/Library/ReadINI -lReadINI -L$(top_srcdir)/Library/MessageGenerator -lmsggen -L$(top_srcdir)/Library/WISECore -lWISECore -lpthread -lm -ldl -Wl,-rpath,./ 
#LDADD = $(INTLLIBS)
