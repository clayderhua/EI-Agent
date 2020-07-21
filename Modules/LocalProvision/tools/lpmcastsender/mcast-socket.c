// base on https://github.com/bk138/Multicast-Client-Server-Example
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "mcast-socket.h"

#define OTA_TAG "LP"
#include "Log.h"

#if 0
static void dump_hex(char* title, unsigned char* raw, int length)
{
	int i;

	printf("dump_hex(%s):\n", title);
	for (i = 0; i < length; i++) {
		printf("%02X ", raw[i]);
	}
	printf("\n");
}
#endif

/*
	Init a socket fd for sending multicast packet.

	multicastIP:
		multicast ip addr
	multicastPort:
		multicast port
	multicastAddr:
		multicast addr instance for send_mcast_message()

	Return socket fd for success, -1 for fail
*/
SOCKET init_send_mcast_socket(char* multicastIP, char* multicastPort, struct addrinfo **multicastAddr)
{

    SOCKET sock = INVALID_SOCKET;
    struct addrinfo hints = { 0 };    /* Hints for name lookup */
	int multicastTTL = LP_MUTICAST_TTL;
	int result;

#ifdef WIN32
    WSADATA trash;
    if(WSAStartup(MAKEWORD(2,0),&trash)!=0) {
        LOGE("WSAStartup failed");
		return -1;
	}
#endif

    //  Resolve destination address for multicast datagrams
    hints.ai_family   = AF_INET; //PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_NUMERICHOST;
	result = getaddrinfo(multicastIP, multicastPort, &hints, multicastAddr);
	SOCKET_NONZERO_GOTO_ERROR(result, "init_send_mcast_socket: getaddrinfo failed");

	// Create socket for sending multicast datagrams
	sock = socket((*multicastAddr)->ai_family, (*multicastAddr)->ai_socktype, 0);
	SOCKET_NOTVALID_GOTO_ERROR(sock, "init_send_mcast_socket: socket() failed");

    // Set TTL of multicast packet
	result = setsockopt(sock,
                    (*multicastAddr)->ai_family == PF_INET6 ? IPPROTO_IPV6        : IPPROTO_IP,
                    (*multicastAddr)->ai_family == PF_INET6 ? IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL,
                    (char*) &multicastTTL, sizeof(multicastTTL));
	SOCKET_RESULT_GOTO_ERROR(result, "TTL setsockopt() failed");

    // set the sending interface
    if((*multicastAddr)->ai_family == PF_INET) {
        in_addr_t iface = INADDR_ANY; /* well, yeah, any */
        result = setsockopt (sock,
                       IPPROTO_IP,
                       IP_MULTICAST_IF,
                       (char*)&iface, sizeof(iface));
		SOCKET_RESULT_GOTO_ERROR(result, "IP_MULTICAST_IF setsockopt() failed");
    }
    if((*multicastAddr)->ai_family == PF_INET6) {
        unsigned int ifindex = 0; /* 0 means 'default interface'*/
		setsockopt (sock,
                       IPPROTO_IPV6,
                       IPV6_MULTICAST_IF,
                       (char*)&ifindex, sizeof(ifindex));
		SOCKET_RESULT_GOTO_ERROR(result, "IPV6_MULTICAST_IF setsockopt() failed");
    }

    return sock;

error:
	if (sock != INVALID_SOCKET) {
		close(sock);
		sock = INVALID_SOCKET;
	}
	if (*multicastAddr) {
		freeaddrinfo(*multicastAddr);
	}
	return -1;
}


/*
	Init a socket fd for reading multicast packet.

	Return socket fd for success, -1 for fail
*/
SOCKET init_recv_mcast_sockt(char* multicastIP, char* multicastPort)
{
	SOCKET sock = INVALID_SOCKET;
	struct addrinfo	  hints	 = { 0 };	 /* Hints for name lookup */
	struct addrinfo*  localAddr = 0;		 /* Local address to bind to */
	struct addrinfo*  multicastAddr = 0;	 /* Multicast Address */
	int yes=1;
	int result;

#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
	int tv = 30000; // 30 sec.
#else
	struct timeval tv;
	tv.tv_sec = 30;  // 30 Secs Timeout
	tv.tv_usec = 0;
#endif

	// Get the family of multicast address
	hints.ai_family = AF_INET; //PF_UNSPEC;
	hints.ai_flags	= AI_NUMERICHOST; // return a numerical network address
	result = getaddrinfo(multicastIP, NULL, &hints, &multicastAddr);
	SOCKET_NONZERO_GOTO_ERROR(result, "getaddrinfo failed 1");

	/*
	   Get a local address with the same family (IPv4 or IPv6) as our multicast group
	   It is port specified address.
	*/
	hints.ai_family	  = multicastAddr->ai_family;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags	  = AI_PASSIVE; // full the address of local machine, so we can bind it.
	result = getaddrinfo(NULL, multicastPort, &hints, &localAddr);
	SOCKET_NONZERO_GOTO_ERROR(result, "getaddrinfo failed 2");

	// Create socket for receiving datagrams
	sock = socket(localAddr->ai_family, localAddr->ai_socktype, 0);
	SOCKET_NOTVALID_GOTO_ERROR(sock, "socket() failed");

	// Enable SO_REUSEADDR to allow multiple instances of this application to receive copies of the multicast datagrams.
	result = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&yes,sizeof(int));
	SOCKET_RESULT_GOTO_ERROR(result, "SO_REUSEADDR setsockopt() failed");

	// set timeout to avoid tcp port is occupy by dead proccess
	result = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,(char*)&tv,sizeof(struct timeval));
	SOCKET_RESULT_GOTO_ERROR(result, "SO_RCVTIMEO setsockopt() failed");

	result = bind(sock, localAddr->ai_addr, localAddr->ai_addrlen);
	SOCKET_RESULT_GOTO_ERROR(result, "bind() failed");

	// Join the multicast group. We do this seperately depending on whether we are using IPv4 or IPv6.
	if ( multicastAddr->ai_family  == PF_INET &&
		 multicastAddr->ai_addrlen == sizeof(struct sockaddr_in) ) //IPv4
		{
			struct ip_mreq multicastRequest;  // Multicast address join structure

			// Specify the multicast group
			memcpy(&multicastRequest.imr_multiaddr,
				   &((struct sockaddr_in*)(multicastAddr->ai_addr))->sin_addr,
				   sizeof(multicastRequest.imr_multiaddr));

			// Accept multicast from any interface
			multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

			// Join the multicast address
			if ( setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &multicastRequest, sizeof(multicastRequest)) != 0 ) {
				LOGE("IP_ADD_MEMBERSHIP setsockopt() failed, errno=%d", errno);
				goto error;
			}
		}
	else if ( multicastAddr->ai_family	== PF_INET6 &&
			  multicastAddr->ai_addrlen == sizeof(struct sockaddr_in6) ) /* IPv6 */
		{
			struct ipv6_mreq multicastRequest;	/* Multicast address join structure */

			/* Specify the multicast group */
			memcpy(&multicastRequest.ipv6mr_multiaddr,
				   &((struct sockaddr_in6*)(multicastAddr->ai_addr))->sin6_addr,
				   sizeof(multicastRequest.ipv6mr_multiaddr));

			/* Accept multicast from any interface */
			multicastRequest.ipv6mr_interface = 0;

			/* Join the multicast address */
			if ( setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*) &multicastRequest, sizeof(multicastRequest)) != 0 ) {
				LOGE("IPV6_ADD_MEMBERSHIP setsockopt() failed, errno=%d", errno);
				goto error;
			}
		}
	else {
		LOGE("Neither IPv4 or IPv6, errno=%d", errno);
		goto error;
	}


	if(localAddr)
		freeaddrinfo(localAddr);
	if(multicastAddr)
		freeaddrinfo(multicastAddr);

	return sock;

 error:
	if (sock != INVALID_SOCKET) {
		close(sock);
	}
	if(localAddr)
		freeaddrinfo(localAddr);
	if(multicastAddr)
		freeaddrinfo(multicastAddr);

	return INVALID_SOCKET;
}

/*
	Read message from multicast network

	sock:
		multicast socket fd
	message:
		message buffer to read
	length:
		length limit of message
	addr:
		addr should be the ipv4(16) or ipv6(40)

	Return the length of mcast message that read, -1 for fail
*/
int read_mcast_message(SOCKET sock,
					   unsigned char* message,
					   int length,
					   char* addr)
{
	int bytes;
	struct sockaddr srcAddr;
	socklen_t addrlen = sizeof(struct sockaddr);
	struct sockaddr_in* addr4;
	struct sockaddr_in6* addr6;

/*
	// read total legnth
	bytes = recvfrom(sock, ((unsigned char*)&totalLength), sizeof(totalLength), 0, &srcAddr, &addrlen);
	SOCKET_RESULT_GOTO_ERROR(bytes, "recv(totalLength) failed");
	if (bytes != sizeof(totalLength)) { //  end of one mcast packet, but data is not complete
		return -1;
	}
*/

	// read data
	bytes = recvfrom(sock, message, length, 0, &srcAddr, &addrlen);
	SOCKET_RESULT_GOTO_ERROR(bytes, "recv(message) failed");
	if (bytes != length) { //  end of one mcast packet, but data is not complete
		LOGE("recv(message) length is not match");
		return -1;
	}

	// resolve addr
	if (addr) {
		if (addrlen == sizeof(struct sockaddr_in)) {
			addr4 = (struct sockaddr_in*) &srcAddr;
			inet_ntop(AF_INET, &(addr4->sin_addr), addr, INET_ADDRSTRLEN);
		} else if (addrlen == sizeof(struct sockaddr_in6)) {
			addr6 = (struct sockaddr_in6*) &srcAddr;
			inet_ntop(AF_INET, &(addr6->sin6_addr), addr, INET6_ADDRSTRLEN);
		}
	}
/*
	if (totalLength > length) { // read remainder
		LOGE("read_mcast_message: cipher size is not engouth, require %d", totalLength);
		recv(sock, message, totalLength-length, 0); // flush buffer
		return -1;
	}
*/
	return length;

error:
	return -1;
}

/*
	Send message to multicast network

	sock:
		multicast socket fd
	addr:
		multicast addr
	message:
		message buffer to send
	length
		length of message to be sent

	return 0 for success, -1 for fail
*/
int send_mcast_message(SOCKET sock,
					   struct addrinfo* addr,
					   const unsigned char* message,
					   int length)
{
	// send data
    if ( sendto(sock, message, length, 0,
                  addr->ai_addr, addr->ai_addrlen) != length )
	{
        LOGE("sendto(message) fail, errno=%d", errno);
		return -1;
	}
	return 0;
}


/*
	Read message from multicast network with simple protocol
	| data length | raw data |

	sock:
		multicast socket fd
	message:
		message buffer to read
	length:
		length limit of message
	addr:
		addr should be the ipv4(16) or ipv6(40)

	Return the length of mcast message that read, -1 for fail
*/
int read_mcast_message_ex(SOCKET sock,
						  unsigned char* message,
						  int length,
						  char* addr)
{
	int32_t dataLength;
	int bytes;
	struct sockaddr srcAddr;
	socklen_t addrlen = sizeof(struct sockaddr);
	struct sockaddr_in* addr4;
	struct sockaddr_in6* addr6;

	// read data legnth
	bytes = recvfrom(sock, ((unsigned char*)&dataLength), sizeof(dataLength), 0, &srcAddr, &addrlen);
	SOCKET_RESULT_GOTO_ERROR(bytes, "recvfrom(dataLength) failed");
	if (bytes != sizeof(dataLength)) { //  end of one mcast packet, but data is not complete
		LOGE("recvfrom(dataLength) length is not match, %d, %d", bytes, dataLength);
		return -1;
	}
	if (dataLength > length) {
		LOGE("read_mcast_message_ex: Buffer is not engouth, dataLength=%d", dataLength);
		return -1;
	}

	// read data
	bytes = recvfrom(sock, message, dataLength, 0, NULL, NULL);
	SOCKET_RESULT_GOTO_ERROR(bytes, "recvfrom(message) failed");
	if (bytes != dataLength) { //  end of one mcast packet, but data is not complete
		LOGE("recvfrom(message) length is not match, %d, %d", bytes, dataLength);
		return -1;
	}

	// resolve addr
	if (addr) {
		if (addrlen == sizeof(struct sockaddr_in)) {
			addr4 = (struct sockaddr_in*) &srcAddr;
			inet_ntop(AF_INET, &(addr4->sin_addr), addr, INET_ADDRSTRLEN);
		} else if (addrlen == sizeof(struct sockaddr_in6)) {
			addr6 = (struct sockaddr_in6*) &srcAddr;
			inet_ntop(AF_INET, &(addr6->sin6_addr), addr, INET6_ADDRSTRLEN);
		}
	}

	/*
	if (dataLength > length) { // read remainder
		LOGE("read_mcast_message_ex: cipher size is not engouth, require %d", dataLength);
		recv(sock, message, dataLength-length, 0); // flush buffer
		return -1;
	}
	*/

	return dataLength;

error:
	return -1;
}


/*
	Send message to multicast network with simple protocol
	| data length | raw data |

	sock:
		multicast socket fd
	addr:
		multicast addr
	message:
		message buffer to send
	length
		length of message to be sent

	return 0 for success, -1 for fail
*/
int send_mcast_message_ex(SOCKET sock,
						  struct addrinfo* addr,
						  const unsigned char* message,
						  int32_t length)
{
	int result;
	// send legnth
	if ( (result = sendto(sock, (unsigned char*) &length, sizeof(length), 0,
                  addr->ai_addr, addr->ai_addrlen) ) != sizeof(length) ) {
		SOCKET_RESULT_GOTO_ERROR(result, "sendto(length) fail");
		return -1;
	}

	// send data
    if ( (result = sendto(sock, message, length, 0,
                  addr->ai_addr, addr->ai_addrlen) ) != length )
	{
        LOGE("sendto(message) fail, errno=%d", errno);
		SOCKET_RESULT_GOTO_ERROR(result, "sendto(message) fail");
		return -1;
	}

	return 0;

error:
	return -1;
}