gcc -Wall -g -o otacodec otacodec.c \
	-I../../lib/des/inc/ \
	-I../../lib/base64/inc/ \
	-L../../lib/base64/src/ -lbase64 \
	-L../../lib/des/src/ -ldes
