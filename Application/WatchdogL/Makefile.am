ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I../../Platform \
              -I../../Platform/Linux \
              -I../../Include \
              -I../../lib/log/inc \
              -Wall -fPIC
bin_PROGRAMS = sawatchdog
sawatchdog_SOURCES = Global.c SAWatchdog.c WatchdogL.c
sawatchdog_LDADD = ../../Platform/libWISEPlatform.la \
                   ../../Platform/libWISEUtil.la \
                   ../../lib/log/src/libLog.la
sawatchdog_LDFLAGS = -lpthread -ldl \
                     -Wl,-rpath,./

