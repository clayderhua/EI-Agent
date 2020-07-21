#include "ProcessParser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wrapper.h"
#include "cJSON.h"
#include "HandlerKernelEx.h"

void UpdateSystemInfo(MSG_CLASSIFY_T* pSysInfo, sys_mon_info_t * sysMonInfo, bool bReset)
{
	MSG_ATTRIBUTE_T *attr = NULL;
	if(pSysInfo == NULL || sysMonInfo == NULL)
		return;


	attr = IoT_FindSensorNode(pSysInfo, SWM_PRC_CPU_USAGE);
	if(attr == NULL && bReset)
	{
		attr = IoT_AddSensorNode(pSysInfo, SWM_PRC_CPU_USAGE);
	}
	IoT_SetDoubleValueWithMaxMin(attr, sysMonInfo->cpuUsage, IoT_READONLY, 100, 0, "%");

	attr = IoT_FindSensorNode(pSysInfo, SWM_SYS_TOTAL_PHYS_MEM);
	if(attr == NULL && bReset)
	{
		attr = IoT_AddSensorNode(pSysInfo, SWM_SYS_TOTAL_PHYS_MEM);
	}
	IoT_SetDoubleValueWithMaxMin(attr, sysMonInfo->totalPhysMemoryKB, IoT_READONLY, sysMonInfo->totalPhysMemoryKB, sysMonInfo->totalPhysMemoryKB, "Kilobyte");

	attr = IoT_FindSensorNode(pSysInfo, SWM_SYS_AVAIL_PHYS_MEM);
	if(attr == NULL && bReset)
	{
		attr = IoT_AddSensorNode(pSysInfo, SWM_SYS_AVAIL_PHYS_MEM);
	}
	IoT_SetDoubleValueWithMaxMin(attr, sysMonInfo->availPhysMemoryKB, IoT_READONLY, sysMonInfo->totalPhysMemoryKB, 0, "Kilobyte");

	attr = IoT_FindSensorNode(pSysInfo, SWM_SYS_USAGE_PHYS_MEM);
	if (attr == NULL && bReset)
	{
		attr = IoT_AddSensorNode(pSysInfo, SWM_SYS_USAGE_PHYS_MEM);
	}
	IoT_SetDoubleValueWithMaxMin(attr, sysMonInfo->usagePhyMemory, IoT_READONLY, 100, 0, "%");
}

void UpdateProcessInfo(MSG_CLASSIFY_T* pProcInfo, prc_mon_info_node_t * prcMonInfoList, bool bReset)
{
	prc_mon_info_node_t *pMonInfo; 
	if(pProcInfo == NULL || prcMonInfoList == NULL)
		return;


	if(bReset)
	{
		MSG_CLASSIFY_T* pNext = pProcInfo->sub_list;
		while(pNext)
		{
			MSG_CLASSIFY_T* pCurrent = pNext;
			pNext = pNext->next;
			IoT_ReleaseAll(pCurrent);
		}
		pProcInfo->sub_list = NULL;
	}

	pMonInfo = prcMonInfoList->next; 
	while(pMonInfo)
	{
		MSG_CLASSIFY_T *pProcss = NULL, *pProcssName = NULL;
		MSG_ATTRIBUTE_T *attr = NULL;
		char procUID[260] = {0};
#ifdef OLDFMT		
		snprintf(procUID, sizeof(procUID), "%d@2D%s", pMonInfo->prcMonInfo.prcID, pMonInfo->prcMonInfo.prcName);
#else
		MSG_CLASSIFY_T *pProcsses = NULL;
		snprintf(procUID, sizeof(procUID), "%s", pMonInfo->prcMonInfo.prcName);
#endif
		pProcssName = IoT_FindGroup(pProcInfo, procUID);
		if(pProcssName == NULL && bReset)
		{
			pProcssName = IoT_AddGroup(pProcInfo, procUID);
		}

#ifdef OLDFMT
		pProcss = pProcssName;
#else
		//pProcsses = IoT_AddGroup(pProcssName, "pid");
		memset(procUID, 0, sizeof(procUID));
		snprintf(procUID, sizeof(procUID), "%d", pMonInfo->prcMonInfo.prcID);
		pProcss = IoT_FindGroup(pProcssName, procUID);
		if(pProcss == NULL && bReset)
		{
			pProcss = IoT_AddGroup(pProcssName, procUID);
		}
#endif
		if(pProcss == NULL && !bReset)
		{
			pMonInfo = pMonInfo->next;
			continue;
		}

		attr = IoT_FindSensorNode(pProcss, SWM_PRC_NAME);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pProcss, SWM_PRC_NAME);
		}
		IoT_SetStringValue(attr, pMonInfo->prcMonInfo.prcName, IoT_READONLY);

		attr = IoT_FindSensorNode(pProcss, SWM_PID);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pProcss, SWM_PID);
		}
		IoT_SetDoubleValue(attr, pMonInfo->prcMonInfo.prcID, IoT_READONLY, NULL);

		attr = IoT_FindSensorNode(pProcss, SWM_OWNER);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pProcss, SWM_OWNER);
		}
		IoT_SetStringValue(attr, pMonInfo->prcMonInfo.ownerName?pMonInfo->prcMonInfo.ownerName:"", IoT_READONLY);

		attr = IoT_FindSensorNode(pProcss, SWM_PRC_CPU_USAGE);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pProcss, SWM_PRC_CPU_USAGE);
		}
		IoT_SetDoubleValueWithMaxMin(attr, pMonInfo->prcMonInfo.cpuUsage, IoT_READONLY, 100, 0, "%");

		attr = IoT_FindSensorNode(pProcss, SWM_PRC_MEM_USAGE);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pProcss, SWM_PRC_MEM_USAGE);
		}
		IoT_SetDoubleValue(attr, pMonInfo->prcMonInfo.memUsage, IoT_READONLY, "Kilobyte");
#ifdef VIRTUAL_MEM
		attr = IoT_FindSensorNode(pProcss, SWM_PRC_VMEM_USAGE);
		if(attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pProcss, SWM_PRC_VMEM_USAGE);
		}
		IoT_SetDoubleValue(attr, pMonInfo->prcMonInfo.memUsage, IoT_READONLY, "Kilobyte");
#endif
		/*
		attr = IoT_FindSensorNode(pProcss, SWM_PRC_GET_STATUS);
		if (attr == NULL && bReset)
		{
			attr = IoT_AddSensorNode(pProcss, SWM_PRC_GET_STATUS);
		}
		IoT_SetBoolValue(attr, pMonInfo->prcMonInfo.enGetStatus, IoT_READWRITE);
		*/
		if (pMonInfo->prcMonInfo.enGetStatus)
		{
			attr = IoT_FindSensorNode(pProcss, SWM_PRC_IS_ACTIVE);
			if (attr == NULL && bReset)
			{
				attr = IoT_AddSensorNode(pProcss, SWM_PRC_IS_ACTIVE);
			}
			IoT_SetBoolValue(attr, pMonInfo->prcMonInfo.isActive, IoT_READONLY);
		}
		
		pMonInfo = pMonInfo->next;
	}
}

void UpdateParameter(MSG_CLASSIFY_T* pParamInfo, int iInterval, bool bGatherStatus, char* listProcStatus, bool bReset)
{
	MSG_ATTRIBUTE_T* attr = NULL;
	if (pParamInfo == NULL)
		return;
	

	attr = IoT_FindSensorNode(pParamInfo, SWM_PARAM_INTERVAL);
	if (attr == NULL && bReset)
	{
		attr = IoT_AddSensorNode(pParamInfo, SWM_PARAM_INTERVAL);
	}
	IoT_SetDoubleValue(attr, iInterval, IoT_READWRITE, "Second");

	attr = IoT_FindSensorNode(pParamInfo, SWM_PARAM_STATUS_EN);
	if (attr == NULL && bReset)
	{
		attr = IoT_AddSensorNode(pParamInfo, SWM_PARAM_STATUS_EN);
	}
	IoT_SetBoolValue(attr, bGatherStatus, IoT_READWRITE);

	attr = IoT_FindSensorNode(pParamInfo, SWM_PARAM_PROCESS_LIST);
	if (attr == NULL && bReset)
	{
		attr = IoT_AddSensorNode(pParamInfo, SWM_PARAM_PROCESS_LIST);
	}
	IoT_SetStringValue(attr, listProcStatus ? listProcStatus : "", IoT_READWRITE);
}

void UpdateInfo(MSG_CLASSIFY_T* pInfo, bool bLogon, bool bReset)
{
	MSG_ATTRIBUTE_T* attr = NULL;
	if (pInfo == NULL)
		return;

	attr = IoT_FindSensorNode(pInfo, SWM_INFO_ERRMSG);
	if (attr == NULL && bReset)
	{
		attr = IoT_AddSensorNode(pInfo, SWM_INFO_ERRMSG);
	}
	IoT_SetStringValue(attr, bLogon ? "" : STATUS_USR_LOGOUT, IoT_READONLY);

	attr = IoT_FindSensorNode(pInfo, SWM_INFO_ERRCODE);
	if (attr == NULL && bReset)
	{
		attr = IoT_AddSensorNode(pInfo, SWM_INFO_ERRCODE);
	}
	IoT_SetDoubleValue(attr, bLogon ? STATUSCODE_SUCCESS : STATUSCODE_USR_LOGOUT, IoT_READONLY, NULL);
}

void UpdateCapability(MSG_CLASSIFY_T* capability, prc_mon_info_node_t * prcMonInfoList, sys_mon_info_t * sysMonInfo, int iInterval, bool bGatherStatus, char* listProcStatus, bool bLogon, bool bReset)
{
	MSG_CLASSIFY_T* pSysInfo = NULL, * pProcInfo = NULL, * pParam = NULL, * pInfo = NULL;
	if(capability == NULL)
		return;

	if(sysMonInfo)
	{
		pSysInfo = IoT_FindGroup(capability,SWM_SYS_MON_INFO);
		if(pSysInfo == NULL && bReset)
		{
			pSysInfo = IoT_AddGroup(capability,SWM_SYS_MON_INFO);
		}
		UpdateSystemInfo(pSysInfo, sysMonInfo, bReset);
	}
	if(prcMonInfoList)
	{
		pProcInfo = IoT_FindGroup(capability,SWM_PRC_MON_INFO);
		if(pProcInfo == NULL && bReset)
		{
			pProcInfo = IoT_AddGroupArray(capability,SWM_PRC_MON_INFO);
		}

		UpdateProcessInfo(pProcInfo, prcMonInfoList, bReset);
	}

	pParam = IoT_FindGroup(capability, SWM_PARAM_INFO);
	if (pParam == NULL && bReset)
	{
		pParam = IoT_AddGroup(capability, SWM_PARAM_INFO);
	}
	UpdateParameter(pParam, iInterval, bGatherStatus, listProcStatus, bReset);

	pInfo = IoT_FindGroup(capability, SWM_INFO_INFO);
	if (pInfo == NULL && bReset)
	{
		pInfo = IoT_AddGroup(capability, SWM_INFO_INFO);
	}
	UpdateInfo(pInfo, bLogon, bReset);
}

bool Parse_swm_kill_prc_req(char *inputstr, unsigned int *size, unsigned int **outputVal)
{
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* pids = NULL;
	cJSON* pid = NULL;

	if(!inputstr) return false;

	root = cJSON_Parse(inputstr);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_CONTENT_STRUCT);
	}

	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pids = cJSON_GetObjectItem(body, "pids");
	
	if(pids!=NULL){
		unsigned int pid_size = cJSON_GetArraySize(pids);
		int i=0;
		*size=pid_size;
		*outputVal = (unsigned int *)calloc(pid_size,sizeof(unsigned int));
		if(*outputVal!=NULL){
			for(i=0;i<pid_size;i++){
				pid = cJSON_GetArrayItem(pids, i);
				(*outputVal)[i]=pid->valueint;
			}
		}
	}

	cJSON_Delete(root);
	return true;
}


bool Parse_swm_restart_prc_req(char *inputstr, unsigned int *size, unsigned int **outputVal)
{
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* pids = NULL;
	cJSON* pid = NULL;

	if(!inputstr) return false;

	root = cJSON_Parse(inputstr);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_CONTENT_STRUCT);
	}

	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pids = cJSON_GetObjectItem(body, "pids");
	
	if(pids!=NULL){
		unsigned int pid_size = cJSON_GetArraySize(pids);
		int i=0;
		*size=pid_size;
		*outputVal = (unsigned int *)calloc(pid_size,sizeof(unsigned int));
		if(*outputVal!=NULL){
			for(i=0;i<pid_size;i++){
				pid = cJSON_GetArrayItem(pids, i);
				(*outputVal)[i]=pid->valueint;
			}
		}
	}

	cJSON_Delete(root);
	return true;
}

int Parser_string(char *pCommData, char* sessionID, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pSUSICommDataItem, SWM_ERROR_REP, pCommData);
	if(sessionID)
		cJSON_AddStringToObject(pSUSICommDataItem, SWM_SESSION_ID, sessionID);

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);	
	//printf("%s\n",out);	
	free(out);
	return outLen;
}

int Parser_errstring(char* pCommData, unsigned long errcode, char* sessionID, char** outputStr)
{
	char* out = NULL;
	int outLen = 0;
	cJSON* pSUSICommDataItem = NULL;
	if (pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();
	cJSON_AddNumberToObject(pSUSICommDataItem, SWM_ERROR_CODE, errcode);
	cJSON_AddStringToObject(pSUSICommDataItem, SWM_ERROR_REP, pCommData);
	if (sessionID)
		cJSON_AddStringToObject(pSUSICommDataItem, SWM_SESSION_ID, sessionID);

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char*)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);
	//printf("%s\n",out);	
	free(out);
	return outLen;
}

bool Parse_swm_exec_prc_req(char* inputstr, char* prcPath, char* prcArg, bool* bHide)
{
	//{"commCmd":171,"handlerName":"ProcessMonitor","content":{"prcPath":"c:/sample.exe","prcArg":"-?","prcHidden":false}}
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* nTarget = NULL;

	if (!inputstr) return false;

	root = cJSON_Parse(inputstr);
	if (!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if (!body)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_CONTENT_STRUCT);
	}

	if (!body)
	{
		cJSON_Delete(root);
		return false;
	}

	nTarget = cJSON_GetObjectItem(body, "prcPath");
	if (nTarget && nTarget->type == cJSON_String && prcPath)
	{
		if(nTarget->valuestring && strlen(nTarget->valuestring)>0)
			strcpy(prcPath, nTarget->valuestring);
	}
		
	nTarget = cJSON_GetObjectItem(body, "prcArg");
	if (nTarget && nTarget->type == cJSON_String && prcArg)
	{
		if (nTarget->valuestring && strlen(nTarget->valuestring) > 0)
			strcpy(prcArg, nTarget->valuestring);
	}
	
	nTarget = cJSON_GetObjectItem(body, "prcHidden");
	if (nTarget && bHide)
	{
		if(nTarget->type == cJSON_True || nTarget->type == cJSON_False)
			*bHide = nTarget->type == cJSON_True ? true : false;
	}

	cJSON_Delete(root);
	return true;
}