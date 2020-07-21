#include "ThresholdCheck.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "srp/susiaccess_handler_api.h"
#include "RuleEngineLog.h"
#include "cJSON.h"
#include "WISEPlatform.h"
#include "actionqueue.h"
#include "IoTMessageGenerate.h"
#include "IPSOParser.h"

void* g_pTriggerQueue = NULL;

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

bool ParseThrAction(cJSON * jsonObj, thr_item_info_t * pThrItemInfo)
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
					target = (thr_action_t*)malloc(sizeof(thr_action_t));
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


bool ParseThrItemInfo(cJSON * jsonObj, thr_item_info_t * pThrItemInfo)
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
				pThrItemInfo->rangeType = (range_type_t)pSubItem->valueint;
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
				ParseThrAction(pSubItem, pThrItemInfo);
			}
			

			pThrItemInfo->checkRetValue = 0;
			pThrItemInfo->checkSrcValList.head = NULL;
			pThrItemInfo->checkSrcValList.nodeCnt = 0;
			pThrItemInfo->checkType = ck_type_avg;
			pThrItemInfo->isNormal = true;
			pThrItemInfo->isInvalid = false;
			pThrItemInfo->repThrTime = 0;
			pThrItemInfo->isTriggered = false;
			bRet = true;
		}
	}
	return bRet;
}


bool ParseThrItemList(cJSON * jsonObj, thr_item_list thrItemList, void (*on_triggered)(void* qtrigger, struct thr_item_info_t* item, MSG_ATTRIBUTE_T* attr))
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
				if(ParseThrItemInfo(subItem, &pThrItemNode->thrItemInfo))
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

bool ParseThrInfo(char * thrJsonStr, thr_item_list thrList, void (*on_triggered)(void* qtrigger, struct thr_item_info_t* item, MSG_ATTRIBUTE_T* attr))
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
				ParseThrItemList(thrItem, thrList, on_triggered);
				bRet = true;
			}
			cJSON_Delete(root);
		}
	}
	return bRet;
}

bool PackSetThrRep(char * repStr, char ** outputStr)
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


void ThresholdCheck_Initialize(void (*on_triggered)(bool isnormal,
													char* sensorname, double value, MSG_ATTRIBUTE_T* attr,
													void *pRev))
{
	if(g_pTriggerQueue == NULL)
		g_pTriggerQueue = actionqueue_init(10);
	else
		RuleEngineLog(Error, " %s> Trigger Queue is Exist!", "ThresholdCheck");

	actionqueue_setcb(g_pTriggerQueue, on_triggered);
}

void ThresholdCheck_Uninitialize()
{
	if(g_pTriggerQueue)
	{
		actionqueue_uninit(g_pTriggerQueue);
		g_pTriggerQueue = NULL;
	}
}

void ThresholdCheck_On_Triggered(void* qtrigger, struct thr_item_info_t* item, MSG_ATTRIBUTE_T* attr)
{
	actionqueue_push(qtrigger, item, attr);
}


bool ThresholdCheck_SetThreshold(thr_item_list pThresholdList, char *pInQuery, char** result, char** warnmsg)
{
	bool bRet = false;
	char * repJsonStr = NULL;
	char repMsg[256] = {0};
	thr_item_list tmpThrItemList = NULL;
	char* buffer = NULL;	
	HANDLER_NOTIFY_SEVERITY severity = Severity_Debug;

	if( pInQuery == NULL)
		return bRet;

	if(pThresholdList == NULL)
		return bRet;

	tmpThrItemList = HandlerThreshold_CreateThrList();
	
	if(!ParseThrInfo(pInQuery, tmpThrItemList, ThresholdCheck_On_Triggered))
	{
		sprintf(repMsg, "%s", "Threshold apply failed!");
		bRet = false;
		goto SET_THRESHOLD_EXIT;
	}

	//pthread_mutex_lock(&pHandlerKernel->ThresholdChkContex.mux);
	

	buffer = (char*)calloc(1,1024);	
	if(HandlerThreshold_UpdateThrInfoList(pThresholdList, tmpThrItemList, &buffer, 1024))
	{
		if(strlen(buffer) > 0)
		{
			bool bNormal = false;
			HandlerThreshold_IsThrItemListNormal(pThresholdList, &bNormal);
			*warnmsg = (char*)calloc(1, strlen(buffer) + 128);
			sprintf(*warnmsg,"{\"subtype\":\"THRESHOLD_CHECK_INFO\",\"thrCheckStatus\":\"%s\",\"msg\":\"%s\"}", bNormal?"True":"False", buffer); /*for custom handler*/
			//sprintf(message,"{\"thrCheckStatus\":\"True\",\"thrCheckMsg\":\"%s\"}", buffer); /*original for standard handler*/
		}
	}
	free(buffer);
	//pthread_mutex_unlock(&pHandlerKernel->ThresholdChkContex.mux);

	sprintf(repMsg, "%s", "Threshold rule apply OK!");
	bRet = true;

SET_THRESHOLD_EXIT:
	HandlerThreshold_DestroyThrList(tmpThrItemList);
	PackSetThrRep(repMsg, result);
	return bRet;
}

void ThresholdCheck_WhenDelThrCheckNormal(thr_item_list thrItemList, char** checkMsg, unsigned int bufLen)
{
	if(NULL == thrItemList || NULL == checkMsg || bufLen == 0) return;
	{
		thr_item_list curThrItemList = thrItemList;
		thr_item_node_t * curThrItemNode = curThrItemList->next;
		char *tmpMsg = NULL;
		unsigned int defbufLen = bufLen;
		while(curThrItemNode)
		{
			if(curThrItemNode->thrItemInfo.isEnable && !curThrItemNode->thrItemInfo.isNormal)
			{
				curThrItemNode->thrItemInfo.isNormal = true;
				//sprintf(tmpMsg, "%d normal", curThrItemNode->thrItemInfo.id);
				{
					int len = strlen(curThrItemNode->thrItemInfo.pathname)+strlen(DEF_NOR_EVENT_STR)+32;
					tmpMsg = (char*)calloc(1, len);
				}
				sprintf(tmpMsg, "%s %s", curThrItemNode->thrItemInfo.pathname, DEF_NOR_EVENT_STR);
			}
			if(tmpMsg && strlen(tmpMsg))
			{
				if(defbufLen<strlen(tmpMsg)+strlen(*checkMsg)+1)
				{
					int newLen = strlen(tmpMsg) + strlen(*checkMsg) + 1024;
					*checkMsg = (char *)realloc(*checkMsg, newLen);
					defbufLen = newLen;
				}	
				if(strlen(*checkMsg))
				{
					sprintf(*checkMsg, "%s;%s", *checkMsg, tmpMsg);
				}
				else
				{
					sprintf(*checkMsg, "%s", tmpMsg);
				}
			}
         if(tmpMsg)free(tmpMsg);
			tmpMsg = NULL;
			curThrItemNode = curThrItemNode->next;
		}
	}
}

bool ThresholdCheck_DeleteAllThrNode(thr_item_list thrList)
{
	bool bRet = false;
	thr_item_node_t * delNode = NULL, *head = NULL;
	if(thrList == NULL) return bRet;
	head = thrList;

	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->thrItemInfo.checkSrcValList.head)
		{
			check_value_node_t * frontValueNode = delNode->thrItemInfo.checkSrcValList.head;
			check_value_node_t * delValueNode = frontValueNode->next;
			while(delValueNode)
			{
				frontValueNode->next = delValueNode->next;
				free(delValueNode);
				delValueNode = frontValueNode->next;
			}
			free(delNode->thrItemInfo.checkSrcValList.head);
			delNode->thrItemInfo.checkSrcValList.head = NULL;
		}
		if(delNode->thrItemInfo.pActionList)
		{
			thr_action_t* pNext = NULL;
			thr_action_t* pAct = delNode->thrItemInfo.pActionList;
			delNode->thrItemInfo.pActionList = NULL;


			while(pAct)
			{
				pNext = pAct->next;
				if(pAct->type == action_type_string)
					free(pAct->sv);
				free(pAct);
				pAct = pNext;
			}
		}
		free(delNode);
		delNode = head->next;
	}

	bRet = true;
	return bRet;
}

void ThresholdCheck_UpdateSensorData(MSG_ATTRIBUTE_T* attr, thr_item_info_t* pThrItemInfo, long long opts)
{
	pThrItemInfo->checkRetValue = 0;
	long long nowTime = MSG_GetTimeTick();
	if (attr == NULL || pThrItemInfo == NULL)
		return;

	if (pThrItemInfo->checkSrcValList.head == NULL)
	{
		pThrItemInfo->checkSrcValList.head = (check_value_node_t*)malloc(sizeof(check_value_node_t));
		pThrItemInfo->checkSrcValList.nodeCnt = 0;
		pThrItemInfo->checkSrcValList.head->checkValTime = DEF_INVALID_TIME;
		pThrItemInfo->checkSrcValList.head->ckV = 0;
		pThrItemInfo->checkSrcValList.head->next = NULL;
	}

	{
		check_value_node_t* frontNode = pThrItemInfo->checkSrcValList.head;
		check_value_node_t* curNode = frontNode->next;
		check_value_node_t* delNode = NULL;
		while (curNode)
		{
			if (nowTime - curNode->checkValTime >= pThrItemInfo->lastingTimeS*1000)
			{
				delNode = curNode;
				frontNode->next = curNode->next;
				curNode = frontNode->next;
				free(delNode);
				pThrItemInfo->checkSrcValList.nodeCnt--;
				if (pThrItemInfo->checkSrcValList.nodeCnt < 0)
					pThrItemInfo->checkSrcValList.nodeCnt = 0;
				delNode = NULL;
			}
			else
			{
				frontNode = curNode;
				curNode = frontNode->next;
			}
		}
	}

	{
		double checkValue = 0;
		check_value_node_t* head = pThrItemInfo->checkSrcValList.head;
		check_value_node_t* newNode = (check_value_node_t*)malloc(sizeof(check_value_node_t));

		if (attr->type == attr_type_numeric)
		{
			checkValue = attr->v;
		}
		else if (attr->type == attr_type_boolean)
		{
			checkValue = attr->bv ? 1 : 0;
		}

		newNode->checkValTime = opts;
		newNode->ckV = checkValue;
		newNode->next = head->next;
		head->next = newNode;
		pThrItemInfo->checkSrcValList.nodeCnt++;
	}
}

void ThresholdCheck_UpdateArrayData(MSG_ATTRIBUTE_T* attr, thr_item_info_t* pThrItemInfo, long long opts)
{
	pThrItemInfo->checkRetValue = 0;
	long long nowTime = MSG_GetTimeTick();
	if (attr == NULL || pThrItemInfo == NULL)
		return;

	if (pThrItemInfo->checkSrcValList.head == NULL)
	{
		pThrItemInfo->checkSrcValList.head = (check_value_node_t*)malloc(sizeof(check_value_node_t));
		pThrItemInfo->checkSrcValList.nodeCnt = 0;
		pThrItemInfo->checkSrcValList.head->checkValTime = DEF_INVALID_TIME;
		pThrItemInfo->checkSrcValList.head->ckV = 0;
		pThrItemInfo->checkSrcValList.head->next = NULL;
	}

	{
		check_value_node_t* frontNode = pThrItemInfo->checkSrcValList.head;
		check_value_node_t* curNode = frontNode->next;
		check_value_node_t* delNode = NULL;
		while (curNode)
		{
			if (nowTime - curNode->checkValTime >= pThrItemInfo->lastingTimeS * 1000)
			{
				delNode = curNode;
				frontNode->next = curNode->next;
				curNode = frontNode->next;
				free(delNode);
				pThrItemInfo->checkSrcValList.nodeCnt--;
				if (pThrItemInfo->checkSrcValList.nodeCnt < 0)
					pThrItemInfo->checkSrcValList.nodeCnt = 0;
				delNode = NULL;
			}
			else
			{
				frontNode = curNode;
				curNode = frontNode->next;
			}
		}
	}

	{
		double checkValue = 0;
		check_value_node_t* head = pThrItemInfo->checkSrcValList.head;

		if (attr->type == attr_type_numeric_array)
		{
			int i = 0;
			for (i = 0; i < attr->value_array_len; i++)
			{
				check_value_node_t* newNode = (check_value_node_t*)malloc(sizeof(check_value_node_t));
				newNode->checkValTime = opts + attr->at[i];
				newNode->ckV = attr->av[i];
				newNode->next = head->next;
				head->next = newNode;
				pThrItemInfo->checkSrcValList.nodeCnt++;
			}
			
		}
		else if (attr->type == attr_type_boolean_array)
		{
			int i = 0;
			for (i = 0; i < attr->value_array_len; i++)
			{
				check_value_node_t* newNode = (check_value_node_t*)malloc(sizeof(check_value_node_t));
				newNode->checkValTime = opts + attr->at[i];
				newNode->ckV = attr->abv[i] ? 1 : 0;
				newNode->next = head->next;
				head->next = newNode;
				pThrItemInfo->checkSrcValList.nodeCnt++;
			}
		}
		
	}
}

bool ThresholdCheck_UpdateValue(thr_item_list pThresholdList, char *pInQuery, char* devID)
{
	bool bRet = false;
	thr_item_node_t *curThrItemNode = NULL;
	MSG_CLASSIFY_T* pCapability = NULL, *pRoot = NULL;
	char handlername[MAX_TOPIC_LEN] = {0};
	long long opts = 0;

	if( pInQuery == NULL)
		return bRet;

	if(pThresholdList == NULL)
		return bRet;

	if(!transfer_get_ipso_handlername(pInQuery, handlername))
		return bRet;

	curThrItemNode = pThresholdList->next;

	pRoot = IoT_CreateRoot(devID);

	pCapability = IoT_AddGroup(pRoot, handlername);
	
	if(!transfer_parse_ipso(pInQuery, pCapability))
		return bRet;

	if (!transfer_get_ipso_opts(pInQuery, &opts))
	{
		opts = MSG_GetTimeTick();
	}

	while(curThrItemNode)
	{
		thr_item_info_t* pThrItemInfo = NULL;
		
		MSG_ATTRIBUTE_T* attr = IoT_FindSensorNodeWithPath(pRoot, curThrItemNode->thrItemInfo.pathname);
		if(attr == NULL )
		{
			curThrItemNode = curThrItemNode->next;
			continue;
		}
		else if (attr->type == attr_type_numeric || attr->type == attr_type_boolean)
		{
			pThrItemInfo = &curThrItemNode->thrItemInfo;
			ThresholdCheck_UpdateSensorData(attr, pThrItemInfo, opts);
		}
		else if (attr->type == attr_type_numeric_array || attr->type == attr_type_boolean_array)
		{
			pThrItemInfo = &curThrItemNode->thrItemInfo;
			ThresholdCheck_UpdateArrayData(attr, pThrItemInfo, opts);
		}
		curThrItemNode = curThrItemNode->next;
	}
	IoT_ReleaseAll(pRoot);
	return bRet;
}

bool ThresholdCheck_CheckSrcVal(thr_item_info_t * pThrItemInfo)
{
	bool bRet = false;
	if(pThrItemInfo == NULL) return bRet;
	{
		long long nowTime = MSG_GetTimeTick();
		
		if(pThrItemInfo->checkSrcValList.nodeCnt > 0)
		{
			long long minCkvTime = 0;
			check_value_node_t * curNode = pThrItemInfo->checkSrcValList.head->next;
			minCkvTime = curNode->checkValTime;
			while(curNode)
			{
				if(curNode->checkValTime < minCkvTime)  minCkvTime = curNode->checkValTime;
				curNode = curNode->next; 
			}

			if(nowTime - minCkvTime >= pThrItemInfo->lastingTimeS*1000)
			{
				switch(pThrItemInfo->checkType)
				{
				case ck_type_avg:
					{
						check_value_node_t * curNode = pThrItemInfo->checkSrcValList.head->next;
						double avgTmpF = 0;
						if(pThrItemInfo->checkSrcValList.nodeCnt > 0)
						{
							while(curNode)
							{
								avgTmpF += curNode->ckV;
								curNode = curNode->next; 
							}						
							avgTmpF = avgTmpF/pThrItemInfo->checkSrcValList.nodeCnt;
							pThrItemInfo->checkRetValue = avgTmpF;
							bRet = true;
						}
						break;
					}
				case ck_type_max:
					{
						check_value_node_t * curNode = pThrItemInfo->checkSrcValList.head->next;
						double maxTmpF = 0;
						if(curNode)
						{
							maxTmpF = curNode->ckV;
							while(curNode)
							{
								if(curNode->ckV > maxTmpF) maxTmpF = curNode->ckV;
								curNode = curNode->next; 
							}
							pThrItemInfo->checkRetValue = maxTmpF;
							bRet = true;
						}
						break;
					}
				case ck_type_min:
					{
						check_value_node_t * curNode = pThrItemInfo->checkSrcValList.head->next;
						double minTmpF = 0;
						if(curNode)
						{
							minTmpF = curNode->ckV;
							while(curNode)
							{
								if(curNode->ckV < minTmpF) minTmpF = curNode->ckV;
								curNode = curNode->next; 
							}
							pThrItemInfo->checkRetValue = minTmpF;
							bRet = true;
						}
						break;
					}
				default: break;
				}
			}
		}
	}
	return bRet;
}

bool ThresholdCheck_CheckThrItem(thr_item_info_t * pThrItemInfo, char * checkRetMsg)
{
	bool bRet = false;
	bool isTrigger = false;
	bool triggerMax = false;
	bool triggerMin = false;
	char tmpRetMsg[1024] = {0};
	char checkTypeStr[64] = {0};
	char descEventStr[256] = {0};
	if(pThrItemInfo == NULL || checkRetMsg== NULL) return bRet;
	{
		switch(pThrItemInfo->checkType)
		{
		case ck_type_avg:
			{
				sprintf(checkTypeStr, DEF_AVG_EVENT_STR);
				break;
			}
		case ck_type_max:
			{
				sprintf(checkTypeStr, DEF_MAX_EVENT_STR);
				break;
			}
		case ck_type_min:
			{
				sprintf(checkTypeStr, DEF_MIN_EVENT_STR);
				break;
			}
		default: break;
		}
	}

	if(pThrItemInfo->checkSrcValList.nodeCnt <= 0)
		return bRet;
	
	if(ThresholdCheck_CheckSrcVal(pThrItemInfo))
	{  
		if(pThrItemInfo->rangeType & range_max)
		{
			if(pThrItemInfo->checkRetValue > pThrItemInfo->maxThr)
			{
				sprintf(tmpRetMsg, "%s(%s:%f)>%s(%f)", pThrItemInfo->pathname, checkTypeStr, pThrItemInfo->checkRetValue, DEF_MAX_THR_EVENT_STR, pThrItemInfo->maxThr);
				if(strlen(descEventStr)) sprintf(tmpRetMsg, "%s:%s", tmpRetMsg, descEventStr);
				triggerMax = true;
			}
		}
		if(pThrItemInfo->rangeType & range_min)
		{
			if(pThrItemInfo->checkRetValue < pThrItemInfo->minThr)
			{
				if(strlen(tmpRetMsg)) sprintf(tmpRetMsg, "%s %s %s(%s:%f)<%s(%f)", tmpRetMsg, DEF_AND_EVENT_STR, pThrItemInfo->pathname, checkTypeStr, pThrItemInfo->checkRetValue, DEF_MIN_THR_EVENT_STR, pThrItemInfo->minThr);
				else sprintf(tmpRetMsg, "%s(%s:%f)<%s(%f)", pThrItemInfo->pathname, checkTypeStr, pThrItemInfo->checkRetValue, DEF_MIN_THR_EVENT_STR, pThrItemInfo->minThr);
				if(strlen(descEventStr)) sprintf(tmpRetMsg, "%s:%s", tmpRetMsg, descEventStr);
				triggerMin = true;
			}
		}
	}
	else
		return bRet;

	switch(pThrItemInfo->rangeType)
	{
	case range_max:
		{
			isTrigger = triggerMax;
			break;
		}
	case range_min:
		{
			isTrigger = triggerMin;
			break;
		}
	case range_maxmin:
		{
			isTrigger = triggerMin || triggerMax;
			break;
		}
	}

	if(isTrigger)
	{
		long long nowTime = MSG_GetTimeTick();
		if(pThrItemInfo->intervalTimeS == DEF_INVALID_TIME || pThrItemInfo->intervalTimeS == 0 || nowTime - pThrItemInfo->repThrTime >= pThrItemInfo->intervalTimeS*1000)
		{
			pThrItemInfo->repThrTime = nowTime;
			pThrItemInfo->isNormal = false;
			bRet = true;
			pThrItemInfo->isTriggered = true;
		}
	}
	else
	{
		if(!pThrItemInfo->isNormal)
		{
			memset(tmpRetMsg, 0, sizeof(tmpRetMsg));
			//sprintf(tmpRetMsg, "%d(%s:%.0f) normal", pThrItemInfo->id, checkTypeStr, pThrItemInfo->checkRetValue.vf);
			sprintf(tmpRetMsg, "%s(%s:%f) %s", pThrItemInfo->pathname, checkTypeStr, pThrItemInfo->checkRetValue, DEF_NOR_EVENT_STR);
			if(strlen(descEventStr)) sprintf(tmpRetMsg, "%s:%s", tmpRetMsg, descEventStr);
			pThrItemInfo->isNormal = true;
			pThrItemInfo->isTriggered = true;
			bRet = true;

			/*if(pThrItemInfo->on_triggered)
				pThrItemInfo->on_triggered(pThrItemInfo->pTriggerQueue, pThrItemInfo, attr);*/
		}
	}

	if(!bRet) sprintf(checkRetMsg,"");
	else sprintf(checkRetMsg, "%s", tmpRetMsg);

	return bRet;
}

bool ThresholdCheck_CheckThr(thr_item_list curThrItemList, char ** checkRetMsg, unsigned int bufLen, bool * isNormal)
{
	bool bRet = false;
	if(curThrItemList == NULL || checkRetMsg == NULL || isNormal == NULL || bufLen == 0) return bRet;
	{
		thr_item_node_t * curThrItemNode = NULL;
		char tmpMsg[1024] = {0};
		unsigned int defLength = bufLen;
		curThrItemNode = curThrItemList->next;
		while(curThrItemNode)
		{
			if(curThrItemNode->thrItemInfo.isEnable)
			{
				memset(tmpMsg, 0, sizeof(tmpMsg));
				ThresholdCheck_CheckThrItem(&curThrItemNode->thrItemInfo, tmpMsg);
				if(strlen(tmpMsg))
				{
					if(bufLen<strlen(*checkRetMsg)+strlen(tmpMsg)+16)
					{
						int newLen = strlen(*checkRetMsg)+strlen(tmpMsg)+2*1024;
						*checkRetMsg = (char*)realloc(*checkRetMsg, newLen);
					}
					if(strlen(*checkRetMsg))sprintf(*checkRetMsg, "%s;%s", *checkRetMsg, tmpMsg);
					else sprintf(*checkRetMsg, "%s", tmpMsg);
				}
				if(*isNormal && !curThrItemNode->thrItemInfo.isNormal)
				{
					*isNormal = curThrItemNode->thrItemInfo.isNormal; 
				}
			}
			curThrItemNode = curThrItemNode->next;
		}
	}
	return bRet = true;
}

bool ThresholdCheck_QueueingAction(thr_item_list curThrItemList)
{
	thr_item_node_t* curThrItem = NULL;
	if(curThrItemList == NULL)
		return false;
	curThrItem = curThrItemList->next;
	while(curThrItem)
	{
		if(curThrItem->thrItemInfo.isTriggered)
		{
			ThresholdCheck_On_Triggered(g_pTriggerQueue, &curThrItem->thrItemInfo, NULL);
			curThrItem->thrItemInfo.isTriggered = false;
		}
		curThrItem = curThrItem->next;
	}
	return true;
}