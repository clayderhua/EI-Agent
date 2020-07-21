AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/Library/DataTransform -I$(top_srcdir)/Lib_Util/cJSON  -I$(top_srcdir)/Include
bin_PROGRAMS = DataTransformTest
DataTransformTest_SOURCES = DataTransformTest.c
DataTransformTest_LDFLAGS = -L$(top_srcdir)/Platform -lWISEPlatform -lWISEUtil -L$(top_srcdir)/Library/DataTransform -ldatatransform -L$(top_srcdir)/Lib_Util/cJSON -lcJSON -lm -lpthread -ldl -Wl,-rpath,./
LDADD = $(INTLLIBS)
