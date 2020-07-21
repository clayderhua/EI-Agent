AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Include/ -I$(top_srcdir)/Lib_Util/Log -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/
bin_PROGRAMS = StorageTest
StorageTest_SOURCES = StorageTest.c
StorageTest_LDFLAGS = -L$(top_srcdir)/Lib_Util/Log -lLog -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -ldl -Wl,-rpath,./
LDADD = $(INTLLIBS)
