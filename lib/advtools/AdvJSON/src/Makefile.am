ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../inc -I../../AdvCC/inc -fPIC
AM_CXXFLAGS = -I../inc -I../../AdvCC/inc -fPIC

lib_LTLIBRARIES = libAdvJSON.la 
libAdvJSON_la_LIBADD = ../../AdvCC/src/libAdvCC.la
libAdvJSON_la_SOURCES = AdvJSON.cpp AdvCollection.cpp AdvVariousType.cpp AdvCJSON.c jsontool.c jsonrule.c jsonparser.c jsonprint.c jsoncollection.c jsoncreator.c

include_HEADERS = ../inc/AdvJSON.h ../inc/AdvJSONParallel.h ../inc/AdvJSONCreator.h ../inc/AdvJSONCreatorParallel.h ../inc/AdvVariousType.h ../inc/AdvCollection.h
