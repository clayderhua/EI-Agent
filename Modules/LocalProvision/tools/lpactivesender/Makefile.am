ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = lpactivesender
lpactivesender_CFLAGS = $(XML_CFLAGS) \
                      -I../../inc \
                      -I../../../../Include \
                      -I../../../../Platform \
                      -I../../../../lib/log/inc \
                      -I../../../../lib/simple-tcp-socket/inc \
                      -I../../../../lib/aes/inc \
                      -I../../../../lib/xmlhelperlib/inc \
                      -Wall -fPIC
lpactivesender_LDADD = ../../../../Platform/libWISEUtil.la \
                     ../../../../lib/simple-tcp-socket/src/libstcp.la \
                     ../../../../lib/aes/src/libaes.la \
                     ../../../../lib/log/src/libLog.la \
                     ../../../../lib/xmlhelperlib/src/libxmlHelper.la
lpactivesender_SOURCES = activesender.c
lpactivesender_LDFLAGS = $(XML_LIBS) -ldl -module -Wl,-rpath,. 
