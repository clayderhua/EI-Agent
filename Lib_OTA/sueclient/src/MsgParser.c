#include <time.h>
#include <stdio.h>

#define LOG_TAG "OTA"
#include "Log.h"
#include "MsgParser.h"
#include "stdlib.h"
#include "string.h"
#include "cJSON.h"
#include "InternalData.h"

static cJSON * ParseTakeOffCoat(cJSON * parseSrc, char * bodyKey);
static cJSON * PackPutOnCoat(char * devID, int cmdID, char * bodyKey, cJSON * bodyItem);
static cJSON * cJSON_CreateTaskStatusItem(PTaskStatusInfo pTSInfo);
static cJSON * PackTaskStatusInfo(PTaskStatusInfo pTSInfo, char * devID, int cmdID);
static cJSON * cJSON_CreateReqDLTaskItem(char * pkgName);
static cJSON * cJSON_CreateScheStatusInfoItem(char * pkgType, int actType, long pkgOwnerId, int status, int errCode);
static cJSON * cJSON_CreateScheInfoItem(char * pkgType, int actType, LHList * pDLScheInfoList, LHList * pDPScheInfoList);
static cJSON * cJSON_CreateScheInfoListItem(char * pkgType, LHList * pScheInfoList, int actType);
static cJSON * cJSON_CreateScheTimeInfoItem(PScheduleInfo pScheInfo);
static cJSON * cJSON_CreateCheckPkgInfoItem(char *pkgType,
										    int upgMode,
											char *version,
											char *os,
											char *arch,
											char **subDevices,
											int subDeviceSize,
											long pkgOwnerId);
static cJSON * cJSON_CreateSWInfosItem(LHList * pSWInfoList);
static cJSON * cJSON_CreateDelPkgRetInfosItem(LHList * pDelPkgRetInfoList);

int ParseCmdID(char * jsonStr)
{
	int retCmdID = CMD_INVALID;
   if(NULL != jsonStr)
	{
		cJSON* root = NULL;
		root = cJSON_Parse(jsonStr);
		if(root)
		{
			cJSON * cmdDataItem = cJSON_GetObjectItem(root, DEF_SUECMDDATA_KEY);
			if(cmdDataItem)
			{
				cJSON *subItem = cJSON_GetObjectItem(cmdDataItem, DEF_CMDID_KEY);
				if(subItem)
				{
					retCmdID = subItem->valueint;
				}
			}
			cJSON_Delete(root);
		}
	}
	return retCmdID;
}

int ParseDLTaskInfo(char * jsonStr, PDLTaskInfo pDLTaskInfo)
{
	int iRet = -1;
	cJSON* parseSrc = NULL;
	cJSON * dlReqInfoItem = NULL;
	int len = 0;

	do
	{
		if(NULL == jsonStr || NULL == pDLTaskInfo) {
			break;
		}
		parseSrc = cJSON_Parse(jsonStr);
		if(!parseSrc) {
			break;
		}

		dlReqInfoItem = ParseTakeOffCoat(parseSrc, DEF_TASKINFO_KEY);
		if(!dlReqInfoItem) {
			break;
		}

		cJSON * subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PKGTYPE_KEY);
		if(!subItem || (len = strlen(subItem->valuestring)+1)<=1) {
			break;
		}
		pDLTaskInfo->pkgType = (char *)malloc(len);
		strcpy(pDLTaskInfo->pkgType, subItem->valuestring);

		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PKGNAME_KEY);
		if(!subItem || (len = strlen(subItem->valuestring)+1)<=1) {
			break;
		}
		pDLTaskInfo->pkgName = (char *)malloc(len);
		strcpy(pDLTaskInfo->pkgName, subItem->valuestring);

		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_URL_KEY);
		if(!subItem || (len = strlen(subItem->valuestring)+1)<=1) {
			break;
		}
		pDLTaskInfo->url = (char *)malloc(len);
		strcpy(pDLTaskInfo->url, subItem->valuestring);

		/*Add subDevice msg by binlin.duan*/
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_SUBDEVICES_KEY);
		if (subItem != NULL)
		{
			pDLTaskInfo->subDeviceSize = cJSON_GetArraySize(subItem);
			pDLTaskInfo->subDevices = (char **)malloc(pDLTaskInfo->subDeviceSize*sizeof(char *));
			int i = 0;
			for (i = 0; i < pDLTaskInfo->subDeviceSize; i++)
			{
				cJSON *psubDevice = cJSON_GetArrayItem(subItem, i);
				len = strlen(psubDevice->valuestring) + 1;
				pDLTaskInfo->subDevices[i] = (char *)malloc(len);
				strcpy(pDLTaskInfo->subDevices[i], psubDevice->valuestring);
			}
		}

		pDLTaskInfo->sercurity = 0;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_SERCURITY_KEY);
		if(subItem)
			pDLTaskInfo->sercurity = subItem->valueint;

		pDLTaskInfo->protocal = 0;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PROTOCAL_KEY);
		if(subItem)
			pDLTaskInfo->protocal = subItem->valueint;

		pDLTaskInfo->dlRetry = 0;
		pDLTaskInfo->dpRetry = 0;
		pDLTaskInfo->isRollBack = 0;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_DLRETRY_KEY);
		if (subItem)
			pDLTaskInfo->dlRetry = subItem->valueint;

		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_ISDEPLOY_KEY);
		if(subItem)
		{
			pDLTaskInfo->isDeploy = subItem->valueint;
			subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_DPRETRY_KEY);
			if (subItem)
				pDLTaskInfo->dpRetry = subItem->valueint;
			subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_ISROLLBACK_KEY);
			if (subItem)
				pDLTaskInfo->isRollBack = subItem->valueint;
			subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PKGOWNERID_KEY);
			if (subItem) {
				pDLTaskInfo->pkgOwnerId = subItem->valueint;
			}
		}
		iRet = 0;
	} while (0);

	if (parseSrc)
		cJSON_Delete(parseSrc);

	if(iRet != 0 && pDLTaskInfo) {
		if(pDLTaskInfo->pkgType) {
			free(pDLTaskInfo->pkgType);
			pDLTaskInfo->pkgType = NULL;
		}
		if(pDLTaskInfo->pkgName) {
			free(pDLTaskInfo->pkgName);
			pDLTaskInfo->pkgName = NULL;
		}
	}

	return iRet;
}

int ParseReqDLRetInfo(char * jsonStr, PDLTaskInfo pDLTaskInfo)
{
	int iRet = -1;
	if(NULL != jsonStr && NULL != pDLTaskInfo)
	{
		cJSON* parseSrc = NULL;
		parseSrc = cJSON_Parse(jsonStr);
		if(parseSrc)
		{
			cJSON * dlReqInfoItem = NULL;
			dlReqInfoItem = ParseTakeOffCoat(parseSrc, DEF_TASKINFO_KEY);
			if(dlReqInfoItem)
			{
				int len = 0;
				cJSON * subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PKGNAME_KEY);
				if(subItem && (len = strlen(subItem->valuestring)+1)>1)
				{
					pDLTaskInfo->pkgName = (char *)malloc(len);
					memset(pDLTaskInfo->pkgName, 0, len);
					strcpy(pDLTaskInfo->pkgName, subItem->valuestring);
					subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PKGTYPE_KEY);
					if(subItem && (len = strlen(subItem->valuestring)+1)>1)
					{
						pDLTaskInfo->pkgType = (char *)malloc(len);
						memset(pDLTaskInfo->pkgType, 0, len);
						strcpy(pDLTaskInfo->pkgType, subItem->valuestring);
						subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_URL_KEY);
						if(subItem && (len = strlen(subItem->valuestring)+1)>1)
						{
							pDLTaskInfo->url = (char *)malloc(len);
							memset(pDLTaskInfo->url, 0, len);
							strcpy(pDLTaskInfo->url, subItem->valuestring);

                            pDLTaskInfo->sercurity = 0;
                            subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_SERCURITY_KEY);
                            if (subItem) pDLTaskInfo->sercurity = subItem->valueint;
                            pDLTaskInfo->protocal = 0;
                            subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PROTOCAL_KEY);
                            if (subItem) pDLTaskInfo->protocal = subItem->valueint;

							subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_ISDEPLOY_KEY);
							if(subItem)
							{
								pDLTaskInfo->isDeploy = subItem->valueint;
							}
						}
					}
					iRet = 0;
				}
			}
			cJSON_Delete(parseSrc);
		}
	}
	return iRet;
}

int ParseReqTaskStatusInfo(char * jsonStr, char ** pkgName)
{
	int iRet = -1;
	if(NULL != jsonStr && NULL != pkgName)
	{
		cJSON* parseSrc = NULL;
		parseSrc = cJSON_Parse(jsonStr);
		if(parseSrc)
		{
			cJSON * dlReqInfoItem = NULL;
			dlReqInfoItem = ParseTakeOffCoat(parseSrc, DEF_TASKINFO_KEY);
			if(dlReqInfoItem)
			{
				int len = 0;
				cJSON * subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PKGNAME_KEY);
				if(subItem && (len = strlen(subItem->valuestring)+1)>1)
				{
					*pkgName = (char *)malloc(len);
					memset(*pkgName, 0, len);
					strcpy(*pkgName, subItem->valuestring);
					iRet = 0;
				}
			}
			cJSON_Delete(parseSrc);
		}
	}
	return iRet;
}

int ParseDPTaskInfo(char * jsonStr, PDPTaskInfo pDPTaskInfo)
{
	int iRet = -1;
	if(NULL != jsonStr && NULL != pDPTaskInfo)
	{
		cJSON* parseSrc = NULL;
		parseSrc = cJSON_Parse(jsonStr);
		if(parseSrc)
		{
			cJSON * dpReqInfoItem = NULL;
            dpReqInfoItem = ParseTakeOffCoat(parseSrc, DEF_TASKINFO_KEY);
            if (dpReqInfoItem)
			{
				int len = 0;
                cJSON * subItem = cJSON_GetObjectItem(dpReqInfoItem, DEF_PKGNAME_KEY);
				if(subItem && (len = strlen(subItem->valuestring)+1)>1)
				{
					pDPTaskInfo->pkgName = (char *)malloc(len);
					memset(pDPTaskInfo->pkgName, 0, len);
					strcpy(pDPTaskInfo->pkgName, subItem->valuestring);
                    subItem = cJSON_GetObjectItem(dpReqInfoItem, DEF_PKGTYPE_KEY);
					if(subItem && (len = strlen(subItem->valuestring)+1)>1)
					{
						pDPTaskInfo->pkgType = (char *)malloc(len);
						memset(pDPTaskInfo->pkgType, 0, len);
						strcpy(pDPTaskInfo->pkgType, subItem->valuestring);
                        pDPTaskInfo->dpRetry = 0;
                        pDPTaskInfo->isRollBack = 0;
                        subItem = cJSON_GetObjectItem(dpReqInfoItem, DEF_DPRETRY_KEY);
                        if (subItem)
							pDPTaskInfo->dpRetry = subItem->valueint;
                        subItem = cJSON_GetObjectItem(dpReqInfoItem, DEF_ISROLLBACK_KEY);
                        if (subItem)
							pDPTaskInfo->isRollBack = subItem->valueint;
                        subItem = cJSON_GetObjectItem(dpReqInfoItem, DEF_PKGOWNERID_KEY);
                        if (subItem)
							pDPTaskInfo->pkgOwnerId = subItem->valueint;
						iRet = 0;
					}
				}
			}
			cJSON_Delete(parseSrc);
		}
	}
	return iRet;
}

int ParseSetScheInfo(char * jsonStr, PScheduleInfo pScheInfo)
{
	cJSON * parseSrc = NULL;
	cJSON * setScheInfoItem = NULL;
	cJSON * subItem = NULL;
	int len = 0;
	int iRet = -1;

	if(NULL == jsonStr || NULL == pScheInfo)
		return iRet;

	do {
		parseSrc = cJSON_Parse(jsonStr);
		if(!parseSrc)
			break;

		//ScheInfo
		setScheInfoItem  = ParseTakeOffCoat(parseSrc, DEF_SCHEINFO_KEY);
		if(!setScheInfoItem)
			break;

		//PkgType
		subItem = cJSON_GetObjectItem(setScheInfoItem, DEF_PKGTYPE_KEY);
		if(!subItem)
			break;
		len = strlen(subItem->valuestring)+1;
		pScheInfo->pkgType = (char *)malloc(len);
		strcpy(pScheInfo->pkgType, subItem->valuestring);

		//ActType
		subItem = cJSON_GetObjectItem(setScheInfoItem, DEF_ACTTYPE_KEY);
		if(!subItem || subItem->valueint <= SAT_UNKNOW || subItem->valueint >= SAT_MAX)
			break;
		pScheInfo->actType = subItem->valueint;

		//ScheType
		subItem = cJSON_GetObjectItem(setScheInfoItem, DEF_SCHETYPE_KEY);
		if(!subItem || subItem->valueint <= ST_UNKNOW || subItem->valueint >= ST_MAX)
			break;
		pScheInfo->scheType = subItem->valueint;

		//StartTime
		subItem = cJSON_GetObjectItem(setScheInfoItem, DEF_STARTTIME_KEY);
		if(!subItem || !subItem->valuestring)
			break;
		len = strlen(subItem->valuestring) +1;
		pScheInfo->startTimeStr = (char*)malloc(len);
		strcpy(pScheInfo->startTimeStr, subItem->valuestring);

		//EndTime
		subItem = cJSON_GetObjectItem(setScheInfoItem, DEF_ENDTIME_KEY);
		if(!subItem || !subItem->valuestring)
			break;
		len = strlen(subItem->valuestring)+1;
		pScheInfo->endTimeStr = (char*)malloc(len);
		strcpy(pScheInfo->endTimeStr, subItem->valuestring);

		//UpgMode
		pScheInfo->upgMode = UM_HIGHEST;
		subItem = cJSON_GetObjectItem(setScheInfoItem, DEF_UPGMODE_KEY);
		if (subItem && subItem->valueint > UM_UNKNOW && subItem->valueint < UM_MAX) {
			pScheInfo->upgMode = subItem->valueint;
		}

		//PkgOwnerId
		pScheInfo->pkgOwnerId = 0;
		subItem = cJSON_GetObjectItem(setScheInfoItem, DEF_PKGOWNERID_KEY);
		if (subItem && subItem->valueint >= 0) {
			pScheInfo->pkgOwnerId = subItem->valueint;
		}

		//SubDevices
		//Add subDevice msg by binlin.duan
		subItem = cJSON_GetObjectItem(setScheInfoItem, DEF_SUBDEVICES_KEY);
		if (subItem != NULL) {
			pScheInfo->subDeviceSize = cJSON_GetArraySize(subItem);
			pScheInfo->subDevices = (char **)malloc(pScheInfo->subDeviceSize*sizeof(char *));
			int i=0;
			for (i = 0; i < pScheInfo->subDeviceSize; i++)
			{
				cJSON *psubDevice = cJSON_GetArrayItem(subItem, i);
				len = strlen(psubDevice->valuestring) + 1;
				pScheInfo->subDevices[i] = (char *)malloc(len);
				memset(pScheInfo->subDevices[i], 0, len);
				strcpy(pScheInfo->subDevices[i], psubDevice->valuestring);
			}
		}

		// success until now
		iRet = 0;
	} while (0);

	if (parseSrc)
		cJSON_Delete(parseSrc);

	// free memory if failed
	if(iRet != 0) {
		if(pScheInfo->pkgType) {
			free(pScheInfo->pkgType);
			pScheInfo->pkgType = NULL;
		}
		if(pScheInfo->startTimeStr) {
			free(pScheInfo->startTimeStr);
			pScheInfo->startTimeStr = NULL;
		}
	}

	return iRet;
}

int ParseDelScheInfo(char * jsonStr,  char** pkgType, int* actType, int* pkgOwnerId)
{
	int iRet = -1;
	cJSON * parseSrc = NULL;
	cJSON * delScheInfoItem = NULL;
	int len = 0;
	cJSON * subItem = NULL;

	if(NULL == jsonStr || NULL == pkgType || NULL == actType) {
		return iRet;
	}

	do {
		parseSrc = cJSON_Parse(jsonStr);
		if(!parseSrc)
			break;

		delScheInfoItem  = ParseTakeOffCoat(parseSrc, DEF_SCHEINFO_KEY);
		if(!delScheInfoItem)
			break;

		//PkgType
		subItem = cJSON_GetObjectItem(delScheInfoItem, DEF_PKGTYPE_KEY);
		if(!subItem || !subItem->valuestring)
			break;
		len = strlen(subItem->valuestring) + 1;
		*pkgType = (char *)malloc(len);
		strcpy(*pkgType, subItem->valuestring);

		//ActType
		subItem = cJSON_GetObjectItem(delScheInfoItem, DEF_ACTTYPE_KEY);
		if(!subItem || subItem->valueint <= SAT_UNKNOW || subItem->valueint >= SAT_MAX)
			break;
		*actType = subItem->valueint;
		iRet = 0;

		//PkgOwnerId
		subItem = cJSON_GetObjectItem(delScheInfoItem, DEF_PKGOWNERID_KEY);
		if(pkgOwnerId && subItem && subItem->valueint <= 0) {
			*pkgOwnerId = subItem->valueint;
		}
	} while (0);

	if (parseSrc)
		cJSON_Delete(parseSrc);

	return iRet;
}

int ParseGetSheInfo(char * jsonStr, char ** pkgType, int * actType)
{
	int iRet = -1;
	if(NULL != jsonStr && NULL != pkgType && NULL != actType)
	{
		cJSON * parseSrc = NULL;
		parseSrc = cJSON_Parse(jsonStr);
		if(parseSrc)
		{
			cJSON * getScheInfoItem = NULL;
			getScheInfoItem  = ParseTakeOffCoat(parseSrc, DEF_SCHEINFO_KEY);
			if(getScheInfoItem)
			{
				cJSON * subItem = NULL;
				subItem = cJSON_GetObjectItem(getScheInfoItem, DEF_ACTTYPE_KEY);
				if(subItem)
				{
					*actType = subItem->valueint;
					subItem = cJSON_GetObjectItem(getScheInfoItem, DEF_PKGTYPE_KEY);
					if(subItem)
					{
						int len = strlen(subItem->valuestring)+1;
						*pkgType = (char*)malloc(len);
						memset(*pkgType, 0, len);
						strcpy(*pkgType, subItem->valuestring);
						iRet = 0;
					}
				}
			}
			cJSON_Delete(parseSrc);
		}
	}
	return iRet;
}

int ParseCheckPkgRetInfo(char * jsonStr, PDLTaskInfo pDLTaskInfo)
{
	int len = 0;
	int iRet = -1;
	cJSON * dlReqInfoItem = NULL;
	cJSON * subItem;
	cJSON* parseSrc = NULL;

	if(NULL == jsonStr || NULL == pDLTaskInfo)
		return iRet;

	do {
		parseSrc = cJSON_Parse(jsonStr);
		if(!parseSrc)
			break;

		dlReqInfoItem = ParseTakeOffCoat(parseSrc, DEF_SCHEINFO_KEY);
		if(!dlReqInfoItem)
			break;

		//PkgName
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PKGNAME_KEY);
		if(!subItem || (len = strlen(subItem->valuestring)+1)<=0)
			break;
		pDLTaskInfo->pkgName = (char *)malloc(len);
		strcpy(pDLTaskInfo->pkgName, subItem->valuestring);

		//PkgType
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PKGTYPE_KEY);
		if(!subItem || (len = strlen(subItem->valuestring)+1)<=0)
			break;
		pDLTaskInfo->pkgType = (char *)malloc(len);
		strcpy(pDLTaskInfo->pkgType, subItem->valuestring);

		//URL
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_URL_KEY);
		if(!subItem || (len = strlen(subItem->valuestring)+1)<=0)
			break;
		pDLTaskInfo->url = (char *)malloc(len);
		strcpy(pDLTaskInfo->url, subItem->valuestring);

		//Sercurity, Protocol
		pDLTaskInfo->sercurity = 0;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_SERCURITY_KEY);
		if (subItem) pDLTaskInfo->sercurity = subItem->valueint;
		pDLTaskInfo->protocal = 0;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PROTOCAL_KEY);
		if (subItem) pDLTaskInfo->protocal = subItem->valueint;

		//DlRetry, IsDp, DpRetry, IsRB
		pDLTaskInfo->dlRetry = 0;
		pDLTaskInfo->dpRetry = 0;
		pDLTaskInfo->isRollBack = 0;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_DLRETRY_KEY);
		if (subItem) pDLTaskInfo->dlRetry = subItem->valueint;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_ISDEPLOY_KEY);
		if (subItem) pDLTaskInfo->isDeploy = subItem->valueint;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_DPRETRY_KEY);
		if (subItem) pDLTaskInfo->dpRetry = subItem->valueint;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_ISROLLBACK_KEY);
		if (subItem) pDLTaskInfo->isRollBack = subItem->valueint;

		//PkgOwnerId
		pDLTaskInfo->pkgOwnerId = 0;
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_PKGOWNERID_KEY);
		if (subItem) pDLTaskInfo->pkgOwnerId = subItem->valueint;

		/*Add subDevice msg by binlin.duan*/
		subItem = cJSON_GetObjectItem(dlReqInfoItem, DEF_SUBDEVICES_KEY);
		if (subItem != NULL)
		{
			pDLTaskInfo->subDeviceSize = cJSON_GetArraySize(subItem);
			pDLTaskInfo->subDevices = (char **)malloc(pDLTaskInfo->subDeviceSize*sizeof(char *));
			int i = 0;
			for (i = 0; i < pDLTaskInfo->subDeviceSize; i++)
			{
				cJSON *psubDevice = cJSON_GetArrayItem(subItem, i);
				len = strlen(psubDevice->valuestring) + 1;
				pDLTaskInfo->subDevices[i] = (char *)malloc(len);
				memset(pDLTaskInfo->subDevices[i], 0, len);
				strcpy(pDLTaskInfo->subDevices[i], psubDevice->valuestring);
			}
		}
		//
		iRet = 0;
	} while (0);

	if (parseSrc)
		cJSON_Delete(parseSrc);

	return iRet;
}

int ParseReqSWInfo(char * jsonStr, char *** swNames, int *cnt)
{
    int iRet = -1;
    if (NULL != jsonStr && NULL != swNames && NULL != cnt)
    {
        cJSON* parseSrc = NULL;
        *cnt = 0;
        parseSrc = cJSON_Parse(jsonStr);
        if (parseSrc)
        {
            cJSON * reqInfoItem = NULL, *arrayItem = NULL, *subItem = NULL;
            reqInfoItem = ParseTakeOffCoat(parseSrc, DEF_TASKINFO_KEY);
            if (reqInfoItem)
            {
                arrayItem = cJSON_GetObjectItem(reqInfoItem, DEF_NAMES_KEY);
                if (arrayItem)
                {
                    int size = 0, i = 0, len = 0;
                    size = cJSON_GetArraySize(arrayItem);
                    if (size > 0)
                    {
                        char *tmpName = NULL;
                        char ** tmpNames = (char**)malloc((size + 1)*sizeof(char*));
                        memset(tmpNames, 0, (size + 1)*sizeof(char*));
                        for (i = 0; i < size; i++)
                        {
                            subItem = cJSON_GetArrayItem(arrayItem, i);
                            len = strlen(subItem->valuestring) + 1;
                            tmpName = (char *)malloc(len);
                            memset(tmpName, 0, len);
                            strcpy(tmpName, subItem->valuestring);
                            tmpNames[i] = tmpName;
                        }
                        *swNames = tmpNames;
                    }
                    *cnt = size;
                }
            }
            cJSON_Delete(parseSrc);
            iRet = 0;
        }
    }
    return iRet;
}

int ParseReqDelPkgInfo(char * jsonStr, char *** pkgNames, int *cnt, long* pkgOwnerId)
{
    int iRet = -1;
    if (NULL != jsonStr && NULL != pkgNames && NULL != cnt)
    {
        cJSON* parseSrc = NULL;
        *cnt = 0;
        parseSrc = cJSON_Parse(jsonStr);
        if (parseSrc)
        {
            cJSON * reqInfoItem = NULL, *arrayItem = NULL, *subItem = NULL;
            reqInfoItem = ParseTakeOffCoat(parseSrc, DEF_TASKINFO_KEY);
            if (reqInfoItem)
            {
				//get pkgOwnerId
				subItem = cJSON_GetObjectItem(reqInfoItem, DEF_PKGOWNERID_KEY);
				if (subItem) *pkgOwnerId = subItem->valueint;
                arrayItem = cJSON_GetObjectItem(reqInfoItem, DEF_PKGNAMES_KEY);
                if (arrayItem)
                {
                    int size = 0, i = 0, len = 0;
                    size = cJSON_GetArraySize(arrayItem);
                    if (size > 0)
                    {
                        char *tmpName = NULL;
                        char ** tmpNames = (char**)malloc((size + 1)*sizeof(char*));
                        memset(tmpNames, 0, (size + 1)*sizeof(char*));
                        for (i = 0; i < size; i++)
                        {
                            subItem = cJSON_GetArrayItem(arrayItem, i);
                            len = strlen(subItem->valuestring) + 1;
                            tmpName = (char *)malloc(len);
                            memset(tmpName, 0, len);
                            strcpy(tmpName, subItem->valuestring);
                            tmpNames[i] = tmpName;
                        }
                        *pkgNames = tmpNames;
                    }
                    *cnt = size;
                }
            }
            cJSON_Delete(parseSrc);
            iRet = 0;
        }
    }
    return iRet;
}

int PackAutoTaskStatusInfo(PTaskStatusInfo pTSInfo, char * devID, char ** jsonStr)
{
	int iRet = -1;
	if(NULL != pTSInfo && pTSInfo->pkgName != NULL && NULL != devID && NULL != jsonStr)
	{
		cJSON * packTarg = PackTaskStatusInfo(pTSInfo, devID, CMD_AUTO_STATUS_REP);
		if(packTarg)
		{
			*jsonStr = cJSON_PrintUnformatted(packTarg);
			if(*jsonStr != NULL) iRet = 0;
			cJSON_Delete(packTarg);
		}
	}
	return iRet;
}

int PackReqpTaskStatusInfo(PTaskStatusInfo pTSInfo, char * devID, char ** jsonStr)
{
	int iRet = -1;
	if(NULL != pTSInfo && pTSInfo->pkgName != NULL && NULL != devID && NULL != jsonStr)
	{
		cJSON * packTarg = PackTaskStatusInfo(pTSInfo, devID, CMD_REQP_STATUS_REP);
		if(packTarg)
		{
			*jsonStr = cJSON_PrintUnformatted(packTarg);
			if(*jsonStr != NULL) iRet = 0;
			cJSON_Delete(packTarg);
		}
	}
	return iRet;
}

int PackReqDLTaskInfo(char * pkgName, char * devID, char ** jsonStr)
{
	int iRet = -1;
	if(pkgName != NULL && NULL != devID && NULL != jsonStr)
	{
		cJSON * reqDLTaskItem = cJSON_CreateReqDLTaskItem(pkgName);
		if(reqDLTaskItem)
		{
			cJSON * packTarg = NULL;
			packTarg = PackPutOnCoat(devID, CMD_REQ_DOWNLOAD_REQ, DEF_TASKREQ_KEY, reqDLTaskItem);
			if(packTarg == NULL)
			{
				cJSON_Delete(reqDLTaskItem);
			}
			else
			{
				*jsonStr = cJSON_PrintUnformatted(packTarg);
				if(*jsonStr != NULL) iRet = 0;
				cJSON_Delete(packTarg);
			}
		}
	}
	return iRet;
}

int PackSetScheRepInfo(char * reqStr, int errCode, char * devID, char ** jsonStr)
{
	int iRet = -1;
	if(reqStr != NULL && NULL != devID && NULL != jsonStr)
	{
		cJSON* parseSrc = NULL;
		parseSrc = cJSON_Parse(reqStr);
		if(parseSrc)
		{
			cJSON * setScheReqItem = NULL;
			setScheReqItem = ParseTakeOffCoat(parseSrc, DEF_SCHEINFO_KEY);
			if(setScheReqItem)
			{
				cJSON * setScheRepItem = NULL, * packTarg = NULL;
				setScheRepItem = cJSON_Duplicate(setScheReqItem, 1);
				cJSON_AddNumberToObject(setScheRepItem, DEF_ERRCODE_KEY, errCode);
				packTarg = PackPutOnCoat(devID, CMD_SET_SCHE_REP, DEF_SCHESTATUS_KEY, setScheRepItem);
				if(packTarg == NULL)
				{
					cJSON_Delete(setScheRepItem);
				}
				else
				{
					*jsonStr = cJSON_PrintUnformatted(packTarg);
					if(*jsonStr != NULL) iRet = 0;
					cJSON_Delete(packTarg);
				}
			}
			cJSON_Delete(parseSrc);
		}
	}
	return iRet;
}

int PackDelScheRepInfo(char * reqStr, int errCode, char * devID, char ** jsonStr)
{
	int iRet = -1;
	if(reqStr != NULL && NULL != devID && NULL != jsonStr)
	{
		cJSON* parseSrc = NULL;
		parseSrc = cJSON_Parse(reqStr);
		if(parseSrc)
		{
			cJSON * delScheReqItem = NULL;
			delScheReqItem = ParseTakeOffCoat(parseSrc, DEF_SCHEINFO_KEY);
			if(delScheReqItem)
			{
				cJSON * delScheRepItem = NULL, * packTarg = NULL;
				delScheRepItem = cJSON_Duplicate(delScheReqItem, 1);
				cJSON_AddNumberToObject(delScheRepItem, DEF_ERRCODE_KEY, errCode);
				packTarg = PackPutOnCoat(devID, CMD_DEL_SCHE_REP, DEF_SCHESTATUS_KEY, delScheRepItem);
				if(packTarg == NULL)
				{
					cJSON_Delete(delScheRepItem);
				}
				else
				{
					*jsonStr = cJSON_PrintUnformatted(packTarg);
					if(*jsonStr != NULL) iRet = 0;
					cJSON_Delete(packTarg);
				}
			}
			cJSON_Delete(parseSrc);
		}
	}
	return iRet;
}

int PackScheStatusInfo(char * pkgType, int actType, long pkgOwnerId, int status, int errCode, char * devID, char ** jsonStr)
{
	int iRet = -1;
	if(pkgType != NULL && NULL != devID && NULL != jsonStr)
	{
		cJSON * scheSIItem = cJSON_CreateScheStatusInfoItem(pkgType, actType, pkgOwnerId, status, errCode);
		if(scheSIItem)
		{
			cJSON * packTarg = NULL;
			packTarg = PackPutOnCoat(devID, CMD_SCHE_STATUS_REP, DEF_SCHESTATUS_KEY, scheSIItem);
			if(packTarg == NULL)
			{
				cJSON_Delete(scheSIItem);
			}
			else
			{
				*jsonStr = cJSON_PrintUnformatted(packTarg);
				if(*jsonStr != NULL) iRet = 0;
				cJSON_Delete(packTarg);
			}
		}
	}
	return iRet;
}

int PackScheInfo(char * pkgType, int actType, LHList * pDLScheInfoList, LHList * pDPScheInfoList, char * devID, char ** jsonStr)
{
	int iRet = -1;
	if(pkgType != NULL && NULL != devID && NULL != jsonStr)
	{
		cJSON * scheInfoItem = cJSON_CreateScheInfoItem(pkgType, actType, pDLScheInfoList, pDPScheInfoList);
		if(scheInfoItem)
		{
			cJSON * packTarg = NULL;
			packTarg = PackPutOnCoat(devID, CMD_SCHE_INFO_REP, DEF_SCHEINFO_KEY, scheInfoItem);
			if(packTarg == NULL)
			{
				cJSON_Delete(scheInfoItem);
			}
			else
			{
				*jsonStr = cJSON_PrintUnformatted(packTarg);
				if(*jsonStr != NULL) iRet = 0;
				cJSON_Delete(packTarg);
			}
		}
	}
	return iRet;
}

int PackCheckPkgInfo(char * pkgType,
					 int upgMode,
					 char * version,
					 char * os,
					 char * arch,
					 char * devID,
					 char ** subDevices,
					 int subDeviceSize,
					 long pkgOwnerId,
					 char ** jsonStr)
{
	int iRet = -1;
	if(NULL != pkgType && NULL!=version && NULL != devID && NULL != jsonStr)
	{
        cJSON * checkInfoItem = cJSON_CreateCheckPkgInfoItem(pkgType, upgMode, version, os, arch, subDevices, subDeviceSize, pkgOwnerId);
		if(checkInfoItem)
		{
			cJSON * packTarg = NULL;
			packTarg = PackPutOnCoat(devID, CMD_CHECK_PKGINFO_REQ, DEF_SCHEINFO_KEY, checkInfoItem);
			if(packTarg == NULL)
			{
				cJSON_Delete(checkInfoItem);
			}
			else
			{
				*jsonStr = cJSON_PrintUnformatted(packTarg);
				if(*jsonStr != NULL) iRet = 0;
				cJSON_Delete(packTarg);
			}
		}
	}
	return iRet;
}

/*
{
  "Data": {
    bodyKey: {
      bodyItem
    },
    "DevID": "00000001-0000-0000-0000-08002714FA72",
    "CmdID": 109
  }
}
*/
static cJSON * ParseTakeOffCoat(cJSON * parseSrc, char * bodyKey)
{
	cJSON * bodyItem = NULL;
	if(parseSrc != NULL && bodyKey != NULL)
	{
		cJSON * subItem = NULL;
		subItem = cJSON_GetObjectItem(parseSrc, DEF_SUECMDDATA_KEY);
		if(subItem)
		{
			bodyItem = cJSON_GetObjectItem(subItem, bodyKey);
		}
	}
	return bodyItem;
}

/*
{
   "SendTS":1565159389,
   "Data":{
	  "CmdID":104,
	  "DevID":"00000001-0000-0000-0000-74FE4835D905",
	  bodyKey: bodyItem
   }
}
*/
static cJSON * PackPutOnCoat(char * devID, int cmdID, char * bodyKey, cJSON * bodyItem)
{
	cJSON * root = NULL;
	if(NULL != devID && NULL != bodyItem && bodyKey != NULL)
	{
		root = cJSON_CreateObject();
		if(root)
		{
			cJSON * dataItem = cJSON_CreateObject();
			if(dataItem)
			{
				long long tick = (long long)time((time_t *) NULL);
				cJSON_AddNumberToObject(root, DEF_SENDTS_KEY, tick);
				cJSON_AddItemToObject(root, DEF_SUECMDDATA_KEY, dataItem);
				cJSON_AddNumberToObject(dataItem, DEF_CMDID_KEY, cmdID);
				cJSON_AddStringToObject(dataItem, DEF_DEVICEID_KEY, devID);
				cJSON_AddItemToObject(dataItem, bodyKey, bodyItem);
			}
			else
			{
				cJSON_Delete(root);
				root = NULL;
			}
		}
	}
	return root;
}

/*
{
	"PkgName":"EPD023B-v1.0.0.1-fb3e2d3c9121556deec1d9445c4a835d.zip",
	"Type":1,
	"Status":1, // SC_QUEUE
	"Action":1,
	"RetryCnt":0
}
*/
static cJSON * cJSON_CreateTaskStatusItem(PTaskStatusInfo pTSInfo)
{
	cJSON * taskStatusItem = NULL;
	if(NULL != pTSInfo && pTSInfo->pkgName != NULL)
	{
		taskStatusItem = cJSON_CreateObject();
		if(taskStatusItem)
		{
			cJSON_AddStringToObject(taskStatusItem, DEF_PKGNAME_KEY, pTSInfo->pkgName);
			cJSON_AddNumberToObject(taskStatusItem, DEF_TYPE_KEY, pTSInfo->taskType);
			switch(pTSInfo->taskType)
			{
			case TASK_DL:
				{
					if(pTSInfo->statusCode == SC_DOING)
					{
						cJSON_AddNumberToObject(taskStatusItem, DEF_PERCENT_KEY, pTSInfo->u.dlPercent);
					}
					else
					{
						if(pTSInfo->u.msg && pTSInfo->statusCode != SC_SUSPEND)
							cJSON_AddStringToObject(taskStatusItem, DEF_MSG_KEY, pTSInfo->u.msg);
					}
					cJSON_AddNumberToObject(taskStatusItem, DEF_STATUS_KEY, pTSInfo->statusCode);
					break;
				}
			case TASK_DP:
				{
					if(pTSInfo->u.msg)
						cJSON_AddStringToObject(taskStatusItem, DEF_MSG_KEY, pTSInfo->u.msg);
					cJSON_AddNumberToObject(taskStatusItem, DEF_STATUS_KEY, pTSInfo->statusCode);
					break;
				}
			default:
				printf("cJSON_CreateTaskStatusItem: Error, invalid taskType\n");
				break;
			}
			if(pTSInfo->statusCode == SC_ERROR)
				cJSON_AddNumberToObject(taskStatusItem, DEF_ERRCODE_KEY, pTSInfo->errCode);
            //when deploy failed and then rollback,report a status with deploying error code.
            if(pTSInfo->statusCode == SC_DOING && pTSInfo->taskType == TASK_DP  && pTSInfo->taskAction == TA_NORMAL && pTSInfo->errCode > 0)
                cJSON_AddNumberToObject(taskStatusItem, DEF_ERRCODE_KEY, pTSInfo->errCode);
            cJSON_AddNumberToObject(taskStatusItem, DEF_ACTION_KEY, pTSInfo->taskAction);
            cJSON_AddNumberToObject(taskStatusItem, DEF_RETRYCNT_KEY, pTSInfo->taskAction == TA_RETRY ? pTSInfo->retryCnt : 0);
			cJSON_AddNumberToObject(taskStatusItem, DEF_PKGOWNERID_KEY, pTSInfo->pkgOwnerId);
        }
	}
	return taskStatusItem;
}

static cJSON * PackTaskStatusInfo(PTaskStatusInfo pTSInfo, char * devID, int cmdID)
{
	cJSON * packTarg = NULL;
	if(NULL != pTSInfo && pTSInfo->pkgName != NULL && NULL != devID)
	{
		cJSON * taskStatusItem = cJSON_CreateTaskStatusItem(pTSInfo);
		if(taskStatusItem)
		{
			packTarg = PackPutOnCoat(devID, cmdID, DEF_TASKSTATUS_KEY, taskStatusItem);
			if(packTarg == NULL)
			{
				cJSON_Delete(taskStatusItem);
			}
		}
	}
	return packTarg;
}

static cJSON * cJSON_CreateReqDLTaskItem(char * pkgName)
{
	cJSON * reqDLTaskItem = NULL;
	if(NULL != pkgName)
	{
		reqDLTaskItem = cJSON_CreateObject();
		cJSON_AddStringToObject(reqDLTaskItem, DEF_PKGNAME_KEY, pkgName);
	}
	return reqDLTaskItem;
}

static cJSON * cJSON_CreateScheStatusInfoItem(char * pkgType, int actType, long pkgOwnerId, int status, int errCode)
{
	cJSON * scheSIItem = NULL;
	if(NULL != pkgType)
	{
		scheSIItem= cJSON_CreateObject();
		cJSON_AddNumberToObject(scheSIItem, DEF_PKGOWNERID_KEY, pkgOwnerId);
		cJSON_AddStringToObject(scheSIItem, DEF_PKGTYPE_KEY, pkgType);
		cJSON_AddNumberToObject(scheSIItem, DEF_ACTTYPE_KEY, actType);
		cJSON_AddNumberToObject(scheSIItem, DEF_STATUS_KEY, status);
		cJSON_AddNumberToObject(scheSIItem, DEF_ERRCODE_KEY, errCode);
	}
	return scheSIItem;
}

static cJSON * cJSON_CreateScheInfoItem(char * pkgType, int actType, LHList * pDLScheInfoList, LHList * pDPScheInfoList)
{
	cJSON * scheInfoItem = NULL;
	if(NULL != pkgType)
	{
		cJSON * eItem = NULL;
		scheInfoItem = cJSON_CreateObject();
		cJSON_AddStringToObject(scheInfoItem, DEF_REQ_PKGTYPE_KEY, pkgType);
		cJSON_AddNumberToObject(scheInfoItem, DEF_REQ_ACTTYPE_KEY, actType);
		eItem = cJSON_CreateArray();
		cJSON_AddItemToObject(scheInfoItem, DEF_E_KEY, eItem);
		if(pDLScheInfoList)
		{
			cJSON * dlScheInfoListItem = cJSON_CreateScheInfoListItem(pkgType, pDLScheInfoList, SAT_DL);
			if(dlScheInfoListItem)
			{
				cJSON_AddItemToArray(eItem, dlScheInfoListItem);
			}
		}
		if(pDPScheInfoList)
		{
			cJSON * dpScheInfoListItem = cJSON_CreateScheInfoListItem(pkgType, pDPScheInfoList, SAT_DP);
			if(dpScheInfoListItem)
			{
				cJSON_AddItemToArray(eItem, dpScheInfoListItem);
			}
		}
	}
	return scheInfoItem;
}

static cJSON * cJSON_CreateScheInfoListItem(char * pkgType, LHList * pScheInfoList, int actType)
{
	cJSON * scheInfoListItem = NULL;
	if(NULL != pkgType && pScheInfoList != NULL)
	{
		if(pScheInfoList)
		{
			PLNode curLNode = NULL;
			PScheduleInfo pScheInfo = NULL;
			cJSON * timeInfoItem = NULL;
			cJSON * eItem = cJSON_CreateArray();
			scheInfoListItem = cJSON_CreateObject();
			cJSON_AddNumberToObject(scheInfoListItem, DEF_ACTTYPE_KEY, actType);
			cJSON_AddItemToObject(scheInfoListItem, DEF_E_KEY, eItem);
			curLNode = pScheInfoList->head;
			while(curLNode)
			{
				pScheInfo = (PScheduleInfo)curLNode->pUserData;
				if(strlen(pkgType) == 0 || strcmp(pkgType, pScheInfo->pkgType) == 0)
				{
					timeInfoItem = cJSON_CreateScheTimeInfoItem(pScheInfo);
					if(timeInfoItem)
					{
                        if (actType == SAT_DL)
                        {

                            UpgMode upgMode = pScheInfo->upgMode > UM_UNKNOW && pScheInfo->upgMode < UM_MAX ?
                                pScheInfo->upgMode : UM_HIGHEST;
                            cJSON_AddNumberToObject(timeInfoItem, DEF_UPGMODE_KEY, upgMode);
                        }
						cJSON_AddItemToArray(eItem, timeInfoItem);
					}
				}
				curLNode = curLNode->next;
			}
		}
	}
	return scheInfoListItem;
}

static cJSON * cJSON_CreateScheTimeInfoItem(PScheduleInfo pScheInfo)
{
	cJSON * scheTimeInfo = NULL;
	if(NULL != pScheInfo)
	{
		scheTimeInfo = cJSON_CreateObject();
		cJSON_AddNumberToObject(scheTimeInfo, DEF_PKGOWNERID_KEY, pScheInfo->pkgOwnerId);
		cJSON_AddStringToObject(scheTimeInfo, DEF_PKGTYPE_KEY, pScheInfo->pkgType);
		cJSON_AddNumberToObject(scheTimeInfo, DEF_SCHETYPE_KEY, pScheInfo->scheType);
		cJSON_AddStringToObject(scheTimeInfo, DEF_STARTTIME_KEY, pScheInfo->startTimeStr);
		cJSON_AddStringToObject(scheTimeInfo, DEF_ENDTIME_KEY, pScheInfo->endTimeStr);
	}
	return scheTimeInfo;
}

static cJSON * cJSON_CreateCheckPkgInfoItem(char *pkgType,
										    int upgMode,
											char *version,
											char *os,
											char *arch,
											char **subDevices,
											int subDeviceSize,
											long pkgOwnerId)
{
	cJSON * checkInfoItem = NULL;
	if(NULL != pkgType && NULL!=version)
	{
		checkInfoItem = cJSON_CreateObject();
		cJSON_AddNumberToObject(checkInfoItem, DEF_PKGOWNERID_KEY, pkgOwnerId);
		cJSON_AddStringToObject(checkInfoItem, DEF_PKGTYPE_KEY, pkgType);
		cJSON_AddStringToObject(checkInfoItem, DEF_VERSION_KEY, version);
		if(os) cJSON_AddStringToObject(checkInfoItem, DEF_OS_KEY, os);
		else cJSON_AddStringToObject(checkInfoItem, DEF_OS_KEY, "");
		if(arch)cJSON_AddStringToObject(checkInfoItem, DEF_ARCH_KEY, arch);
		else cJSON_AddStringToObject(checkInfoItem, DEF_ARCH_KEY, "");
        int tmpUpgMode = upgMode > UM_UNKNOW && upgMode < UM_MAX ? upgMode : UM_HIGHEST;
        cJSON_AddNumberToObject(checkInfoItem, DEF_UPGMODE_KEY, tmpUpgMode);

		//add subDevice
		if (subDevices != NULL&&subDeviceSize>0)
		{
			const char ** csubDevices = (const char **)malloc(subDeviceSize * sizeof(char *));
			int i = 0;
			for (i = 0;i < subDeviceSize; i++){
				const char * temp = subDevices[i];
				csubDevices[i] = temp;
			}
			cJSON * psubDevices = cJSON_CreateStringArray(csubDevices, subDeviceSize);
			cJSON_AddItemToObject(checkInfoItem, DEF_SUBDEVICES_KEY, psubDevices);

		}
		///
    }
	return checkInfoItem;
}

int PackScheInfoList(LHList * pDLScheInfoList, LHList * pDPScheInfoList, char ** jsonStr)
{
	int iRet = -1;
	if(NULL != jsonStr)
	{
		cJSON * packTarg = NULL;
		packTarg = cJSON_CreateArray();
		if(pDLScheInfoList)
		{
			cJSON * dlScheInfoListItem = cJSON_CreateScheInfoListItem("", pDLScheInfoList, SAT_DL);
			if(dlScheInfoListItem)
			{
				cJSON_AddItemToArray(packTarg, dlScheInfoListItem);
			}
		}
		if(pDPScheInfoList)
		{
			cJSON * dpScheInfoListItem = cJSON_CreateScheInfoListItem("", pDPScheInfoList, SAT_DP);
			if(dpScheInfoListItem)
			{
				cJSON_AddItemToArray(packTarg, dpScheInfoListItem);
			}
		}
		*jsonStr = cJSON_PrintUnformatted(packTarg);
		if(*jsonStr != NULL) iRet = 0;
		cJSON_Delete(packTarg);
	}
	return iRet;
}

int ParseScheInfoList(char * jsonStr, LHList * pDLScheInfoList, LHList * pDPScheInfoList)
{
	cJSON * eItem = NULL, *subItem = NULL;
	cJSON * listItem = NULL;
	cJSON * scheEItem = NULL, * scheSubItem = NULL, * infoSubItem = NULL;
	PLHList scheInfoList = NULL;
	ScheActType actType;
	PScheduleInfo pScheInfo = NULL;
	int arraySize, i, j = 0, len = 0, iFlag = 0, scheESize;
	int iRet = -1;

	if(NULL == jsonStr)
		return iRet;

	eItem = cJSON_Parse(jsonStr);
	if(!eItem)
		return iRet;

	arraySize = cJSON_GetArraySize(eItem);
	for(i = 0; i< arraySize; i++)
	{
		listItem = cJSON_GetArrayItem(eItem, i);
		if(!listItem)
			continue;

		subItem = cJSON_GetObjectItem(listItem, DEF_ACTTYPE_KEY);
		if(!subItem)
			continue;

		actType = subItem->valueint;
		if(actType == SAT_DL) {
			scheInfoList = pDLScheInfoList;
		} else if(actType == SAT_DP) {
			scheInfoList = pDPScheInfoList;
		}
		if(!scheInfoList)
			continue;

		scheEItem = cJSON_GetObjectItem(listItem, DEF_E_KEY);
		if(!scheEItem)
			continue;

		scheESize = cJSON_GetArraySize(scheEItem);
		for(j = 0; j<scheESize; j++)
		{
			scheSubItem = cJSON_GetArrayItem(scheEItem, j);
			if(!scheSubItem)
				continue;

			pScheInfo = (PScheduleInfo)malloc(sizeof(ScheduleInfo));
			memset(pScheInfo, 0, sizeof(ScheduleInfo));
			iFlag = 0;
			pScheInfo->actType = actType;

			do {
				//PkgType
				infoSubItem = cJSON_GetObjectItem(scheSubItem, DEF_PKGTYPE_KEY);
				if(!infoSubItem)
					break;
				len = strlen(infoSubItem->valuestring)+1;
				pScheInfo->pkgType = (char *)malloc(len);
				strcpy(pScheInfo->pkgType, infoSubItem->valuestring);

				//ScheType
				infoSubItem = cJSON_GetObjectItem(scheSubItem, DEF_SCHETYPE_KEY);
				if(!infoSubItem || infoSubItem->valueint <= ST_UNKNOW || infoSubItem->valueint >= ST_MAX)
					break;
				pScheInfo->scheType = infoSubItem->valueint;

				//StartTime
				infoSubItem = cJSON_GetObjectItem(scheSubItem, DEF_STARTTIME_KEY);
				if(!infoSubItem || !infoSubItem->valuestring)
					break;
				len = strlen(infoSubItem->valuestring) +1;
				pScheInfo->startTimeStr = (char*)malloc(len);
				strcpy(pScheInfo->startTimeStr, infoSubItem->valuestring);

				//EndTime
				infoSubItem = cJSON_GetObjectItem(scheSubItem, DEF_ENDTIME_KEY);
				if(!infoSubItem || !infoSubItem->valuestring)
					break;
				len = strlen(infoSubItem->valuestring)+1;
				pScheInfo->endTimeStr = (char*)malloc(len);
				strcpy(pScheInfo->endTimeStr, infoSubItem->valuestring);

				//PkgOwnerId
				pScheInfo->pkgOwnerId = 0;
				infoSubItem = cJSON_GetObjectItem(scheSubItem, DEF_PKGOWNERID_KEY);
				if(infoSubItem) {
					pScheInfo->pkgOwnerId = infoSubItem->valueint;
				}

				//UpgMode
				if (actType == SAT_DL){
					pScheInfo->upgMode = UM_HIGHEST;
					infoSubItem = cJSON_GetObjectItem(scheSubItem, DEF_UPGMODE_KEY);
					if (infoSubItem && infoSubItem->valueint > UM_UNKNOW && infoSubItem->valueint < UM_MAX) {
						pScheInfo->upgMode = infoSubItem->valueint;
					}
				}
				iFlag = 1;
			} while (0);

			if(!iFlag) {
				if(pScheInfo->pkgType) free(pScheInfo->pkgType);
				if(pScheInfo->endTimeStr) free(pScheInfo->endTimeStr);
				if(pScheInfo->startTimeStr) free(pScheInfo->startTimeStr);
				free(pScheInfo);
			} else {
				LOGD("Add schedule %d: pkgType=%s, ScheType=%d, time=%s~%s, pkgOwnerId=%ld",
					  j, pScheInfo->pkgType, pScheInfo->scheType, pScheInfo->startTimeStr, pScheInfo->endTimeStr, pScheInfo->pkgOwnerId);
				LHAddNode(scheInfoList, pScheInfo);
			}
		}
	}
	if (eItem)
		cJSON_Delete(eItem);
	iRet = 0;

	return iRet;
}

static cJSON * cJSON_CreateSWInfosItem(LHList * pSWInfoList)
{
    cJSON * eItem = cJSON_CreateArray();
    if (pSWInfoList != NULL)
    {
        cJSON *swInfoItem = NULL;
        PLNode curLNode = NULL;
        PSWInfo pSWInfo = NULL;
        curLNode = pSWInfoList->head;
        while (curLNode)
        {
            pSWInfo = (PSWInfo)curLNode->pUserData;
            if (pSWInfo != NULL &&
                pSWInfo->name != NULL && pSWInfo->version != NULL)
            {
                swInfoItem = cJSON_CreateObject();
                cJSON_AddStringToObject(swInfoItem, DEF_NAME_KEY, pSWInfo->name);
                cJSON_AddStringToObject(swInfoItem, DEF_VERSION_KEY, pSWInfo->version);
                cJSON_AddNumberToObject(swInfoItem, DEF_USABLE_KEY, pSWInfo->usable);
                cJSON_AddItemToArray(eItem, swInfoItem);
            }
            curLNode = curLNode->next;
        }
    }
    return eItem;
}

int PackSWInfoList(LHList * pSWInfoList, char * devID, char ** jsonStr)
{
    int iRet = -1;
    if (NULL != jsonStr)
    {
        cJSON * swInfosItem = cJSON_CreateSWInfosItem(pSWInfoList);
        if (swInfosItem)
        {
            cJSON * packTarg = NULL;
            packTarg = PackPutOnCoat(devID, CMD_SWINFO_REP, DEF_E_KEY, swInfosItem);
            if (packTarg == NULL)
            {
                cJSON_Delete(swInfosItem);
            }
            else
            {
                *jsonStr = cJSON_PrintUnformatted(packTarg);
                if (*jsonStr != NULL) iRet = 0;
                cJSON_Delete(packTarg);
            }
        }
    }
    return iRet;
}

static cJSON * cJSON_CreateDelPkgRetInfosItem(LHList * pDelPkgRetInfoList)
{
    cJSON * eItem = cJSON_CreateArray();
    if (pDelPkgRetInfoList != NULL)
    {
        cJSON *infoItem = NULL;
        PLNode curLNode = NULL;
        PDelPkgRetInfo pDelPkgRetInfo = NULL;
        curLNode = pDelPkgRetInfoList->head;
        while (curLNode)
        {
            pDelPkgRetInfo = (PDelPkgRetInfo)curLNode->pUserData;
            if (pDelPkgRetInfo != NULL && pDelPkgRetInfo->name != NULL)
            {
                infoItem = cJSON_CreateObject();
				cJSON_AddNumberToObject(infoItem, DEF_PKGOWNERID_KEY, pDelPkgRetInfo->pkgOwnerId);
                cJSON_AddStringToObject(infoItem, DEF_PKGNAME_KEY, pDelPkgRetInfo->name);
                cJSON_AddNumberToObject(infoItem, DEF_ERRCODE_KEY, pDelPkgRetInfo->errCode);
                cJSON_AddItemToArray(eItem, infoItem);
            }
            curLNode = curLNode->next;
        }
    }
    return eItem;
}

int PackDelPkgRetInfoList(LHList * pDelPkgRetInfoList, char * devID, char ** jsonStr)
{
    int iRet = -1;
    if (NULL != jsonStr)
    {
        cJSON * infosItem = cJSON_CreateDelPkgRetInfosItem(pDelPkgRetInfoList);
        if (infosItem)
        {
            cJSON * packTarg = NULL;
            packTarg = PackPutOnCoat(devID, CMD_DEL_PKG_REP, DEF_E_KEY, infosItem);
            if (packTarg == NULL)
            {
                cJSON_Delete(infosItem);
            }
            else
            {
                *jsonStr = cJSON_PrintUnformatted(packTarg);
                if (*jsonStr != NULL) iRet = 0;
                cJSON_Delete(packTarg);
            }
        }
    }
    return iRet;
}
