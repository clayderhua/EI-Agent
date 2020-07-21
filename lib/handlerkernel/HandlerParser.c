#include "HandlerParser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "WISEPlatform.h"
#include "srp/susiaccess_handler_ex_api.h"
#include "cJSON.h"

#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"
#define AGENTINFO_SESSIONID				"sessionID"
#define AGENTINFO_CONTENT				"content"

#define REQUEST_ITEMS					"requestItems"
#define REQUEST_ITEMS_LIST				"e"
#define REQUEST_ITEMS_NAME				"n"
#define REQUEST_ALL						"All"
#define REQUEST_THR_ENABLE				"enable"
#define REQUEST_THR_MAX					"max"
#define REQUEST_THR_MIN					"min"
#define REQUEST_THR_TYPE				"type"
#define REQUEST_THR_LTIME				"lastingTimeS"
#define REQUEST_THR_ITIME				"intervalTimeS"
#define REQUEST_SENSOR_ITEMS			"sensorIDList"
#define REQUEST_THR_ACTION				"action"
#define REQUEST_ACT_ONCE				"once"
#define REQUEST_ACT_NORMAL				"normal"

#define REPLY_SET_THR					"setThrRep"
#define REPLY_DEL_ALL_THR				"delAllThrRep"
#define REPLY_THR_CHECK_STATUS			"thrCheckStatus"
#define REPLY_THR_CHECK_MSG				"thrCheckMsg"
#define REPLY_SENSOR_ITEMS				"sensorInfoList"

#define REPORT_INTERVAL_SEC				"autoUploadIntervalSec"
#define REPORT_INTERVAL_MS				"autoUploadIntervalMs"
#define REPORT_TIMEOUT_MS				"autoUploadTimeoutMs"

bool HANDLERPARSER_API HandlerParser_ParseReceivedCMD(void* data, int datalen, int * cmdID, char* sessionID)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(data == NULL) return false;
	if(datalen<=0) return false;

	root = cJSON_Parse((char *)data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		body = root;
	}

	target = cJSON_GetObjectItem(body, AGENTINFO_CMDTYPE);
	if(target)
	{
		*cmdID = target->valueint;
	}

	if(sessionID != NULL)
	{
		target = cJSON_GetObjectItem(body, AGENTINFO_SESSIONID);
		if(target)
		{
			strcpy(sessionID, target->valuestring);
		}
		else
		{
			target = cJSON_GetObjectItem(body, AGENTINFO_CONTENT);
			if(target)
			{
				target = cJSON_GetObjectItem(target, AGENTINFO_SESSIONID);
				if(target)
				{
					strncpy(sessionID, target->valuestring, MAX_SESSION_LEN);
				}
			}
		}		
	}

	cJSON_Delete(root);
	return true;
}

bool HANDLERPARSER_API HandlerParser_ParseAutoReportCmd(char * cmdJsonStr, char * handlername, unsigned int * intervalTimeS, char ** reqItems, bool * reqAll)
{
	/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["HWM"],"commCmd":2053,"type":"WSN"}}*/
	bool bRet = false;
	cJSON* root = NULL;

	if(cmdJsonStr == NULL || NULL == intervalTimeS || reqItems == NULL || reqAll == NULL) return false;

	root = cJSON_Parse(cmdJsonStr);
	if(root)
	{
		cJSON* target;
		cJSON*  body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(!body)
		{
			body = cJSON_GetObjectItem(root, AGENTINFO_CONTENT);
			if(!body)
			{
				cJSON_Delete(root);
				return bRet;
			}
		}

		target = cJSON_GetObjectItem(body, REPORT_INTERVAL_SEC);
		if(target)
		{
			*intervalTimeS = target->valueint;
			target = cJSON_GetObjectItem(body, REQUEST_ITEMS);
			if(target)
			{
				cJSON* items;
				if(cJSON_GetObjectItem(target, "All") != NULL)
				{
					bRet = true;
					*reqAll = true;
				}
				else if((items = cJSON_GetObjectItem(target, handlername)) != NULL)
				{
					char check[256] = {0};
					char * tmpJsonStr = cJSON_PrintUnformatted(items);
					sprintf(check, "{\"e\":[{\"n\":\"%s\"}]}", handlername);
					if(strcmp(tmpJsonStr, check) == 0)
					{
						*reqAll = true;
					}
					else
					{
						cJSON *eArray, *dateEntry;
						int len;
						char optsSelect[128];

						// append "handlername/opTS/$date" to always select opTS (customize opTS)
						// free first
						free(tmpJsonStr);
						// get "e" array
						eArray = cJSON_GetObjectItem(items, TAG_E_NODE);
						// append opTS/$date
						dateEntry = cJSON_CreateObject();
						sprintf(optsSelect, "%s/opTS/$date", handlername);
						cJSON_AddStringToObject(dateEntry, "n", optsSelect);
						cJSON_AddItemToArray(eArray, dateEntry);
						// convert to string
						tmpJsonStr = cJSON_PrintUnformatted(items);
						
						// allocate reqItems 
						len = strlen(tmpJsonStr) + 1;
						*reqItems = (char *)calloc(1, len + 1);
						strcpy(*reqItems, tmpJsonStr);
						*reqAll = false;
					}
					free(tmpJsonStr);
					bRet = true;
				}
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

bool HANDLERPARSER_API HandlerParser_ParseAutoUploadCmd(char * cmdJsonStr, char * handlername, unsigned int * intervalTimeMs, unsigned int * timeoutMs, char ** reqItems, bool * reqAll)
{
	bool bRet = false;
	cJSON* root = NULL;

	if(cmdJsonStr == NULL || NULL == intervalTimeMs || timeoutMs == NULL || reqItems == NULL || reqAll == NULL) return false;

	root = cJSON_Parse(cmdJsonStr);
	if(root)
	{
		cJSON* target;
		cJSON* body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(!body)
		{
			body = cJSON_GetObjectItem(root, AGENTINFO_CONTENT);
			if(!body)
			{
				cJSON_Delete(root);
				return bRet;
			}
		}
		target = cJSON_GetObjectItem(body, REPORT_INTERVAL_MS);
		if(target)
		{
			*intervalTimeMs = target->valueint;
			target = cJSON_GetObjectItem(body, REPORT_TIMEOUT_MS);
			if(target)
			{
				*timeoutMs = target->valueint;
				target = cJSON_GetObjectItem(body, REQUEST_ITEMS);
				if(target)
				{
					cJSON* items;
					if(cJSON_GetObjectItem(target, "All") != NULL)
					{
						bRet = true;
						*reqAll = true;
					}
					else if((items = cJSON_GetObjectItem(target, REQUEST_ITEMS_LIST)) != NULL)
					{
						int size = cJSON_GetArraySize(items);
						int i=0;
						for(i=0;i<size;i++)
						{
							cJSON *item = cJSON_GetArrayItem(items, i);
							if(item)
							{
								cJSON* name = cJSON_GetObjectItem(item, REQUEST_ITEMS_NAME);
								if(strcmp(name->valuestring, handlername) == 0)
								{
									*reqAll = true;
									bRet = true;
									break;
								}
							}
						}

						if(!bRet)
						{
							char * tmpJsonStr = cJSON_PrintUnformatted(items);
							int len = strlen(tmpJsonStr) + 1;
							*reqItems = (char *)calloc(1, len + 1);
							strcpy(*reqItems, tmpJsonStr);
							*reqAll = false;
							free(tmpJsonStr);
							bRet = true;
						}							
					}
				}
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

bool HandlerParser_ParseThrAction(cJSON * jsonObj, thr_item_info_t * pThrItemInfo)
{
	int size = 0;
	int i = 0;
	if(jsonObj == NULL)
		return false;
	if(pThrItemInfo == NULL)
		return false;

	size = cJSON_GetArraySize(jsonObj);

	for(i=0; i<size;i++)
	{
		
		cJSON * pAction = cJSON_GetArrayItem(jsonObj, i);
		if(pAction)
		{
			thr_action_t* target = NULL;

			cJSON * pItem = cJSON_GetObjectItem(pAction, REQUEST_ITEMS_NAME);
			if(pItem)
			{
				cJSON * pSubItem = NULL;
				/*thr_action_t* target = NULL;
				thr_action_t* pAct = pThrItemInfo->pActionList;
				while(pAct)
				{
					if(strcmp(pItem->valuestring, pAct->pathname)==0)
					{
						target = pAct;
						break;
					}
					pAct = pAct->next;
				}*/

				if(target == NULL)
				{
					target = malloc(sizeof(thr_action_t));
					memset(target, 0, sizeof(thr_action_t));
					strncpy(target->pathname, pItem->valuestring, sizeof(target->pathname));
					if(pThrItemInfo->pActionList == NULL)
						pThrItemInfo->pActionList = target;
					else
					{
						thr_action_t* pAct = pThrItemInfo->pActionList;
						while(pAct)
						{
							if(pAct->next == NULL)
							{
								pAct->next = target;
								break;
							}
							pAct = pAct->next;
						}
					}
				}
				else
				{
					if(target->type == action_type_string)
						free(target->sv);
				}
				
				pSubItem =  cJSON_GetObjectItem(pAction, "bv");
				if(pSubItem)
				{
					target->type = action_type_boolean;
					if(pSubItem->type == cJSON_False)
					{
						target->bv = false;
					}
					else if(pSubItem->type == cJSON_True)
					{
						target->bv = true;
					}
					else if(pSubItem->type == cJSON_Number)
					{
						target->bv = pSubItem->valueint==1?true:false;
					}
				}
				else
				{
					pSubItem =  cJSON_GetObjectItem(pAction, "v");
					if(pSubItem)
					{
						target->type = action_type_numeric;
						target->v = pSubItem->valuedouble;
					}
					else
					{
						pSubItem =  cJSON_GetObjectItem(pAction, "sv");
						if(pSubItem)
						{
							target->type = action_type_string;
							target->sv = strdup(pSubItem->valuestring);
						}
					}
				}

				pSubItem =  cJSON_GetObjectItem(pAction, REQUEST_ACT_ONCE);
				if(pSubItem)
				{
					target->once = pSubItem->type==cJSON_True?true:false;
				}

				pSubItem =  cJSON_GetObjectItem(pAction, REQUEST_ACT_NORMAL);
				if(pSubItem)
				{
					target->normal = pSubItem->type==cJSON_True?true:false;
				}
			}
		}

	}

	return true;
}

bool HandlerParser_ParseThrItemInfo(cJSON * jsonObj, thr_item_info_t * pThrItemInfo)
{
	bool bRet = false;
	if(jsonObj == NULL || pThrItemInfo == NULL) return bRet;
	{
		cJSON * pSubItem = NULL;
		pSubItem = cJSON_GetObjectItem(jsonObj, REQUEST_ITEMS_NAME);
		if(pSubItem)
		{
			strncpy(pThrItemInfo->pathname, pSubItem->valuestring, sizeof(pThrItemInfo->pathname));

			pThrItemInfo->isEnable = false;
				
			pSubItem = cJSON_GetObjectItem(jsonObj, REQUEST_THR_ENABLE);
			if(pSubItem)
			{
				if(pSubItem->type == cJSON_False)
					pThrItemInfo->isEnable = false;
				else if(pSubItem->type == cJSON_True)
					pThrItemInfo->isEnable = true;
				else if(pSubItem->type == cJSON_String)
				{
					if(strcasecmp(pSubItem->valuestring, "true")==0)
					{
						pThrItemInfo->isEnable = true;
					}
				}
			}

			pSubItem = cJSON_GetObjectItem(jsonObj, REQUEST_THR_MAX);
			if(pSubItem)
			{
				pThrItemInfo->maxThr = (float)pSubItem->valuedouble;
			}
			else
			{
				pThrItemInfo->maxThr = 0;
			}

			pSubItem = cJSON_GetObjectItem(jsonObj, REQUEST_THR_MIN);
			if(pSubItem)
			{
				pThrItemInfo->minThr = (float)pSubItem->valuedouble;
			}
			else
			{
				pThrItemInfo->minThr = 0;
			}

			pSubItem = cJSON_GetObjectItem(jsonObj, REQUEST_THR_TYPE);
			if(pSubItem)
			{
				pThrItemInfo->rangeType = pSubItem->valueint;
			}
			else
			{
				pThrItemInfo->rangeType = range_unknow;
			}

			pSubItem = cJSON_GetObjectItem(jsonObj,  REQUEST_THR_LTIME);
			if(pSubItem)
			{
				pThrItemInfo->lastingTimeS = pSubItem->valueint;
			}
			else
			{
				pThrItemInfo->lastingTimeS = DEF_INVALID_TIME;
			}

			pSubItem = cJSON_GetObjectItem(jsonObj, REQUEST_THR_ITIME);
			if(pSubItem)
			{
				pThrItemInfo->intervalTimeS = pSubItem->valueint;
			}
			else
			{
				pThrItemInfo->intervalTimeS = DEF_INVALID_TIME;
			}
			
			pSubItem = cJSON_GetObjectItem(jsonObj, REQUEST_THR_ACTION);
			if(pSubItem)
			{
				HandlerParser_ParseThrAction(pSubItem, pThrItemInfo);
			}
			

			pThrItemInfo->checkRetValue = 0;
			pThrItemInfo->checkSrcValList.head = NULL;
			pThrItemInfo->checkSrcValList.nodeCnt = 0;
			pThrItemInfo->checkType = ck_type_avg;
			pThrItemInfo->isNormal = true;
			pThrItemInfo->isInvalid = false;
			pThrItemInfo->repThrTime = 0;
			pThrItemInfo->isTriggered = false;
			//pThrItemInfo->on_triggered = NULL;
			bRet = true;
		}
	}
	return bRet;
}

bool HandlerParser_ParseThrItemList(cJSON * jsonObj, thr_item_list thrItemList)
{
	bool bRet = false;
	if(jsonObj == NULL || thrItemList == NULL) return bRet;
	{
		thr_item_node_t * head = thrItemList;
		cJSON * subItem = NULL;
		int nCount = cJSON_GetArraySize(jsonObj);
		int i = 0;
		for(i=0; i<nCount; i++)
		{
			subItem = cJSON_GetArrayItem(jsonObj, i);
			if(subItem)
			{
				thr_item_node_t * pThrItemNode = NULL;
				pThrItemNode = (thr_item_node_t *)malloc(sizeof(thr_item_node_t));
				memset(pThrItemNode, 0, sizeof(thr_item_node_t));
				if(HandlerParser_ParseThrItemInfo(subItem, &pThrItemNode->thrItemInfo))
				{
					pThrItemNode->next = head->next;
					head->next = pThrItemNode;
					pThrItemNode->thrItemInfo.isTriggered = false;
				}
				else
				{
					free(pThrItemNode);
					pThrItemNode = NULL;
				}
			}
		}
		bRet = true;
	}
	return bRet;
}

bool HANDLERPARSER_API HandlerParser_ParseThrInfo(char * thrJsonStr, thr_item_list thrList)
{
	bool bRet = false;
	if(thrJsonStr == NULL || thrList == NULL) return bRet;
	{
		cJSON * root = NULL;
		root = cJSON_Parse(thrJsonStr);
		if(root)
		{
			cJSON * thrItem = NULL; 
			cJSON * body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
			if(!body)
			{
				body = cJSON_GetObjectItem(root, AGENTINFO_CONTENT);
				if(!body)
				{
					cJSON_Delete(root);
					return bRet;
				}
			}

			thrItem = body->child;  /*Ignore thread name, ex: susictrlThr or nmThr*/
			
			while(thrItem->type != cJSON_Array)
			{
				thrItem = thrItem->next;
			}

			if(thrItem)
			{
				HandlerParser_ParseThrItemList(thrItem, thrList);
				bRet = true;
			}
			cJSON_Delete(root);
		}
	}
	return bRet;
}

bool HANDLERPARSER_API HandlerParser_PackSetThrRep(char * repStr, char ** outputStr)
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

bool HANDLERPARSER_API HandlerParser_PackDelAllThrRep(char * repStr, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(repStr == NULL || outputStr == NULL) return false;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, REPLY_DEL_ALL_THR, repStr);

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(calloc(1, outLen));
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);	
	free(out);
	return true;
}

bool HANDLERPARSER_API HandlerParser_ParseSensorDataCmd(char * cmdJsonStr, char ** reqItems)
{
	bool bRet = false;
	cJSON* root = NULL;

	if(cmdJsonStr == NULL || reqItems == NULL) return false;

	root = cJSON_Parse(cmdJsonStr);
	if(root)
	{
		cJSON* target;
		cJSON* body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(!body)
		{
			body = cJSON_GetObjectItem(root, AGENTINFO_CONTENT);
			if(!body)
			{
				cJSON_Delete(root);
				return bRet;
			}
		}

		target = cJSON_GetObjectItem(body, REQUEST_SENSOR_ITEMS);
		if(target)
		{
			cJSON* items = cJSON_GetObjectItem(target, REQUEST_ITEMS_LIST);
			if(items != NULL)
			{
				char * tmpJsonStr = cJSON_PrintUnformatted(items);
				int len = strlen(tmpJsonStr) + 1;
				*reqItems = (char *)calloc(1, len + 1);
				strcpy(*reqItems, tmpJsonStr);
				free(tmpJsonStr);
				bRet = true;					
			}
		}

		cJSON_Delete(root);
	}
	return bRet;
}

bool HANDLERPARSER_API HandlerParser_PackSensorCMDRep(char * repStr, char * sessionID , char ** outputStr)
{
	cJSON* pReqRoot = NULL;
	cJSON *pReqItemRoot, *pSensorRoot;
	char * out = NULL;
	int outLen = 0;

	if(repStr == NULL || outputStr == NULL) return false;
	pReqItemRoot = cJSON_Parse(repStr);
	if(pReqItemRoot == NULL) return false;
	pReqRoot = cJSON_CreateObject();
	pSensorRoot = cJSON_CreateObject();
	cJSON_AddStringToObject(pReqRoot, AGENTINFO_SESSIONID, sessionID);
	cJSON_AddItemToObject(pReqRoot, REPLY_SENSOR_ITEMS, pSensorRoot);
	cJSON_AddItemToObject(pSensorRoot, REQUEST_ITEMS_LIST, pReqItemRoot);

	out = cJSON_PrintUnformatted(pReqRoot);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(calloc(1, outLen));
	strcpy(*outputStr, out);
	cJSON_Delete(pReqRoot);	
	free(out);
	return true;
}

bool HANDLERTHRESHOLD_API HandlerParser_QueueingAction(MSG_CLASSIFY_T* pCapability,thr_item_list curThrItemList, void* pTriggerQueue, void (*on_triggered)(void* qtrigger, struct thr_item_info_t* item, MSG_ATTRIBUTE_T* attr))
{
	thr_item_node_t* curThrItem = NULL;
	if(curThrItemList == NULL || on_triggered == NULL || pTriggerQueue == NULL)
		return false;
	curThrItem = curThrItemList->next;
	while(curThrItem)
	{
		if(curThrItem->thrItemInfo.isTriggered)
		{
			MSG_ATTRIBUTE_T* attr = NULL;
			if(pCapability)
				attr = IoT_FindSensorNodeWithPath(pCapability, curThrItem->thrItemInfo.pathname);
			on_triggered(pTriggerQueue, &curThrItem->thrItemInfo, attr);
			curThrItem->thrItemInfo.isTriggered = false;
		}
		curThrItem = curThrItem->next;
	}
	return true;
}
