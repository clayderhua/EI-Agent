#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "aes.h"
#include "mcast-socket.h"

#define LOG_TAG "MCAST"
#include "Log.h"

static SOCKET g_socket;

static mbedtls_aes_context g_aesReadCtx;

#define LP_AES_KEY	0x86, 0x53, 0x8B, 0x25, 0x46, 0xCF, 0x35, 0x22, 0xE5, 0x65, 0xD8, 0x36, 0x78, 0x81, 0x66, 0xA4
#define LP_AES_IV 	0xB5, 0x88, 0x6D, 0x37, 0xF8, 0x77, 0x1F, 0xCC, 0x51, 0x12, 0x05, 0x19, 0xA0, 0x62, 0x79, 0x90

unsigned char g_aesKey[16] = { LP_AES_KEY };
unsigned char g_aesIV[16] = { LP_AES_IV };

static void print_usage(char* binName)
{
	LOGI("%s [multicast addr] [multicast port]", binName);
}

static int init(char *multicastIP, char *multicastPort)
{
	// init socket
	g_socket = init_recv_mcast_sockt(multicastIP, multicastPort);
	if (g_socket == INVALID_SOCKET) {
		LOGE("init_send_mcast_socket fail");
		return -1;
	}

	// init aes
	init_aes_decode(&g_aesReadCtx, g_aesKey, 128);

	return 0;
}

#ifdef ENCDOE_DATA_LENGTH

static int read_aes_mcast_message(SOCKET sock, unsigned char* message, int length, char* addr)
{
	unsigned char cipher[MAX_BUFFER_SIZE];
	int32_t cipherLength;
	int readLength;
	int rc;

	// read length
	readLength = read_mcast_message(sock, cipher, 16, addr);
	if (readLength != 16) {
		LOGE("read_tcp_message(cipherLength) fail, %d", readLength);
		return -1;
	}
	// decode length
	rc = aes_decode(&g_aesReadCtx, g_aesIV, cipher, message, readLength);
	if (rc == -1) {
		LOGE("aes_decode(cipherLength) fail");
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
	readLength = read_mcast_message(sock, cipher, (int) cipherLength, NULL);
	if (readLength != (int) cipherLength) {
		LOGE("read_tcp_message(cipher) fail");
		return -1;
	}

	return aes_decode(&g_aesReadCtx, g_aesIV, cipher, message, readLength);
}

#else // !ENCDOE_DATA_LENGTH

static int read_aes_mcast_message(SOCKET sock, unsigned char* message, int length, char* addr)
{
	unsigned char cipher[MAX_BUFFER_SIZE];
	int readLength;

	// read data
	readLength = read_mcast_message_ex(sock, cipher, sizeof(cipher), addr);
	if (readLength < 0) {
		LOGE("read_mcast_message_ex(cipher) fail");
		return -1;
	}
	if (readLength > length) {
		LOGE("read_aes_tcp_message: buffer is not enough, %d, %d", readLength, length);
		return -1;
	}

	return aes_decode(&g_aesReadCtx, g_aesIV, cipher, message, readLength);
}

#endif // ENCDOE_DATA_LENGTH

int main(int argc, char* argv[])
{
	char message[MAX_BUFFER_SIZE];
	int length;
	char* ptr;

	if (argc < 3) {
		print_usage(argv[0]);
		return 0;
	}

	if (init(argv[1], argv[2]) != 0) {
		return -1;
	}

	while (1) {
		LOGI("waiting packet...");
		length = read_aes_mcast_message(g_socket, (unsigned char*) message, sizeof(message), NULL);
		ptr = message;
		while (length > 0) {
			LOGD("receive packet [%s]", ptr);
			length -= (strlen(ptr)+1);
			ptr += strlen(ptr)+1;
			if (*ptr == '\0') { // following is the pendding dummy data
				break;
			}
		}
	}
	close(g_socket);

	return 0;
}