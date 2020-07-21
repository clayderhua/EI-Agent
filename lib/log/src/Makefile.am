AM_CPPFLAGS = -I../inc -Wall -fPIC
lib_LTLIBRARIES = libLog.la
libLog_la_SOURCES = Log.c inotify_file_monitor.c udp-socket.c
libLog_la_LDFLAGS = -ldl -lpthread -Wl,-rpath,./
