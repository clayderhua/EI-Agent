#include "NetInfoParser.h"
#include <stdlib.h>
#include <string.h>
#include "WISEPlatform.h"

void UpdateNetInfo(MSG_CLASSIFY_T* pNetInfoRoot, net_info_list netInfoList, bool bReset)
{
	MSG_CLASSIFY_T* pListItem = NULL;
	net_info_node_t *pNetInfo = NULL; 
	if(pNetInfoRoot == NULL || netInfoList == NULL)
		return;

	pListItem = IoT_FindGroup(pNetInfoRoot, NET_MON_INFO_LIST);
	if(pListItem == NULL && bReset)
	{
			pListItem = IoT_AddGroupArray(pNetInfoRoot, NET_MON_INFO_LIST);
	}
	
	if(bReset)
	{
		MSG_CLASSIFY_T* pNext = pListItem->sub_list;
		while(pNext)
		{
			MSG_CLASSIFY_T* pCurrent = pNext;
			pNext = pNext->next;
			IoT_ReleaseAll(pCurrent);
		}
		pListItem->sub_list = NULL;
	}

	pNetInfo = netInfoList->next; 
	while(pNetInfo)
	{
		MSG_CLASSIFY_T *pNet = NULL, *pNetName = NULL;
		MSG_ATTRIBUTE_T *attr = NULL;
		char netUID[260] = {0};
#ifdef OLDFMT		
		snprintf(netUID, sizeof(netUID), "Index%d@2D%s", pNetInfo->netInfo.index, pNetInfo->netInfo.adapterName);
#else
		MSG_CLASSIFY_T *pProcsses = NULL;
		snprintf(netUID, sizeof(netUID), "%d", pNetInfo->netInfo.index);
#endif
		pNetName = IoT_FindGroup(pListItem, netUID);
		if(pNetName == NULL && bReset)
		{
			pNetName = IoT_AddGroup(pListItem, netUID);
		}

#ifdef OLDFMT
		pNet = pNetName;
#else
		//pNet = IoT_AddGroup(pNetName, "net");
		memset(netUID, 0, sizeof(netUID));
		snprintf(netUID, sizeof(netUID), "%s", pNetInfo->netInfo.adapterName);
		pNet = IoT_FindGroup(pNetName, netUID);
		if(pNet == NULL && bReset)
		{
			pNet = IoT_AddGroup(pNetName, netUID);
		}
#endif
		if(pNet == NULL && !bReset)
		{
			pNetInfo = pNetInfo->next;
			continue;
		}

		attr = IoT_FindSensorNode(pNet, NET_INFO_ADAPTER_NAME);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_ADAPTER_NAME);
		}
		IoT_SetStringValue(attr, pNetInfo->netInfo.adapterName, IoT_READONLY);

		attr = IoT_FindSensorNode(pNet, NET_INFO_ADAPTER_DESCRI);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_ADAPTER_DESCRI);
		}
		IoT_SetStringValue(attr, pNetInfo->netInfo.adapterDescription, IoT_READONLY);

		attr = IoT_FindSensorNode(pNet, NET_INFO_TYPE);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_TYPE);
		}
		IoT_SetStringValue(attr, pNetInfo->netInfo.type, IoT_READONLY);

		attr = IoT_FindSensorNode(pNet, NET_INFO_STATUS);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_STATUS);
		}
		IoT_SetStringValue(attr, pNetInfo->netInfo.netStatus, IoT_READONLY);

		attr = IoT_FindSensorNode(pNet, NET_INFO_SPEED_MBPS);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_SPEED_MBPS);
		}
		IoT_SetDoubleValueWithMaxMin(attr, pNetInfo->netInfo.netSpeedMbps, IoT_READONLY, pNetInfo->netInfo.netSpeedMbps, pNetInfo->netInfo.netSpeedMbps, "Mbps");

		attr = IoT_FindSensorNode(pNet, NET_INFO_USAGE);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_USAGE);
		}
		IoT_SetDoubleValueWithMaxMin(attr, pNetInfo->netInfo.netUsage, IoT_READONLY, 100, 0, "%");

		/*attr = IoT_FindSensorNode(pNet, NET_INFO_SEND_BYTE);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_SEND_BYTE);
		}*/
		attr = IoT_FindSensorNode(pNet, NET_INFO_SEND_INTERVAL);
		if (attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_SEND_INTERVAL);
		}
		IoT_SetDoubleValue(attr, pNetInfo->netInfo.sendDataByte, IoT_READONLY, "Byte");

		attr = IoT_FindSensorNode(pNet, NET_INFO_SEND_THROUGHPUT);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_SEND_THROUGHPUT);
		}
		IoT_SetDoubleValueWithMaxMin(attr, pNetInfo->netInfo.sendThroughput, IoT_READONLY, 100, 0, "%");

		/*attr = IoT_FindSensorNode(pNet, NET_INFO_RECV_BYTE);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_RECV_BYTE);
		}*/
		attr = IoT_FindSensorNode(pNet, NET_INFO_RECV_INTERVAL);
		if (attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_RECV_INTERVAL);
		}
		IoT_SetDoubleValue(attr, pNetInfo->netInfo.recvDataByte, IoT_READONLY, "Byte");

		attr = IoT_FindSensorNode(pNet, NET_INFO_RECV_THROUGHPUT);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pNet, NET_INFO_RECV_THROUGHPUT);
		}
		IoT_SetDoubleValueWithMaxMin(attr, pNetInfo->netInfo.recvThroughput, IoT_READONLY, 100, 0, "%");

		pNetInfo = pNetInfo->next;
	}
}
