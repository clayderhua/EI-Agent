ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../inc -fPIC
AM_CXXFLAGS = -I../inc -fPIC ${CFLAGS}

lib_LTLIBRARIES = libAdvCompression.la 
libAdvCompression_la_SOURCES = AdvCompression.cpp
libAdvCompression_la_CFLAGS = -I../inc -fPIC
libAdvCompression_la_LDFLAGS = -lz

include_HEADERS = ../inc/AdvCompression.h
