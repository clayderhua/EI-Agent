AM_CPPFLAGS = -I../../Library3rdParty/cjson/inc \
			  -I../../Include \
			  -I../../Platform \
			  -I../../Platform/Linux \
			  -Wall -fPIC
lib_LIBRARIES = libdatatransform.a
libdatatransform_a_LIBADD = ../../Platform/libWISEPlatform.la \
                            ../../Platform/libWISEUtil.la
libdatatransform_a_SOURCES = DataTransform.c
AM_LDFLAGS = -L../../Library3rdParty/cjson/src/ -lcJSON -ldl -Wl,-rpath,.
