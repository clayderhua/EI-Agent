#ifndef _NETINFOLIST_H_
#define _NETINFOLIST_H_

#include "stdbool.h"

#define DEF_INVALID_VALUE                          (-999)   

#define DEF_ADP_NAME_LEN    256
#define DEF_ADP_DESP_LEN    256
#define DEF_TAG_NAME_LEN    128
#define DEF_N_LEN			256

typedef struct net_info_t{
	unsigned int index; //windows & linux: 0,1,2...
	unsigned int id; //windows: 11,25...
	char type[128];
	char baseName[DEF_ADP_NAME_LEN];
	char adapterName[DEF_ADP_NAME_LEN];
	char alias[DEF_ADP_NAME_LEN];
	char adapterDescription[DEF_ADP_DESP_LEN];
	char mac[128];
	char netStatus[128];
	unsigned int netSpeedMbps;
	double netUsage;
	unsigned int sendDataByte;
	unsigned int recvDataByte;
	double sendThroughput;
	double recvThroughput;
	//unsigned int sendDataByteInterval;
	//unsigned int recvDataByteInterval;

	unsigned int oldInDataByte;
	unsigned int oldOutDataByte;
	unsigned int initInDataByte;
	unsigned int initOutDataByte;
	long long currentTimestamp;
	bool isFoundFlag;
}net_info_t;

typedef struct net_info_node_t{
	net_info_t netInfo;
	struct net_info_node_t * next;
}net_info_node_t;

typedef net_info_node_t * net_info_list;

#ifdef __cplusplus
extern "C" {
#endif

	net_info_list CreateNetInfoList();

	int DeleteAllNetInfoNode(net_info_list netInfoList);

	bool GetNetInfo(net_info_list netInfoList, bool* changed);

	void SetWorkDir(char* strdir);

#ifdef __cplusplus
}
#endif

#endif