#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "tcp-socket.h"

#define LOG_TAG "STCP"
#include "Log.h"

/*
	ip:
		"127.0.0.1" if you would like to listen local request only
		NULL to listen all request from network
*/
SOCKET init_server_tcp_socket(char* ip, char* port)
{
    SOCKET sock = INVALID_SOCKET;
    struct addrinfo hints = { 0 };    // Hints for name lookup
	int result;
	struct addrinfo *listenAddr = NULL;
	int yes = 1;

#ifdef WIN32
    WSADATA trash;
    if(WSAStartup(MAKEWORD(2,0),&trash)!=0) {
        LOGE("WSAStartup failed");
		return -1;
	}
	int tv = 10000; // 10 sec.
#else
	struct timeval tv;
	tv.tv_sec = 10;  // 10 Secs Timeout
	tv.tv_usec = 0;
#endif

    //  Resolve destination address for multicast datagrams
    hints.ai_family   = AF_INET; // PF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;
	//result = getaddrinfo(ip, port, &hints, &listenAddr);
	result = getaddrinfo(ip, port, &hints, &listenAddr);
	SOCKET_NONZERO_GOTO_ERROR(result, "getaddrinfo failed");

	// Create socket for tcp server
	sock = socket(listenAddr->ai_family, listenAddr->ai_socktype, listenAddr->ai_protocol);
	SOCKET_NOTVALID_GOTO_ERROR(sock, "socket() failed");

	// Enable SO_REUSEADDR to allow multiple instances to avoid port occupy by previous test
	result = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&yes,sizeof(int));
	SOCKET_RESULT_GOTO_ERROR(result, "SO_REUSEADDR setsockopt() failed");

	// set timeout to avoid tcp port is occupy by dead proccess
	result = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,(char*)&tv,sizeof(struct timeval));
	SOCKET_RESULT_GOTO_ERROR(result, "SO_RCVTIMEO setsockopt() failed");

    result = bind(sock, listenAddr->ai_addr, listenAddr->ai_addrlen);
	SOCKET_RESULT_GOTO_ERROR(result, "bind() failed");

	result = listen(sock, 5);
	SOCKET_RESULT_GOTO_ERROR(result, "listen() failed");

	if(listenAddr) {
		freeaddrinfo(listenAddr);
	}
    return sock;

error:
	if (sock != INVALID_SOCKET) {
		close(sock);
	}
	if(listenAddr) {
		freeaddrinfo(listenAddr);
	}
    return INVALID_SOCKET;
}

SOCKET init_client_tcp_socket(const char* serverIP, const char* serverPort)
{
    SOCKET sock = INVALID_SOCKET;
    struct addrinfo hints = { 0 };    /* Hints for name lookup */
	int result;
	struct addrinfo *serverAddr;

#ifdef WIN32
    WSADATA trash;
    if(WSAStartup(MAKEWORD(2,0),&trash)!=0) {
        LOGE("WSAStartup failed");
		return -1;
	}
#endif

    //  Resolve destination address for multicast datagrams
    hints.ai_family   = AF_INET; // PF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;
	result = getaddrinfo(serverIP, serverPort, &hints, &serverAddr);
	SOCKET_NONZERO_GOTO_ERROR(result, "getaddrinfo failed");

	// Create socket for tcp client
	sock = socket(serverAddr->ai_family, serverAddr->ai_socktype, 0);
	SOCKET_NOTVALID_GOTO_ERROR(sock, "socket() failed");

	result = connect(sock, serverAddr->ai_addr, serverAddr->ai_addrlen);
	SOCKET_RESULT_GOTO_ERROR(result, "connecting stream socket fail");

	if(serverAddr)
		freeaddrinfo(serverAddr);

    return sock;

error:
	if (sock != INVALID_SOCKET) {
		close(sock);
		sock = INVALID_SOCKET;
	}
	if(serverAddr)
		freeaddrinfo(serverAddr);

    return sock;
}

int send_tcp_message(SOCKET sock, const unsigned char* message, int length)
{
	int result;

	result = send(sock, message, length, 0);
	SOCKET_RESULT_GOTO_ERROR(result, "send(cipher1) fail");
    if ( result != length ) {
        LOGE("send(cipher1) fail");
		return -1;
	}

	return 0;

error:
	return -1;
}

int read_tcp_message(SOCKET sock, unsigned char* message, int length)
{
	int bytes, remaindBytes;

	remaindBytes = length;
	while (remaindBytes) {
		bytes = recv(sock, message+(length-remaindBytes), remaindBytes, 0);
		SOCKET_RESULT_GOTO_ERROR(bytes, "recv(cipher) failed");
		if (bytes == 0) { //  reach EOF but data is not complete
			return -1;
		}
		remaindBytes -= bytes;
	}

error:
	return length;
}

/*
	Send tcp message with simple protocol
	| data length | raw data |

	return 0 for success, -1 for fail
*/
int send_tcp_message_ex(SOCKET sock, const unsigned char* message, int32_t length)
{
	int result;

	result = send_tcp_message(sock, (unsigned char*) &length, sizeof(length));
	if (result == -1) {
		return -1;
	}

	return send_tcp_message(sock, message, length);
}

/*
	Read tcp message with simple protocol
	| data length | raw data |

	sock:
		socket fd
	message:
		buffer to save message
	length:
		the buffer size of message

	Return number of read bytes, -1 for fail
*/
int read_tcp_message_ex(SOCKET sock, unsigned char* message, int length)
{
	int32_t dataLength;
	int bytes;

	bytes = read_tcp_message(sock, (unsigned char*) &dataLength, sizeof(dataLength));
	if (bytes != sizeof(dataLength)) {
		LOGE("read_tcp_message(dataLength) fail");
		return -1;
	}
	if (dataLength > length) {
		LOGE("read_tcp_message_ex:  buffer is not engouth");
	}

	bytes = read_tcp_message(sock, message, dataLength);
	if (bytes != dataLength) {
		LOGE("read_tcp_message(message) fail");
		return -1;
	}

	return dataLength;
}