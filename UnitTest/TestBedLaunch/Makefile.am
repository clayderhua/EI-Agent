AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Lib_EI/WISECore/ -I$(top_srcdir)/Lib_Util/ReadINI/
bin_PROGRAMS = TestBedLauncher
TestBedLauncher_SOURCES = testbedlauncher.cpp
TestBedLauncher_LDFLAGS = -L$(top_srcdir)/Lib_EI/WISECore -lWISECore -lWISEUtil -L$(top_srcdir)/Lib_Util/ReadINI -lReadINI -lpthread -ldl -Wl,-rpath,./ 
LDADD = $(INTLLIBS)
