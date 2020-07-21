#ifndef __MCAST_SOCKET__
#define __MCAST_SOCKET__

#include <stdint.h>
#include "common-socket.h"

#define MAX_BUFFER_SIZE          3000    // DevID + CredentialURL + IoTKey + TLSType + CAFile + CAPath + CertFile + KeyFile + CertPW
#define LP_MUTICAST_TTL          60

#ifdef __cplusplus
extern "C" {
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
SOCKET init_send_mcast_socket(char* multicastIP, char* multicastPort, struct addrinfo **multicastAddr);

/*
	Init a socket fd for reading multicast packet.

	Return socket fd for success, -1 for fail
*/
SOCKET init_recv_mcast_sockt(char* multicastIP, char* multicastPort);


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
					   char* addr);

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
					   int length);


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
						  char* addr);

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
						  int32_t length);

#ifdef __cplusplus
}
#endif

#endif