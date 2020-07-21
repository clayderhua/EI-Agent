#ifndef __TCP_SOCKET__
#define __TCP_SOCKET__

#include <stdint.h>
#include "common-socket.h"

#ifdef __cplusplus
extern "C" {
#endif

SOCKET init_server_tcp_socket(char* ip, char* port);
SOCKET init_client_tcp_socket(const char* serverIP, const char* serverPort);
int send_tcp_message(SOCKET sock, const unsigned char* message, int length);
int read_tcp_message(SOCKET socket, unsigned char* message, int length);
int send_tcp_message_ex(SOCKET sock, const unsigned char* message, int length);
int read_tcp_message_ex(SOCKET socket, unsigned char* message, int length);

#ifdef __cplusplus
}
#endif

#endif