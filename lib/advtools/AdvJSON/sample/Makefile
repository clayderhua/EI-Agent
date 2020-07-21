
#CC=arm-openwrt-linux-gcc
#CPP=arm-openwrt-linux-g++
#AR=arm-openwrt-linux-ar

CC = gcc
CPP=g++
AR=ar

INCPATH=-I. -I../inc
LIBPATH=
LIBS=../src/.libs/libAdvJSON.a ../../AdvCC/src/.libs/libAdvCC.a
#-DHAVE_CONFIG_H -I. -I..    -I../include -I../../AdvJSON/inc -g -O2 -MT test.o -MD -MP -MF .deps/test.Tpo -c -o test.o test.c
CFLAGS=
LDFLAGS = 

TEST=test
TESTC=testc

TEST_DEPEND=test.o
TESTC_DEPEND=testc.o

all: $(TEST) $(TESTC)

$(TEST): $(TEST_DEPEND)
	$(CPP) -o $@ $^ $(CFLAGS) $(INCPATH) $(LIBPATH) $(LIBS) $(LDFLAGS)

$(TESTC): $(TESTC_DEPEND)
	$(CC) -o $@ $^ $(CFLAGS) $(INCPATH) $(LIBPATH) $(LIBS) $(LDFLAGS)


.c.o:
	$(CC) -c $(INCPATH) $(CFLAGS) $<

.cpp.o:
	$(CPP) -c -std=c++11 $(INCPATH) $(CFLAGS) $<

clean:
	rm -rf *.o $(TEST) $(TESTC)
