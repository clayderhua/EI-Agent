ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = $(XML_CFLAGS) -I../inc -I../../../Platform -Wall -fPIC
lib_LTLIBRARIES = libxmlHelper.la
libxmlHelper_la_SOURCES = XmlHelperLib.c
libxmlHelper_la_LDFLAGS = $(XML_LIBS) -Wl,-rpath,. -ldl -release 1.0.0.0
