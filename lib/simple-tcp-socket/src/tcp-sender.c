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
static mbedtls_aes_context g_aesSendCtx;

unsigned char g_aesKey[16] = { LP_AES_KEY };
unsigned char g_aesIV[16] = { LP_AES_IV };

static void print_usage(char* binName)
{
	LOGI("%s [server addr] [server port] [message1] [message2] ...", binName);
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

static int init(char *serverIP, char *serverPort)
{
	// init socket
	g_sock = init_client_tcp_socket(serverIP, serverPort);
	if (g_sock == -1) {
		LOGE("init_client_tcp_socket fail");
		return -1;
	}

	// init aes
	init_aes_encode(&g_aesSendCtx, g_aesKey, 128);

	return 0;
}

#ifdef ENCDOE_DATA_LENGTH
static int send_aes_tcp_message(SOCKET sock, const unsigned char* message, int length)
{
	unsigned char cipher[MAX_BUFFER_SIZE];
	unsigned char cipherLength[16]; // fix size 16 for AES block
	int aesMsgLength, aesLength;
	int32_t sendLength;

	if (length > sizeof(cipher)) {
		LOGE("send_aes_tcp_message: cipher buffer is not enough, %d, %d", length, sizeof(cipher));
		return -1;
	}

	// encode message
	memset(cipher, 0, sizeof(cipher));
	aesMsgLength = aes_encode(&g_aesSendCtx, g_aesIV, message, length, cipher, sizeof(cipher));
	if (aesMsgLength == -1) {
		return -1;
	}

	// encode length, fix size 16
	memset(cipherLength, 0, sizeof(cipherLength));
	sendLength = (int32_t) aesMsgLength;
	aesLength = aes_encode(&g_aesSendCtx, g_aesIV, (unsigned char*) &sendLength, sizeof(sendLength), cipherLength, sizeof(cipherLength));
	if (aesLength == -1) {
		return -1;
	}

	// send length
	if (send_tcp_message(sock, cipherLength, (int32_t) aesLength) == -1) {
		LOGE("send_tcp_message(sendLength) fail");
		return -1;
	}
	// send data
	return send_tcp_message(sock, cipher, (int32_t) aesMsgLength);
}

#else // !ENCDOE_DATA_LENGTH

static int send_aes_tcp_message(SOCKET sock, const unsigned char* message, int length)
{
	unsigned char cipher[MAX_BUFFER_SIZE];
	int32_t aesMsgLength;

	if (length > sizeof(cipher)) {
		LOGE("send_aes_tcp_message: cipher buffer is not enough, %d, %ld", length, sizeof(cipher));
		return -1;
	}

	// encode message
	memset(cipher, 0, sizeof(cipher));
	aesMsgLength = (int32_t) aes_encode(&g_aesSendCtx, g_aesIV, message, length, cipher, sizeof(cipher));
	if (aesMsgLength == -1) {
		LOGE("send_aes_tcp_message: aes_encode fail");
		return -1;
	}

	return send_tcp_message_ex(sock, cipher, (int32_t) aesMsgLength);
}
#endif // ENCDOE_DATA_LENGTH

int main(int argc, char* argv[])
{
	// socket
	unsigned char buffer[MAX_BUFFER_SIZE];
	unsigned char* ptr;
	int length = 0, totalLength = 0;
	int i, rc;

	if (argc < 4) {
		print_usage(argv[0]);
		return 0;
	}

	// check input limit
	ptr = buffer;
	for (i = 3; i < argc; i++) {
		length = strlen(argv[i])+1;
		totalLength += length;
		if (totalLength > MAX_BUFFER_SIZE) {
			LOGE("send buffer is not engouth...");
			return -1;
		}
		memcpy(ptr, argv[i], length);
		ptr += length;
	}

	if (init(argv[1], argv[2]) != 0) {
		return -1;
	}

	rc = send_aes_tcp_message(g_sock, buffer, totalLength);
	close(g_sock);

	LOGI("done, rc=%d", rc);

	return 0;
}