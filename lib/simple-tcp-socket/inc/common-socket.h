#ifndef __COMMON_SOCKET__
#define __COMMON_SOCKET__

#define SOCKET int

#ifdef WIN32
	#undef SOCKET
	#undef socklen_t
	//#define WINVER 0x0501
	#include <ws2tcpip.h>
	#define EWOULDBLOCK WSAEWOULDBLOCK
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
		if (result == SOCKET_ERROR) {\
			LOGE("%s: " msg ", WSAGetLastError=%d", __FUNCTION__, WSAGetLastError());\
			goto error;\
		}\
	} while (0)

	#define SOCKET_NONZERO_GOTO_ERROR(result, msg) do {\
		if (result != 0) {\
			LOGE("%s: " msg ", WSAGetLastError=%d", __FUNCTION__, WSAGetLastError());\
			goto error;\
		}\
	} while (0)

	#define SOCKET_NOTVALID_GOTO_ERROR(sock, msg) do {\
		if (sock == INVALID_SOCKET) {\
			LOGE("%s: " msg ", WSAGetLastError=%d", __FUNCTION__, WSAGetLastError());\
			goto error;\
		}\
	} while (0)
#else
	#define SOCKET_RESULT_GOTO_ERROR(result, msg) do {\
		if (result == -1) {\
			LOGE("%s: " msg ", errno=%d", __FUNCTION__, errno);\
			goto error;\
		}\
	} while (0)

	#define SOCKET_NONZERO_GOTO_ERROR(result, msg) do {\
		if (result != 0) {\
			LOGE("%s: " msg ", errno=%d", __FUNCTION__, errno);\
			goto error;\
		}\
	} while (0)

	#define SOCKET_NOTVALID_GOTO_ERROR(sock, msg) do {\
		if (sock == INVALID_SOCKET) {\
			LOGE("%s: " msg ", errno=%d", __FUNCTION__, errno);\
			goto error;\
		}\
	} while (0)
#endif


#endif