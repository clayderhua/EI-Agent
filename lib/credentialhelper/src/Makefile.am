ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../inc \
            -I../../../Platform \
            -I../../../Platform/Linux/ \
            -I../../../Library3rdParty/cjson/inc \
			-I../../../Library3rdParty/curl/include \
            -Wall -fPIC
AM_CXXFLAGS = -I../inc -Wall -fPIC
lib_LTLIBRARIES = libcredentialhelper.la
libcredentialhelper_la_LIBADD = ../../../Platform/libWISEPlatform.la \
                                ../../../Platform/libWISEUtil.la
libcredentialhelper_la_SOURCES = CredentialHelper.c
libcredentialhelper_la_LDFLAGS = -L../../../Library3rdParty/cjson/src/ -lcJSON \
                                 -L../../../Library3rdParty/curl/lib/.libs -lcurl \
                                 -Wl,-rpath,. \
                                 -ldl -release 1.0.0.0
