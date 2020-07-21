AM_CPPFLAGS = -I../../Library3rdParty/jpeg-8d1 \
              -Wall -fPIC
bin_PROGRAMS = ScreenshotHelper
ScreenshotHelper_SOURCES = ScreenshotL.c ScreenshotHelperL.c
ScreenshotHelper_LDADD = ../../Library3rdParty/jpeg-8d1/libjpeg.la
ScreenshotHelper_LDFLAGS = $(X11_LIBS) -lX11 \
						   -lpthread -ldl \
						   -Wl,-rpath,.
#LDADD = $(top_srcdir)/Library3rdParty/jpeg-8d1/.libs/libjpeg.a $(INTLLIBS)
