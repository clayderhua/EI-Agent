#ifndef __COMMON_SOCKET__
#define __COMMON_SOCKET__

#define SOCKET int

#ifdef WIN32
	#undef SOCKET
	#undef socklen_t
	//#define WINVER 0x0501
	#include <winsock2.h>
	#include <ws2tcpip.h>
	//#include <windows.h>
	//#define EWOULDBLOCK WSAEWOULDBLOCK
	#define close closesocket
	#define socklen_t int
	typedef unsigned int in_addr_t;
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/un.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#define INVALID_SOCKET -1
#endif


#ifdef WIN32
	#define SOCKET_RESULT_GOTO_ERROR(result, msg) do {\
		if (result == SOCKET_ERROR) { \
			if (WSAGetLastError() == WSAEWOULDBLOCK) { result = 0; } \
			else { \
				fprintf(stderr, "%s: " msg ", WSAGetLastError=%d\n", __FUNCTION__, WSAGetLastError()); \
				goto error; \
			} \
		}\
	} while (0)

	#define SOCKET_NONZERO_GOTO_ERROR(result, msg) do {\
		if (result != 0) {\
			fprintf(stderr, "%s: " msg ", WSAGetLastError=%d\n", __FUNCTION__, WSAGetLastError());\
			goto error;\
		}\
	} while (0)

	#define SOCKET_NOTVALID_GOTO_ERROR(sock, msg) do {\
		if (sock == INVALID_SOCKET) {\
			fprintf(stderr, "%s: " msg ", WSAGetLastError=%d\n", __FUNCTION__, WSAGetLastError());\
			goto error;\
		}\
	} while (0)
#else
	#define SOCKET_RESULT_GOTO_ERROR(result, msg) do {\
		if (result == -1) {\
			if (errno == EAGAIN) { result = 0; } \
			else { \
				fprintf(stderr, "%s: " msg ", errno=%d\n", __FUNCTION__, errno);\
				goto error;\
			} \
		} \
	} while (0)

	#define SOCKET_NONZERO_GOTO_ERROR(result, msg) do {\
		if (result != 0) {\
			fprintf(stderr, "%s: " msg ", errno=%d\n", __FUNCTION__, errno);\
			goto error;\
		}\
	} while (0)

	#define SOCKET_NOTVALID_GOTO_ERROR(sock, msg) do {\
		if (sock == INVALID_SOCKET) {\
			fprintf(stderr, "%s: " msg ", errno=%d\n", __FUNCTION__, errno);\
			goto error;\
		}\
	} while (0)
#endif


#endif