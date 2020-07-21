#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "util_path.h"

#define LOG_TAG "OTA"
#include "Log.h"
#include "SUESchedule.h"
#include "ErrorDef.h"
#include "ZSchedule.h"
#include "ScheduleData.h"
#include "ListHelper.h"
#include "MsgParser.h"
#include "Log.h"
#include "SUEClientCore.h"

typedef struct{
	pthread_mutex_t dlScheInfoMutex;
	PLHList dlScheInfoList;
	pthread_mutex_t dpScheInfoMutex;
	PLHList dpScheInfoList;
	ScheOutputMsgCB outputMsgCB;
	void * opCBUserData;
	char * cfgPath; // "xxx/Schedule.cfg"
}SUEScheContext;

typedef SUEScheContext * PSUEScheContext;

static void FreeScheInfoCB(void * userData);
static int MatchScheInfoCB(void * pUserData, void * key);
static int MatchScheInfoWithPkgNameCB(void * pUserData, void * key);
static void SUEZScheDLCheck(int * leaveFlag, PScheduleInfo pScheInfo);
static int SUEZScheDLTask(int * leaveFlag, PScheduleInfo pScheInfo);
static int SUEZScheDPTask(int * leaveFlag, PScheduleInfo pScheInfo);
static int SUEZScheTask(int * leaveFalg, void * userData);
static int ProcSetScheInfoReq(char * msg, SUEScheHandle sueScheHandle);
static int ProcDelScheInfoReq(char * msg, SUEScheHandle sueScheHandle);
static int ProcCheckPkgInfoRep(char * msg, SUEScheHandle sueScheHandle);
static int ProcScheInfoReq(char * msg, SUEScheHandle sueScheHandle);
extern char * GetOS();
extern char * GetDevID();
extern char * GetArch();
extern char * SuecGetErrorMsg(unsigned int errorCode);

PSUEScheContext CurSUEScheContext = NULL;

//#define SCHE_CFG_FILE     "Schedule.cfg"
static int GetScheInfoFromCfg(PLHList dlScheInfoList, PLHList dpScheInfoList, char * cfgPath);
static int FillScheInfoToCfg(PLHList dlScheInfoList, PLHList dpScheInfoList, char * cfgPath);
static int StartupSchedule(void * args, void * pUserData);

SUEScheHandle InitSUESche(char * scheDir)
{
	PSUEScheContext pSUEScheContext = NULL;
	if(CurSUEScheContext == NULL)
	{
        char *tmpScheDir = NULL;
        int len = 0;
        char modulePath[MAX_PATH] = { 0 };
		CurSUEScheContext = (PSUEScheContext)malloc(sizeof(SUEScheContext));
		memset(CurSUEScheContext, 0, sizeof(SUEScheContext));
		pthread_mutex_init(&CurSUEScheContext->dlScheInfoMutex, NULL);	
		CurSUEScheContext->dlScheInfoList = LHInitList(FreeScheInfoCB);
		pthread_mutex_init(&CurSUEScheContext->dpScheInfoMutex, NULL);	
		CurSUEScheContext->dpScheInfoList = LHInitList(FreeScheInfoCB);
		CurSUEScheContext->outputMsgCB = NULL;
        if (scheDir == NULL)
        {
            util_module_path_get(modulePath);
            tmpScheDir = modulePath;
        }
        else
			tmpScheDir = scheDir;
        len = strlen(tmpScheDir) + strlen(SCHE_CFG_FILE) + 2; // +2 for '/' and '\0'
        CurSUEScheContext->cfgPath = (char *)malloc(len);
        if (tmpScheDir[strlen(tmpScheDir) - 1] == FILE_SEPARATOR)
            sprintf(CurSUEScheContext->cfgPath, "%s%s", tmpScheDir, SCHE_CFG_FILE);
        else
            sprintf(CurSUEScheContext->cfgPath, "%s%c%s", tmpScheDir, FILE_SEPARATOR, SCHE_CFG_FILE);
	}
	pthread_mutex_lock(&CurSUEScheContext->dlScheInfoMutex);
	pthread_mutex_lock(&CurSUEScheContext->dpScheInfoMutex);
	GetScheInfoFromCfg(CurSUEScheContext->dlScheInfoList, CurSUEScheContext->dpScheInfoList, CurSUEScheContext->cfgPath);
	pthread_mutex_unlock(&CurSUEScheContext->dpScheInfoMutex);
	pthread_mutex_unlock(&CurSUEScheContext->dlScheInfoMutex);
	pSUEScheContext = CurSUEScheContext;
	return pSUEScheContext;
}

int UninitSUESche(SUEScheHandle sueScheHandle)
{
	if(sueScheHandle != NULL)
	{
		PSUEScheContext pSUEScheContext = (PSUEScheContext)sueScheHandle;
		pthread_mutex_lock(&pSUEScheContext->dlScheInfoMutex);
		pthread_mutex_lock(&pSUEScheContext->dpScheInfoMutex);
		FillScheInfoToCfg(pSUEScheContext->dlScheInfoList, pSUEScheContext->dpScheInfoList, pSUEScheContext->cfgPath);
		pthread_mutex_unlock(&pSUEScheContext->dpScheInfoMutex);
		pthread_mutex_unlock(&pSUEScheContext->dlScheInfoMutex);
		if(pSUEScheContext->dlScheInfoList)
		{
			LHDestroyList(pSUEScheContext->dlScheInfoList);
			pSUEScheContext->dlScheInfoList = NULL;
		}
		pthread_mutex_destroy(&pSUEScheContext->dlScheInfoMutex);
		if(pSUEScheContext->dpScheInfoList)
		{
			LHDestroyList(pSUEScheContext->dpScheInfoList);
			pSUEScheContext->dpScheInfoList = NULL;
		}
		pthread_mutex_destroy(&pSUEScheContext->dpScheInfoMutex);
		if(CurSUEScheContext->cfgPath) free(CurSUEScheContext->cfgPath);
		CurSUEScheContext->cfgPath = NULL;
		if(CurSUEScheContext)
		{
			free(CurSUEScheContext);
			CurSUEScheContext = NULL;
		}
	}
	return 0;
}

int StartSUESche()
{
	// start schedule download thread
	pthread_mutex_lock(&CurSUEScheContext->dlScheInfoMutex);
	if(!LHListIsEmpty(CurSUEScheContext->dlScheInfoList))
		LHIterate(CurSUEScheContext->dlScheInfoList, StartupSchedule, NULL);
	pthread_mutex_unlock(&CurSUEScheContext->dlScheInfoMutex);

	// start schedule deploy thread
	pthread_mutex_lock(&CurSUEScheContext->dpScheInfoMutex);
	if(!LHListIsEmpty(CurSUEScheContext->dpScheInfoList))
		LHIterate(CurSUEScheContext->dpScheInfoList, StartupSchedule, NULL);
	pthread_mutex_unlock(&CurSUEScheContext->dpScheInfoMutex);

	return 0;
}

int SetSUESheOutPutMsgCB(SUEScheHandle sueScheHandle, ScheOutputMsgCB outputMsgCB, void * userData)
{
	int retCode = SUEC_SUCCESS;
	if(NULL == outputMsgCB || sueScheHandle == NULL) return SUEC_I_PARAMETER_ERROR;
	{
		PSUEScheContext pSUEScheContext = (PSUEScheContext)sueScheHandle;
		pSUEScheContext->outputMsgCB = outputMsgCB;
		pSUEScheContext->opCBUserData = userData;
	}
	return retCode;
}

int ProcSUEScheCmd(SUEScheHandle sueScheHandle, char * jsonCmd)
{
	int retCode = SUEC_SUCCESS;
	if(NULL == jsonCmd || sueScheHandle == NULL) return SUEC_I_PARAMETER_ERROR;
	{
		int cmdID = CMD_INVALID;
		cmdID = ParseCmdID(jsonCmd);
		switch(cmdID)
		{
		case CMD_SET_SCHE_REQ:
			{
				retCode = ProcSetScheInfoReq(jsonCmd, sueScheHandle);
				break;
			}
		case CMD_DEL_SCHE_REQ:
			{
				retCode = ProcDelScheInfoReq(jsonCmd, sueScheHandle);
				break;
			}
		case CMD_CHECK_PKGINFO_REP:
			{
				retCode = ProcCheckPkgInfoRep(jsonCmd, sueScheHandle);
				break;
			}
		case CMD_SCHE_INFO_REQ:
			{
				retCode = ProcScheInfoReq(jsonCmd, sueScheHandle);
				break;
			}
		default: 
			{
				retCode = SUEC_I_MSG_PARSE_FAILED;
				break;
			}
		}
	}
	if(retCode != SUEC_SUCCESS) LOGE("SUEClient Input message error!ErrCode:%d, ErrMsg:%s", 
		retCode, SuecGetErrorMsg(retCode));
	return retCode;
}

void InterceptorSUEScheDLRet(SUEScheHandle sueScheHandle, char * pkgName, int retCode)
{
	if(sueScheHandle != NULL && pkgName != NULL)
	{
		PSUEScheContext pSUEScheContext = (PSUEScheContext)sueScheHandle;
		PLHList dlScheInfoList = NULL;
		PLNode findNode = NULL;
		pthread_mutex_lock(&pSUEScheContext->dlScheInfoMutex);
		dlScheInfoList = pSUEScheContext->dlScheInfoList;
		findNode = LHFindNode(dlScheInfoList, MatchScheInfoWithPkgNameCB, pkgName);
		if(findNode)
		{
			PScheduleInfo pScheInfo = (PScheduleInfo)findNode->pUserData;
			pScheInfo->dlRetCode = retCode;
			pScheInfo->isDLRetChecking = 0;
		}
		pthread_mutex_unlock(&pSUEScheContext->dlScheInfoMutex);
	}
}

static void FreeScheInfoCB(void * userData)
{
	if(NULL != userData)
	{
		PScheduleInfo pScheInfo = (PScheduleInfo)userData;
		if(pScheInfo->zScheHandle)
		{
			ZScheStop(pScheInfo->zScheHandle);
			ZScheDestory(pScheInfo->zScheHandle);
			pScheInfo->zScheHandle = NULL;
		}
		if(pScheInfo->pObtainedDLInfo)
		{
			SUECCoreFreeDLTaskInfo(pScheInfo->pObtainedDLInfo);
			pScheInfo->pObtainedDLInfo = NULL;
		}
		if(pScheInfo->pkgType)
		{
			free(pScheInfo->pkgType);
			pScheInfo->pkgType = NULL;
		}
		if(pScheInfo->startTimeStr)
		{
			free(pScheInfo->startTimeStr);
			pScheInfo->startTimeStr = NULL;
		}
		if(pScheInfo->endTimeStr)
		{
			free(pScheInfo->endTimeStr);
			pScheInfo->endTimeStr = NULL;
		}
		//free subdevice
		if (pScheInfo->subDevices != NULL&&pScheInfo->subDeviceSize > 0)
		{
			int i = 0;
			for (i = 0; i < pScheInfo->subDeviceSize; i++){
				if (pScheInfo->subDevices[i] != NULL)
				{
					free(pScheInfo->subDevices[i]);
					pScheInfo->subDevices[i] = NULL;
				}
			}
			free(pScheInfo->subDevices);
			pScheInfo->subDevices = NULL;
		}
		//
		free(pScheInfo);
		pScheInfo = NULL;
	}
}

static int MatchScheInfoCB(void * pUserData, void * key)
{
	int iRet = 0;
	if(pUserData != NULL && key != NULL)
	{
		char * curPkgType = (char*)key;
		PScheduleInfo curScheInfo = (PScheduleInfo)pUserData;
		if(curScheInfo->pkgType && !strcmp(curPkgType, curScheInfo->pkgType))
		{
			iRet = 1;
		}
	}
	return iRet;
}

static int MatchScheInfoWithPkgNameCB(void * pUserData, void * key)
{
	int iRet = 0;
	if(pUserData != NULL && key != NULL)
	{
		char * curPkgType = (char*)key;
		PScheduleInfo curScheInfo = (PScheduleInfo)pUserData;
		if(curScheInfo->pObtainedDLInfo && curScheInfo->pObtainedDLInfo->pkgName && 
			!strcmp(curPkgType, curScheInfo->pObtainedDLInfo->pkgName))
		{
			iRet = 1;
		}
	}
	return iRet;
}

static int ScheTimeS2T(PScheduleInfo pScheInfo, PZScheTimeRange pScheTR)
{
	int iRet = SUEC_SUCCESS;
	if(pScheInfo == NULL || pScheInfo == NULL)
		return SUEC_I_PARAMETER_ERROR;
	{
		int cnt = 0;
		iRet = SUEC_S_SCHE_TIME_FORMAT_ERROR; 
		pScheTR->scheType = pScheInfo->scheType;
		switch(pScheInfo->scheType)
		{
		case ST_ONCE:
			{
				cnt = sscanf(pScheInfo->startTimeStr, "%d-%d-%d-%d:%d:%d", &pScheTR->startTime.year, &pScheTR->startTime.month,
					&pScheTR->startTime.day, &pScheTR->startTime.hour, &pScheTR->startTime.minute, &pScheTR->startTime.second);
				if(cnt == 6)
				{
					cnt = sscanf(pScheInfo->endTimeStr, "%d-%d-%d-%d:%d:%d", &pScheTR->endTime.year, &pScheTR->endTime.month,
						&pScheTR->endTime.day, &pScheTR->endTime.hour, &pScheTR->endTime.minute, &pScheTR->endTime.second);
					if(cnt == 6) iRet = SUEC_SUCCESS;
				}
				break;
			}
		case ST_EVERY_MONTH:
		case ST_EVERY_WEEK:
			{
				cnt = sscanf(pScheInfo->startTimeStr, "%d-%d:%d:%d", &pScheTR->startTime.day, &pScheTR->startTime.hour, 
					&pScheTR->startTime.minute, &pScheTR->startTime.second);
				if(cnt == 4)
				{
					cnt = sscanf(pScheInfo->endTimeStr, "%d-%d:%d:%d", &pScheTR->endTime.day, &pScheTR->endTime.hour, 
						&pScheTR->endTime.minute, &pScheTR->endTime.second);
					if(cnt == 4) iRet = SUEC_SUCCESS;
				}
				break;
			}
		case ST_EVERY_DAY:
			{
				cnt = sscanf(pScheInfo->startTimeStr, "%d:%d:%d", &pScheTR->startTime.hour, 
					&pScheTR->startTime.minute, &pScheTR->startTime.second);
				if(cnt == 3)
				{
					cnt = sscanf(pScheInfo->endTimeStr, "%d:%d:%d", &pScheTR->endTime.hour, 
						&pScheTR->endTime.minute, &pScheTR->endTime.second);
					if(cnt == 3) iRet = SUEC_SUCCESS;
				}
				break;
			}
		default:
			LOGE("ScheTimeS2T: Error, invalid scheType");
			break;
		}
	}
	return iRet;
}

static int SUEScheEnterNotify(void * userData)
{
	int ret = SUEC_CORE_SUCCESS;
	PSUEScheContext pSUEScheContext = CurSUEScheContext;
	PScheduleInfo pScheInfo = (PScheduleInfo)userData;
	if(pScheInfo)
	{
		if(pScheInfo->actType == SAT_DL)
			ret = SUECCoreResumeDLTask(pScheInfo->pkgType, NULL);
		else if(pScheInfo->actType == SAT_DP)
			ret = SUECCoreResumeDPTask(pScheInfo->pkgType, NULL);

		if (ret == SUEC_I_OBJECT_NOT_FOUND) { // ignore resume not found error
			ret = SUEC_CORE_SUCCESS;
		}

		LOGI("Schedule enter notify!PkgType:%s, ActType:%d, ScheType:%d", 
			pScheInfo->pkgType, pScheInfo->actType, pScheInfo->scheType);
		if(pSUEScheContext->outputMsgCB && ret == SUEC_CORE_SUCCESS)
		{
			char * outMsg = NULL;
			if(PackScheStatusInfo(pScheInfo->pkgType, pScheInfo->actType, pScheInfo->pkgOwnerId, SST_ENTER, 0, GetDevID(), &outMsg) == 0)
			{
				LOGD("SUEClient output message: %s",outMsg);
				pSUEScheContext->outputMsgCB(outMsg, strlen(outMsg)+1, pSUEScheContext->opCBUserData);
				if(outMsg) free(outMsg);
			}
		}
	}
	return ret;
}

static int SUEZScheLeaveNotify(void * userData)
{
	int ret = SUEC_CORE_SUCCESS;
	PSUEScheContext pSUEScheContext = CurSUEScheContext;
	PScheduleInfo pScheInfo = (PScheduleInfo)userData;
	if(pScheInfo)
	{
		if(pScheInfo->actType == SAT_DL)
			ret = SUECCoreSuspendDLTask(pScheInfo->pkgType, NULL);
		else if(pScheInfo->actType == SAT_DP)
			ret = SUECCoreSuspendDPTask(pScheInfo->pkgType, NULL);
		LOGI("Schedule leave notify!PkgType:%s, ActType:%d, ScheType:%d", 
			pScheInfo->pkgType, pScheInfo->actType, pScheInfo->scheType);
		if(pSUEScheContext->outputMsgCB)
		{
			char * outMsg = NULL;
			if(PackScheStatusInfo(pScheInfo->pkgType, pScheInfo->actType, pScheInfo->pkgOwnerId, SST_LEAVE, 0, GetDevID(), &outMsg) == 0)
			{
				LOGD("SUEClient output message: %s",outMsg);
				// outputMsgCB->SUEScheOutputMsgCB->OTAOutputMsgCB->g_sendcustcbf()
				pSUEScheContext->outputMsgCB(outMsg, strlen(outMsg)+1, pSUEScheContext->opCBUserData);
				if(outMsg) free(outMsg);
			}
		}
	}
	return ret;
}

static void SUEZScheDLCheck(int * leaveFlag, PScheduleInfo pScheInfo)
{
#define DEF_VERSION      "0.0.0.0"
#define DEF_TIMEOUT_MS   5000
	PSUEScheContext pSUEScheContext = CurSUEScheContext;

	if(leaveFlag != NULL && pScheInfo != NULL)
	{
		if(pSUEScheContext->outputMsgCB)
		{
            char * outMsg = NULL, *curPkgVer = NULL, *pkgVer = DEF_VERSION;
            SUECCoreGetRecordPkgVersion(pScheInfo->pkgType, &curPkgVer);
            if (curPkgVer != NULL)
				pkgVer = curPkgVer;
            if (PackCheckPkgInfo(pScheInfo->pkgType, pScheInfo->upgMode, pkgVer, GetOS(),
				GetArch(), GetDevID(),pScheInfo->subDevices,pScheInfo->subDeviceSize, pScheInfo->pkgOwnerId, &outMsg) == 0)
			{
				LOGD("SUEClient output message: %s",outMsg);
				pScheInfo->isDLInfoChecking = 1;
				if(pSUEScheContext->outputMsgCB(outMsg, strlen(outMsg)+1, pSUEScheContext->opCBUserData) == 0)
				{
					int cnt = DEF_TIMEOUT_MS/10, i = 0;
					for(i = 0; i<cnt && !*leaveFlag && pScheInfo->isDLInfoChecking; i++)   //wait result,once check interval 10ms
					{
						usleep(1000*10);
					}
					if(pScheInfo->isDLInfoChecking)
					{
						pScheInfo->isDLInfoChecking = 0;
					}
				}
				if(outMsg) free(outMsg);
			}
            if (curPkgVer != NULL) free(curPkgVer);
		}
	}
}

/*
	return 0 for download task is already exist or new download task is added sucessfully.
	others for download complete or fail
*/
static int SUEZScheProcCheckRet(PDLTaskInfo pDLTaskInfo)
{
	int iRet = 0;
	if(pDLTaskInfo != NULL)
	{
		PSUEScheContext pSUEScheContext = CurSUEScheContext;
		PTaskStatusInfo pTaskStatusInfo = NULL;
		SUECCoreGetTaskStatus(pDLTaskInfo->pkgName, &pTaskStatusInfo);
		if(pTaskStatusInfo) //about pkgname's task exist
		{
			int errCode = SUEC_SUCCESS;
            SUECCoreSetTaskNorms(pDLTaskInfo->pkgName, pDLTaskInfo->dlRetry, pDLTaskInfo->dpRetry, pDLTaskInfo->isRollBack);
			if((pTaskStatusInfo->taskType == TASK_DL && pTaskStatusInfo->statusCode == SC_FINISHED) || // already download
				pTaskStatusInfo->taskType == TASK_DP) //package exist
			{
				LOGD("SUEClient schedule checked package, %s already exist!",pDLTaskInfo->pkgName);
				errCode = SUEC_S_PKG_FILE_EXIST;
				iRet = 1;
			}
			else  //package download task exist
			{
				if(pTaskStatusInfo->taskType == TASK_DL && pTaskStatusInfo->statusCode == SC_SUSPEND) //if suspend then resume
				{
					SUECCoreResumeDLTask(NULL, pTaskStatusInfo->pkgName);
				}
				LOGD("SUEClient schedule checked package %s download task already exist!",pDLTaskInfo->pkgName);
				errCode = SUEC_S_PKG_DL_TASK_EXIST;
			}
			// send error response to schedule fail
			if(errCode != SUEC_SUCCESS && pSUEScheContext->outputMsgCB)
			{
				char * outMsg = NULL;
				TaskStatusInfo tsInfo;
				memset(&tsInfo, 0, sizeof(TaskStatusInfo));
				tsInfo.taskType = TASK_DL;
				tsInfo.pkgName = pDLTaskInfo->pkgName;
				tsInfo.pkgOwnerId = pDLTaskInfo->pkgOwnerId;
				tsInfo.errCode = errCode;
				tsInfo.statusCode = SC_ERROR;
				if(PackAutoTaskStatusInfo(&tsInfo, GetDevID(), &outMsg)==0)
				{
					LOGD("SUEClient output message: %s",outMsg);
					pSUEScheContext->outputMsgCB(outMsg, strlen(outMsg)+1, pSUEScheContext->opCBUserData);
					if(outMsg) free(outMsg);
				}
			}
			SUECCoreFreeTSInfo(pTaskStatusInfo);
			pTaskStatusInfo = NULL;
		}
		else //about pkgname's task  not exist, then add
		{
			LOGD("SUEClient schedule checked package %s add download task!",pDLTaskInfo->pkgName);
			SUECCoreAddDLTask(pDLTaskInfo);
            SUECCoreSetTaskNorms(pDLTaskInfo->pkgName, pDLTaskInfo->dlRetry, pDLTaskInfo->dpRetry, pDLTaskInfo->isRollBack);
		}
	}
	return iRet;
}

static int SUEZScheDLTask(int * leaveFlag, PScheduleInfo pScheInfo)
{
#define DEF_SCHE_DL_INTERVAL_MS   1800000 //30*60*1000
	int isDownloadComplete = 0;
	if(leaveFlag != NULL && pScheInfo != NULL)
	{
		if(!*leaveFlag)
		{
			unsigned int scheDLintervalMS = DEF_SCHE_DL_INTERVAL_MS;
			SUEZScheDLCheck(leaveFlag, pScheInfo);  //check download pkginfo
			if(pScheInfo->pObtainedDLInfo && !*leaveFlag)
			{
				pScheInfo->dlRetCode = 0;
				pScheInfo->isDLRetChecking = 1; //raise monitor dl result flag
				isDownloadComplete = SUEZScheProcCheckRet(pScheInfo->pObtainedDLInfo); //process checked download pkginfo
				if(isDownloadComplete == 0) //downloading
				{
					while(pScheInfo->isDLRetChecking && !*leaveFlag) //wait dl result
					{
						usleep(1000*100);
					}
				}
				else //package exist, not need monitor dl result
				{
					pScheInfo->isDLRetChecking = 0; //put down monitor dl result flag
				}
			}
			else
				scheDLintervalMS = scheDLintervalMS / 3;
			if(pScheInfo->pObtainedDLInfo)
				SUECCoreFreeDLTaskInfo(pScheInfo->pObtainedDLInfo);
			pScheInfo->pObtainedDLInfo = NULL;
			if (pScheInfo->dlRetCode == SUEC_S_PKG_FT_DL_ERROR)
				scheDLintervalMS = DEF_SCHE_DL_INTERVAL_MS / 3;
			if(!*leaveFlag)
			{
				int cnt = scheDLintervalMS / 100, i = 0; 
				for(i = 0; i<cnt && !*leaveFlag; i++) //sleep 30 or 10 minute
				{
					usleep(1000*100);
				}
			}
		}
	}
	// return 1 to continue to monitor, 0 to stop to monitor
	return (isDownloadComplete == 0)? 1: 0; 
}

static int SUEZScheDPTask(int * leaveFlag, PScheduleInfo pScheInfo)
{
#define DEF_SCHE_DP_INTERVAL_MS   1800000 //30*60*1000

	LOGD("SUEZScheDPTask: Entry");

	if(leaveFlag != NULL && pScheInfo != NULL)
	{
		DPTaskInfo dpTaskInfo;
		int addDpRet = 0;
        unsigned int scheDPintervalMS = DEF_SCHE_DP_INTERVAL_MS;
        memset(&dpTaskInfo, 0, sizeof(DPTaskInfo));
		dpTaskInfo.pkgName = NULL;
		dpTaskInfo.pkgType = pScheInfo->pkgType;
		dpTaskInfo.pkgOwnerId = pScheInfo->pkgOwnerId;
		if(!*leaveFlag)
		{
			addDpRet = SUECCoreAddDPTask(&dpTaskInfo);
		}
        if (addDpRet != SUEC_SUCCESS)
			scheDPintervalMS = scheDPintervalMS/3;
		if(!*leaveFlag)
		{
			int cnt = scheDPintervalMS/100, i = 0; 
			for(i = 0; i<cnt && !*leaveFlag; i++)  //sleep 30 or 10 minute
			{
				usleep(1000*100);
			}
		}
	}
	return 1;
}

static int SUEZScheTask(int * leaveFalg, void * userData)
{
	int iRet = 0;
	PScheduleInfo pScheInfo = (PScheduleInfo)userData;
	if(pScheInfo)
	{
		LOGD("Schedule tasking!PkgType:%s, ActType:%d, ScheType:%d, PkgOwnerId:%ld", 
			pScheInfo->pkgType, pScheInfo->actType, pScheInfo->scheType, pScheInfo->pkgOwnerId);
		if(pScheInfo->actType == SAT_DL)
		{
			iRet = SUEZScheDLTask(leaveFalg, pScheInfo);
		}
		else if(pScheInfo->actType == SAT_DP)
		{
			iRet = SUEZScheDPTask(leaveFalg, pScheInfo);
		}
	}
	return iRet;
}

static int ProcSetScheInfoReq(char * msg, SUEScheHandle sueScheHandle)
{
	int retCode = SUEC_SUCCESS;
	int addFlag = 0;
	PScheduleInfo pScheInfo = NULL;

	if(msg == NULL || sueScheHandle == NULL)
		return SUEC_I_PARAMETER_ERROR;

	do {
		PSUEScheContext pSUEScheContext = (PSUEScheContext)sueScheHandle;
		PLHList scheInfoList = NULL;
		PLNode pLNode = NULL;
		pthread_mutex_t * pCurMutex = NULL;
		ZScheTimeRange zScheTR; // TR: time range
		int isModified = 1;
		PScheduleInfo pFindScheInfo = NULL;

		pScheInfo = (PScheduleInfo)malloc(sizeof(ScheduleInfo));
		memset(pScheInfo, 0, sizeof(ScheduleInfo));
		if(ParseSetScheInfo(msg, pScheInfo)) {
			retCode = SUEC_I_MSG_PARSE_FAILED;
			break;
		}
		
		if(pScheInfo->actType == SAT_DL) {
			scheInfoList = pSUEScheContext->dlScheInfoList;
			pCurMutex = &pSUEScheContext->dlScheInfoMutex;
		}
		else if(pScheInfo->actType == SAT_DP) {
			scheInfoList = pSUEScheContext->dpScheInfoList;
			pCurMutex = &pSUEScheContext->dpScheInfoMutex;
		} else {
			retCode = SUEC_S_SCHE_TYPE_ERROR;
			break;
		}

		pthread_mutex_lock(pCurMutex);
		memset(&zScheTR, 0, sizeof(ZScheTimeRange));
		retCode = ScheTimeS2T(pScheInfo, &zScheTR);
		if(retCode != SUEC_SUCCESS) {
			break;
		}
		
		pLNode = LHFindNode(scheInfoList, MatchScheInfoCB, pScheInfo->pkgType);
		if(pLNode) { //found, check if it need to modified
			ZScheTimeRange zScheFindTR;
			
			memset(&zScheFindTR, 0, sizeof(ZScheTimeRange));
			pFindScheInfo = (PScheduleInfo)pLNode->pUserData;
			ZScheGetTimeRange(pFindScheInfo->zScheHandle, &zScheFindTR);
			if ((ZScheCmpTimeRange(&zScheFindTR, &zScheTR) == 0) &&
				(pScheInfo->actType == SAT_DL && pScheInfo->upgMode == pFindScheInfo->upgMode))
			{
				pFindScheInfo = NULL;
				isModified = 0;
			}
		}

		if(isModified)
		{
			retCode = SUEC_S_SCHE_STARTUP_ERROR;
			pScheInfo->zScheHandle = ZScheCreate(&zScheTR);
			if(pScheInfo->zScheHandle)
			{
				if(pFindScheInfo) {
					LHDelNode(scheInfoList, MatchScheInfoCB, pScheInfo->pkgType);
				}
				if(ZScheRun(pScheInfo->zScheHandle, SUEScheEnterNotify, SUEZScheLeaveNotify, SUEZScheTask, pScheInfo) == 0) {
					LHAddNode(scheInfoList, pScheInfo);
					addFlag = 1;
					retCode = SUEC_SUCCESS;
				}
			}
		}
		pthread_mutex_unlock(pCurMutex);
		
		if(pSUEScheContext->outputMsgCB) {
			char * outMsg = NULL;
			if(PackSetScheRepInfo(msg, retCode, GetDevID(), &outMsg)==0) {
				LOGD("SUEClient set schdule response: %s",outMsg);
				pSUEScheContext->outputMsgCB(outMsg, strlen(outMsg)+1, pSUEScheContext->opCBUserData);
				if(outMsg) free(outMsg);
			}
		}

		// write to schedule file Schedule.cfg
		pthread_mutex_lock(&pSUEScheContext->dlScheInfoMutex);
		pthread_mutex_lock(&pSUEScheContext->dpScheInfoMutex);
		FillScheInfoToCfg(pSUEScheContext->dlScheInfoList, pSUEScheContext->dpScheInfoList, pSUEScheContext->cfgPath);
		pthread_mutex_unlock(&pSUEScheContext->dpScheInfoMutex);
		pthread_mutex_unlock(&pSUEScheContext->dlScheInfoMutex);
	} while (0);
	
	if(!addFlag && pScheInfo) { // if not add to schedule list and pScheInfo exist
		FreeScheInfoCB(pScheInfo);
		pScheInfo = NULL;
	}
	return retCode;
}

static int ProcDelScheInfoReq(char * msg, SUEScheHandle sueScheHandle)
{
	int retCode = SUEC_SUCCESS;
	if(msg == NULL || sueScheHandle == NULL) return SUEC_I_PARAMETER_ERROR;
	{
		char * pkgType = NULL;
		int actType = SAT_UNKNOW;
		int pkgOwnerId = 0;
		PSUEScheContext pSUEScheContext = (PSUEScheContext)sueScheHandle;
		if(ParseDelScheInfo(msg, &pkgType, &actType, &pkgOwnerId) == 0)
		{
			retCode = SUEC_S_SCHE_OBJ_NOT_EXIST;
			if(actType & SAT_DL)
			{
				pthread_mutex_lock(&pSUEScheContext->dlScheInfoMutex);
				if(strlen(pkgType) > 0)
				{
					if(LHDelNode(pSUEScheContext->dlScheInfoList, MatchScheInfoCB, pkgType) == 0) retCode = SUEC_SUCCESS;
				}
				else if(pSUEScheContext->dlScheInfoList->head != NULL)
				{
					LHDelAllNode(pSUEScheContext->dlScheInfoList);
					retCode = SUEC_SUCCESS;
				}
				pthread_mutex_unlock(&pSUEScheContext->dlScheInfoMutex);
			}
			if(actType & SAT_DP)
			{
				pthread_mutex_lock(&pSUEScheContext->dpScheInfoMutex);
				if(strlen(pkgType) > 0)
				{
					if(LHDelNode(pSUEScheContext->dpScheInfoList, MatchScheInfoCB, pkgType) == 0) retCode = SUEC_SUCCESS;
				}
				else if(pSUEScheContext->dpScheInfoList->head != NULL)
				{
					LHDelAllNode(pSUEScheContext->dpScheInfoList);
					retCode = SUEC_SUCCESS;
				}
				pthread_mutex_unlock(&pSUEScheContext->dpScheInfoMutex);
			}
			if(pkgType)
				free(pkgType);
			pkgType = NULL;

			if(pSUEScheContext->outputMsgCB)
			{
				char * outMsg = NULL;
				if(PackDelScheRepInfo(msg, retCode, GetDevID(), &outMsg)==0)
				{
					LOGD("SUEClient output message: %s",outMsg);
					pSUEScheContext->outputMsgCB(outMsg, strlen(outMsg)+1, pSUEScheContext->opCBUserData);
					if(outMsg) free(outMsg);
				}
			}
			pthread_mutex_lock(&pSUEScheContext->dlScheInfoMutex);
			pthread_mutex_lock(&pSUEScheContext->dpScheInfoMutex);
			FillScheInfoToCfg(pSUEScheContext->dlScheInfoList, pSUEScheContext->dpScheInfoList, pSUEScheContext->cfgPath);
			pthread_mutex_unlock(&pSUEScheContext->dpScheInfoMutex);
			pthread_mutex_unlock(&pSUEScheContext->dlScheInfoMutex);
		}
	}
	return retCode;
}

static int ProcCheckPkgInfoRep(char * msg, SUEScheHandle sueScheHandle)
{
	int retCode = SUEC_SUCCESS;
	if(msg == NULL || sueScheHandle == NULL) return SUEC_I_PARAMETER_ERROR;
	{
		int isGiveup = 1;
		PDLTaskInfo pDLTaskInfo = NULL;
		PSUEScheContext pSUEScheContext = (PSUEScheContext)sueScheHandle;
		pDLTaskInfo = (PDLTaskInfo)malloc(sizeof(DLTaskInfo));
		memset(pDLTaskInfo, 0, sizeof(DLTaskInfo));
		if(ParseCheckPkgRetInfo(msg, pDLTaskInfo) == 0)
		{
			PLNode pLNode = NULL;
			pthread_mutex_lock(&pSUEScheContext->dlScheInfoMutex);
			pLNode = LHFindNode(pSUEScheContext->dlScheInfoList, MatchScheInfoCB, pDLTaskInfo->pkgType);
			if(pLNode)
			{
				PScheduleInfo pScheInfo = (PScheduleInfo)pLNode->pUserData;
				if(pScheInfo->isDLInfoChecking)
				{
					pScheInfo->pObtainedDLInfo = pDLTaskInfo;
					pScheInfo->isDLInfoChecking = 0;
					isGiveup = 0;
				}
			}
			pthread_mutex_unlock(&pSUEScheContext->dlScheInfoMutex);
			if(isGiveup)
			{
				if(pDLTaskInfo->pkgName) free(pDLTaskInfo->pkgName);
				if(pDLTaskInfo->pkgType) free(pDLTaskInfo->pkgType);
				if(pDLTaskInfo->url) free(pDLTaskInfo->url);
				free(pDLTaskInfo);
				//free subdevice
				if (pDLTaskInfo->subDevices != NULL && pDLTaskInfo->subDeviceSize > 0)
				{
					int i = 0;
					for (i = 0; i < pDLTaskInfo->subDeviceSize; i++){
						if (pDLTaskInfo->subDevices[i] != NULL)
						{
							free(pDLTaskInfo->subDevices[i]);
							pDLTaskInfo->subDevices[i] = NULL;
						}
					}
					free(pDLTaskInfo->subDevices);
					pDLTaskInfo->subDevices = NULL;
				}
				//
			}
		}
		else
		{
			free(pDLTaskInfo);
			retCode = SUEC_I_MSG_PARSE_FAILED;
		}
	}
	return retCode;
}

static int ProcScheInfoReq(char * msg, SUEScheHandle sueScheHandle)
{
	int retCode = SUEC_SUCCESS;
	if(msg == NULL || sueScheHandle == NULL) return SUEC_I_PARAMETER_ERROR;
	{
		char * pkgType = NULL;
		int actType = 0;
		PSUEScheContext pSUEScheContext = (PSUEScheContext)sueScheHandle;
		if(ParseGetSheInfo(msg, &pkgType, &actType) == 0)
		{
			PLHList dlScheInfoList = NULL;
			PLHList dpScheInfoList = NULL;
			pthread_mutex_lock(&pSUEScheContext->dpScheInfoMutex);
			pthread_mutex_lock(&pSUEScheContext->dlScheInfoMutex);
			if(actType & SAT_DL) dlScheInfoList = pSUEScheContext->dlScheInfoList;
			if(actType & SAT_DP) dpScheInfoList = pSUEScheContext->dpScheInfoList;
			if(pSUEScheContext->outputMsgCB)
			{
				char * outMsg = NULL;
				if(PackScheInfo(pkgType, actType, dlScheInfoList, dpScheInfoList, GetDevID(), &outMsg) == 0)
				{
					LOGD("SUEClient output message: %s",outMsg);
					pSUEScheContext->outputMsgCB(outMsg, strlen(outMsg)+1, pSUEScheContext->opCBUserData);
					if(outMsg) free(outMsg);
				}
			}
			pthread_mutex_unlock(&pSUEScheContext->dlScheInfoMutex);
			pthread_mutex_unlock(&pSUEScheContext->dpScheInfoMutex);
		}
		else retCode = SUEC_I_MSG_PARSE_FAILED;
		if(pkgType) free(pkgType);
		pkgType = NULL;
	}
	return retCode;
}

static int GetScheInfoFromCfg(PLHList dlScheInfoList, PLHList dpScheInfoList, char * cfgPath)
{
	int iRet = -1;
	if(dlScheInfoList == NULL || dpScheInfoList == NULL || cfgPath == NULL) return iRet;
	{
		FILE * fp = fopen(cfgPath, "rb");
		if(fp)
		{
			long fileLen = 0;
			fseek(fp, 0, SEEK_END);
			fileLen = ftell(fp);
			if(fileLen > 0)
			{
				long readLen = fileLen + 1, realReadLen = 0;
				char * readBuf = (char *)malloc(readLen);
				memset(readBuf, 0, readLen);
				fseek(fp, 0, SEEK_SET);
				realReadLen =  fread(readBuf, sizeof(char), readLen, fp);
				if(realReadLen > 0)
				{
					ParseScheInfoList(readBuf, dlScheInfoList, dpScheInfoList);
				}
				free(readBuf);
			}
			fclose(fp);
		}
	}
	return iRet;
}

static int FillScheInfoToCfg(PLHList dlScheInfoList, PLHList dpScheInfoList, char * cfgPath)
{
	int iRet = -1;
	if(dlScheInfoList == NULL || dpScheInfoList == NULL || cfgPath == NULL) return iRet;
	{
		char * jsonStr = NULL;
		PackScheInfoList(dlScheInfoList, dpScheInfoList, &jsonStr);
		if(jsonStr != NULL)
		{
			FILE * fp = fopen(cfgPath, "wb");
			if(fp)
			{
				fwrite(jsonStr, sizeof(char), strlen(jsonStr), fp);
				iRet = 0;
				fclose(fp);
			}
			free(jsonStr);
		}
	}
	return iRet;
}

static int StartupSchedule(void * args, void * pUserData)
{
	int iRet = -1;
	PScheduleInfo pScheInfo = (PScheduleInfo)pUserData;
	if(pScheInfo != NULL)
	{
		ZScheTimeRange zScheTR;
		memset(&zScheTR, 0, sizeof(ZScheTimeRange));
		if(ScheTimeS2T(pScheInfo, &zScheTR) == SUEC_SUCCESS)
		{
			pScheInfo->zScheHandle = ZScheCreate(&zScheTR);
			if(pScheInfo->zScheHandle)
			{
				if(ZScheRun(pScheInfo->zScheHandle, SUEScheEnterNotify, SUEZScheLeaveNotify, SUEZScheTask, pScheInfo) == 0)
				{
					iRet = 0;
				}
			}
		}
	}
	return iRet;
}
