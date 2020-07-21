ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = lptcpreceiver
lptcpreceiver_CFLAGS = $(XML_CFLAGS) \
            -I../../inc \
            -I../../../../lib/log/inc \
            -I../../../../lib/simple-tcp-socket/inc \
            -I../../../../lib/aes/inc \
            -Wall -fPIC
lptcpreceiver_LDADD = ../../../../lib/simple-tcp-socket/src/libstcp.la \
                    ../../../../lib/aes/src/libaes.la \
                    ../../../../lib/log/src/libLog.la
lptcpreceiver_SOURCES = tcpreceiver.c
lptcpreceiver_LDFLAGS = $(XML_LIBS) -ldl -module -Wl,-rpath,. 
