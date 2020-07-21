#include "pktparser.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "cJSON.h"

#define BASICINFO_BODY_STRUCT			"susiCommData"
#define BASICINFO_CONTENT				"content"
#define BASICINFO_HANDLERNAME			"handlerName"
#define BASICINFO_CMDTYPE				"commCmd"
#define BASICINFO_AGENTID				"agentID"
#define BASICINFO_TIMESTAMP				"sendTS"

#define AGENTINFO_DEVID					"devID"
#define AGENTINFO_HOSTNAME				"hostname"
#define AGENTINFO_SN					"sn"
#define AGENTINFO_MAC					"mac"
#define AGENTINFO_VERSION				"version"
#define AGENTINFO_TYPE					"type"
#define AGENTINFO_PRODUCT				"product"
#define AGENTINFO_MANUFACTURE			"manufacture"
#define AGENTINFO_STATUS				"status"

#define GLOBAL_OS_INFO					"osInfo"
#define GLOBAL_OS_VERSION				"osVersion"
#define GLOBAL_AGENT_VERSION			"cagentVersion"
#define GLOBAL_AGENT_TYPE				"cagentType"
#define GLOBAL_BIOS_VERSION				"biosVersion"
#define GLOBAL_PLATFORM_NAME			"platformName"
#define GLOBAL_PROCESSOR_NAME			"processorName"
#define GLOBAL_OS_ARCH					"osArch"
#define GLOBAL_SYS_TOTAL_PHYS_MEM		"totalPhysMemKB"
#define GLOBAL_SYS_MACS					"macs"
#define GLOBAL_SYS_IP					"IP"

char *pkg_parser_print(PJSON item)
{
	if(item == NULL)
		return NULL;
	return cJSON_Print(item);
}

char *pkg_parser_print_unformatted(PJSON item)
{
	cJSON *node = NULL;
	if(item == NULL)
		return NULL;
	node = (cJSON *)item;
	return cJSON_PrintUnformatted(node);
}

void pkg_parser_free(PJSON ptr)
{
	cJSON *pAgentInfo = ptr;
	cJSON_Delete(pAgentInfo);
}

PJSON pkg_parser_packet_create(bool bRMMSupport, susiaccess_packet_body_t const * pPacket)
{
	/*
{"commCmd":271,"requestID":103, XXX}
	*/
   cJSON *pReqInfoHead = NULL;
   cJSON* root = NULL;
   cJSON* datetime = NULL;
   long long tick = 0;

   if(!pPacket) return NULL;
   if(pPacket->content)
	   pReqInfoHead = cJSON_Parse(pPacket->content);
   else
	   pReqInfoHead = cJSON_CreateObject();

   if(!pReqInfoHead) return NULL;

   if(!bRMMSupport)
   {
	   cJSON* node = cJSON_CreateObject();
	   cJSON_AddItemToObject(node, BASICINFO_CONTENT, pReqInfoHead);
	   pReqInfoHead = node;
   }

   cJSON_AddNumberToObject(pReqInfoHead, BASICINFO_CMDTYPE, pPacket->cmd);
   cJSON_AddStringToObject(pReqInfoHead, BASICINFO_AGENTID, pPacket->devId);
   cJSON_AddStringToObject(pReqInfoHead, BASICINFO_HANDLERNAME, pPacket->handlerName);
   //cJSON_AddNumberToObject(root, BASICINFO_CATALOG, pPacket->catalogID);

   {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		tick = (long long)tv.tv_sec*1000 + (long long)tv.tv_usec/1000;
   }
   datetime = cJSON_CreateObject();
   cJSON_AddItemToObject(pReqInfoHead, "sendTS", datetime);
   cJSON_AddNumberToObject(datetime, "$date", tick);

   if(bRMMSupport)
   {
	   root = cJSON_CreateObject();
	   cJSON_AddItemToObject(root, BASICINFO_BODY_STRUCT, pReqInfoHead);
   }
   else
	   root = pReqInfoHead;

   return root;
}

char* pkg_parser_packet_print(susiaccess_packet_body_t * pkt)
{
	char* pReqInfoPayload = NULL;
	PJSON ReqInfoJSON = NULL;
	if(!pkt)
		return pReqInfoPayload;
	ReqInfoJSON = pkg_parser_packet_create(false, pkt);
	pReqInfoPayload = pkg_parser_print_unformatted(ReqInfoJSON);
	pkg_parser_free(ReqInfoJSON);
	return pReqInfoPayload;
}

char* pkg_parser_internel_packet_print(susiaccess_packet_body_t * pkt)
{
	char* pReqInfoPayload = NULL;
	PJSON ReqInfoJSON = NULL;
	if(!pkt)
		return pReqInfoPayload;
	ReqInfoJSON = pkg_parser_packet_create(true, pkt);
	pReqInfoPayload = pkg_parser_print_unformatted(ReqInfoJSON);
	pkg_parser_free(ReqInfoJSON);
	return pReqInfoPayload;
}

int pkg_parser_recv_message_parse(void* data, int datalen, susiaccess_packet_body_t * pkt)
{
	/*{"commCmd":251,"catalogID":4,"requestID":10}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* content = NULL;
	bool bRMMSupport = true;
	if(!data) return false;

	if(!pkt) return false;

	memset(pkt, 0 , sizeof(susiaccess_packet_body_t));
	pkt->type = pkt_type_custom;
	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, BASICINFO_BODY_STRUCT);
	if(!body)
	{
		bRMMSupport = false;
		body = root;
	}

	target = body->child;
	while (target)
	{
		if(!strcmp(target->string, BASICINFO_CMDTYPE))
		{
			pkt->cmd = target->valueint;
			pkt->type = bRMMSupport?pkt_type_susiaccess:pkt_type_susiaccess;
		}
		else if(!strcmp(target->string, BASICINFO_AGENTID))
		{
			pkt->type = bRMMSupport?pkt_type_susiaccess:pkt_type_susiaccess;
			strncpy(pkt->devId, target->valuestring, sizeof(pkt->devId));
		}
		else if(!strcmp(target->string, BASICINFO_HANDLERNAME))
		{
			pkt->type = bRMMSupport?pkt_type_susiaccess:pkt_type_susiaccess;
			strncpy(pkt->handlerName, target->valuestring, sizeof(pkt->handlerName));
		}
		else
		{
			if(!content)
				content = cJSON_CreateObject();

			cJSON_AddItemToObject(content, target->string, cJSON_Duplicate(target,true));
		}
		target = target->next;
	}

	if(content)
	{
		char* strcontent = cJSON_PrintUnformatted(content);
		cJSON_Delete(content);
		pkt->content = calloc(strlen(strcontent)+1, 1);
		if(pkt->content)
			strncpy(pkt->content, strcontent, strlen(strcontent));
		free(strcontent);
	}

	cJSON_Delete(root);
	return true;
}