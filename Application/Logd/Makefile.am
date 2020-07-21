ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I../../lib/log/inc \
              $(RMM3X) \
              -Wall -fPIC
bin_PROGRAMS = logd
logd_SOURCES = logd.c udp-socket.c
logd_LDFLAGS = -Wl,-rpath,. -lm
