#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "aes.h"
#include "tcp-socket.h"
#include "lp.h"

#define READ_NEXT_STR(startPtr, curPtr, length) do {\
	if (curPtr == NULL) { curPtr = startPtr; } \
	else { \
		curPtr += strlen(curPtr) + 1;\
		if ((curPtr - startPtr) >= length || *curPtr == '\0') { curPtr = NULL; }\
	}\
} while (0)

SOCKET g_sock;
static mbedtls_aes_context g_aesReadCtx;

unsigned char g_aesKey[16] = { LP_AES_KEY };
unsigned char g_aesIV[16] = { LP_AES_IV };

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

static int init(char* serverIP, char *serverPort)
{
	// init socket
	g_sock = init_server_tcp_socket(serverIP, serverPort);
	if (g_sock == INVALID_SOCKET) {
		LOGE("init_server_tcp_socket fail");
		return -1;
	}

	// init aes
	init_aes_decode(&g_aesReadCtx, g_aesKey, 128);

	return 0;
}

/*
	Read and decrypt aes tcp message

	return the bytes number of message.
	return -1, read failed
*/
#ifdef ENCDOE_DATA_LENGTH
static int read_aes_tcp_message(SOCKET localSocket, unsigned char* message, int length, char* addr)
{
	SOCKET sock;
	int32_t cipherLength;
	int readLength;
	unsigned char cipher[MAX_BUFFER_SIZE];
	struct sockaddr clientAddr;
	socklen_t addrlen = sizeof(struct sockaddr);
	struct sockaddr_in* addr4;
	struct sockaddr_in6* addr6;
	int rc;

	while (1) {
		sock = accept(localSocket, (struct sockaddr *) &clientAddr, &addrlen);
		if (sock != INVALID_SOCKET || errno != EAGAIN) {
			break;
		}
		usleep(100);
	}
	SOCKET_NOTVALID_GOTO_ERROR(sock, "accept fail");

	// resolv addr
	if (addr) {
		if (addrlen == sizeof(struct sockaddr_in)) {
			addr4 = (struct sockaddr_in*) &clientAddr;
			inet_ntop(AF_INET, &(addr4->sin_addr), addr, INET_ADDRSTRLEN);
		} else if (addrlen == sizeof(struct sockaddr_in6)) {
			addr6 = (struct sockaddr_in6*) &clientAddr;
			inet_ntop(AF_INET, &(addr6->sin6_addr), addr, INET6_ADDRSTRLEN);
		}
	}

	// read legnth
	readLength = read_tcp_message(sock, cipher, 16); // 16 is one AES block
	if (readLength != 16) {
		LOGE("read_tcp_message(cipherLength) fail, %d", readLength);
		close(sock);
		return -1;
	}
	// decode length
	rc = aes_decode(&g_aesReadCtx, g_aesIV, cipher, message, (int) readLength);
	if (rc == -1) {
		LOGE("aes_decode(cipherLength) fail");
		close(sock);
		return -1;
	}
	// copy length
	memcpy(&cipherLength, message, sizeof(cipherLength));
	if (cipherLength > sizeof(cipher) || cipherLength > length) {
		LOGE("buffer is not enough, %d, %ld %d", cipherLength, sizeof(cipher), length);
		close(sock);
		return -1;
	}

	// read data
	readLength = read_tcp_message(sock, cipher, (int) cipherLength);
	close(sock);
	if (readLength != (int) cipherLength) {
		LOGE("read_tcp_message(cipher) fail");
		return -1;
	}
	// decode data
	return aes_decode(&g_aesReadCtx, g_aesIV, cipher, message, (int) readLength);

error:
	return -1;
}

#else // !ENCDOE_DATA_LENGTH

static int read_aes_tcp_message(SOCKET localSocket, unsigned char* message, int length, char* addr)
{
	SOCKET sock;
	int readLength;
	unsigned char cipher[MAX_BUFFER_SIZE];
	struct sockaddr clientAddr;
	socklen_t addrlen = sizeof(struct sockaddr);
	struct sockaddr_in* addr4;
	struct sockaddr_in6* addr6;

	while (1) {
		sock = accept(localSocket, (struct sockaddr *) &clientAddr, &addrlen);
		if (sock != INVALID_SOCKET || errno != EAGAIN) {
			break;
		}
		usleep(100);
	}
	SOCKET_NOTVALID_GOTO_ERROR(sock, "accept fail");

	// resolv addr
	if (addr) {
		if (addrlen == sizeof(struct sockaddr_in)) {
			addr4 = (struct sockaddr_in*) &clientAddr;
			inet_ntop(AF_INET, &(addr4->sin_addr), addr, INET_ADDRSTRLEN);
		} else if (addrlen == sizeof(struct sockaddr_in6)) {
			addr6 = (struct sockaddr_in6*) &clientAddr;
			inet_ntop(AF_INET, &(addr6->sin6_addr), addr, INET6_ADDRSTRLEN);
		}
	}

	// read data
	readLength = read_tcp_message_ex(sock, cipher, sizeof(cipher));
	close(sock);
	if (readLength < 0) {
		LOGE("read_tcp_message_ex(cipher) fail");
		return -1;
	}
	if (readLength > length) {
		LOGE("read_aes_tcp_message: buffer is not enough, %d, %d", readLength, length);
		return -1;
	}

	// decode data
	return aes_decode(&g_aesReadCtx, g_aesIV, cipher, message, (int) readLength);

error:
	return -1;
}

#endif // ENCDOE_DATA_LENGTH


static int recv_discoer_ack(char* message, int length, char* addr)
{
	char* ptr = NULL, *deviceID;

	// function
	READ_NEXT_STR(message, ptr, length);
	// device id
	READ_NEXT_STR(message, ptr, length);
	
	deviceID = ptr;
	printf("recv_discoer_ack: addr=%s, deviceID=%s\n", addr, deviceID);

	return 0;
}

static int do_socket(fd_set* rfds)
{
	int length = 0;
	char buffer[MAX_BUFFER_SIZE];
	char addr[40];

	if (g_sock == INVALID_SOCKET || !FD_ISSET(g_sock, rfds)) {
		LOGE("do_socket fail, g_sock is %s", (g_sock == INVALID_SOCKET)? "invalid": "valid");
		return -1;
	}
	
	length = read_aes_tcp_message(g_sock, (unsigned char*) buffer, sizeof(buffer), addr);
	if (length <= 0) { // decrypt fail
		LOGE("read_aes_tcp_message fail");
		return -1;
	}
	
	if (strcmp(buffer, "discover-ack") == 0) {
		recv_discoer_ack(buffer, sizeof(buffer), addr);
	} else {
		LOGE("do_socket: unknow command [%s]", buffer);
		return -1;
	}
	
	return 0;
}

int main(int argc, char* argv[])
{
	// socket
	int ret;
	fd_set rfds, rfdsCopy;
	int fdmax;
	struct timeval tv;

	if (init(NULL, LP_TCP_SERVER_PORT)) {
		LOGE("init fail");
		return -1;
	}
	
	// init select
	FD_ZERO(&rfds);
	FD_SET(g_sock, &rfds);
	fdmax = g_sock;
	rfdsCopy = rfds;
	
	if (argc >= 2) {
		tv.tv_sec = strtol(argv[1], NULL, 10);
	} else {
		tv.tv_sec = 10; // default 10 sec.
	}
	tv.tv_usec = 0;
	
	while (1) {
		LOGD("loop waiting, tv_sec=%ld, tv_usec=%ld..............................", tv.tv_sec, tv.tv_usec);
		ret = select(fdmax+1, &rfds, NULL, NULL, &tv);
		
		if (ret == -1) {
			LOGE("select failed: errno=%d", errno);
			break;
		} else if (ret == 0) { // timeout, end
			break;
		} else { // socket event
			do_socket(&rfds);
		}
		// cacluate timeout time
		rfds = rfdsCopy;
	}

	close(g_sock);
	LOGI("done");

	return 0;
}

