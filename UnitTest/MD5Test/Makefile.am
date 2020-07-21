AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Lib_Util/MD5
bin_PROGRAMS = MD5Test
MD5Test_SOURCES = MD5Test.c
MD5Test_LDFLAGS = -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Lib_Util/MD5 -lmd5 -lpthread -ldl -Wl,-rpath,./ -version-info 1:0:1
LDADD = $(INTLLIBS)
