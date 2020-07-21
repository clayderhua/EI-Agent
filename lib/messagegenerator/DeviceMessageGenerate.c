#include "DeviceMessageGenerate.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "util_string.h"
#include "cJSON.h"
#include "WISEPlatform.h"
#include "sys/time.h"

#define TAG_SUSICOMM_CMD				"commCmd"
#define TAG_SUSICOMM_AGENTID			"agentID"
#define TAG_SUSICOMM_HANDLER_NAME		"handlerName"
#define TAG_SUSICOMM_SENDTS				"sendTS"
#define TAG_SUSICOMM_TIMESTAMP			"$date"

#define TAG_AGENTINFO_DEVID				"devID"
#define TAG_AGENTINFO_HOSTNAME			"hostname"
#define TAG_AGENTINFO_SERIAL			"sn"
#define TAG_AGENTINFO_MAC				"mac"
#define TAG_AGENTINFO_SW_VERSION		"version"
#define TAG_AGENTINFO_TYPE				"type"
#define TAG_AGENTINFO_TAG				"tag"
#define TAG_AGENTINFO_PRODUCT_NAME		"product"
#define TAG_AGENTINFO_MANUFACTURE		"manufacture"
#define TAG_AGENTINFO_LOGIN_ACCOUNT		"account"
#define TAG_AGENTINFO_LOGIN_PASSWORD	"password"
#define TAG_AGENTINFO_CONNECT_STATUS	"status"

#define TAG_OSINFO_ROOT					"osInfo"
#define TAG_OSINFO_AGENT_VERSION		"cagentVersion"
#define TAG_OSINFO_OS_VERSION			"osVersion"
#define TAG_OSINFO_BIOS_VERSION			"biosVersion"
#define TAG_OSINFO_PLATFROM_NAME		"platformName"
#define TAG_OSINFO_PROCESSOR_NAME		"processorName"
#define TAG_OSINFO_OS_ARCH				"osArch"
#define TAG_OSINFO_TOTAL_MEMORY			"totalPhysMemKB"
#define TAG_OSINFO_MAC_LIST				"macs"
#define TAG_OSINFO_LOCAL_IP				"IP"

#define TAG_HANDLER_LIST				"handlerlist"

#define TAG_EVENT_NOTIFY				"eventnotify"
#define TAG_EVENT_SUBTYPE				"subtype"
#define TAG_EVENT_MESSAGE				"msg"
#define TAG_EVENT_EXTRAMSG				"extMsg"
#define TAG_EVENT_SEVERITY				"severity"
#define TAG_EVENT_HANDLER				"handler"

#define TAG_MESSAGE_CONTAIN				"content"

#pragma region Add_Resource

char *DEV_UTF8toAnsi(const char* str)
{
	int len = 0;
	char *strOutput = NULL;
	if(!IsUTF8(str))
	{
		len = strlen(str)+1;
		strOutput = (char *)malloc(len);
		memcpy(strOutput, str, len);
		
	}
	else
	{
		char * tempStr=UTF8ToANSI(str);
		len = strlen(tempStr)+1;
		strOutput = (char *)malloc(len);
		memcpy(strOutput, tempStr, len);
		StringFree(tempStr);
		tempStr = NULL;
	}
	return strOutput;	
}

char * DEV_AnsitoUTF8(char* wText)
{
	char * utf8RetStr = NULL;
	int tmpLen = 0;
	if(!wText)
		return utf8RetStr;
	if(!IsUTF8(wText))
	{
		char* tmputf8RetStr = ANSIToUTF8(wText);
		tmpLen = !tmputf8RetStr ? 0 : strlen(tmputf8RetStr);
		if(tmpLen == 1)
		{
			if(tmputf8RetStr) StringFree(tmputf8RetStr);
			tmputf8RetStr = UnicodeToUTF8((wchar_t *)wText);
			utf8RetStr = (char *)malloc(strlen(tmputf8RetStr)+1);
			memcpy(utf8RetStr, tmputf8RetStr, strlen(tmputf8RetStr));
			StringFree(tmputf8RetStr);
		}
		else if (tmpLen > 1)
		{
			utf8RetStr = (char *)malloc(tmpLen + 1);
			memcpy(utf8RetStr, tmputf8RetStr, tmpLen);
			StringFree(tmputf8RetStr);
		}
	}
	else
	{
		tmpLen = strlen(wText)+1;
		utf8RetStr = (char *)malloc(tmpLen);
		memcpy(utf8RetStr, wText, tmpLen);
	}
	return utf8RetStr;
}

long long DEV_GetTimeTick()
{
	return MSG_GetTimeTick();
}

bool DEV_CreateAgentInfo(susiaccess_agent_profile_body_t const * pProfile, int status, long long tick, char* strInfo, int length)
{
	int iRet = 0;
	if(!pProfile || !strInfo)
		return false;

#ifdef _WISEPAAS_02_DEF_H_
	iRet = snprintf(strInfo, length, DEF_AGENTINFO_JSON, pProfile->parentID?pProfile->parentID:"",
		pProfile->hostname?pProfile->hostname:"",
		pProfile->sn?pProfile->sn:(pProfile->mac?pProfile->mac:""),
		pProfile->mac?pProfile->mac:"",
		pProfile->version?pProfile->version:"",
		pProfile->type?pProfile->type:"IPC",
		pProfile->product?pProfile->product:"",
		pProfile->manufacture?pProfile->manufacture:"",
		pProfile->account?pProfile->account:"anonymous",
		pProfile->passwd?pProfile->passwd:"",
		status,
		pProfile->productId?pProfile->productId:"",
		pProfile->devId?pProfile->devId:"",
		tick);
#else
	iRet = snprintf(strInfo, length, DEF_AGENTINFO_JSON, pProfile->devId?pProfile->devId:"",
		pProfile->parentID?pProfile->parentID:"",
		pProfile->hostname?pProfile->hostname:"",
		pProfile->sn?pProfile->sn:(pProfile->mac?pProfile->mac:""),
		pProfile->mac?pProfile->mac:"",
		pProfile->version?pProfile->version:"",
		pProfile->type?pProfile->type:"IPC",
		pProfile->product?pProfile->product:"",
		pProfile->manufacture?pProfile->manufacture:"",
		pProfile->account?pProfile->account:"anonymous",
		pProfile->passwd?pProfile->passwd:"",
		status,
		pProfile->devId?pProfile->devId:"",
		tick);
#endif
	if(iRet>=0)
		return true;
	else
		return false;
}

bool DEV_GetAgentInfoTopic(char* devID, char * topic, int length)
{
	int iRet = 0;
	if(!devID || !topic)
		return false; 
	iRet = snprintf(topic, length, DEF_INFOACK_TOPIC, devID);
	if(iRet>=0)
		return true;
	else
		return false;
}

bool DEV_CreateWillMessage(susiaccess_agent_profile_body_t const * pProfile, long long tick, char* strInfo, int length)
{
	return DEV_CreateAgentInfo(pProfile, AGENT_STATUS_OFFLINE, tick, strInfo, length);
}

bool DEV_GetWillMessageTopic(char* devID, char * topic, int length)
{
	int iRet = 0;
	if(!devID || !topic)
		return false; 
	iRet = snprintf(topic, length, DEF_WILLMSG_TOPIC, devID);
	if(iRet>=0)
		return true;
	else
		return false;
}

bool DEV_CreateOSInfo(susiaccess_agent_profile_body_t* profile, long long tick, char* strInfo, int length)
{
	int iRet = 0;
	if(!profile || !strInfo)
		return false;

	iRet = snprintf(strInfo, length, DEF_OSINFO_JSON, profile->version?profile->version:"",
		profile->type?profile->type:"IPC",
		profile->osversion?profile->osversion:"",
		profile->biosversion?profile->biosversion:"",
		profile->platformname?profile->platformname:"",
		profile->processorname?profile->processorname:"",
		profile->osarchitect?profile->osarchitect:"",
		profile->totalmemsize,
		profile->maclist?profile->maclist:profile->mac,
		profile->localip?profile->localip:"");
	if(iRet>=0)
		return true;
	else
		return false;
}

bool DEV_GetActionReqTopic(char* devID, char* productTag, char * topic, int length)
{
	int iRet = 0;
	if(!devID)
		return false; 
	
#ifdef _WISEPAAS_02_DEF_H_
	iRet = snprintf(topic, length, DEF_CALLBACKREQ_TOPIC, productTag, devID);
#else
	iRet = snprintf(topic, length, DEF_AGENTACT_TOPIC, devID);
#endif
	if(iRet>=0)
		return true;
	else
		return false;
}

char*  DEV_CreateHandlerList(char* devID, char** handldelist, int count)
{
	cJSON* root = NULL;
	cJSON* datetime = NULL;
	char *buff = NULL;
	char *buff2 = NULL;
	if(!devID || !handldelist)
		return NULL;

	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, TAG_HANDLER_LIST, cJSON_CreateStringArray(handldelist, count));
	buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if(!buff)
		return NULL;

	buff2 = DEV_CreateFullMessage(devID, "general", 124, buff);
	free(buff);
	return buff2;
}

char* DEV_CreateEventNotify(char* subtype, char* message, char * extMsg)
{
	return DEV_CreateEventNotifyTs(subtype, message, extMsg, 0);
}

char* DEV_CreateEventNotifyTs(char* subtype, char* message, char * extMsg, unsigned long long timestamp)
{
	cJSON* root = NULL;
	char *buff = NULL;
	if(!subtype || !message)
		return buff;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, TAG_EVENT_SUBTYPE, subtype);
	cJSON_AddStringToObject(root, TAG_EVENT_MESSAGE, message);
	if(extMsg)
	{
		cJSON* msg = cJSON_Parse(extMsg);
		if(msg)
		{
			cJSON_AddItemToObject(root, TAG_EVENT_EXTRAMSG, msg); //fixed to 'general'
		}
		else
		{
			cJSON_AddStringToObject(root, TAG_EVENT_EXTRAMSG, extMsg); //fixed to 'general'
		}
	}

	if(timestamp > 0)
	{
		cJSON* oproot = cJSON_CreateObject();
		cJSON_AddNumberToObject(oproot,"$date", timestamp);
		cJSON_AddItemToObject(root,"opTS", oproot);
	}

	buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return buff;
}

char* DEV_CreateFullEventNotify(char* devID, int severity, char* handler, char* subtype, char* message, char * extMsg)
{
	return DEV_CreateFullEventNotifyTs(devID, severity, handler, subtype, message, extMsg, 0);
}

char* DEV_CreateFullEventNotifyTs(char* devID, int severity, char* handler, char* subtype, char* message, char * extMsg, unsigned long long timestamp)
{
	cJSON* root = NULL;
	cJSON* node = NULL;
	cJSON* oproot = NULL;
	cJSON* pfinfoNode = NULL;
	cJSON* datetime = NULL;
	char* buff = NULL;
	char* buff2 = NULL;
	if(!subtype || !message)
		return NULL;

	buff = DEV_CreateEventNotifyTs(subtype, message, extMsg, timestamp);
	if(!buff)
		return buff;

	root = cJSON_CreateObject();
	pfinfoNode = cJSON_CreateObject();
	//node = cJSON_Parse((const char *)requestData);
	if(buff)
	{
		node = cJSON_Parse((const char *)buff);
		free(buff);
	}
	
	if(node)
	{
		cJSON* chNode = node->child;
		while(chNode)
		{
			if(strcmp(chNode->string,"opTS")==0)
				oproot = cJSON_Duplicate(chNode, true);
			else
				cJSON_AddItemToObject(pfinfoNode, chNode->string, cJSON_Duplicate(chNode, true));	
			chNode = chNode->next;
		}
		cJSON_AddNumberToObject(pfinfoNode, TAG_EVENT_SEVERITY, severity);
		cJSON_AddStringToObject(pfinfoNode, TAG_EVENT_HANDLER, handler);
	}
	cJSON_Delete(node);

	cJSON_AddItemToObject(root, "eventnotify", pfinfoNode);	
	
	if(oproot == NULL)
	{
		long long tick = 0;
		oproot = cJSON_CreateObject();
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			tick = (long long)tv.tv_sec*1000 + (long long)tv.tv_usec/1000;
	    }
		cJSON_AddNumberToObject(oproot,"$date", tick);
	}
	
	if(oproot)
	{
		cJSON_AddItemToObject(root,"opTS", cJSON_Duplicate(oproot, true));
		cJSON_Delete(oproot);
	}

	buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	buff2 = DEV_CreateFullMessage(devID, handler, 2059, buff);
	free(buff);
	if(!buff2)
		return buff2;

	root = cJSON_Parse(buff2);
	free(buff2);
	if(!root)
		return NULL;

	buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return buff;
}

bool DEV_GetEventNotifyTopic(char* devID, char* productTag, char * topic, int length)
{
	int iRet = 0;
	if(!devID || !topic)
		return false; 
#ifdef _WISEPAAS_02_DEF_H_
		iRet = snprintf(topic, length, DEF_EVENTNOTIFY_TOPIC, productTag, devID);
#else
		iRet = snprintf(topic, length, DEF_EVENTNOTIFY_TOPIC, devID);
#endif
	if(iRet>=0)
		return true;
	else
		return false;
}


char* DEV_CreateFullMessage(char* devID, char* handler, int cmdId, char* msg)
{
	cJSON* root = NULL;
	char *buff = NULL;
	if(!devID || !handler)
		return buff;

	root = cJSON_CreateObject();

	if(msg != NULL)
	{
		cJSON* data = cJSON_Parse(msg);
		if(!data)	
		{
			data = cJSON_CreateString(msg);
		}
		cJSON_AddItemToObject(root, TAG_MESSAGE_CONTAIN, data);
	}
	
	
	if(cJSON_GetObjectItem(root, TAG_SUSICOMM_AGENTID) == NULL)
		cJSON_AddStringToObject(root, TAG_SUSICOMM_AGENTID, devID);

	if(cJSON_GetObjectItem(root, TAG_SUSICOMM_HANDLER_NAME) == NULL)
		cJSON_AddStringToObject(root, TAG_SUSICOMM_HANDLER_NAME, handler);

	if(cJSON_GetObjectItem(root, TAG_SUSICOMM_CMD) == NULL)
		cJSON_AddNumberToObject(root, TAG_SUSICOMM_CMD, cmdId); //fixed to 2059

	if(cJSON_GetObjectItem(root, TAG_SUSICOMM_SENDTS) == NULL)
	{
		cJSON* datetime = NULL;
		long long tick = DEV_GetTimeTick();
		datetime = cJSON_CreateObject();
		cJSON_AddItemToObject(root, TAG_SUSICOMM_SENDTS, datetime);
		cJSON_AddNumberToObject(datetime, TAG_SUSICOMM_TIMESTAMP, tick);
	}

	buff = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return buff;
}

#pragma endregion Add_Resource

void DEV_ReleaseBuffer(char* buff)
{
	if(buff != NULL)
		free(buff);
}

char* DEV_PrintPacket(susiaccess_packet_body_t const * pPacket)
{
	char *contain = NULL;
	char *buff = NULL;
	if(pPacket == NULL)
		return NULL;

	if(pPacket->content != NULL)
	{
		cJSON* root = cJSON_Parse(pPacket->content);
		if(!root)	
		{
			root = cJSON_CreateString(pPacket->content);
		}
		contain = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
	}
	
	buff = DEV_CreateFullMessage(pPacket->devId, pPacket->handlerName, pPacket->cmd, contain);
	if(contain != NULL)
		free(contain);
	return buff;
}

bool DEV_ParseMessage(const void* data, int datalen, susiaccess_packet_body_t * pkt)
{
	/*{"commCmd":251,"catalogID":4,"requestID":10}*/
	//char* strInput = NULL;
	cJSON* root = NULL;
	cJSON* devid = NULL;
	cJSON* target = NULL;
	cJSON* content = NULL;

	if(!data) return false;

	if(!pkt) return false;

	memset(pkt, 0 , sizeof(susiaccess_packet_body_t));

	//strInput = scparser_utf8toansi(data);

	//root = cJSON_Parse(strInput);
	root = cJSON_Parse(data);
	
	//free(strInput);
	//strInput = NULL;
	
	if(!root) return false;

	devid = cJSON_GetObjectItem(root, TAG_SUSICOMM_AGENTID);
	if(!devid)
	{
		pkt->content = strdup((char*)data);
		pkt->type = pkt_type_custom;
		cJSON_Delete(root);
		return true;
	}

	target = root->child;
	while (target)
	{
		if(!strcmp(target->string, TAG_SUSICOMM_CMD))
			pkt->cmd = target->valueint;
		else if(!strcmp(target->string, TAG_SUSICOMM_AGENTID))
			strncpy(pkt->devId, target->valuestring, sizeof(pkt->devId));
		else if(!strcmp(target->string, TAG_SUSICOMM_HANDLER_NAME))
			strncpy(pkt->handlerName, target->valuestring, sizeof(pkt->handlerName));
		else if(!strcmp(target->string, TAG_MESSAGE_CONTAIN))
		{
			if(!content)
				content = cJSON_CreateObject();

			cJSON_AddItemToObject(content, target->string, cJSON_Duplicate(target,true));
		}
		else
		{
			if(!content)
				content = cJSON_CreateObject();

			cJSON_AddItemToObject(content, target->string, cJSON_Duplicate(target,true));

			pkt->type = pkt_type_custom;
		}
		target = target->next;
	}

	if(content)
	{
		char* strInput = NULL;
		char* strcontent = cJSON_PrintUnformatted(content);
		cJSON_Delete(content);
		strInput = DEV_UTF8toAnsi(strcontent);
		pkt->content = strdup(strInput);
		free(strcontent);
		free(strInput);
		strInput = NULL;
	}

	cJSON_Delete(root);
	return true;
}
