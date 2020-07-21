AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Lib_Util/MessageGenerator -I$(top_srcdir)/Include
bin_PROGRAMS = MsgGenTest
MsgGenTest_SOURCES = MsgGenTest.c
MsgGenTest_LDFLAGS = -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Lib_Util/MessageGenerator -lmsggen -L$(top_srcdir)/Lib_Util/cJSON -lcJSON -lm -lpthread -ldl -Wl,-rpath,./
LDADD = $(INTLLIBS)
