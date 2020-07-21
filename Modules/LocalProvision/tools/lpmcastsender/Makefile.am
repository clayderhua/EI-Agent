ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = lpmcastsender
lpmcastsender_CFLAGS = $(XML_CFLAGS) \
            -I../../inc \
            -I../../../../lib/log/inc \
            -I../../../../lib/mcast-socket/inc \
            -I../../../../lib/aes/inc \
            -Wall -fPIC
lpmcastsender_LDADD = ../../../../lib/mcast-socket/src/libmcast.la \
                    ../../../../lib/aes/src/libaes.la \
                    ../../../../lib/log/src/libLog.la
lpmcastsender_SOURCES = mcastsender.c
lpmcastsender_LDFLAGS = $(XML_LIBS) -ldl -module -Wl,-rpath,. 
