
CUR_DIR		:= $(shell pwd)
LIBZ_DIR	:= $(CUR_DIR)/zlib123
#LIBMINI_DIR	:= $(LIBZ_DIR)/contrib/minizip

.PHONY: all clean install

all: libz.a 
#libminizip.a 

libz.a: 
ifneq ($(strip $(HOST)),)
	cd $(LIBZ_DIR); \
	chmod +x ./configure; \
	CC="$(CC)" LD="$(LD)" AR="$(AR) rc" \
	CFLAGS="-fPIC" ./configure
else
	cd $(LIBZ_DIR); \
	chmod +x ./configure; \
	CFLAGS="-fPIC" ./configure
endif
	$(MAKE) -C 	$(LIBZ_DIR)
	@- cp 	-f	$(LIBZ_DIR)/$@ 	$(CUR_DIR)

#libminizip.a: libz.a
#	$(MAKE) -C 	$(LIBMINI_DIR)
#	$(AR) rc $@ $(LIBMINI_DIR)/*.o  


clean:
	@- $(RM)	$(CUR_DIR)/libz.a 
#	@- $(RM)	$(CUR_DIR)/libminizip.a
	$(MAKE) -C 	$(LIBZ_DIR) 	clean
#	$(MAKE) -C 	$(LIBMINI_DIR) 	clean


install:
	# Nothing to do

