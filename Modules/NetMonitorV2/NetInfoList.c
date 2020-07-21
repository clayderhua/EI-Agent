#include "NetInfoList.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "NetMonitorLog.h"
#include "util_string.h"

#include "util_os.h"

#ifdef WIN32
#include <IPHlpApi.h> 
#include <Mprapi.h>
#include <ifmib.h> 
#pragma comment(lib, "IPHLPAPI.lib") 
#pragma comment(lib, "Mprapi.lib") 
#include <Windows.h>
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

static char g_strWorkDir[256] = {0};

net_info_list CreateNetInfoList()
{
	net_info_node_t *head = NULL;
	head = (net_info_node_t *)malloc(sizeof(net_info_node_t));
	if (head)
	{
		memset(head, 0, sizeof(net_info_node_t));
		head->next = NULL;
		head->netInfo.index = DEF_INVALID_VALUE;
		head->netInfo.netSpeedMbps = DEF_INVALID_VALUE;
		head->netInfo.netUsage = DEF_INVALID_VALUE;
		head->netInfo.recvDataByte = DEF_INVALID_VALUE;
		head->netInfo.recvThroughput = DEF_INVALID_VALUE;
		head->netInfo.sendDataByte = DEF_INVALID_VALUE;
		head->netInfo.sendThroughput = DEF_INVALID_VALUE;
	}
	return head;
}

net_info_node_t *FindNetInfoNodeWithIndex(net_info_list netInfoList, int index)
{
	net_info_node_t *findNode = NULL, *head = NULL;
	if (netInfoList == NULL)
		return findNode;
	head = netInfoList;
	findNode = head->next;
	while (findNode)
	{
		if (findNode->netInfo.id == index)
			break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

net_info_node_t *FindNetInfoNodeWithAdapterName(net_info_list netInfoList, char *adpName)
{
	net_info_node_t *findNode = NULL, *head = NULL;
	if (netInfoList == NULL || adpName == NULL)
		return findNode;
	head = netInfoList;
	findNode = head->next;

	while (findNode)
	{
		if (!strcmp(findNode->netInfo.adapterName, adpName))
			break;
		else
		{
			findNode = findNode->next;
		}
	}
	return findNode;
}

int DeleteNetInfoNodeWithIndex(net_info_list netInfoList, int index)
{
	int iRet = -1;
	net_info_node_t *delNode = NULL, *head = NULL;
	net_info_node_t *p = NULL;
	if (netInfoList == NULL)
		return iRet;
	head = netInfoList;
	p = head;
	delNode = head->next;
	while (delNode)
	{
		if (delNode->netInfo.index == index)
		{
			p->next = delNode->next;
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		else
		{
			p = delNode;
			delNode = delNode->next;
		}
	}
	if (iRet == -1)
		iRet = 1;
	return iRet;
}

int DeleteAllNetInfoNode(net_info_list netInfoList)
{
	int iRet = -1;
	net_info_node_t *delNode = NULL, *head = NULL;
	if (netInfoList == NULL)
		return iRet;
	head = netInfoList;
	delNode = head->next;
	while (delNode)
	{
		head->next = delNode->next;
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

void DestroyNetInfoList(net_info_list netInfoList)
{
	if (NULL == netInfoList)
		return;
	DeleteAllNetInfoNode(netInfoList);
	free(netInfoList);
	netInfoList = NULL;
}

int InsertNetInfoNode(net_info_list netInfoList, net_info_t *pNetInfo)
{
	int iRet = -1;
	net_info_node_t *newNode = NULL, *findNode = NULL, *head = NULL;
	if (pNetInfo == NULL || netInfoList == NULL)
		return iRet;
	head = netInfoList;
	findNode = FindNetInfoNodeWithIndex(netInfoList, pNetInfo->index);
	if (findNode == NULL)
	{
		newNode = (net_info_node_t *)malloc(sizeof(net_info_node_t));
		memset(newNode, 0, sizeof(net_info_node_t));
		memcpy((char *)&newNode->netInfo, (char *)pNetInfo, sizeof(net_info_t));
		newNode->next = head->next;
		head->next = newNode;
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

bool IsNetInfoListEmpty(net_info_list netInfoList)
{
	bool bRet = true;
	net_info_node_t *curNode = NULL, *head = NULL;
	if (netInfoList == NULL)
		return bRet;
	head = netInfoList;
	curNode = head->next;
	if (curNode != NULL)
		bRet = false;
	return bRet;
}

void formatNetStr(char *s)
{
	int len = strlen(s);
	char *p = s;
	int i = 0;
	char t[100] = {0};

	while (*p == ' ')
	{
		p++;
		i++;
	}
	if (s[len - 1] == '\n')
		s[strlen(s) - 1] = 0;
	if (s[len - 1] == '\r')
		s[strlen(s) - 1] = 0;

	if (i > 0)
	{
		strncpy(t, s + i, strlen(s) - i);
		memset(s, 0, strlen(s) + 1);
		strncpy(s, t, strlen(t) + 1);
	}
}

//--------------------------------------linux function define-----------------------------------
#ifndef WIN32
unsigned long getCurrentTime()    
{    
	struct timeval tv;    
	gettimeofday(&tv,NULL);    
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;    
}    

void getRecvSendBytes(char *adaptername, int *recvBytes, int *sendBytes)
{
	int a[10]={0};
	FILE *fp; 
	char str[1000];
	char *adapter[20]={9};

	strcpy(adapter, adaptername);
	if((fp = fopen("/proc/net/dev", "r+")) == NULL)  
	{  
		//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
		return;
	} 

	while( fgets(str, sizeof(str), fp) )
	{
		{
			int i = 0;
			int j = 0;
			int m=0;
			char p[20] = {0};
			char temp[20] = {0};
			if(strstr(str, adapter) != NULL)
			{
				while(str[i] != ':')
					i++;
				while(str[i]!= '\n' && m<8)
				{
					while(str[i] >= '0'&& str[i] <= '9')
					{
						p[j]=str[i];
						i++;
						j++;
					}
					if(j>0 && m<8)
					{
						memcpy(temp, p,j);
						temp[j]='\0';
						a[m++] = atoi(temp);
					}
					i++;
					j=0;
				}

			}

		}

	}
	fclose(fp);

	*recvBytes = a[0];
	*sendBytes = a[7];
}

void getNetUsage(char *adapterName, unsigned int *netSpeedMbps, unsigned int *RecvBytes, double *recvThroughput, unsigned int *sendBytes, double *sendThroughput, double *netUsage)
{
	int recvBytes1 = 0;
	int sendBytes1 = 0;
	int recvBytes2 = 0;
	int sendBytes2 = 0;

	unsigned long startTime;
	unsigned long endTime;
	unsigned long bytes = 0;
	float interval = 0.0;
	int intervalSecond = 1;
	unsigned long BandWidth = 0;

	//startTime  = getCurrentTime();
	getRecvSendBytes(adapterName, &recvBytes1, &sendBytes1);
	app_os_sleep(intervalSecond *1000);//ms
	//endTime  = getCurrentTime();
	getRecvSendBytes(adapterName, &recvBytes2, &sendBytes2);
	//interval = (float)(endTime - startTime)/1000;

	*RecvBytes =recvBytes2 - recvBytes1;
	*sendBytes = sendBytes2 - sendBytes1;

	if( (*netSpeedMbps) != 0)
	{
		BandWidth = (*netSpeedMbps)*1024*1024;
		*recvThroughput = (float)(*RecvBytes)*8/intervalSecond/BandWidth;
		*sendThroughput = (float)(*sendBytes)*8/intervalSecond/BandWidth;
		*netUsage = (float)((*sendBytes) + (*RecvBytes))*8/intervalSecond/BandWidth;
	}
	else
	{
		*recvThroughput = 0.0;
		*sendThroughput = 0.0;
		*netUsage = 0.0;
	}
	return ;
}

int launch_process(char * appPath)
{
    int bRet = 0;
    int status = 0;
    pid_t pid = fork();
	if ( 0 == pid ) {/* Child process */
#ifdef ANDROID
		int ret = execlp("/system/bin/sh", "sh", "-c", appPath, NULL);
#else
		int ret = execlp("/bin/sh", "sh", "-c", appPath, NULL);
#endif
		if(ret != 0)
			NETWORKLog(Error, "~~~~ launch_process: %s~~~~ \n", strerror(errno));
		exit(ret);
	} else if (pid < 0){ /* fork() failed */
        	bRet = -1;    
	}
	else
	{
		waitpid(pid, &status, WUNTRACED | WCONTINUED);
		if(status != 0)
			NETWORKLog(Error, "~~~~ launch_process: %s~~~~ \n", strerror(errno));
	}
	return bRet;
}

void _getNetInfo(char *str, char c)
{
	char s[256] = {0};
	sprintf(s, "%s/netInfo.sh", g_strWorkDir);
	if(!util_is_file_exist(s))
	{
		NETWORKLog(Error, "~~~~ launch_process: File [%s] not exist!~~~~ \n", s);
		return;
	}
	sprintf(s, "%s/netInfo.sh \"%s\" \"%c\"", g_strWorkDir, str, c);
	launch_process(s);
}

void getAdapterNameList(char *str, char c)
{
	_getNetInfo(str,c);
}

void getAdapterNameListEx(char *str, char c)
{
	_getNetInfo(str,c);
}
void getNetworkCardList(char *str, char c)
{
	_getNetInfo(str,c);
}

void getWiredStatus(char *str, char c)
{
	_getNetInfo(str,c);
}

void getWiredSpeedMbpsTemp(char *str, char c)
{
	_getNetInfo(str,c);
}

void getWirelessAdapterNameList(char *str, char c)
{
	_getNetInfo(str,c);
}

void getWirelessSpeedMbps(char *str, char c)
{
	_getNetInfo(str,c);
}

void getWirelessStatus(char *str, char c)
{
	_getNetInfo(str,c);
}

void getRecvBytes(char *str, char c)
{	
	_getNetInfo(str,c);
}

void getSendBytes(char *str, char c)
{	
	_getNetInfo(str,c);
}

int getNumberOfAdapters(char* fileName)
{
	int lineNum = 0;
	FILE *fp = NULL;
	char str[128] = {0};

	if((fp=fopen(fileName,"r+")) == NULL)
	{
		//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
		return false;
	}
	while(fgets(str,sizeof(str),fp))
	{
		lineNum++;
	}
	fclose(fp);
	return lineNum;
}

bool whetherExistWirelessCard(char* fileName)
{
	FILE *fp = NULL;
	char str[128] = {0};
	bool bRet = false;
//	char ch;

	if((fp=fopen(fileName,"r+")) == NULL)
	{
		//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
		return false;
	}
	fgets(str,sizeof(str),fp);
//NETWORKLog(Debug, "~~~~ whetherExistWirelessCard: %s~~~~ \n", str);
	if(strlen(str)<5) 
	{
		bRet = true;
	}
	fclose(fp);
	return bRet;
}

bool isNetDisconnect(char* fileName)
{
	FILE *fp = NULL;
	char str[128] = {0};
	bool bRet = false;

	if((fp=fopen(fileName,"r+")) == NULL)
	{
		//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
		return false;
	}
	fgets(str,sizeof(str),fp);
	if(strlen(str)<5) 
	{
		NETWORKLog(Debug, "isNetDisconnect: %d, %s", strlen(str), str);
		bRet = true;
	}
	fclose(fp);
	return bRet;
}

bool getNetStatus(char *str, char c)
{
	_getNetInfo(str,c);
}

#endif 
//----------------------------------------------------------------------------------------------

//--------------------------------------other function define-----------------------------------
#ifdef WIN32
bool GetAdapterNameEx(char* index, char* str)
{
	bool isFind = false;

	FILE *fd = NULL;
	char buf[512] = {0};
	char buf1[512] = {0};
	char Idx[512] = {0};
	char Met[512] = {0};
	char MTU[512] = {0};
	char status[512] = {0};
	char name[512] = {0};

	sprintf(buf, "cmd.exe /c netsh interface ip show interface");
	fd = _popen(buf, "r");
	while (fgets(buf1, sizeof(buf1), fd))
	{
		//puts(buf);
		//Idx     Met         MTU          state               name
		sscanf(buf1, "%s%s%s%s%[^/n/r]", Idx,Met,MTU,status, name);

		if( !strcmp(Idx, index) )
		{
			isFind = true;
			formatNetStr(name);
			strcpy(str, name);
			break;
		}
	}
	_pclose(fd);
	return isFind;
}

bool GetNetworkFriendlyName(wchar_t* guidName, char* friendlyName, unsigned long* bufLen)
{
	/*Query Network friendly name from registry.*/
	bool bRet = false;
	HKEY hk;
	char regName[256] = { 0 };
	wchar_t* guid = NULL;
	char* newguid[256] = { 0 };
	if (NULL == guidName) return bRet;
	if (strstr(guidName, "\\DEVICE\\TCPIP_") >= 0)
		guid = guidName + strlen("\\DEVICE\\TCPIP_");

	sprintf(regName, "SYSTEM\\CurrentControlSet\\Control\\Network\\{4d36e972-e325-11ce-bfc1-08002be10318}\\%ws\\Connection", guid);
	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_READ, &hk))
		return bRet;
	else
	{
		char valueName[] = "Name";
		char valueData[256] = { 0 };
		unsigned long  valueDataSize = sizeof(valueData);
		if (ERROR_SUCCESS != RegQueryValueEx(hk, valueName, 0, NULL, (LPBYTE)valueData, &valueDataSize))
		{
			RegCloseKey(hk);
			return bRet;
		}
		RegCloseKey(hk);
		bRet = valueDataSize == 0 ? false : true;
		if (bRet)
		{
			//unsigned int cpyLen = valueDataSize < *bufLen ? valueDataSize : *bufLen; 
			if (friendlyName)
				memcpy(friendlyName, valueData, valueDataSize);
			if (bufLen)
				*bufLen = valueDataSize;
		}
	}
	return bRet;
}

bool GetAdapterFriendlyNameWithGUIDName(wchar_t * guidName, char * friendlyName, int index)
{
	bool bRet = false;
	if(guidName == NULL || friendlyName == NULL) return bRet;
	{
		bRet = GetNetworkFriendlyName(guidName, friendlyName, NULL);
		if (bRet)
			return bRet;
	}
	{ /*MPR API Trigger WMI Query*/
		HANDLE hMprConfig; 
		DWORD dwRet=0; 
		PIP_INTERFACE_INFO plfTable = NULL; 
		DWORD dwBufferSize=0; 
		char szFriendName[256] = {0}; 
		DWORD tchSize = sizeof(TCHAR) * 256;
		UINT i = 0;
		dwRet = MprConfigServerConnect(NULL, &hMprConfig);
		if(dwRet == NO_ERROR)
		{
			dwRet = MprConfigGetFriendlyName(hMprConfig, guidName,  
				(PWCHAR)szFriendName, tchSize);
			if(dwRet == NO_ERROR) 
			{
				char * tmpName = UnicodeToANSI(szFriendName);
				strcpy(friendlyName, tmpName);
				free(tmpName);
				bRet = true;
			}
			else if(dwRet = ERROR_NOT_FOUND) //1168L 0x490
			{
				char Idx[512] = {0};
				itoa(index, Idx, 10);
				GetAdapterNameEx(Idx, friendlyName);
			}
			MprConfigServerDisconnect(hMprConfig);
		}

	}
	return bRet;
}
#endif

long long NetInfo_GetTimeTick()
{
	long long tick = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tick = (long long)tv.tv_sec * 1000 + (long long)tv.tv_usec / 1000;
	return tick;
}

bool GetNetInfoWithIndex(net_info_t * pNetInfo, unsigned int index)
{
	bool bRet = false;
	long long curTs = NetInfo_GetTimeTick();
	long long diffts = 0;
#ifdef WIN32

	MIB_IFTABLE *pIfTable;
	MIB_IFROW *pIfRow;
	unsigned int dwSize = 0;
	unsigned int dwRetVal = 0;
	
	pIfTable = (MIB_IFTABLE *)malloc(sizeof (MIB_IFTABLE));
	memset(pIfTable, 0, sizeof(MIB_IFTABLE));
	dwSize = sizeof (MIB_IFTABLE);

	if (GetIfTable(pIfTable, &dwSize, false) == ERROR_INSUFFICIENT_BUFFER) 
	{
		free(pIfTable);
		pIfTable = (MIB_IFTABLE *)malloc(dwSize);
		memset(pIfTable, 0, dwSize);
	}

	if ((dwRetVal = GetIfTable(pIfTable, &dwSize, false)) == NO_ERROR)
	{
		unsigned int i = 0;
		for (i = 0; i < pIfTable->dwNumEntries; i++) 
		{
			pIfRow = (MIB_IFROW *)&pIfTable->table[i];
			if(pIfRow->dwIndex == index)
			{
				//if(strlen(pNetInfo->adapterName) == 0)
				{
					memset(pNetInfo->adapterName, 0, sizeof(pNetInfo->adapterName));
					GetAdapterFriendlyNameWithGUIDName(pIfRow->wszName, pNetInfo->adapterName, index);
				}
				pNetInfo->id = index;
				if(strlen(pNetInfo->adapterDescription) == 0)
				{
					strcpy(pNetInfo->adapterDescription, pIfRow->bDescr);
				}
				if(strlen(pNetInfo->type) == 0)
				{
					switch (pIfRow->dwType) 
					{
					case IF_TYPE_OTHER:
						strcpy(pNetInfo->type, "Other");
						break;
					case IF_TYPE_ETHERNET_CSMACD:
						strcpy(pNetInfo->type, "Ethernet");
						break;
					case IF_TYPE_ISO88025_TOKENRING:
						strcpy(pNetInfo->type, "Token Ring");
						break;
					case IF_TYPE_PPP:
						strcpy(pNetInfo->type, "PPP");
						break;
					case IF_TYPE_SOFTWARE_LOOPBACK:
						strcpy(pNetInfo->type, "Software Lookback");
						break;
					case IF_TYPE_ATM:
						strcpy(pNetInfo->type, "ATM");
						break;
					case IF_TYPE_IEEE80211:
						strcpy(pNetInfo->type, "IEEE 802.11 Wireless");
						break;
					case IF_TYPE_TUNNEL:
						strcpy(pNetInfo->type, "Tunnel type encapsulation");
						break;
					case IF_TYPE_IEEE1394:
						strcpy(pNetInfo->type, "IEEE 1394 Firewire");
						break;
					default:
						strcpy(pNetInfo->type, "Unknown type");
						break;
					}
				}

				pNetInfo->netSpeedMbps = pIfRow->dwSpeed/1000000;
				pNetInfo->netStatus;

				switch (pIfRow->dwOperStatus) 
				{
				case IF_OPER_STATUS_CONNECTED:
				case IF_OPER_STATUS_OPERATIONAL:
					strcpy(pNetInfo->netStatus, "Connected");
					break;
				default:
					strcpy(pNetInfo->netStatus, "Disconnect");
					break;
				}

				if (pNetInfo->currentTimestamp == 0)
					pNetInfo->currentTimestamp = curTs;
				diffts = (curTs - pNetInfo->currentTimestamp) / 1000;
				if(pIfRow->dwOutOctets > 0)
				{
					if(pNetInfo->initOutDataByte == 0) pNetInfo->initOutDataByte = pIfRow->dwOutOctets;
					//pNetInfo->sendDataByte = pIfRow->dwOutOctets - pNetInfo->initOutDataByte;
					if (diffts <= 1)
						pNetInfo->sendDataByte = pIfRow->dwOutOctets - pNetInfo->oldOutDataByte;
					else
						pNetInfo->sendDataByte = (pIfRow->dwOutOctets - pNetInfo->oldOutDataByte) / diffts;

					if((pNetInfo->oldOutDataByte != 0) && (pIfRow->dwSpeed != 0))
					{
						pNetInfo->sendThroughput = (double)pNetInfo->sendDataByte * 100 * 8 / pIfRow->dwSpeed;
					}
					else
					{
						//pNetInfo->sendThroughput = 0;
					}
					/*if (curTs - pNetInfo->currentTimestamp > 1000)
						pNetInfo->sendDataByteInterval = (pIfRow->dwOutOctets - pNetInfo->oldOutDataByte) / ((curTs - pNetInfo->currentTimestamp) / 1000);*/
					pNetInfo->oldOutDataByte = pIfRow->dwOutOctets;
				}
				else
				{
					pNetInfo->sendDataByte = 0;
					//pNetInfo->sendThroughput = 0;
				}

				if(pIfRow->dwInOctets)
				{
					if(pNetInfo->initInDataByte == 0) pNetInfo->initInDataByte = pIfRow->dwInOctets;
					//pNetInfo->recvDataByte = pIfRow->dwInOctets - pNetInfo->initInDataByte;
					if (diffts <= 1)
						pNetInfo->recvDataByte = pIfRow->dwInOctets - pNetInfo->oldInDataByte;
					else
						pNetInfo->recvDataByte = (pIfRow->dwInOctets - pNetInfo->oldInDataByte) / diffts;

					if((pNetInfo->oldInDataByte != 0) && (pIfRow->dwSpeed != 0))
					{
						pNetInfo->recvThroughput = (double)pNetInfo->recvDataByte * 100 * 8 / (pIfRow->dwSpeed);
					}
					else
					{
						//pNetInfo->recvThroughput = 0;
					}
					/*if (curTs - pNetInfo->currentTimestamp > 1000)
						pNetInfo->recvDataByteInterval = (pIfRow->dwInOctets - pNetInfo->oldInDataByte) / ((curTs - pNetInfo->currentTimestamp) / 1000);*/
					pNetInfo->oldInDataByte = pIfRow->dwInOctets;
				}
				else
				{
					pNetInfo->recvDataByte = 0;
					//pNetInfo->recvThroughput = 0;
				}
				pNetInfo->netUsage = pNetInfo->sendThroughput+pNetInfo->recvThroughput;
				bRet = true;
				break;
			}
		}
		pNetInfo->currentTimestamp = curTs;
	}
	if (pIfTable != NULL) 
	{
		free(pIfTable);
		pIfTable = NULL;
	}

#else

	FILE *fp;
	char str[100] = {0};
	char aliasStr[256] = {0};
	int line = 0;
	int i = 0;
	char ch = 0;
	bool bFlag = false;

	pNetInfo->index = index;

	memset(aliasStr,0,sizeof(aliasStr));
	getAdapterNameListEx("none",'N');
	if((fp=fopen("out.txt","r+")) == NULL)
	{
		//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
		return false;
	}
	while(fgets(str,sizeof(str),fp))
	{
		formatNetStr(str);
		if((strstr(str,pNetInfo->adapterName)!=NULL) && (strstr(str, ":")!=NULL))
		{
			if(strlen(aliasStr))
				sprintf(aliasStr, "%s;%s", aliasStr, str);
			else 
				sprintf(aliasStr, "%s", str);
		}
		continue;
	}
	strcpy(pNetInfo->alias, aliasStr);
	fclose(fp);

//NETWORKLog(Debug, "~~~~ adapterName: %s~~~~ \n", pNetInfo->adapterName);
	bFlag = false;
	getNetStatus(pNetInfo->adapterName,'5');
	bFlag = isNetDisconnect("out.txt");
	strcpy(pNetInfo->netStatus, bFlag?"Disconnect":"Connected");

	if(strcmp(pNetInfo->type, "Ethernet") ==0 )//wired
	{
		NETWORKLog(Debug, "Ethernet, adapter=[%s], netStatus=[%s]\n", pNetInfo->adapterName, pNetInfo->netStatus);
		if(strcmp(pNetInfo->netStatus, "Connected") == 0)
		{
			//get BitRate
			char digitNum[20] = {0};
			getWiredSpeedMbpsTemp(pNetInfo->adapterName, 'b');
			if((fp=fopen("out.txt","r+")) == NULL)
			{
				//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
				NETWORKLog(Error, "note: %s\n", strerror(errno));
				return false;
			}
			fgets(str,sizeof(str),fp);
			formatNetStr(str);
			sscanf(str, "%[0-9]", digitNum);
			pNetInfo->netSpeedMbps = atoi(digitNum);	
			fclose(fp);

			//getNetUsage(pNetInfo->adapterName, &(pNetInfo->netSpeedMbps), &(pNetInfo->recvDataByte), &(pNetInfo->recvThroughput), &(pNetInfo->sendDataByte), &(pNetInfo->sendThroughput), &(pNetInfo->netUsage));
			goto done1;
		}
		else
		{
			pNetInfo->netSpeedMbps = 0;
			pNetInfo->netUsage = 0.0;
			pNetInfo->recvDataByte = 0;
			pNetInfo->recvThroughput = 0.0;
			pNetInfo->sendDataByte = 0;
			pNetInfo->sendThroughput = 0.0;
		}
	}
	else if(strcmp(pNetInfo->type, "Wireless") ==0 )//wireless
	{
		NETWORKLog(Debug, "Wireless, adapter=[%s], netStatus=[%s]\n", pNetInfo->adapterName, pNetInfo->netStatus);
		if(strcmp(pNetInfo->netStatus, "Connected") == 0)
		{
			//get BitRate 
			getWirelessSpeedMbps(pNetInfo->adapterName, '1');
			if((fp=fopen("out.txt","r+")) == NULL)
			{
				//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
				NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
				return false;
			}
			fgets(str,sizeof(str),fp);
			pNetInfo->netSpeedMbps = atoi(str);
			fclose(fp);

			//getNetUsage(pNetInfo->adapterName, &(pNetInfo->netSpeedMbps), &(pNetInfo->recvDataByte), &(pNetInfo->recvThroughput), &(pNetInfo->sendDataByte), &(pNetInfo->sendThroughput), &(pNetInfo->netUsage));
			goto done1;
		}
		else
		{
			pNetInfo->netSpeedMbps = 0;
			pNetInfo->netUsage = 0.0;
			pNetInfo->recvDataByte = 0;
			pNetInfo->recvThroughput = 0.0;
			pNetInfo->sendDataByte = 0;
			pNetInfo->sendThroughput = 0.0;
		}
	}


done1:
	{
		int recvBytes1 = 0;
		int sendBytes1 = 0;

		if (pNetInfo->currentTimestamp == 0)
			pNetInfo->currentTimestamp = curTs;
		diffts = (curTs - pNetInfo->currentTimestamp) / 1000;

		getRecvBytes(pNetInfo->adapterName, '3');
		if((fp=fopen("out.txt","r+")) == NULL)
		{
			//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
			NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
			return false;
		}
		fgets(str,sizeof(str),fp);
		formatNetStr(str);
		recvBytes1 = atoi(str);	
		if(recvBytes1)
		{
			if(pNetInfo->initInDataByte == 0) pNetInfo->initInDataByte = recvBytes1;
			//pNetInfo->recvDataByte = recvBytes1 - pNetInfo->initInDataByte;
			if (diffts <= 1)
				pNetInfo->recvDataByte = recvBytes1 - pNetInfo->oldInDataByte;
			else
				pNetInfo->recvDataByte = (recvBytes1 - pNetInfo->oldInDataByte) / diffts;

			if((pNetInfo->oldInDataByte != 0) && (pNetInfo->netSpeedMbps != 0))
			{
				//pNetInfo->recvThroughput = (recvBytes1 - pNetInfo->oldInDataByte)*100/(pNetInfo->netSpeedMbps/8.00);
				pNetInfo->recvThroughput = (double)pNetInfo->recvDataByte * 8 * 100 / (pNetInfo->netSpeedMbps * 1000 * 1000);
			}
			else
			{
				pNetInfo->recvThroughput = 0;
			}
			pNetInfo->oldInDataByte = recvBytes1;
		}
		else
		{
			pNetInfo->recvDataByte = 0;
			pNetInfo->recvThroughput = 0;
		}
		fclose(fp);


		getSendBytes(pNetInfo->adapterName, '4');
		if((fp=fopen("out.txt","r+")) == NULL)
		{
			//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
			NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
			return false;
		}
		fgets(str,sizeof(str),fp);
		formatNetStr(str);
		sendBytes1 = atoi(str);	
		if(sendBytes1 > 0)
		{
			if(pNetInfo->initOutDataByte == 0) pNetInfo->initOutDataByte = sendBytes1;
			//pNetInfo->sendDataByte = sendBytes1 - pNetInfo->initOutDataByte;
			if (diffts <= 1)
				pNetInfo->sendDataByte = sendBytes1 - pNetInfo->oldOutDataByte;
			else
				pNetInfo->sendDataByte = (sendBytes1 - pNetInfo->oldOutDataByte) / diffts;

			if((pNetInfo->oldOutDataByte != 0) && (pNetInfo->netSpeedMbps != 0))
			{
				//pNetInfo->sendThroughput = (sendBytes1 - pNetInfo->oldOutDataByte)*100/(pNetInfo->netSpeedMbps/8.00);
				pNetInfo->sendThroughput = (double)pNetInfo->sendDataByte * 8 * 100 / (pNetInfo->netSpeedMbps * 1000 * 1000);
			}
			else
			{
				pNetInfo->sendThroughput = 0;
			}
			pNetInfo->oldOutDataByte = sendBytes1;
		}
		else
		{
			pNetInfo->sendDataByte = 0;
			pNetInfo->sendThroughput = 0;
		}
		fclose(fp);

		pNetInfo->netUsage = pNetInfo->recvThroughput + pNetInfo->sendThroughput;
	}

	pNetInfo->currentTimestamp = curTs;

	getNetworkCardList("none",'L');
	if((fp=fopen("out.txt","r+")) == NULL)
	{
		//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
		return false;
	}
	//if netcard supports "lspci"
	fgets(str,sizeof(str),fp);
	formatNetStr(str);
	if(strcmp(str, "default") == 0)
	{
		if(strcmp(pNetInfo->type, "Ethernet") ==0 )//wired
		{
			strcpy(pNetInfo->adapterDescription, "Ethernet Network Adapter");
		}
		else if(strcmp(pNetInfo->type, "Wireless") ==0 )//wireless
		{
			strcpy(pNetInfo->adapterDescription, "Wireless Network Adapter");
		}
	}
	fclose(fp);

	bRet = true;

#endif

	return bRet;
}

int ANSITOUnicode(const char * str, wchar_t* dest)
{
#ifdef WIN32

	int textLen = 0;
	textLen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	if(dest != NULL)
		MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)dest, textLen);
	return textLen;
#endif
}

bool ResetNetinfoListNodeisFoundFlag(net_info_list netInfoList)
{
	net_info_node_t * curNode = NULL, *head = NULL;
	if(netInfoList == NULL) 
		return false;
	head = netInfoList;
	curNode = head->next;
	while(curNode)
	{
		curNode->netInfo.isFoundFlag = false;
		curNode = curNode->next;
	}
	return true;
}

bool DelNodeWhenAdaperIsDisabled(net_info_list netInfoList)
{
	bool changeFlag = false;
	net_info_list preNetNode = NULL;
	net_info_list curNetNode = NULL;

	if(!netInfoList)
		return false;
	{
		preNetNode = netInfoList;
		curNetNode = preNetNode->next;
		while(curNetNode)
		{
			if(!curNetNode->netInfo.isFoundFlag)
			{
				changeFlag = true;
				preNetNode->next = curNetNode->next;
				if(curNetNode)
				{
					free(curNetNode);		
					curNetNode = NULL;
				}
				curNetNode = preNetNode;
			}
			preNetNode = curNetNode;
			curNetNode = curNetNode->next;
		}
	}
	return changeFlag;
}

bool GetNetInfo(net_info_list netInfoList, bool* changed)
{
#ifdef WIN32

	bool bRet = false;
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	PIP_ADAPTER_INFO pAdInfo = NULL;
	unsigned long ulSizeAdapterInfo = 0;
	unsigned int dwStatus = ERROR_INVALID_FUNCTION;
	bool changeFlag = false;
	int index = 1;

	if(netInfoList == NULL) return bRet;
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
		return bRet;  
	}
	pAdInfo = pAdapterInfo; 

	ResetNetinfoListNodeisFoundFlag(netInfoList);

	while(pAdInfo)  
	{  	
		if(pAdInfo->Type != MIB_IF_TYPE_ETHERNET && pAdInfo->Type != IF_TYPE_IEEE80211) 
		{
			pAdInfo = pAdInfo->Next; 
			continue;
		}
		{
			net_info_t * pNetInfo = NULL;
			net_info_node_t * findNode = NULL;
			findNode = FindNetInfoNodeWithIndex(netInfoList, pAdInfo->Index);
			if(findNode)
			{
				findNode->netInfo.isFoundFlag = true;
				pNetInfo = &findNode->netInfo;
				if(GetNetInfoWithIndex(pNetInfo, pAdInfo->Index))
				{
					//if(strlen(pNetInfo->adapterName)==0)
					{
						char tmpName[256] = {0};
						char guidName[256] = {0};
						wchar_t wcGUIDName[256] = {0};
						memset(pNetInfo->adapterName, 0, sizeof(pNetInfo->adapterName));
						sprintf_s(guidName, sizeof(guidName), "\\DEVICE\\TCPIP_%s", pAdInfo->AdapterName);
						ANSITOUnicode(guidName, wcGUIDName);
						GetAdapterFriendlyNameWithGUIDName(wcGUIDName, tmpName, pAdInfo->Index);
						strcpy(pNetInfo->adapterName, tmpName);
					}
				}
			}
			else
			{
				net_info_t netInfo;
				memset(&netInfo, 0, sizeof(net_info_t));
				if(GetNetInfoWithIndex(&netInfo, pAdInfo->Index))
				{
					netInfo.isFoundFlag = true;
					//if(strlen(netInfo.adapterName)==0)
					{
						char tmpName[256] = {0};
						char guidName[256] = {0};
						wchar_t wcGUIDName[256] = {0};
						memset(netInfo.adapterName, 0, sizeof(netInfo.adapterName));
						sprintf_s(guidName, sizeof(guidName), "\\DEVICE\\TCPIP_%s", pAdInfo->AdapterName);
						ANSITOUnicode(guidName, wcGUIDName);
						GetAdapterFriendlyNameWithGUIDName(wcGUIDName, tmpName, pAdInfo->Index);
						strcpy(netInfo.adapterName, tmpName);
						netInfo.index = index++;
					}
					InsertNetInfoNode(netInfoList, &netInfo);
				}

				changeFlag = true;
			}
		}	
		pAdInfo = pAdInfo->Next; 
	}
	free(pAdapterInfo);

	if( DelNodeWhenAdaperIsDisabled(netInfoList) )
		changeFlag = true;
#else
	bool changeFlag = false;
	bool bRet = false;
	int adapterNum = 0;
	static int preAdapterNum = 0;
	int i = 0;
	FILE *fp;
	int line = 0;
	char str[100] = {0};
	int index = 0;
	char ch;
	net_info_t netInfoTemp;
	net_info_t * pNetInfo = &netInfoTemp;
	bool bFlag = false;

	memset( &netInfoTemp, 0, sizeof(net_info_t));
	ResetNetinfoListNodeisFoundFlag(netInfoList);
	getAdapterNameList("none",'M');
	adapterNum = getNumberOfAdapters("out.txt");

	for(i=0; i<adapterNum; i++)
	{
		memset(pNetInfo, 0, sizeof(net_info_t));
		getAdapterNameList("none",'M');
		if((fp=fopen("out.txt","r+")) == NULL)
		{
			//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
			NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
			return false;
		}
		line = 0;
		while(fgets(str,sizeof(str),fp))
		{
			if(line == i)
				break;
			line++;
		}
		formatNetStr(str);
		strcpy(pNetInfo->adapterName, str);
		fclose(fp);

		index = i;
		//is wired or wireless
		bFlag = false;
		strcpy(pNetInfo->type, "Ethernet");
		getWirelessAdapterNameList("none", '0');
		bFlag = whetherExistWirelessCard("out.txt");
		//NETWORKLog(Debug, "~~~~ has wireless card: %s~~~~ \n", bFlag?"No":"Yes");
		if(!bFlag)//exist:0
		{
			getWirelessAdapterNameList("none", '0');
			if((fp=fopen("out.txt","r+")) == NULL)
			{
				//fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
				NETWORKLog(Error, "~~~~ note: %s~~~~ \n", strerror(errno));
				return false;
			}
			while(fgets(str,sizeof(str),fp))
			{
				formatNetStr(str);
				if(strcmp(pNetInfo->adapterName, str) == 0)
				{
					strcpy(pNetInfo->type, "Wireless");
					break;
				}	
			}
			fclose(fp);
		}

		{
			net_info_node_t * findNode = NULL;
			findNode = FindNetInfoNodeWithAdapterName(netInfoList, pNetInfo->adapterName);

			if(findNode)
			{
				//net_info_t netInfo;
				net_info_t * ppNetInfo = NULL;
//NETWORKLog(Debug, "~~~~ NetInfo Found: %s~~~~ \n", pNetInfo->adapterName);
				//memset( ppNetInfo, 0, sizeof(net_info_t));
				ppNetInfo = &findNode->netInfo;
				ppNetInfo->isFoundFlag = true;

				if(GetNetInfoWithIndex(ppNetInfo, index) != true)
					NETWORKLog(Debug, "GetNetInfoWithIndex() function error.");
			}
			else
			{
//NETWORKLog(Debug, "~~~~ NetInfo Not Found: %s~~~~ \n", pNetInfo->adapterName);
				changeFlag = true;
				//pNetInfo->isFoundFlag = true;

				if(GetNetInfoWithIndex(pNetInfo, index))
				{
					pNetInfo->isFoundFlag = true;
					InsertNetInfoNode(netInfoList, pNetInfo);
					{
						net_info_t * qNetInfo = NULL;
						qNetInfo = netInfoList->next;
					}
				}
			}

		}
	}

	if( DelNodeWhenAdaperIsDisabled(netInfoList) ) 
	{
		changeFlag = true;
	}
#endif 
	*changed = changeFlag;
	return bRet = true;
}

void SetWorkDir(char* strdir)
{
	if(strdir == NULL)
		return;
	if(strlen(strdir) <= 0)
		return;
	strncpy(g_strWorkDir, strdir, sizeof(g_strWorkDir));
}