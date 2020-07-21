ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = lptcpsender
lptcpsender_CFLAGS = $(XML_CFLAGS) \
            -I../../inc \
            -I../../../../Include \
            -I../../../../lib/log/inc \
            -I../../../../lib/simple-tcp-socket/inc \
            -I../../../../lib/aes/inc \
            -Wall -fPIC
lptcpsender_LDADD = ../../../../lib/simple-tcp-socket/src/libstcp.la \
                  ../../../../lib/aes/src/libaes.la \
                  ../../../../lib/log/src/libLog.la
lptcpsender_SOURCES = tcpsender.c
lptcpsender_LDFLAGS = $(XML_LIBS) -ldl -module -Wl,-rpath,. 
