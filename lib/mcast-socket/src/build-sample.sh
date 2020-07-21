gcc -Wall -o mcast-sender \
	mcast-socket.c mcast-sender.c \
	-I../../aes/inc/ \
	-I../inc \
	-I../../log/inc \
	-L../../aes/src/.libs -laes \
	-Wl,-rpath,. 

gcc -Wall -o mcast-receiver \
	mcast-socket.c mcast-receiver.c \
	-I../../aes/inc/ \
	-I../inc \
	-I../../log/inc \
	-L../../aes/src/.libs -laes \
	-Wl,-rpath,. 
