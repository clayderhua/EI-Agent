#ifndef __UDP_SOCKET_H__
#define __UDP_SOCKET_H__

#include <stdint.h>

#include "common-socket.h"

#ifdef __cplusplus
extern "C" {
#endif

SOCKET init_server_udp_socket(char* ip, int port, int is_nonblocking, struct addrinfo **listenAddr);
SOCKET init_client_udp_socket(const char* serverIP, int serverPort, int is_nonblocking, struct addrinfo **serverAddr);
int read_udp_message(SOCKET sock, unsigned char* message, int length);
int send_udp_message(SOCKET sock, const unsigned char* message, int length, struct addrinfo *serverAddr);
int send_udp_message_ex(SOCKET sock,
						unsigned char* buf,
						int32_t buflen,
						const unsigned char* message,
						int32_t length,
						struct addrinfo *serverAddr);
int read_udp_message_ex(SOCKET sock, 
						unsigned char* message,
						int32_t length);

#ifdef __cplusplus
}
#endif

#endif