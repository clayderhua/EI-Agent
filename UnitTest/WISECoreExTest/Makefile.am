AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Include -I$(top_srcdir)/Lib_Util/Log -I$(top_srcdir)/Lib_EI/WISECore/ -I$(top_srcdir)/Lib_Util/ReadINI/ -I$(top_srcdir)/Lib_Util/MessageGenerator/ -I$(top_srcdir)/Lib_Util/cJSON/
bin_PROGRAMS = WISECoreExTest
WISECoreExTest_SOURCES = WISECoreExTest.c ConfigReader.c
WISECoreExTest_LDFLAGS = -L$(top_srcdir)/Lib_Util/Log -lLog -L$(top_srcdir)/Lib_EI/WISECore -lWISECore -lWISEUtil -L$(top_srcdir)/Lib_Util/ReadINI -lReadINI -L$(top_srcdir)/Lib_Util/MessageGenerator -lmsggen -L$(top_srcdir)/Lib_Util/cJSON -lcJSON -lpthread -ldl -lm -Wl,-rpath,./ 
LDADD = $(INTLLIBS)
