#include "RuleParser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "srp/susiaccess_handler_api.h"

cJSON* RuleParser_FindThrehsoldNode(cJSON* root, char* devid, char* handlerName)
{
	int size = 0;
	int i=0;
	if(root == NULL || handlerName == NULL)
		return NULL;
	
	size = cJSON_GetArraySize(root);

	for(i=0; i<size; i++)
	{
		cJSON* newroot = NULL;
		cJSON* handlerNode = NULL;
		cJSON* body = NULL;
		cJSON* dev = NULL;
		cJSON* node = cJSON_GetArrayItem(root, i);
		if(node == NULL)
			continue;

		body = cJSON_GetObjectItem(node, AGENTINFO_BODY_STRUCT);
		if(body == NULL)
		{	
			body = cJSON_GetObjectItem(node, AGENTINFO_CONTENT_STRUCT);
			newroot = node;
		}
		else
		{
			newroot = body;
		}
		if(body == NULL)
			continue;

		if(devid != NULL && strlen(devid)>0)
		{
			dev = cJSON_GetObjectItem(newroot, AGENTINFO_DEVICEID);
			if(dev == NULL)
				continue;
			if(strcmp(dev->valuestring, devid) != 0)
				continue;
		}
		handlerNode = cJSON_GetObjectItem(newroot, AGENTINFO_HANDLERNAME);
		if(handlerNode == NULL)
			continue;

		if(strcmp(handlerNode->valuestring, handlerName) == 0)
		{
			cJSON*  target = cJSON_GetObjectItem(body, AGENTINFO_THRESHOLD);
			return target;
		}
	}
	return NULL;
}

bool RuleParser_AddThrehsoldNode(cJSON* root, char* devID, char* handlerName, cJSON* rule)
{
	char hName[MAX_TOPIC_LEN] = {0}; 
	cJSON *thresholds = NULL, *newrule = NULL;
	if(root == NULL, handlerName == NULL, rule == NULL)
		return false;
	if(handlerName == NULL)
		strcpy(hName, "All");
	else
		strncpy(hName, handlerName, sizeof(hName));
	thresholds = RuleParser_FindThrehsoldNode(root, devID, hName);
	if(thresholds == NULL)
	{
		cJSON* body = cJSON_CreateObject();
		cJSON* handlerNode = cJSON_CreateObject();
		cJSON_AddItemToArray(root, handlerNode);
		cJSON_AddItemToObject(handlerNode, AGENTINFO_BODY_STRUCT, body);
		cJSON_AddStringToObject(body, AGENTINFO_HANDLERNAME, hName);
		if(devID && strlen(devID) > 0)
		{
			cJSON_AddStringToObject(body, AGENTINFO_DEVICEID, devID);
		}
		
		thresholds = cJSON_CreateArray();
		cJSON_AddItemToObject(body, AGENTINFO_THRESHOLD, thresholds);
	}

	if(devID && strlen(devID) > 0)
	{
		cJSON *node = rule->child;

		while(node)
		{
			if(newrule == NULL)
				newrule = cJSON_CreateObject();
			if(strcmp(node->string, AGENTINFO_NAME)==0)
			{
				if(strstr(node->valuestring, devID)==0)
				{
					char newName[260] = {0};
					sprintf(newName, "%s/%s", devID, node->valuestring);
					cJSON_AddStringToObject(newrule, node->string, newName);
				}
				else
					cJSON_AddItemToObject(newrule, node->string, cJSON_Duplicate(node,1));
			}
			else
				cJSON_AddItemToObject(newrule, node->string, cJSON_Duplicate(node,1));
			node = node->next;
		}
	}
	else
	{
		newrule =  cJSON_Duplicate(rule,1);
	}

	cJSON_AddItemToArray(thresholds, newrule);
	return true;
}

bool RuleParser_GetHandlerInfo(cJSON* node, char** devid, char** name)
{
	char* tmp = NULL;
	char *p = NULL;

	if(node == NULL) 
		return false;

	tmp = node->valuestring;
	if (4 <= strlen(tmp) && ( strncmp("0000",tmp,4) == 0 || strncmp("0017",tmp,4) == 0)) 
	{
		char *q = NULL;
		p = strstr(tmp, "/");
		if(p == 0)
		{
			*devid = strdup(tmp);
		}
		else
		{
			*devid = (char*)calloc(1, p-tmp+1);
			strncpy(*devid, tmp, p-tmp);

			p++;
			q = strstr(p, "/");
			*name = (char*)calloc(1, q-p+1);
			strncpy(*name, p, q-p);
		}
	}
	else
	{
		*devid = NULL;
		p = strstr(tmp, "/");
		if(p == 0)
		{
			*name = strdup(tmp);
		}
		else
		{
			*name = (char*)calloc(1, p-tmp+1);
			strncpy(*name, tmp, p-tmp);
		}
	}
	
	return true;
}

char* RuleParser_GetHandlerName(cJSON* node)
{
	char* result = NULL;
	char* tmp = NULL;
	char *p = NULL;
	if(node == NULL) 
		return result;
	tmp = node->valuestring;
	p = strstr(tmp, "/");
	if(p == 0)
	{
		result = strdup(tmp);
	}
	else
	{
		result = (char*)calloc(1, p-tmp+1);
		strncpy(result, tmp, p-tmp);
	}
	return result;
}

bool RuleParser_ParseThrInfo(char * thrJsonStr, char* devID, char** thrHandlerStr)
{
	bool bRet = false;
	cJSON* pReqInfoHead = NULL;
	if(thrJsonStr == NULL || thrHandlerStr == NULL) return bRet;
	{
		cJSON * root = NULL;	
		pReqInfoHead = cJSON_CreateArray();
		root = cJSON_Parse(thrJsonStr);
		if(root)
		{
			cJSON * commDataItem = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
			if(commDataItem == NULL)
				commDataItem = cJSON_GetObjectItem(root, AGENTINFO_CONTENT_STRUCT);
			if(commDataItem)
			{
				cJSON * thrItem = NULL; 
				thrItem = commDataItem->child;  //Ignore thread name, ex: susictrlThr or nmThr

				while(thrItem->type != cJSON_Array)
				{
					thrItem = thrItem->next;
				}

				if(thrItem)
				{
					int size = cJSON_GetArraySize(thrItem);
					int i=0;
					for(i=0; i<size; i++)
					{
						char* curDevID = NULL;
						char* handlerName = NULL;
						cJSON * url = NULL;
						cJSON * rule = cJSON_GetArrayItem(thrItem, i);
						if(rule == NULL)
							continue;
						url = cJSON_GetObjectItem(rule, AGENTINFO_NAME);
						if(url == NULL)
							continue;
						if(RuleParser_GetHandlerInfo(url, &curDevID, &handlerName))
						{
							if(curDevID == NULL)
							{
								curDevID = (char *)calloc(1, strlen(devID)+1);
								strcpy(curDevID, devID);
							}
							RuleParser_AddThrehsoldNode(pReqInfoHead, curDevID, handlerName, rule);
							free(curDevID);
							free(handlerName);
						}
					}
				}
			}
			cJSON_Delete(root);
		}
	}

	if(pReqInfoHead)
	{
		*thrHandlerStr = cJSON_PrintUnformatted(pReqInfoHead);
		cJSON_Delete(pReqInfoHead);
		bRet = true;
	}
	return bRet;
}

bool RuleParser_PackSetThrRep(char * repStr, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(repStr == NULL || outputStr == NULL) return false;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, REPLY_SET_THR, repStr);

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(calloc(1, outLen));
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);	
	free(out);
	return true;
}

bool RuleParser_ActionCommand(char* itemname, char* devid, char* plugin, char* snesor)
{
	char* buff = NULL;
	char* data = NULL;

	if(itemname == NULL || devid == NULL || plugin == NULL || snesor == NULL)
		return false;

	buff = strstr(itemname,"/");
	if(buff == 0)
		return false;

	if(buff == itemname)
		buff = itemname+1;
	else
		buff = itemname;

	data = strstr(buff,"/");
	if(data)
	{
		strncpy(devid, buff, data-buff);
		buff = data+1;
	}
	else
		return false;

	data = strstr(buff, "/");
	if(data)
	{
		strncpy(plugin, buff, data-buff);
		buff = data+1;
	}
	else
		return false;

	strcpy(snesor, buff);

	return true;
}