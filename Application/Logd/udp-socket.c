#ifndef WIN32
#include <sys/ioctl.h>
#endif

#include "udp-socket.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/*
	ip:
		"127.0.0.1" if you would like to listen local request only
		NULL to listen all request from network
*/
SOCKET init_server_udp_socket(char* ip, int port, int is_nonblocking, struct addrinfo **listenAddr)
{
    SOCKET sock = INVALID_SOCKET;
    struct addrinfo hints = { 0 };    // Hints for name lookup
	int result;
	int yes = 1;
	char portStr[12];

#ifdef WIN32
	u_long non_blocking = is_nonblocking;
    WSADATA trash;
    if(WSAStartup(MAKEWORD(2,0),&trash)!=0) {
        fprintf(stderr, "WSAStartup failed\n");
		return -1;
	}
	int tv = 10000; // 10 sec.
#else
	int non_blocking = is_nonblocking;
	struct timeval tv;
	tv.tv_sec = 10;  // 10 Secs Timeout
	tv.tv_usec = 0;
#endif

	snprintf(portStr, sizeof(portStr), "%d", port);
	portStr[sizeof(portStr)-1] = '\0';
    //  Resolve destination address for multicast datagrams
    hints.ai_family   = AF_INET; // PF_UNSPEC
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;
	result = getaddrinfo(ip, portStr, &hints, listenAddr);
	SOCKET_NONZERO_GOTO_ERROR(result, "getaddrinfo failed");

	// Create socket for UDP server
	sock = socket((*listenAddr)->ai_family, (*listenAddr)->ai_socktype, (*listenAddr)->ai_protocol);
	SOCKET_NOTVALID_GOTO_ERROR(sock, "socket() failed");

	// Enable SO_REUSEADDR to allow multiple instances to avoid port occupy by previous test
	result = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&yes,sizeof(int));
	SOCKET_RESULT_GOTO_ERROR(result, "SO_REUSEADDR setsockopt() failed");

	// set timeout to avoid tcp port is occupy by dead proccess
	result = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,(char*)&tv,sizeof(struct timeval));
	SOCKET_RESULT_GOTO_ERROR(result, "SO_RCVTIMEO setsockopt() failed");

	// set non-blocking
#ifdef WIN32
	result = ioctlsocket(sock, FIONBIO, &non_blocking);
#else
	result = ioctl(sock, FIONBIO, &non_blocking);
#endif

    result = bind(sock, (*listenAddr)->ai_addr, (*listenAddr)->ai_addrlen);
	SOCKET_RESULT_GOTO_ERROR(result, "bind() failed");

    return sock;

error:
	if (sock != INVALID_SOCKET) {
		close(sock);
	}
	if(*listenAddr) {
		freeaddrinfo(*listenAddr);
	}
    return INVALID_SOCKET;
}

SOCKET init_client_udp_socket(const char* serverIP, int serverPort, int is_nonblocking, struct addrinfo **serverAddr)
{
    SOCKET sock = INVALID_SOCKET;
    struct addrinfo hints = { 0 };    /* Hints for name lookup */
	int result;
	char serverPortStr[12];
	
#ifdef WIN32
	u_long non_blocking = is_nonblocking;
    WSADATA trash;
    if(WSAStartup(MAKEWORD(2,0),&trash)!=0) {
        fprintf(stderr, "WSAStartup failed\n");
		return -1;
	}
#else
	int non_blocking = is_nonblocking;
#endif

	snprintf(serverPortStr, sizeof(serverPortStr), "%d", serverPort);
	serverPortStr[sizeof(serverPortStr)-1] = '\0';
    //  Resolve destination address for multicast datagrams
    hints.ai_family   = AF_INET; // PF_UNSPEC
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;
	result = getaddrinfo(serverIP, serverPortStr, &hints, serverAddr);
	SOCKET_NONZERO_GOTO_ERROR(result, "getaddrinfo failed");

	// Create socket for tcp client
	sock = socket((*serverAddr)->ai_family, (*serverAddr)->ai_socktype, (*serverAddr)->ai_protocol);
	SOCKET_NOTVALID_GOTO_ERROR(sock, "socket() failed");
	
#ifdef WIN32
	result = ioctlsocket(sock, FIONBIO, &non_blocking);
#else
	result = ioctl(sock, FIONBIO, &non_blocking);
#endif
	SOCKET_NONZERO_GOTO_ERROR(result, "FIONBIO setsockopt() failed");

    return sock;

error:
	if (sock != INVALID_SOCKET) {
		close(sock);
		sock = INVALID_SOCKET;
	}
	if(*serverAddr) {
		freeaddrinfo(*serverAddr);
	}

    return sock;
}

/*
	Return the bytes send to socket or -1 for fail
*/
int send_udp_message(SOCKET sock, const unsigned char* message, int length, struct addrinfo *serverAddr)
{
	int result;

	result = sendto(sock, message, length, 0, serverAddr->ai_addr, serverAddr->ai_addrlen);
	SOCKET_RESULT_GOTO_ERROR(result, "sendto(message) fail");
    if ( result != length ) {
        fprintf(stderr, "sendto(message) length is not match (%d, %d)\n", result, length);
		return -1;
	}
	return result;

error:
	return -1;
}

/*
	recvAddr:
		the address that transfer the datagrams
	Return
		the bytes read from socket
		-1 for fail
		0 for timeout or non-blocking no data
*/
int read_udp_message(SOCKET sock, unsigned char* message, int length)
{
	int bytes;
	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(addr);
	
	bytes = recvfrom(sock, message, length, 0, (struct sockaddr*) &addr, &addrlen);
	SOCKET_RESULT_GOTO_ERROR(bytes, "recvfrom(message) failed");

error:
	return bytes;
}

/*
	Send tcp message with simple protocol
	| data length | raw data |
	
	buf:
		the buffer that save formated message

	return 0 for success, -1 for fail
*/
int send_udp_message_ex(SOCKET sock,
						unsigned char* buf,
						int32_t buflen,
						const unsigned char* message,
						int32_t length,
						struct addrinfo *serverAddr)
{
	if (buflen < length + sizeof(int32_t)) {
		fprintf(stderr, "send_udp_message_ex: buffer size is not engouth\n");
		return -1;
	}
	memcpy(buf, (unsigned char*) &length, sizeof(int32_t));
	memcpy(buf + sizeof(int32_t), message, length);
	return send_udp_message(sock, buf, sizeof(int32_t) + length, serverAddr);
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

	Return number of read bytes, 0 for no data, -1 for fail
*/
int read_udp_message_ex(SOCKET sock, 
						unsigned char* message,
						int32_t length)
{
	int32_t *dataLength;
	int bytes, ret;
	
	bytes = read_udp_message(sock, message, length);
	if (bytes == 0) { // no more data
		return 0;
	} else if (bytes == -1) { // error
		return -1;
	}
	dataLength = (int32_t *) message;
	ret = *dataLength;
	if ((*dataLength + sizeof(int32_t)) != bytes) {
		fprintf(stderr, "Invalid message size: dataLength=%d, bytes=%d\n", *dataLength, bytes);
		return -1;
	}

	memcpy(message, message+sizeof(int32_t), length-sizeof(int32_t));
	return ret;
}
