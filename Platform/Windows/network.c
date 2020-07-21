/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2016/01/22 by Scott Chang								    */
/* Modified Date: 2016/01/22 by Scott Chang									*/
/* Abstract     : Cross platform Network API definition	for Windows			*/
/* Reference    : None														*/
/****************************************************************************/
#include "network.h"
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <ws2def.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

static unsigned char g_cacheMAC[6];
static unsigned char g_dockerMAC[6];

void network_init(void)
{
	//Add code
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
}

int network_close(socket_handle network_handle)
{
	struct linger ling_opt;

	ling_opt.l_linger = 1;
	ling_opt.l_onoff  = 1;
	setsockopt(network_handle, SOL_SOCKET, SO_LINGER, (char*)&ling_opt, sizeof(ling_opt) );
	return closesocket(network_handle);
}

int network_connect(char const * const host_name, unsigned int host_port, socket_handle *network_handle)
{
	int iRet = -1;
	//Add code
	int sock = INVALID_SOCKET;
	struct addrinfo hints;
	struct addrinfo *ainfo = NULL, *rp = NULL;
	int s;
	u_long val = 1;
	if(host_name == NULL) return iRet;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_flags = AI_ADDRCONFIG;
	hints.ai_socktype = SOCK_STREAM;

	s = getaddrinfo(host_name, NULL, &hints, &ainfo);
	if(s)
	{
		return iRet;
	}
	for(rp = ainfo; rp != NULL; rp = rp->ai_next)
	{
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock == INVALID_SOCKET) continue;

		if(rp->ai_family == PF_INET)
		{
			((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(host_port);
		}
		else if(rp->ai_family == PF_INET6)
		{
			((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(host_port);
		}else{
			continue;
		}
		if(connect(sock, rp->ai_addr, rp->ai_addrlen) != -1){
			break;
		}
		//shutdown(sock, SD_BOTH);
		network_close(sock);
		//closesocket(sock);
	}
	if(!rp){
		goto done;
	}

	if(ioctlsocket(sock, FIONBIO, &val)){
		network_close(sock);
		//closesocket(sock);
		goto done;
	}

	*network_handle = sock;
	iRet = 0;
done:
	freeaddrinfo(ainfo);
	return iRet;
}


network_status_t network_waitsock(socket_handle network_handle, int mode, int timeoutms)
{
	int waitret = 0;
	struct timeval timeout;
	int rc = -1;
	fd_set readfd, writefd;
	if(timeoutms >= 0)
	{
		timeout.tv_sec = timeoutms/1000;
		timeout.tv_usec = (timeoutms%1000) * 1000;
	}
	else
	{
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
	}

	FD_ZERO(&readfd);
	FD_ZERO(&writefd);

	if (mode & network_waitsock_read)
		FD_SET(network_handle, &readfd);

	if (mode & network_waitsock_write)
		FD_SET(network_handle, &writefd);

	rc = select(network_handle+1, &readfd, &writefd, NULL, &timeout);

	if (-1 == rc)
	{
		return network_waitsock_error;
	}

	if (FD_ISSET(network_handle, &readfd))
	{
		waitret |= network_waitsock_read;
	}
	if (FD_ISSET(network_handle, &writefd))
	{
		waitret |= network_waitsock_write;
	}
	if(waitret > 0) return (network_status_t)waitret;
	return network_waitsock_timeout;
}

int network_send(socket_handle network_handle, char const * sendbuffer, unsigned int len)
{
	int iRet = 0;
	//Add code
	iRet = send(network_handle, (char *)sendbuffer, len, 0);

	return iRet;
}

int network_recv(socket_handle network_handle, char * recvbuffer, unsigned int len)
{
	return recv(network_handle, (char *)recvbuffer, (int)len, 0);
}

void network_cleanup(void)
{
	//Add code
	//WSACleanup();
}

WISEPLATFORM_API int network_host_name_get(char * phostname, int size)
{
	int iRet = -1;
	char hostName[256] = {0};
	if(phostname == NULL) return iRet;
	network_init();
	iRet = gethostname(hostName, size);
	network_cleanup();
	if(!iRet)
	{
		strcpy(phostname, hostName);
	}
	return iRet;
}

WISEPLATFORM_API int network_ip_get(char * pipaddr, int size)
{
	int iRet = -1;
	char hostName[256] = {0};
	if(pipaddr == NULL) return iRet;
	network_init();
	iRet = gethostname(hostName, sizeof(hostName));
	if(!iRet)
	{
		struct in_addr addr;
		struct hostent *phe = gethostbyname(hostName);
		if (phe == 0) {
			iRet = -1;
			return iRet;
		}
		if(phe->h_addr_list[0] == 0) {
			iRet = -1;
			return iRet;
		}
		memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr));
		strcpy(pipaddr, inet_ntoa(addr));
		iRet = 0;
	}
	return iRet;
}

WISEPLATFORM_API int network_ip_list_get(char ipsStr[][16], int n)
{
	int iRet = 0;
	char hostName[256] = { 0 };
	if (ipsStr == NULL || n == 0) return iRet;
	network_init();
	iRet = gethostname(hostName, sizeof(hostName));
	if (!iRet)
	{
		int index = 0;
		int cnt = 0;
		struct in_addr addr;
		struct hostent* phe = gethostbyname(hostName);
		if (phe == 0) {
			iRet = 0;
			return iRet;
		}

		for (cnt = 0; cnt < phe->h_length; cnt++)
		{
			if (phe->h_addr_list[index] != 0) {
				memcpy(&addr, phe->h_addr_list[index], sizeof(struct in_addr));
				strcpy(ipsStr[index], inet_ntoa(addr));
				index++;
			}
			if (index >= n)
				break;
		}
		iRet = index;
	}
	return iRet;
}

// refer to OUI table, https://code.wireshark.org/review/gitweb?p=wireshark.git;a=blob_plain;f=manuf
static int isValidMAC(unsigned char* macAddr)
{
	int i = 0, ret = 1, limit;
	unsigned char emptyMAC[6] = {0};
	unsigned int macInt = (macAddr[2] << 16) | (macAddr[1] << 8) | macAddr[0];
	unsigned int invalidList[] = {
		0x000000,        //No Mac, lo
		0xFFFFFF, //No Mac
		0x888888        //Intel No Mac
	};

	unsigned int vmList[] = {
		0x690500,        //vmware
		0x290C00,        //vmware
		0x565000,        //vmware
		0x141C00,        //vmware
		0x421C00,        //parallels
		0xFF0300,         //microsoft virtual pc
		0x4B0F00,        //virtual iron
		0x3E1600,        //Red Hat xen , Oracle vm , xen source, novell xen
		0x270008,        //virtualbox
		0x000000
	};

	unsigned int dockerList[] = {
		0x4202,
		0x4102
	};

	limit = sizeof(invalidList)/sizeof(unsigned int);
	for (i = 0; i < limit; i++) {
		if (macInt == invalidList[i]) {
			return 0;
		}
	}

	limit = sizeof(vmList)/sizeof(unsigned int);
	for (i = 0; i < limit; i++) {
		if (macInt == vmList[i]) {
			if (memcmp(g_cacheMAC, emptyMAC, sizeof(g_cacheMAC)) == 0) { // no cache
				// cache VM MAC if real mac is not found, we can refer to it.
				memcpy(g_cacheMAC, macAddr, sizeof(g_cacheMAC));
			}
			ret = 0;
		}
	}

	// check docker
	macInt = (macAddr[1] << 8) | macAddr[0];
	limit = sizeof(dockerList)/sizeof(unsigned int);
	for (i = 0; i < limit; i++) {
		if (macInt == dockerList[i]) {
			if (memcmp(g_dockerMAC, emptyMAC, sizeof(g_dockerMAC)) == 0) { // no cache
				// cache docker MAC if real mac is not found, we can refer to it.
				memcpy(g_dockerMAC, macAddr, sizeof(g_dockerMAC));
			}
			ret = 0;
		}
	}

	return ret;
}

WISEPLATFORM_API int network_mac_get(char * macstr)
{
	// Use IPHlpApi
	int iRet = -1;
#ifdef MULTI_AGENT_RUN
	if(macstr == NULL) return iRet;
	memcpy(macstr, MultiAgentMac, strlen(MultiAgentMac) + 1);
	iRet = 0;
#else
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	PIP_ADAPTER_INFO pAdInfo = NULL;
	ULONG            ulSizeAdapterInfo = 0;
	DWORD            dwStatus;

	MIB_IFROW MibRow = {0};
	ulSizeAdapterInfo = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);

	if (GetAdaptersInfo( pAdapterInfo, &ulSizeAdapterInfo) != ERROR_SUCCESS)
	{
		free (pAdapterInfo);
		pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);
	}

	dwStatus = GetAdaptersInfo(pAdapterInfo, &ulSizeAdapterInfo);

	if(dwStatus != ERROR_SUCCESS)
	{
		free(pAdapterInfo);
		return  iRet;
	}

	pAdInfo = pAdapterInfo;
	while(pAdInfo)
	{
		memset(&MibRow, 0, sizeof(MIB_IFROW));
		MibRow.dwIndex = pAdInfo->Index;
		MibRow.dwType = pAdInfo->Type;

		if(GetIfEntry(&MibRow) == NO_ERROR)
		{
			//          if (MibRow.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL &&
			//             strlen(pAdInfo->GatewayList.IpAddress.String) != 0)
			if (MibRow.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
			{

				sprintf(macstr, "%02X:%02X:%02X:%02X:%02X:%02X",
					MibRow.bPhysAddr[0],
					MibRow.bPhysAddr[1],
					MibRow.bPhysAddr[2],
					MibRow.bPhysAddr[3],
					MibRow.bPhysAddr[4],
					MibRow.bPhysAddr[5]);

				iRet = 0;
				break;
			}
		}
		pAdInfo = pAdInfo->Next;
	}
	if(pAdapterInfo)free(pAdapterInfo);
#endif
	return iRet;
}


WISEPLATFORM_API int network_mac_get_ex(char * macstr)
{
	// Use IPHlpApi
	int iRet = -1;
#ifdef MULTI_AGENT_RUN

	if(macstr == NULL) return iRet;
	memcpy(macstr, MultiAgentMac, strlen(MultiAgentMac) + 1);
	iRet = 0;
#else
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	PIP_ADAPTER_INFO pAdInfo = NULL;
	ULONG            ulSizeAdapterInfo = 0;
	DWORD            dwStatus;
	unsigned char* macPtr = NULL;
	unsigned char emptyMAC[6] = {0};

	MIB_IFROW MibRow = {0};
	ulSizeAdapterInfo = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);


	if (GetAdaptersInfo( pAdapterInfo, &ulSizeAdapterInfo) != ERROR_SUCCESS)
	{
		free (pAdapterInfo);
		pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);
	}

	dwStatus = GetAdaptersInfo(pAdapterInfo, &ulSizeAdapterInfo);

	if(dwStatus != ERROR_SUCCESS)
	{
		free(pAdapterInfo);
		return  iRet;
	}

	pAdInfo = pAdapterInfo;
	while(pAdInfo)
	{
		memset(&MibRow, 0, sizeof(MIB_IFROW));
		MibRow.dwIndex = pAdInfo->Index;
		MibRow.dwType = pAdInfo->Type;

		if(GetIfEntry(&MibRow) == NO_ERROR)
		{
			//          if (MibRow.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL &&
			//             strlen(pAdInfo->GatewayList.IpAddress.String) != 0)
			if (MibRow.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL && isValidMAC(MibRow.bPhysAddr))
			{
				sprintf(macstr, "%02X%02X%02X%02X%02X%02X",
					MibRow.bPhysAddr[0],
					MibRow.bPhysAddr[1],
					MibRow.bPhysAddr[2],
					MibRow.bPhysAddr[3],
					MibRow.bPhysAddr[4],
					MibRow.bPhysAddr[5]);

				iRet = 0;
				break;
			}
		}
		pAdInfo = pAdInfo->Next;
	}
	if(pAdapterInfo)free(pAdapterInfo);

	if (macstr[0] == '\0') { // no mac found
		if (memcmp(emptyMAC, g_cacheMAC, sizeof(g_cacheMAC)) != 0) { // has cache
			macPtr = g_cacheMAC;
		} else if (memcmp(emptyMAC, g_dockerMAC, sizeof(g_dockerMAC)) != 0) { // has cache
			macPtr = g_dockerMAC;
		}
		if (macPtr) {
			sprintf(macstr,"%02X%02X%02X%02X%02X%02X",
						(unsigned char) macPtr[0],
						(unsigned char) macPtr[1],
						(unsigned char) macPtr[2],
						(unsigned char) macPtr[3],
						(unsigned char) macPtr[4],
						(unsigned char) macPtr[5]);
			iRet = 0;
		}
	}

#endif
	return iRet;
}

WISEPLATFORM_API int network_mac_list_get(char macsStr[][20], int n)
{
	int iRet = 0;
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdInfo = NULL;
	ULONG            ulSizeAdapterInfo = 0;
	DWORD            dwStatus;

	MIB_IFROW MibRow = {0};
	int macIndex = 0;
	if(n <= 0) return iRet;
	ulSizeAdapterInfo = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);


	if (GetAdaptersInfo( pAdapterInfo, &ulSizeAdapterInfo) != ERROR_SUCCESS)
	{
		free (pAdapterInfo);
		pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);
	}

	dwStatus = GetAdaptersInfo(pAdapterInfo,   &ulSizeAdapterInfo);

	if(dwStatus != ERROR_SUCCESS)
	{
		free(pAdapterInfo);
		return  iRet;
	}

	pAdInfo = pAdapterInfo;
	while(pAdInfo)
	{
		if(pAdInfo->Type != MIB_IF_TYPE_ETHERNET && pAdInfo->Type != IF_TYPE_IEEE80211)
		{
			pAdInfo = pAdInfo->Next;
			continue;
		}

		memset(&MibRow, 0, sizeof(MIB_IFROW));
		MibRow.dwIndex = pAdInfo->Index;
		MibRow.dwType = pAdInfo->Type;

		if(GetIfEntry(&MibRow) == NO_ERROR)
		{
			//if (MibRow.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
			if ((MibRow.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL || MibRow.dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED) && isValidMAC(MibRow.bPhysAddr))
			{

				sprintf(macsStr[macIndex], "%02X:%02X:%02X:%02X:%02X:%02X",
					MibRow.bPhysAddr[0],
					MibRow.bPhysAddr[1],
					MibRow.bPhysAddr[2],
					MibRow.bPhysAddr[3],
					MibRow.bPhysAddr[4],
					MibRow.bPhysAddr[5]);
				macIndex++;
				if(macIndex >= n) break;
			}
		}
		pAdInfo = pAdInfo->Next;
	}
	free(pAdapterInfo);
	iRet = macIndex;
	return iRet;
}

WISEPLATFORM_API int network_local_ip_get(int socket, char* clientip, int size)
{
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	//int res = getpeername(socket, (struct sockaddr *)&addr, &addr_size); //server IP
	int res = getsockname(socket, (struct sockaddr *)&addr, &addr_size);
	if(res == 0)
	{
		strcpy(clientip, inet_ntoa(addr.sin_addr));
	}
	return res;
}

WISEPLATFORM_API int send_broadcast_packet(ULONG addr_long, unsigned char* packet, int packet_len)
{
	SOCKET sockfd;
	struct sockaddr_in addr;
	int optVal = 1;
	int iRet = -1;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd > 0)
	{
		iRet = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&optVal, sizeof(optVal));
		memset((void*)&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(9);
		addr.sin_addr.s_addr = addr_long;
		iRet = sendto(sockfd, (const char *)packet, packet_len, 0, (struct sockaddr *)&addr, sizeof(addr));
		if (iRet == -1) {
			fprintf(stderr, "sendto fail...errno=%d\n", errno);
		}
		else {
			printf("sendto, iRet=%d\n", iRet);
		}
		closesocket(sockfd);
	}
	WSACleanup();
	return iRet;
}

#ifdef WINXP
WISEPLATFORM_API bool network_magic_packet_send(char * mac, int size)
{
	bool bRet = false;
	if(size < 6) return bRet;
	{
		unsigned char packet[102];
		struct sockaddr_in addr;
		SOCKET sockfd;
		int i = 0, j = 0;
		bool optVal = true;
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		for(i=0;i<6;i++)  
		{
			packet[i] = 0xFF;  
		}
		for(i=1;i<17;i++)
		{
			for(j=0;j<6;j++)
			{
				packet[i*6+j] = mac[j];
			}
		}

		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(sockfd > 0)
		{
			int iRet = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,(char *)&optVal, sizeof(optVal));
			if(iRet == 0)
			{
				memset((void*)&addr, 0, sizeof(addr));
				addr.sin_family = AF_INET;
				addr.sin_port = htons(9);
				addr.sin_addr.s_addr=INADDR_BROADCAST;
				iRet = sendto(sockfd, (const char *)packet, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr));
				if(iRet != SOCKET_ERROR) bRet = TRUE;
			}
			closesocket(sockfd);
		}
		WSACleanup();
	}
	return bRet;
}
#else

WISEPLATFORM_API int send_local_broadcast_WOL(char* mac, int size)
{

	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
	LPSOCKADDR addr = NULL;
	ULONG outBufLen = 0;
	char buff[16];
	ULONG mask, broadcast, loopback=0x100007f, addr_long;
	int i, j;
	unsigned char packet[102];
	void* ptr;

	// create packet content
	if (size < 6) {
		return -1;
	}

	for (i = 0; i<6; i++) {
		packet[i] = 0xFF;
	}
	for (i = 1; i<17; i++) {
		for (j = 0; j<6; j++) {
			packet[i * 6 + j] = mac[j];
		}
	}

	// get loopback long address
	inet_pton(AF_INET, "127.0.0.1", &loopback);

	GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen);
	pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
	if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST, NULL, pAddresses, &outBufLen) != NO_ERROR) {
		free(pAddresses);
		return -1;
	}

	pCurrAddresses = pAddresses;
	while (pCurrAddresses)
	{
		if (pCurrAddresses->OperStatus != IfOperStatusUp) {
			pCurrAddresses = pCurrAddresses->Next;
			continue;
		}

		pUnicast = pCurrAddresses->FirstUnicastAddress;
		while (pUnicast) {
			addr = pUnicast->Address.lpSockaddr;

			if (addr->sa_family == AF_INET) {
				struct sockaddr_in  *sa_in = (struct sockaddr_in *)addr;
				ptr = &sa_in->sin_addr;
				addr_long = *((ULONG*)ptr);

				// get ipv4 mask value from prefix number
				ConvertLengthToIpv4Mask(pUnicast->OnLinkPrefixLength, &mask);
#if 0
				// for debug
				inet_ntop(AF_INET, &(sa_in->sin_addr), buff, sizeof(buff));
				printf("%s Address: %s / %ld / %ld\n", pCurrAddresses->AdapterName, buff, pUnicast->OnLinkPrefixLength, mask);
				inet_ntop(AF_INET, &mask, buff, sizeof(buff));
				printf("mask=%s\n", buff);
#endif
				if (addr_long != loopback) {
					broadcast = ~mask | addr_long;
					inet_ntop(AF_INET, &broadcast, buff, sizeof(buff));
					printf("local boradcast=%s\n", buff);
					send_broadcast_packet(broadcast, packet, sizeof(packet));
				}
			}
			pUnicast = pUnicast->Next;
		}

		pCurrAddresses = pCurrAddresses->Next;
	}

	free(pAddresses);

	return 0;
}


WISEPLATFORM_API bool network_magic_packet_send(char * mac, int size)
{
	bool bRet = false;
	if(size < 6) return bRet;
	{
		unsigned char packet[102];
		struct sockaddr_in addr;
		SOCKET sockfd;
		int i = 0, j = 0;
		bool optVal = true;
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		for(i=0;i<6;i++)
		{
			packet[i] = 0xFF;
		}
		for(i=1;i<17;i++)
		{
			for(j=0;j<6;j++)
			{
				packet[i*6+j] = mac[j];
			}
		}

		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(sockfd > 0)
		{
			int iRet = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,(char *)&optVal, sizeof(optVal));
			if(iRet == 0)
			{
				memset((void*)&addr, 0, sizeof(addr));
				addr.sin_family = AF_INET;
				addr.sin_port = htons(9);
				addr.sin_addr.s_addr=INADDR_BROADCAST;
				iRet = sendto(sockfd, (const char *)packet, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr));
				if(iRet != SOCKET_ERROR) bRet = TRUE;
			}
			closesocket(sockfd);
		}
		WSACleanup();
	}

	bRet = (!send_local_broadcast_WOL(mac, size) & bRet);

	return bRet;
}
#endif