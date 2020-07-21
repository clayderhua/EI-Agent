#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "aes.h"
#include "lp.h"
#include "tcp-socket.h"

#define LOG_TAG "STCP"
#include "Log.h"

SOCKET g_sock;
static mbedtls_aes_context g_aesReadCtx;

unsigned char g_aesKey[16] = { LP_AES_KEY };
unsigned char g_aesIV[16] = { LP_AES_IV };

static void print_usage(char* binName)
{
	LOGI("%s [server addr] [server port]", binName);
}

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

	sock = accept(localSocket, (struct sockaddr *) &clientAddr, &addrlen);
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

	sock = accept(localSocket, (struct sockaddr *) &clientAddr, &addrlen);
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


int main(int argc, char* argv[])
{
	// socket
	unsigned char buffer[MAX_BUFFER_SIZE];
	int length = 0;
	char addr[40];

	if (argc < 3) {
		print_usage(argv[0]);
		return 0;
	}

	if (init(argv[1], argv[2])) {
		LOGE("init fail");
		return -1;
	}

	while (1) {
		LOGD("waiting for packet...");
		length = read_aes_tcp_message(g_sock, buffer, sizeof(buffer), addr);
		if (length <= 0) { // decrypt fail
			LOGE("read_aes_tcp_message fail");
			return -1;
		}
		LOGD("receive message [%s][%s]", addr, buffer);
	}

	LOGI("done");

	return 0;
}