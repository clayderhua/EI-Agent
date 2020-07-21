ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../../../Library3rdParty/curl/include \
            -I../inc -I../../../lib/base64/inc \
            -Wall -fPIC
lib_LTLIBRARIES = libfileTransfer.la
libfileTransfer_la_SOURCES = FileTransferLib.c Util.c
libfileTransfer_la_LDFLAGS = -L../../../lib/base64/src/ -lbase64 \
                             -L../../../Library3rdParty/curl/lib/.libs -lcurl \
							 -Wl,-rpath,. \
							 -ldl -release $$(cat ../version.h | egrep "MAIN_VERSION|SUB_VERSION|BUILD_VERSION|SVN_REVISION" | awk 'BEGIN { ORS="." }; {print $$3}' | sed 's/\.$$//')
