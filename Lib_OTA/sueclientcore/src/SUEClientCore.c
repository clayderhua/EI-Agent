#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>

#include "util_path.h"
#include "util_power.h"
#include "util_os.h"

#define LOG_TAG "OTA"
#include "Log.h"
#include "cp_fun.h"

#include "SUEClientCore.h"
#include "InternalData.h"
#include "FileTransferLib.h"
#include "md5.h"
#include "base64.h"
#include "des.h"
#include "MiniUnzipLib.h"
#include "PkgParams.h"
#include "iniparser.h"
#include "SUECDBAccess.h"
#include "PackageLogParser.h"

#ifdef WIN32
#include "wrapper.h"
#endif

#define MSI_EXE_APP                    "C:\\Windows\\System32\\msiexec.exe"
#define DEF_ADVC_INSTALLER_ARGS        " /quiet /qn /norestart "
#define DEF_INNO_INSTALLER_ARGS        " /verysilent /norestart"
#define DEF_DP_LOG_FILE_NAME           "OTAPackageLog.log"
#define DEF_SELF_PKG_TYPE              "OTAClient"

#define FREE_STRING(str) do {\
	if (str) { free(str); str = NULL; } \
} while (0)

static CoreContext  CurCoreContext; // = CORE_CONTEXT_INITIALIZE;
static PLHList pDpFrontList = NULL;

static int InitCfgParams(PCfgParams pCfgParams);
static int GetParamsFormCfg(PCoreContext pCoreContext);
static void InitSysInfo(PCoreContext pCoreContext);
static void CleanCoreContext(PCoreContext pCoreContext);
static int CheckSysStartupDP();
static int CheckSysStartup(PCoreContext pCoreContext);

extern PPkgProcContext CreatePkgProcContext();
static void DestroyPkgProcContext(PPkgProcContext pPkgProcContext);

static int SavePkgProcScene(PPkgProcContext pPkgProcContext, int resumeType);
static int ResumePkgProcScene(PCoreContext pCoreContext, int resumeType);
static void ReviseProcContext(PPkgProcContext pPkgProcContext);
static PPkgProcContext FindPkgRBScene();

static int TaskProcStart(PCoreContext pCoreContext);
static int TaskProcStop(PCoreContext pCoreContext);
static void * PkgProcThreadStart(void *args);
static void PkgDLAndDPProc(PPkgProcContext pPkgProcContext);
static void PkgDLProc(PPkgProcContext pPkgProcContext);
static int ProcDLStartStatus(PPkgProcContext pPkgProcContext);
static int ProcResumeDLDoing(PPkgProcContext pPkgProcContext);
static int ProcDLFinishedStatus(PPkgProcContext pPkgProcContext);
static int StartPkgDLThread(PPkgProcContext pPkgProcContext);
static void * DLThreadStart(void *args);
static int StopPkgDLThread(PPkgProcContext pPkgProcContext);
static void PkgDPProc(PPkgProcContext pPkgProcContext);
static int ProcDPStartStatus(PPkgProcContext pPkgProcContext);
static int ProcResumeDPDoing(PPkgProcContext pPkgProcContext);
static int ProcDPFinishedStatus(PPkgProcContext pPkgProcContext);
static int StartPkgDPThread(PPkgProcContext pPkgProcContext);
static void * DPThreadStart(void *args);
static int StopPkgDPThread(PPkgProcContext pPkgProcContext);
static int StopPkgThread(void * args, void * pUserData);

static int PkgStepByStep(PPkgProcContext pPkgProcContext);
static void PkgStepAction(PPkgProcContext pCurOprtPkgContext, int * runningFlag);
static PkgStep PkgGetNextStep(TaskType curTT, TaskAction curTA, PPkgProcContext pPkgProcContext);
static int GetCurPkgFLPath(PPkgProcContext pPkgProcContext);
static int PkgFTDownload(PPkgProcContext pPkgProcContext, int * runningFlag);
static int PkgMD5Check(char * pkgPath, int * isQuit);
static int PkgTagsCheck(PPkgProcContext pPkgProcContext);
static int GetCurPkgSLPath(PPkgProcContext pPkgProcContext);
static int GetCurPkgSLPath2(PPkgProcContext pPkgProcContext);
static int PkgUnzipSL(PPkgProcContext pPkgProcContext);
static int PkgUnzipSL2(PPkgProcContext pPkgProcContext);
static int GetCurPkgTLPath(PPkgProcContext pPkgProcContext);
static int PkgRunDeployFile(PPkgProcContext pPkgProcContext, int * runningFlag);
static void PkgRebootSys(PPkgProcContext pPkgProcContext);
static int PkgCheckDPRet(PPkgProcContext pPkgProcContext, int * runningFlag);
static void PkgRecordSWInfo(PPkgProcContext pPkgProcContext, int * runningFlag);
static int PkgDPRollback(PPkgProcContext pPkgProcContext, int * runningFlag);

static CP_PID_T SUECLaunchExeFile(PPkgProcContext pPkgProcContext, char * curDir);
static int RunRetCheckScript(char * retCheckScriptPath, int * isRunning, char * curDir);
static int StartDPCheckThread(PPkgProcContext pPkgProcContext, char * curDir, PPkgDpCheckCBInfo * ppTmpCBInfo);
//static void StopDPCheckThread(PPkgProcContext pPkgProcContext);
static void * DPCheckThreadStart(void *args);
static int NotifyDPCheckMsg(const void * const checkHandle, char * msg, unsigned int msgLen);
static void UpgSWVersionInterceptor(char * msg, char * pkgType);
static int CheckIsDPSuccessMsg(const char * str, const char * subStr);
static void ParseSWInfoFromDPRetMsg(char * dpRetMsg, PSWInfo * ppSWInfo);

static int DES_BASE64Decode(char * srcBuf, char *destBuf, int destLen);
static int DES_BASE64Encode(char * srcBuf, char *destBuf, int destLen);
static int GetFileMD5(char * filePath, char * retMD5Str, int * runFlag);
static int FileSuffixMatch(char * fileName, char * suffix);
static void FTTransCB(void *userData, int64_t totalSize, int64_t nowSize, int64_t averageSpeed);
static int ClearPkgPath(PPkgProcContext pPkgProcContext);
static void GetCurDirFromDPFile(char * dpFilePath, char ** ppCurDir);
static int FindPkgBKFileCB(unsigned int depth, const char * filePath, void * userData);
static void NotifyTaskStatus(PPkgProcContext pPkgProcContext);

static void LHFreeSWInfoCB(void * pUserData);
static int MatchPkgProcContextN(void * pUserData, void * key);
static int MatchPkgProcContextT(void * pUserData, void * key);
static int MatchPkgDpCheckCBInfo(void * pUserData, void * key);
static void FreePkgDpCheckCBInfo(void * pUserData);
static void FreePkgProcContext(void * pUserData);
static void LHFreeDelPkgRetInfoCB(void * pUserData);

static void ClearDPLogFile(char * curDir);
//static void DeleteDPTask(PDPTaskInfo pDPTaskInfo);
static int CheckPkgRootPathObtainDPTask(unsigned int depth, const char * filePath, void * userData);
//static int DefDeployCheckCB(const void * const checkHandle, NotifyDPCheckMsgCB notifyDpMsgCB, int * isQuit, void * userData);
extern char * SuecCoreGetErrorMsg(unsigned int errorCode);

static int WriteSubDevicesArgs(char ** subDevices, int subDeviceSize, char * pkgName, char *filePath);


//---------------------------interface implement S--------------------------
int SUECCoreInit(char * cfgFile, char* tags)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext;

    LOGI("SUECCore init...");
	memset(&CurCoreContext, 0, sizeof(CurCoreContext));
	pCoreContext = &CurCoreContext;

    if (!pCoreContext->isInited)
    {
        int len = 0;
        pCoreContext->isInited = 1;
        if (NULL != cfgFile) {
            len = strlen(cfgFile) + 1;
            pCoreContext->cfgFile = (char *)malloc(len);
            strcpy(pCoreContext->cfgFile, cfgFile);
        }
        InitCfgParams(&pCoreContext->cfgParams);
        if (pCoreContext->cfgFile) {
            retCode = GetParamsFormCfg(pCoreContext);
            if (retCode != SUEC_CORE_SUCCESS)
				goto done;
        }
        if (!util_is_file_exist(pCoreContext->cfgParams.pkgRootPath))
			util_create_directory(pCoreContext->cfgParams.pkgRootPath);
		pCoreContext->tags = tags;
        InitSysInfo(pCoreContext);

        pCoreContext->pkgDpCheckCBList = LHInitList(FreePkgDpCheckCBInfo);
        pthread_mutex_init(&pCoreContext->endecodeMutex, NULL);

        pthread_mutex_init(&pCoreContext->pkgProcContextListMutex, NULL);
        pCoreContext->pkgProcContextList = LHInitList(FreePkgProcContext);

        if (FTGlobalInit() != 0) {
            retCode = SUEC_CORE_I_FT_INIT_FAILED;
            goto done;
        }
        SUECInitDB(pCoreContext->cfgParams.envFileDir);
		// compatible to old OTA database
		AddColumn2PkgProcScene("PkgownerId", "Integer");

    done:
        if (retCode != SUEC_CORE_SUCCESS) {
            pCoreContext->isInited = 0;
            CleanCoreContext(pCoreContext);
        }
    }
    else
    {
        retCode = SUEC_CORE_I_ALREADY_INIT;
    }
    if (retCode == SUEC_CORE_SUCCESS)
		LOGI("SUECCore init success!");
    else
		LOGE("SUECCore init failed!ErrCode:%d, ErrMsg:%s", retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreUninit()
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    LOGI("SUECCore uninit...");
    if (pCoreContext->isInited)
    {
        CleanCoreContext(pCoreContext);
        pCoreContext->isInited = 0;
    }
    LOGI("SUECCore uninit success!");
    SUECUninitDB();
    return retCode;
}

int SUECCoreStart(void * rsvParam)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    LOGI("SUECCore startup...");
    if (pCoreContext->isInited)
    {
        if (!pCoreContext->isStarted)
        {
            //dhl add
            if (CheckSysStartupDP() && CheckSysStartup(pCoreContext))
				ResumePkgProcScene(pCoreContext, DEF_SYS_STARTUP_RT);
            ResumePkgProcScene(pCoreContext, DEF_COMMON_RT);
            retCode = TaskProcStart(pCoreContext);
            if (retCode == SUEC_CORE_SUCCESS)
            {
                pCoreContext->isStarted = 1;
            }
        }
        else
			retCode = SUEC_CORE_I_ALREADY_START;
    }
    else
		retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode == SUEC_CORE_SUCCESS)
		LOGI("SUECCore startup success!");
    else
		LOGE("SUECCore startup failed!ErrCode:%d, ErrMsg:%s", retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreStop()
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    LOGI("SUECCore Stop...");
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            TaskProcStop(pCoreContext);
            pCoreContext->isStarted = 0;
        }
        else retCode = SUEC_CORE_I_NOT_START;
    }
    else retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode == SUEC_CORE_SUCCESS) LOGI("SUECCore Stop success!");
    else LOGE("SUECCore Stop failed!ErrCode:%d, ErrMsg:%s", retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreSetTaskStatusCB(TaskStatusCB taskStatusCB, void * userData)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = (PCoreContext)&CurCoreContext;
    if (NULL == taskStatusCB)return SUEC_CORE_I_PARAMETER_ERROR;
    if (pCoreContext->isInited)
    {
        if (!pCoreContext->isStarted)
        {
            pCoreContext->taskStatusCB = taskStatusCB;
            pCoreContext->taskUserData = userData;
        }
        else retCode = SUEC_CORE_I_ALREADY_START;
    }
    else retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS) LOGE("SUECCore set task status callback failed!ErrCode:%d, ErrMsg:%s",
        retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreGetTaskStatus(char* pkgName, PTaskStatusInfo * ppTaskStatusInfo)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pkgName == NULL || ppTaskStatusInfo == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            PLNode pLNode = NULL;
            pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
            pLNode = LHFindNode(pCoreContext->pkgProcContextList, MatchPkgProcContextN, pkgName);
            if (pLNode)
            {
                PPkgProcContext pPkgProcContext = (PPkgProcContext)pLNode->pUserData;
                pthread_mutex_lock(&pPkgProcContext->dataMutex);
                if (pPkgProcContext->taskStatusInfo.taskType != TASK_UNKNOW &&
                    pPkgProcContext->taskStatusInfo.statusCode != SC_UNKNOW)
                {
                    int len = 0;
                    PTaskStatusInfo pTaskStatusInfo = NULL;
                    len = sizeof(TaskStatusInfo);
                    pTaskStatusInfo = (PTaskStatusInfo)malloc(len);
                    memset(pTaskStatusInfo, 0, len);
                    len = strlen(pkgName) + 1;
                    pTaskStatusInfo->pkgName = (char *)malloc(len);
                    memset(pTaskStatusInfo->pkgName, 0, len);
                    strcpy(pTaskStatusInfo->pkgName, pkgName);
                    pTaskStatusInfo->taskType = pPkgProcContext->taskStatusInfo.taskType;
                    pTaskStatusInfo->statusCode = pPkgProcContext->taskStatusInfo.statusCode;
                    pTaskStatusInfo->errCode = pPkgProcContext->taskStatusInfo.errCode;
                    if (pTaskStatusInfo->taskType == TASK_DL && pTaskStatusInfo->statusCode == SC_DOING)
                    {
                        pTaskStatusInfo->u.dlPercent = pPkgProcContext->taskStatusInfo.u.dlPercent;
                    }
                    else
                    {
                        if (pPkgProcContext->taskStatusInfo.u.msg)
                        {
                            len = strlen(pPkgProcContext->taskStatusInfo.u.msg) + 1;
                            pTaskStatusInfo->u.msg = (char *)malloc(len);
                            memset(pTaskStatusInfo->u.msg, 0, len);
                            strcpy(pTaskStatusInfo->u.msg, pPkgProcContext->taskStatusInfo.u.msg);
                        }
                    }
                    *ppTaskStatusInfo = pTaskStatusInfo;
                }
                else retCode = SUEC_CORE_I_PKG_TASK_STATUS_UNKNOW;
                pthread_mutex_unlock(&pPkgProcContext->dataMutex);
            }
            else retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
            pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
        }
        else retCode = SUEC_CORE_I_NOT_START;
    }
    else retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS) LOGE("SUECCore get task status failed!ErrCode:%d, ErrMsg:%s",
        retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreSetDpCheckCB(char* packageType, DeployCheckCB dpCheckCB, void * userData)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (NULL == packageType || dpCheckCB == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    if (pCoreContext->isInited && !pCoreContext->isStarted)
    {
        PLNode findNode = LHFindNode(pCoreContext->pkgDpCheckCBList, MatchPkgDpCheckCBInfo, packageType);
        if (findNode)
        {
            PPkgDpCheckCBInfo pPkgDpCheckCBInfo = (PPkgDpCheckCBInfo)findNode->pUserData;
            pPkgDpCheckCBInfo->dpCheckCB = dpCheckCB;
            pPkgDpCheckCBInfo->dpCheckUserData = userData;
        }
        else
        {
            PPkgDpCheckCBInfo pPkgDpCheckCBInfo = (PPkgDpCheckCBInfo)malloc(sizeof(PkgDpCheckCBInfo));
            pPkgDpCheckCBInfo->dpCheckCB = dpCheckCB;
            pPkgDpCheckCBInfo->dpCheckUserData = userData;
            {
                int len = strlen(packageType) + 1;
                pPkgDpCheckCBInfo->pkgType = (char *)malloc(len);
                memset(pPkgDpCheckCBInfo->pkgType, 0, len);
                strcpy(pPkgDpCheckCBInfo->pkgType, packageType);

            }
            LHAddNode(pCoreContext->pkgDpCheckCBList, (void *)pPkgDpCheckCBInfo);
        }
    }
    else
    {
        if (!pCoreContext->isInited) retCode = SUEC_CORE_I_NOT_INIT;
        if (pCoreContext->isStarted) retCode = SUEC_CORE_I_ALREADY_START;
    }
    if (retCode != SUEC_CORE_SUCCESS) LOGE("SUECCore set deploy check callback failed!ErrCode:%d, ErrMsg:%s",
        retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreAddDLTask(PDLTaskInfo pDLTaskInfo)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;

    if (pDLTaskInfo == NULL ||
		pDLTaskInfo->pkgName == NULL ||
		pDLTaskInfo->pkgType == NULL ||
		pDLTaskInfo->url == NULL)
	{
		return SUEC_CORE_I_PARAMETER_ERROR;
	}

    if (pCoreContext->isInited)
    {
        PLNode findNode = NULL;
        PPkgProcContext pPkgProcContext = NULL;
        pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
        findNode = LHFindNode(pCoreContext->pkgProcContextList, MatchPkgProcContextN, pDLTaskInfo->pkgName);
        if (findNode == NULL)
        {
            TaskType curTaskType = TASK_UNKNOW;
            TaskStatusCode curTaskSC = SC_QUEUE;
            PkgStep curPkgStep = STEP_INIT;
            char * pkgPath = NULL;
            int len = 0;

            len = strlen(pCoreContext->cfgParams.pkgRootPath) + strlen(pDLTaskInfo->pkgType) + strlen(pDLTaskInfo->pkgName) + 3;
            pkgPath = (char *)malloc(len);
            memset(pkgPath, 0, len);
            sprintf(pkgPath, "%s%c%s%c%s", pCoreContext->cfgParams.pkgRootPath, FILE_SEPARATOR, pDLTaskInfo->pkgType,
                FILE_SEPARATOR, pDLTaskInfo->pkgName);
            if (util_is_file_exist(pkgPath))
            {
                retCode = SUEC_CORE_I_PKG_FILE_EXIST;
                if (pDLTaskInfo->isDeploy)
					curTaskType = TASK_DP;
            }
            else
				curTaskType = TASK_DL;

            if (curTaskType != TASK_UNKNOW)
            {
                long long order = (long long)time((time_t *)NULL);
                pPkgProcContext = CreatePkgProcContext();
                len = strlen(pDLTaskInfo->pkgName) + 1;
                pPkgProcContext->pkgName = (char *)malloc(len);
                strcpy(pPkgProcContext->pkgName, pDLTaskInfo->pkgName);
                len = strlen(pDLTaskInfo->pkgType) + 1;
                pPkgProcContext->pkgType = (char *)malloc(len);
                strcpy(pPkgProcContext->pkgType, pDLTaskInfo->pkgType);
                len = strlen(pDLTaskInfo->url) + 1;
                pPkgProcContext->pkgURL = (char *)malloc(len);
                strcpy(pPkgProcContext->pkgURL, pDLTaskInfo->url);

				/*add subDevice binlin.duan*/
				if (pDLTaskInfo->subDevices != NULL)
				{
					pPkgProcContext->subDeviceSize = pDLTaskInfo->subDeviceSize;
					pPkgProcContext->subDevices = (char **)malloc(pPkgProcContext->subDeviceSize*sizeof(char *));
					int i = 0;
					for (i = 0; i < pPkgProcContext->subDeviceSize; i++)
					{
						len = strlen(pDLTaskInfo->subDevices[i]) + 1;
						pPkgProcContext->subDevices[i] = (char *)malloc(len);
						memset(pPkgProcContext->subDevices[i], 0, len);
						strcpy(pPkgProcContext->subDevices[i], pDLTaskInfo->subDevices[i]);
					}
				}
				//
                pPkgProcContext->dlProtocal = pDLTaskInfo->protocal;
                pPkgProcContext->dlSercurity = pDLTaskInfo->sercurity;
                pPkgProcContext->thenDP = pDLTaskInfo->isDeploy;
                if (pPkgProcContext->thenDP)
                {
                    pPkgProcContext->dpRetryCnt = pDLTaskInfo->dpRetry;
                    pPkgProcContext->isRB = pDLTaskInfo->isRollBack;
                }
                pPkgProcContext->dlRetryCnt = pDLTaskInfo->dlRetry;
                pPkgProcContext->taskStatusInfo.taskAction = TA_NORMAL;
                pPkgProcContext->order = order;
                pPkgProcContext->taskStatusInfo.taskType = curTaskType;
                pPkgProcContext->taskStatusInfo.statusCode = curTaskSC;
				pPkgProcContext->taskStatusInfo.pkgOwnerId = pDLTaskInfo->pkgOwnerId;
                //Task status info:pkgName=NULL,u.dlPercent=0,u.msg=NULL,errCode=0;
                pPkgProcContext->pkgStep = curPkgStep;
				LHAddNode(pCoreContext->pkgProcContextList, pPkgProcContext);
                LOGI("Package %s download queue!", pPkgProcContext->pkgName);
                NotifyTaskStatus(pPkgProcContext);
            }
            free(pkgPath);
            pkgPath = NULL;
        }
        else
        {
            pPkgProcContext = (PPkgProcContext)findNode->pUserData;
            pthread_mutex_lock(&pPkgProcContext->dataMutex);
            //pPkgProcContext->dlRetryCnt = pDLTaskInfo->dlRetry;
            if (pPkgProcContext->taskStatusInfo.taskType == TASK_DL)
            {
                if (pPkgProcContext->taskStatusInfo.statusCode == SC_SUSPEND)
                {
                    pPkgProcContext->taskStatusInfo.statusCode = pPkgProcContext->suspendSCCode;
                    LOGI("Package %s download resume,status:%d!",
                        pPkgProcContext->pkgName, pPkgProcContext->taskStatusInfo.statusCode);
                    //Task status info:before suspend info
                    NotifyTaskStatus(pPkgProcContext);
                }
                else if (pPkgProcContext->taskStatusInfo.statusCode < SC_FINISHED)
					retCode = SUEC_CORE_I_DLING;
                else
					retCode = SUEC_CORE_I_PKG_FILE_EXIST;
            }
            else if (pPkgProcContext->taskStatusInfo.taskType == TASK_DP) {
                if (pDLTaskInfo->isDeploy && pPkgProcContext->taskStatusInfo.statusCode == SC_SUSPEND)
                {
                    pPkgProcContext->taskStatusInfo.statusCode = pPkgProcContext->suspendSCCode;
                    LOGI("Package %s deploy resume,status:%d!",
                        pPkgProcContext->pkgName, pPkgProcContext->taskStatusInfo.statusCode);
                    NotifyTaskStatus(pPkgProcContext);
                }
                retCode = SUEC_CORE_I_PKG_FILE_EXIST;
            }
            if (pDLTaskInfo->isDeploy && !pPkgProcContext->thenDP)
            {
                pPkgProcContext->dpRetryCnt = pDLTaskInfo->dpRetry;
                pPkgProcContext->isRB = pDLTaskInfo->isRollBack;
                pPkgProcContext->thenDP = pDLTaskInfo->isDeploy;
                retCode = SUEC_CORE_SUCCESS;
                if (pPkgProcContext->taskStatusInfo.taskType == TASK_DL && pPkgProcContext->taskStatusInfo.statusCode == SC_FINISHED)
                {
                    //retCode = SUEC_CORE_I_PKG_FILE_EXIST;
                    pPkgProcContext->taskStatusInfo.taskType = TASK_DP;
                    pPkgProcContext->taskStatusInfo.statusCode = SC_QUEUE;
                    pPkgProcContext->taskStatusInfo.u.msg = NULL;
                    //Task status info:pkgName=[NULL],u.dlPercent=0,u.msg=NULL,errCode=0;
                    NotifyTaskStatus(pPkgProcContext);
                    pPkgProcContext->pkgStep = STEP_CHECK_DPF;
                }
            }
            pthread_mutex_unlock(&pPkgProcContext->dataMutex);
        }
        pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
    }
    else
		retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS)
		LOGE("SUECCore add download task failed!ErrCode:%d, ErrMsg:%s", retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreSuspendDLTask(char* pkgType, char * pkgName)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            PPkgProcContext pPkgProcContext = NULL;
            PPkgProcContext pSuspendContext = NULL;
            PLNode curNode = NULL;
            int isThenBreak = 0;
            retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
            pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
            curNode = pCoreContext->pkgProcContextList->head;
            while (curNode)
            {
                pSuspendContext = NULL;
                pPkgProcContext = (PPkgProcContext)curNode->pUserData;
                pthread_mutex_lock(&pPkgProcContext->dataMutex);
                if (pkgName != NULL)
                {
                    if (!strcasecmp(pPkgProcContext->pkgName, pkgName))
                    {
                        pSuspendContext = pPkgProcContext;
                        isThenBreak = 1;
                    }
                }
                else if (pkgType != NULL)
                {
                    if (!strcasecmp(pPkgProcContext->pkgType, pkgType)) pSuspendContext = pPkgProcContext;
                }
                else pSuspendContext = pPkgProcContext;

                if (pSuspendContext)
                {
                    retCode = SUEC_CORE_SUCCESS;
                    if (pSuspendContext->taskStatusInfo.taskType == TASK_DL && pSuspendContext->taskStatusInfo.statusCode <= SC_DOING)
                    {
                        if (StopPkgDLThread(pSuspendContext) == SUEC_CORE_SUCCESS)
                        {
                            pSuspendContext->suspendSCCode = pSuspendContext->taskStatusInfo.statusCode;
                            pSuspendContext->taskStatusInfo.statusCode = SC_SUSPEND;
                            pSuspendContext->taskStatusInfo.errCode = SUEC_CORE_SUCCESS;
                            //pSuspendContext->taskStatusInfo.u.msg = NULL;
                            LOGI("Package %s download suspend!", pSuspendContext->pkgName);
                            NotifyTaskStatus(pSuspendContext);
                        }
                    }
                }
                pthread_mutex_unlock(&pPkgProcContext->dataMutex);
                if (isThenBreak) break;
                curNode = curNode->next;
            }
            pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
        }
        else retCode = SUEC_CORE_I_NOT_START;
    }
    else retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS) LOGE("SUECCore suspend download task failed!ErrCode:%d, ErrMsg:%s",
        retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreResumeDLTask(char* pkgType, char * pkgName)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            PPkgProcContext pPkgProcContext = NULL;
            PPkgProcContext pResumeContext = NULL;
            PLNode curNode = NULL;
            int isThenBreak = 0;
            retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
            pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
            curNode = pCoreContext->pkgProcContextList->head;
            while (curNode)
            {
                pResumeContext = NULL;
                pPkgProcContext = (PPkgProcContext)curNode->pUserData;
                pthread_mutex_lock(&pPkgProcContext->dataMutex);
                if (pkgName != NULL)
                {
                    if (!strcasecmp(pPkgProcContext->pkgName, pkgName))
                    {
                        pResumeContext = pPkgProcContext;
                        isThenBreak = 1;
                    }
                }
                else if (pkgType != NULL)
                {
                    if (!strcasecmp(pPkgProcContext->pkgType, pkgType)) pResumeContext = pPkgProcContext;
                }
                else pResumeContext = pPkgProcContext;

                if (pResumeContext)
                {
                    retCode = SUEC_CORE_SUCCESS;
                    if (pResumeContext->taskStatusInfo.taskType == TASK_DL &&
                        pResumeContext->taskStatusInfo.statusCode == SC_SUSPEND)
                    {
                        pResumeContext->taskStatusInfo.statusCode = pResumeContext->suspendSCCode;
                        LOGI("Package %s download resume,status:%d!",
                            pResumeContext->pkgName, pResumeContext->taskStatusInfo.statusCode);
                        NotifyTaskStatus(pResumeContext);
                        if (pResumeContext->taskStatusInfo.statusCode == SC_DOING)
                        {
                            StartPkgDLThread(pResumeContext);
                        }
                    }
                }
                pthread_mutex_unlock(&pPkgProcContext->dataMutex);
                if (isThenBreak) break;
                curNode = curNode->next;
            }
            pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
        }
        else retCode = SUEC_CORE_I_NOT_START;
    }
    else
		retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS)
		LOGE("SUECCore resume download task failed!ErrCode:%d, ErrMsg:%s", retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreDelDLTask(char* pkgType, char * pkgName)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
            if (pkgName == NULL && pkgType == NULL)
            {
                LHDelAllNode(pCoreContext->pkgProcContextList);
            }
            else if (pkgName != NULL)
            {
                LHDelNode(pCoreContext->pkgProcContextList, MatchPkgProcContextN, pkgName);
            }
            else
            {
                LHDelNode(pCoreContext->pkgProcContextList, MatchPkgProcContextT, pkgType);
            }
            pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
        }
        else retCode = SUEC_CORE_I_NOT_START;
    }
    else
		retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS)
		LOGE("SUECCore delete (pkgType:%s, pkgName:%s)download task failed!ErrCode:%d, ErrMsg:%s", pkgType, pkgName, retCode, SuecCoreGetErrorMsg(retCode));
    else
		LOGI("SUECCore delete (pkgType:%s, pkgName:%s)download task success!", pkgType, pkgName);
    return retCode;
}
//Wei.Gang add
void DeleteDPTask(PDPTaskInfo pDPTaskInfo)
{
	PPkgProcContext pPkgProcContext = NULL;
	PLNode findNode = NULL;
	PCoreContext pCoreContext = &CurCoreContext;

	if (pDPTaskInfo->pkgName)
	{
		pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
		findNode = LHFindNode(pCoreContext->pkgProcContextList, MatchPkgProcContextN, pDPTaskInfo->pkgName);
		if (findNode) //Related dp task object exist in context list
		{
			pPkgProcContext = (PPkgProcContext)findNode->pUserData;
			pthread_mutex_lock(&pPkgProcContext->dataMutex);
			if (pPkgProcContext->thenDP == 0) //if thenDP flag is 0, the set 1
			{
				pPkgProcContext->thenDP = 1;
				if (pPkgProcContext->dpRetryCnt == 0) pPkgProcContext->dpRetryCnt = pDPTaskInfo->dpRetry;
				if (pPkgProcContext->isRB == 0) pPkgProcContext->isRB = pDPTaskInfo->isRollBack;
				LOGI("Package %s deploy task add!", pPkgProcContext->pkgName);
			}
			if (pPkgProcContext->taskStatusInfo.taskType == TASK_DL && pPkgProcContext->taskStatusInfo.statusCode == SC_FINISHED)
			{ //if current pkg download finished the change status to deploy queue;
				pPkgProcContext->taskStatusInfo.u.msg = NULL;
				pPkgProcContext->pkgStep = STEP_CHECK_DPF;
			}
			pthread_mutex_unlock(&pPkgProcContext->dataMutex);
		}
		pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
	}
}

int SUECCoreCheckPkgExist(PDPTaskInfo pDPTaskInfo){
#define DEF_FILE_WILDCARD   "*.zip"
	int retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
	PCoreContext pCoreContext = &CurCoreContext;
	if (pCoreContext->isInited)
	{
		int iRet = 0;
		char * itRootPath = NULL;
		if (pDPTaskInfo->pkgType)
		{
			int len = strlen(pCoreContext->cfgParams.pkgRootPath) + strlen(pDPTaskInfo->pkgType) + 2;
			itRootPath = (char *)malloc(len);
			memset(itRootPath, 0, len);
			sprintf(itRootPath, "%s%c%s", pCoreContext->cfgParams.pkgRootPath, FILE_SEPARATOR, pDPTaskInfo->pkgType);
			if (util_is_file_exist(itRootPath))
			{
				iRet = util_file_iterate(itRootPath, DEF_FILE_WILDCARD, 1, 1,
					CheckPkgRootPathObtainDPTask, pDPTaskInfo);
			}
			//Wei.Gang add
			if (iRet > 0)
			{
				retCode = SUEC_CORE_SUCCESS;
			}
		}
	}
	return retCode;
}
//Wei.Gang add end
int SUECCoreAddDPTask(PDPTaskInfo pDPTaskInfo)
{
#define DEF_FILE_WILDCARD   "*.zip"
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pDPTaskInfo == NULL)
		return SUEC_CORE_I_PARAMETER_ERROR;
    if (pCoreContext->isInited)
    {
        int iRet = 0;
        char * itRootPath = NULL;
        retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
        if (pDPTaskInfo->pkgType)
        {
            int len = strlen(pCoreContext->cfgParams.pkgRootPath) + strlen(pDPTaskInfo->pkgType) + 2;
            itRootPath = (char *)malloc(len);
            sprintf(itRootPath, "%s%c%s", pCoreContext->cfgParams.pkgRootPath, FILE_SEPARATOR, pDPTaskInfo->pkgType);
            if (util_is_file_exist(itRootPath))
            {
                iRet = util_file_iterate(itRootPath, DEF_FILE_WILDCARD, 1, 1,
                    CheckPkgRootPathObtainDPTask, pDPTaskInfo);
            }
			//Wei.Gang add
			if (iRet == 0){
				DeleteDPTask(pDPTaskInfo);
				iRet = 1;
			}
			//Wei.Gang add end
            free(itRootPath);
        }
        else
        {
			// if has no pkgType, then set 2 depth for search all pkgType
            itRootPath = pCoreContext->cfgParams.pkgRootPath;
            iRet = util_file_iterate(pCoreContext->cfgParams.pkgRootPath, DEF_FILE_WILDCARD, 2, 1,
                CheckPkgRootPathObtainDPTask, pDPTaskInfo);
        }
        if (iRet > 0) retCode = SUEC_CORE_SUCCESS;
    }
    else
		retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS)
		LOGE("SUECCore add deploy task failed!ErrCode:%d, ErrMsg:%s", retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreSuspendDPTask(char* pkgType, char * pkgName)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            PPkgProcContext pPkgProcContext = NULL;
            PPkgProcContext pSuspendContext = NULL;
            PLNode curNode = NULL;
            int isThenBreak = 0;
            retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
            pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
            curNode = pCoreContext->pkgProcContextList->head;
            while (curNode)
            {
                pSuspendContext = NULL;
                pPkgProcContext = (PPkgProcContext)curNode->pUserData;
                pthread_mutex_lock(&pPkgProcContext->dataMutex);
                if (pkgName != NULL)
                {
                    if (!strcasecmp(pPkgProcContext->pkgName, pkgName))
                    {
                        pSuspendContext = pPkgProcContext;
                        isThenBreak = 1;
                    }
                }
                else if (pkgType != NULL)
                {
                    if (!strcasecmp(pPkgProcContext->pkgType, pkgType)) pSuspendContext = pPkgProcContext;
                }
                else pSuspendContext = pPkgProcContext;
                if (pSuspendContext)
                {
                    retCode = SUEC_CORE_SUCCESS;
                    if (pSuspendContext->taskStatusInfo.taskType == TASK_DP && pSuspendContext->taskStatusInfo.statusCode == SC_QUEUE)
                    {
                        pSuspendContext->suspendSCCode = pSuspendContext->taskStatusInfo.statusCode;
                        pSuspendContext->taskStatusInfo.statusCode = SC_SUSPEND;
                        LOGI("Package %s deploy suspend!", pSuspendContext->pkgName);
                        NotifyTaskStatus(pSuspendContext);
                    }
                }
                pthread_mutex_unlock(&pPkgProcContext->dataMutex);
                if (isThenBreak) break;
                curNode = curNode->next;
            }
            pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
        }
        else retCode = SUEC_CORE_I_NOT_START;
    }
    else retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS) LOGE("SUECCore suspend deploy task failed!ErrCode:%d, ErrMsg:%s",
        retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreResumeDPTask(char* pkgType, char * pkgName)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            PPkgProcContext pPkgProcContext = NULL;
            PPkgProcContext pResumeContext = NULL;
            PLNode curNode = NULL;
            int isThenBreak = 0;
            retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
            pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
            curNode = pCoreContext->pkgProcContextList->head;
            while (curNode)
            {
                pResumeContext = NULL;
                pPkgProcContext = (PPkgProcContext)curNode->pUserData;
                pthread_mutex_lock(&pPkgProcContext->dataMutex);
                if (pkgName != NULL)
                {
                    if (!strcasecmp(pPkgProcContext->pkgName, pkgName))
                    {
                        pResumeContext = pPkgProcContext;
                        isThenBreak = 1;
                    }
                }
                else if (pkgType != NULL)
                {
                    if (!strcasecmp(pPkgProcContext->pkgType, pkgType)) pResumeContext = pPkgProcContext;
                }
                else pResumeContext = pPkgProcContext;

                if (pResumeContext)
                {
                    retCode = SUEC_CORE_SUCCESS;
                    if (pResumeContext->taskStatusInfo.taskType == TASK_DP &&
                        pResumeContext->taskStatusInfo.statusCode == SC_SUSPEND)
                    {
                        pResumeContext->taskStatusInfo.statusCode = pResumeContext->suspendSCCode;
                        LOGI("Package %s deploy resume,status:%d!",
                            pResumeContext->pkgName, pResumeContext->taskStatusInfo.statusCode);
                        NotifyTaskStatus(pResumeContext);
                    }
                }
                pthread_mutex_unlock(&pPkgProcContext->dataMutex);
                if (isThenBreak) break;
                curNode = curNode->next;
            }
            pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
        }
        else
			retCode = SUEC_CORE_I_NOT_START;
    }
    else
		retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS)
		LOGE("SUECCore resume deploy task failed!ErrCode:%d, ErrMsg:%s", retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

int SUECCoreDelDPTask(char* pkgType, char * pkgName)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pkgName == NULL && pkgType == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            PPkgProcContext pPkgProcContext = NULL;
            PPkgProcContext pStopPkgProcContext = NULL;
            PLNode curNode = pCoreContext->pkgProcContextList->head;
            retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
            while (curNode)
            {
                pStopPkgProcContext = NULL;
                pPkgProcContext = (PPkgProcContext)curNode->pUserData;
                if (pkgName != NULL)
                {
                    if (!strcasecmp(pPkgProcContext->pkgName, pkgName)) pStopPkgProcContext = pPkgProcContext;
                }
                else
                {
                    if (!strcasecmp(pPkgProcContext->pkgType, pkgType)) pStopPkgProcContext = pPkgProcContext;
                }
                if (pStopPkgProcContext)
                {
                    if (pPkgProcContext->taskStatusInfo.taskType == TASK_DP && pPkgProcContext->taskStatusInfo.statusCode == SC_QUEUE)
                    {
                        pPkgProcContext->taskStatusInfo.taskType = TASK_DL;
                        pPkgProcContext->taskStatusInfo.statusCode = SC_FINISHED;
                        pPkgProcContext->thenDP = 0;
                        retCode = SUEC_CORE_SUCCESS;
                    }
                    else retCode = SUEC_CORE_I_PKG_STATUS_NOT_FIT;
                }
                curNode = curNode->next;
            }
        }
        else retCode = SUEC_CORE_I_NOT_START;
    }
    else retCode = SUEC_CORE_I_NOT_INIT;
    if (retCode != SUEC_CORE_SUCCESS) LOGE("SUECCore delete (pkgType:%s, pkgName:%s)deploy task failed!ErrCode:%d, ErrMsg:%s",
        pkgType, pkgName, retCode, SuecCoreGetErrorMsg(retCode));
    else LOGI("SUECCore delete (pkgType:%s, pkgName:%s)deploy task success!",
        pkgType, pkgName);
    return retCode;
}

int SUECCoreCheckTaskExist(char * pkgName, int taskType)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pkgName == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            PLNode curNode = NULL;
            pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
            curNode = LHFindNode(pCoreContext->pkgProcContextList, MatchPkgProcContextN, pkgName);
            if (curNode)
            {
                if (taskType == TASK_DP)
                {
                    PPkgProcContext pPkgProcContext = (PPkgProcContext)curNode->pUserData;
                    pthread_mutex_lock(&pPkgProcContext->dataMutex);
                    if (!pPkgProcContext->thenDP) retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
                    pthread_mutex_unlock(&pPkgProcContext->dataMutex);
                }
            }
            else retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
            pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
        }
        else retCode = SUEC_CORE_I_NOT_START;
    }
    else retCode = SUEC_CORE_I_NOT_INIT;
    //if(retCode != SUEC_CORE_SUCCESS) LOGE("SUECCore check (%s,%d)task exist failed!ErrCode:%d, ErrMsg:%s",
    //	pkgName, taskType, retCode, SuecCoreGetErrorMsg(retCode));
    return retCode;
}

void SUECCoreFreeTSInfo(PTaskStatusInfo pTaskStatusInfo)
{
    if (pTaskStatusInfo)
    {
        if (pTaskStatusInfo->pkgName)
        {
            free(pTaskStatusInfo->pkgName);
            pTaskStatusInfo->pkgName = NULL;
        }
        if (!(pTaskStatusInfo->taskType == TASK_DL && pTaskStatusInfo->statusCode == SC_DOING))
        {
            if (pTaskStatusInfo->u.msg)
            {
                free(pTaskStatusInfo->u.msg);
                pTaskStatusInfo->u.msg = NULL;
            }
        }
        free(pTaskStatusInfo);
    }
}

void SUECCoreFreeDLTaskInfo(PDLTaskInfo pDLTaskInfo)
{
    if (pDLTaskInfo)
    {
        if (pDLTaskInfo->pkgName) free(pDLTaskInfo->pkgName);
        pDLTaskInfo->pkgName = NULL;
        if (pDLTaskInfo->pkgType) free(pDLTaskInfo->pkgType);
        pDLTaskInfo->pkgType = NULL;
        if (pDLTaskInfo->url) free(pDLTaskInfo->url);
        pDLTaskInfo->url = NULL;
        free(pDLTaskInfo);
    }
}

PLHList SUECCoreGetSWInfos(char ** swNames, int cnt)
{
    PLHList pSWInfoList = NULL;
    if (cnt > 0 && swNames != NULL)
    {
        int i = 0;
        PSWInfo pSWInfo = NULL;
        for (i = 0; i < cnt; i++)
        {
            if (SelectOneSWInfo(swNames[i], &pSWInfo))
            {
                if (pSWInfoList == NULL) pSWInfoList = LHInitList(LHFreeSWInfoCB);
                LHAddNode(pSWInfoList, pSWInfo);
            }
            free(swNames[i]);
            swNames[i] = NULL;
        }
        free(swNames);
        swNames = NULL;
    }
    else
    {
        SelectAllSWInfo(&pSWInfoList);
    }
    return pSWInfoList;
}

void SUECCoreGetRecordPkgVersion(char * pkgType, char ** version)
{
    SelectOnePkgInfo(pkgType, version);
}

PLHList SUECCoreDelDLPkgs(char ** pkgNames, int cnt, long pkgOwnerId)
{
	char *fFilePath = NULL, *ptr;
	char buf[MAX_PATH];
    PLHList pDelPkgRetList = NULL;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pCoreContext->isInited)
    {
        if (pCoreContext->isStarted)
        {
            if (cnt > 0 && pkgNames != NULL)
            {
                int i = 0;
                char * pkgName = NULL;
                PLNode fNode = NULL;
                int retCode = SUEC_CORE_SUCCESS;
				int len = 0;

                pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
                for (i = 0; i < cnt; i++)
                {
                    PDelPkgRetInfo pDelRetInfo = NULL;
                    retCode = SUEC_CORE_SUCCESS;
                    fNode = NULL;
                    pkgName = pkgNames[i];
                    fNode = LHFindNode(pCoreContext->pkgProcContextList, MatchPkgProcContextN, pkgName);
                    if (fNode)
                    {
                        PPkgProcContext pCurPkgProcContext = (PPkgProcContext)fNode->pUserData;
                        pthread_mutex_lock(&pCurPkgProcContext->dataMutex);
                        if (pCurPkgProcContext->taskStatusInfo.taskType == TASK_DL)
                        {
                            int curSC = pCurPkgProcContext->taskStatusInfo.statusCode;
                            if (curSC == SC_DOING)
                            {
                                StopPkgDLThread(pCurPkgProcContext);
                            }
                            pCurPkgProcContext->taskStatusInfo.statusCode = SC_ERROR;
                            pCurPkgProcContext->taskStatusInfo.errCode = SUEC_CORE_S_DL_ABORT;
                            if (curSC < SC_FINISHED)
                            {
                                pCurPkgProcContext->taskStatusInfo.u.msg = NULL;
                                NotifyTaskStatus(pCurPkgProcContext);
                            }
                        }
                        else retCode = SUEC_CORE_I_DLING;
                        pthread_mutex_unlock(&pCurPkgProcContext->dataMutex);

                    }

					// remove zip
					fFilePath = cp_find_file(pCoreContext->cfgParams.pkgRootPath, pkgName, 2, true);
					if (fFilePath) {
						remove(fFilePath);
						usleep(1000*500);
						free(fFilePath);
					} else {
						retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
					}

					// remove folder
					ptr = strstr(pkgName, ".zip");
					if (ptr) {
						strncpy(buf, pkgName, ptr-pkgName);
						buf[ptr-pkgName] = '\0';
						fFilePath = cp_find_file(pCoreContext->cfgParams.pkgRootPath, buf, 2, true);
						if (fFilePath) {
							util_rm_dir(fFilePath);
							usleep(1000*500);
							free(fFilePath);
						}
					}

					// remove database entry
					DeleteOnePkgScene(pkgName);

					if (pDelPkgRetList == NULL) pDelPkgRetList = LHInitList(LHFreeDelPkgRetInfoCB);
					pDelRetInfo = (PDelPkgRetInfo)malloc(sizeof(DelPkgRetInfo));
					len = strlen(pkgName) + 1;
					pDelRetInfo->name = (char *)malloc(len);
					memset(pDelRetInfo->name, 0, len);
					strcpy(pDelRetInfo->name, pkgName);
					pDelRetInfo->errCode = retCode;
					pDelRetInfo->pkgOwnerId = pkgOwnerId;
					LHAddNode(pDelPkgRetList, pDelRetInfo);

                    if (retCode != SUEC_CORE_SUCCESS)
						LOGE("SUECCore delete pkg:%s failed!ErrCode:%d, ErrMsg:%s", pkgName, retCode, SuecCoreGetErrorMsg(retCode));
                    else
						LOGI("SUECCore delete pkg:%s success!", pkgName);
                }
                pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
            }
        }
    }
    return pDelPkgRetList;
}

int SUECCoreSetTaskNorms(char * pkgName, int dlRetryCnt, int dpRetryCnt, int isRB)
{
    int retCode = SUEC_CORE_SUCCESS;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pkgName == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        PLNode findNode = NULL;
        PPkgProcContext pPkgProcContext = NULL;
        pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
        findNode = LHFindNode(pCoreContext->pkgProcContextList, MatchPkgProcContextN, pkgName);
        if (findNode != NULL)
        {
            pPkgProcContext = (PPkgProcContext)findNode->pUserData;
            pthread_mutex_lock(&pPkgProcContext->dataMutex);
            if (dlRetryCnt>=0) pPkgProcContext->dlRetryCnt = dlRetryCnt;
            if (dpRetryCnt >= 0) pPkgProcContext->dpRetryCnt = dpRetryCnt;
            if (isRB >= 0) pPkgProcContext->isRB = isRB;
            pthread_mutex_unlock(&pPkgProcContext->dataMutex);
        }
        else retCode = SUEC_CORE_I_OBJECT_NOT_FOUND;
        pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
    }
    return retCode;
}

char * SUECCoreGetErrMsg(int errCode)
{
    return SuecCoreGetErrorMsg(errCode);
}
//---------------------------interface implement E--------------------------

static int InitCfgParams(PCfgParams pCfgParams)
{
    if (pCfgParams)
    {
        int len = 0;
        char modulePath[MAX_PATH] = { 0 };
        util_module_path_get(modulePath);
        len = strlen(modulePath) + strlen(DEF_PKG_FOLDER_NAME) + 1;
        pCfgParams->pkgRootPath = (char *)malloc(len);
        memset(pCfgParams->pkgRootPath, 0, len);
        sprintf(pCfgParams->pkgRootPath, "%s%s", modulePath, DEF_PKG_FOLDER_NAME);

        len = strlen(modulePath) + 1;
        pCfgParams->envFileDir = (char *)malloc(len);
        memset(pCfgParams->envFileDir, 0, len);
        strcpy(pCfgParams->envFileDir, modulePath);

        pCfgParams->isSysStartupDP = DEF_IS_SYS_STARTUP_DP_DISENABLE;
        pCfgParams->deployWaitTM = DEF_DEPLOY_WAIT_TM;
        pCfgParams->resumePeriodMaxTS = DEF_RESUME_PERIOD_MAXTS;
        pCfgParams->dlRetryIntervalTS = DEF_DL_RETRY_INTERVAL_TS;
    }
    return 0;
}

static int GetParamsFormCfg(PCoreContext pCoreContext)
{
#define PKG_ROOTPATH_INI_ENTRY                   "SUECCore:PkgRootPath"
#define SYS_STARTUP_DP_INI_ENTRY                 "SUECCore:SysStartupDP"
#define RESUME_PERIOD_MAXTS_INI_ENTRY            "SUECCore:ResumePeriodMaxTS"
#define DEPLOY_WAIT_TM_INI_ENTRY                 "SUECCore:DeployWaitMaxTM"
#define DOWNLOAD_RETRY_INTREVAL_TM_INI_ENTRY     "SUECCore:DLRetryIntervalTS"
#define ENV_FILE_DIR_INI_ENTRY                   "Other:EnvFileDir"
#define WEB_SOCKET_URL_ENTRY                     "SUECCore:WebSocketURL"
#define SUEC_CORE_DEBUG_ENTRY                    "SUECCore:DLDebug"
    int retCode = SUEC_CORE_SUCCESS;
	char * realCfgPath = NULL, *tmpCfgPath = NULL;
	dictionary * dic = NULL;
	char modulePath[MAX_PATH] = { 0 };
	const char * pkgRootPath = NULL;

	// save config file name to realCfgPath
    if (!pCoreContext->cfgFile) {
		return retCode;
	}
	util_module_path_get(modulePath);

	// combine "modulePath/pCoreContext->cfgFile"
	if (*pCoreContext->cfgFile != '/' && // is not in root
		(strstr(pCoreContext->cfgFile, ":\\") == NULL) && // is not a file from network
		(strstr(pCoreContext->cfgFile, ":/") == NULL))  // is not a file from network
	{
		tmpCfgPath = (char *)malloc(strlen(modulePath) + strlen(pCoreContext->cfgFile) + 1);
		sprintf(tmpCfgPath, "%s%s", modulePath, pCoreContext->cfgFile);
		realCfgPath = tmpCfgPath;
	}
	else
		realCfgPath = pCoreContext->cfgFile;

	// read config from realCfgPath
	do
	{
		if (!util_is_file_exist(realCfgPath)) {
			break;
		}
		dic = iniparser_load(realCfgPath);
		if (!dic) {
			break;
		}

		pkgRootPath = iniparser_getstring(dic, PKG_ROOTPATH_INI_ENTRY, NULL);
		if (pkgRootPath != NULL && strlen(pkgRootPath) > 0)
		{
			char * tmpRootPath = NULL;

			// get pkgRootPath folder
			if (*pkgRootPath != '/' && (strstr(pkgRootPath, ":\\") == NULL) && (strstr(pkgRootPath, ":/") == NULL)) {
				// combine "modulePath/pkgRootPath"
				tmpRootPath = (char *) malloc(strlen(pkgRootPath) + strlen(modulePath) + 1);
				sprintf(tmpRootPath, "%s%s", modulePath, pkgRootPath);
			}
			else {
				tmpRootPath = (char *)malloc(strlen(pkgRootPath) + 1);
				strcpy(tmpRootPath, pkgRootPath);
			}

			// create pkgRootPath folder
			if (!util_is_file_exist(tmpRootPath)) {
				util_create_directory(tmpRootPath);
			}

			// save pkgRootPath folder
			if (util_is_file_exist(tmpRootPath)) {
				if (pCoreContext->cfgParams.pkgRootPath)
					free(pCoreContext->cfgParams.pkgRootPath);
				pCoreContext->cfgParams.pkgRootPath = tmpRootPath;
			}
			else { // Error, no tmpRootPath! pkgRootPath is not changed
				free(tmpRootPath);
			}
		}
		{
			const char * envFileDir = NULL;
			envFileDir = iniparser_getstring(dic, ENV_FILE_DIR_INI_ENTRY, NULL);
			if (envFileDir != NULL && strlen(envFileDir) > 0)
			{
				char * tmpPath = NULL;
				// get envFileDir folder
				if (*envFileDir != '/' && (strstr(envFileDir, ":\\") == NULL) && (strstr(envFileDir, ":/") == NULL)) {
					tmpPath = (char *)malloc(strlen(envFileDir) + strlen(modulePath) + 1);
					sprintf(tmpPath, "%s%s", modulePath, envFileDir);
				} else {
					tmpPath = (char *)malloc(strlen(envFileDir) + 1);
					strcpy(tmpPath, envFileDir);
				}
				// create envFileDir folder
				if (!util_is_file_exist(tmpPath)) {
					util_create_directory(tmpPath);
				}
				// save envFileDir folder
				if (util_is_file_exist(tmpPath)) {
					if (pCoreContext->cfgParams.envFileDir)free(pCoreContext->cfgParams.envFileDir);
					pCoreContext->cfgParams.envFileDir = tmpPath;
				}
				else
					free(tmpPath);
			}
		}
		{
			const char * startupDP = NULL;
			startupDP = iniparser_getstring(dic, SYS_STARTUP_DP_INI_ENTRY, NULL);
			if (startupDP != NULL && strlen(startupDP) > 0 && strcasecmp(startupDP, "True") == 0) {
				pCoreContext->cfgParams.isSysStartupDP = DEF_IS_SYS_STARTUP_DP_ENABLE;
			}
		}
		{
			const char * resumePeriodMaxTS = NULL;
			resumePeriodMaxTS = iniparser_getstring(dic, RESUME_PERIOD_MAXTS_INI_ENTRY, NULL);
			if (resumePeriodMaxTS != NULL && strlen(resumePeriodMaxTS) > 0) {
				pCoreContext->cfgParams.resumePeriodMaxTS = (unsigned int) strtol(resumePeriodMaxTS, NULL, 10);
			}
		}
		{
			const char * deployWaitTM = NULL;
			deployWaitTM = iniparser_getstring(dic, DEPLOY_WAIT_TM_INI_ENTRY, NULL);
			if (deployWaitTM != NULL && strlen(deployWaitTM) > 0)
			{
				unsigned int tmpUint = (unsigned int) strtol(deployWaitTM, NULL, 10);
				if (tmpUint > 0)
					pCoreContext->cfgParams.deployWaitTM = tmpUint;
			}
		}
		{
			const char * dlRetryIntervalTS = NULL;
			dlRetryIntervalTS = iniparser_getstring(dic, DOWNLOAD_RETRY_INTREVAL_TM_INI_ENTRY, NULL);
			if (dlRetryIntervalTS != NULL && strlen(dlRetryIntervalTS) > 0)
			{
				unsigned int tmpUint = (unsigned int) strtol(dlRetryIntervalTS, NULL, 10);
				if (tmpUint > 0)
					pCoreContext->cfgParams.dlRetryIntervalTS = tmpUint;
			}
		}
		{
			const char * webSocketUrl = NULL;
			webSocketUrl = iniparser_getstring(dic, WEB_SOCKET_URL_ENTRY, NULL);
			if (webSocketUrl != NULL && strlen(webSocketUrl) > 0) {
				pCoreContext->cfgParams.webSocketUrl = (char *)malloc(strlen(webSocketUrl) + 1);
				strcpy(pCoreContext->cfgParams.webSocketUrl, webSocketUrl);
			}
		}
		{
			const char *valueStr = iniparser_getstring(dic, SUEC_CORE_DEBUG_ENTRY, NULL);
			if (valueStr != NULL && strlen(valueStr) > 0) {
				pCoreContext->cfgParams.downloadDebug = (char*) malloc(strlen(valueStr) + 1);
				strcpy(pCoreContext->cfgParams.downloadDebug, valueStr);
			}
		}

		iniparser_freedict(dic);
	} while (0);

	if (tmpCfgPath)
		free(tmpCfgPath);
    return retCode;
}

static void InitSysInfo(PCoreContext pCoreContext)
{
    if (pCoreContext != NULL)
    {
        unsigned long int len = 0;

        if (pCoreContext->osName == NULL)
        {
			if (util_os_get_os_name(NULL, &len)) {
				pCoreContext->osName = (char*) malloc(len);
				util_os_get_os_name(pCoreContext->osName, NULL);
			}
        }
        if (pCoreContext->arch == NULL)
        {
			if (util_os_get_architecture(NULL, (int *) &len)) {
				pCoreContext->arch = (char*)malloc(len);
				util_os_get_architecture(pCoreContext->arch, NULL);
			}
        }
    }
}

#define FREE_STRING(str) do {\
	if (str) { free(str); str = NULL; } \
} while (0)

static void CleanCoreContext(PCoreContext pCoreContext)
{
    if (pCoreContext)
    {
        if (pCoreContext->isStarted)
        {
            SUECCoreStop();
        }
        LHDestroyList(pCoreContext->pkgDpCheckCBList);
        pthread_mutex_destroy(&pCoreContext->endecodeMutex);
        LHDestroyList(pCoreContext->pkgProcContextList);
        pthread_mutex_destroy(&pCoreContext->pkgProcContextListMutex);

		FREE_STRING(pCoreContext->osName);
		FREE_STRING(pCoreContext->arch);
		FREE_STRING(pCoreContext->cfgParams.pkgRootPath);
		FREE_STRING(pCoreContext->cfgParams.envFileDir);
		FREE_STRING(pCoreContext->cfgParams.webSocketUrl);
		FREE_STRING(pCoreContext->cfgParams.downloadDebug);
		FREE_STRING(pCoreContext->cfgFile);

        FTGlobalCleanup();
        LHDestroyList(pDpFrontList);
        pDpFrontList = NULL;
    }
}

static int CheckSysStartupDP()
{
    return CurCoreContext.cfgParams.isSysStartupDP;
}

static int CheckSysStartup(PCoreContext pCoreContext)
{
    int iRet = 0;
    if (pCoreContext != NULL)
    {
        unsigned int elapseTimeS = util_os_get_tick_count() / 1000;
        LOGI("System elapsed time S: %d", elapseTimeS);
        if (pCoreContext->cfgParams.resumePeriodMaxTS > elapseTimeS) iRet = 1;
    }
    //dhl debug
    //iRet = 1;
    return iRet;
}

extern PPkgProcContext CreatePkgProcContext()
{
    PPkgProcContext pPkgProcContext = NULL;
    int len = sizeof(PkgProcContext);
    pPkgProcContext = (PPkgProcContext)malloc(len);
    memset(pPkgProcContext, 0, len);
    pthread_mutex_init(&pPkgProcContext->dataMutex, NULL);
    return pPkgProcContext;
}

static void DestroyPkgProcContext(PPkgProcContext pPkgProcContext)
{
    if (pPkgProcContext)
    {
        StopPkgThread(NULL, pPkgProcContext);

		FREE_STRING(pPkgProcContext->pkgName);
		FREE_STRING(pPkgProcContext->pkgType);
		FREE_STRING(pPkgProcContext->pkgURL);
		FREE_STRING(pPkgProcContext->taskStatusInfo.pkgName);
        if (!(pPkgProcContext->taskStatusInfo.taskType == TASK_DL &&
            pPkgProcContext->taskStatusInfo.statusCode < SC_FINISHED) &&
            pPkgProcContext->taskStatusInfo.u.msg != NULL)
        {
            free(pPkgProcContext->taskStatusInfo.u.msg);
            pPkgProcContext->taskStatusInfo.u.msg = NULL;
        }

		FREE_STRING(pPkgProcContext->pkgPath);
		FREE_STRING(pPkgProcContext->pkgBKPath);
		FREE_STRING(pPkgProcContext->pkgTypePath);
		FREE_STRING(pPkgProcContext->pkgFirstUnzipPath);
		FREE_STRING(pPkgProcContext->pkgInfoXmlPath);
		FREE_STRING(pPkgProcContext->pkgSecondZipPath);
		FREE_STRING(pPkgProcContext->pkgSecondUnzipPath);
		FREE_STRING(pPkgProcContext->pkgDeployXmlPath);
		FREE_STRING(pPkgProcContext->pkgDeployFilePath);
		FREE_STRING(pPkgProcContext->pkgRetCheckScriptPath);
		FREE_STRING(pPkgProcContext->rbParam);
		// free xmlPkgInfo
		FREE_STRING(pPkgProcContext->xmlPkgInfo.os);
		FREE_STRING(pPkgProcContext->xmlPkgInfo.arch);
		FREE_STRING(pPkgProcContext->xmlPkgInfo.tags);
		FREE_STRING(pPkgProcContext->xmlPkgInfo.version);
		FREE_STRING(pPkgProcContext->xmlPkgInfo.secondZipName);
		FREE_STRING(pPkgProcContext->xmlPkgInfo.zipPwd);
		FREE_STRING(pPkgProcContext->xmlPkgInfo.installerTool);
		// free xmlDeployInfo
		FREE_STRING(pPkgProcContext->xmlDeployInfo.execFileName);
		FREE_STRING(pPkgProcContext->xmlDeployInfo.retCheckScript);

        pthread_mutex_destroy(&pPkgProcContext->dataMutex);
        free(pPkgProcContext);
    }
}

static int SavePkgProcScene(PPkgProcContext pPkgProcContext, int resumeType)
{
	static PkgProcContext pkgContext; // cache last pkg context
    int iRet = 0;

	// copy taskinfo percent(ignore taskinfo percent change) then compare if changed.
	if (pPkgProcContext->taskStatusInfo.statusCode == SC_DOING) {
		pkgContext.taskStatusInfo.u.dlPercent = pPkgProcContext->taskStatusInfo.u.dlPercent;
	}
	// if change, save pkg context
	if (memcmp(&pkgContext, pPkgProcContext, sizeof(pkgContext)) != 0) {
		ReplacePkgScene(pPkgProcContext, resumeType);
		memcpy(&pkgContext, pPkgProcContext, sizeof(pkgContext));
	}
    return iRet;
}

static int ResumePkgProcScene(PCoreContext pCoreContext, int resumeType)
{
    int iRet = 0;
    /*if (pCoreContext != NULL && pCoreContext->pkgProcContextList != NULL &&
    CheckSysStartupDP() && CheckSysStartup(pCoreContext))*/
    if (pCoreContext != NULL && pCoreContext->pkgProcContextList != NULL)
    {
        PLHList pkgProcContextList = pCoreContext->pkgProcContextList;
        if (resumeType == DEF_ALL_RT)
			SelectAllPkgScene(pkgProcContextList);
        else SelectPkgSceneWithResumeType(pkgProcContextList, resumeType);
        {
            PLNode curNode = pkgProcContextList->head;
            PPkgProcContext pPkgProcContext = NULL;
            while (curNode)
            {
                pPkgProcContext = (PPkgProcContext)curNode->pUserData;
                ReviseProcContext(pPkgProcContext);
                if (pPkgProcContext->taskStatusInfo.taskAction == TA_ROLLBACK)
                {
                    PPkgProcContext rbProcContext = FindPkgRBScene(pPkgProcContext->pkgType);
                    if (rbProcContext != NULL)
                    {
                        ReviseProcContext(rbProcContext);
                        pPkgProcContext->rbParam = rbProcContext;
                    }
                }
                curNode = curNode->next;
            }
        }
        //if (resumeType == DEF_ALL_RT) DeleteAllPkgScene();
        //else DeletePkgSceneWithResumeType(resumeType);
    }
    return iRet;
}

static void ReviseProcContext(PPkgProcContext pPkgProcContext)
{
    if (pPkgProcContext)
    {
        if (pPkgProcContext->pkgStep > STEP_INIT)
			GetCurPkgFLPath(pPkgProcContext);
		if (pPkgProcContext->pkgStep > STEP_UNZIP_SL && CheckZipComment(pPkgProcContext->pkgPath, NULL, 0) != 1)
			GetCurPkgSLPath(pPkgProcContext);
		if (pPkgProcContext->pkgStep > STEP_UNZIP_SL && CheckZipComment(pPkgProcContext->pkgPath, NULL, 0) == 1)
			GetCurPkgSLPath2(pPkgProcContext);
        if (pPkgProcContext->pkgStep > STEP_READXML_SL)
			GetCurPkgTLPath(pPkgProcContext);
        if (pPkgProcContext->pkgStep == STEP_RUN_DPF)
			pPkgProcContext->pkgStep = STEP_REBOOT_SYS;
        else if (pPkgProcContext->pkgStep == STEP_REBOOT_SYS)
			pPkgProcContext->pkgStep = STEP_CHECK_DP_RET;
    }
}

static PPkgProcContext FindPkgRBScene(char * pkgType)
{
    PPkgProcContext pPkgProcContext = NULL;
    if (pkgType != NULL) SelectOnePkgSceneWithRTPT(DEF_RB_RT, pkgType, &pPkgProcContext);
    return pPkgProcContext;
}

static int TaskProcStart(PCoreContext pCoreContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pCoreContext == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        if (!pCoreContext->isPkgProcThreadRunning)
        {
            pCoreContext->isPkgProcThreadRunning = 1;
            if (pthread_create(&pCoreContext->pkgProcThreadT, NULL, PkgProcThreadStart, pCoreContext) != 0)
            {
                pCoreContext->isPkgProcThreadRunning = 0;
                retCode = SUEC_CORE_I_TASK_PROCESS_START_FAILED;
            }
        }
    }
    return retCode;
}

static int TaskProcStop(PCoreContext pCoreContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pCoreContext == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        if (pCoreContext->isPkgProcThreadRunning)
        {
            pCoreContext->isPkgProcThreadRunning = 0;
        }
        pthread_join(pCoreContext->pkgProcThreadT, NULL);
        pCoreContext->pkgProcThreadT = 0;
        //add other thread stop
    }
    return retCode;
}

static void FreeDPFrontInfo(void * pUserData)
{
    char * pkgType = (char*)pUserData;
    free(pkgType);
}

static int DPFrontInfoMatch(void * pUserData, void * key)
{
    int iRet = 0;
    if (pUserData != NULL && key != NULL)
    {
        char * matchPkgType = (char*)key;
        char * curPkgType = (char*)pUserData;
        if (!strcmp(matchPkgType, curPkgType))
        {
            iRet = 1;
        }
    }
    return iRet;
}

static int isDPFrontWithType(PPkgProcContext pPkgProcContext)
{
    int iRet = 0;
    if (pDpFrontList == NULL)
		pDpFrontList = LHInitList(FreeDPFrontInfo);
    {
        PLNode findNode = NULL;
        findNode = LHFindNode(pDpFrontList, DPFrontInfoMatch, pPkgProcContext->pkgType);
        if (!findNode)
        {
            int len = strlen(pPkgProcContext->pkgType) + 1;
            char * pkgType = (char*)malloc(len);
            memset(pkgType, 0, len);
            strcpy(pkgType, pPkgProcContext->pkgType);
            LHAddNode(pDpFrontList, pkgType);
            iRet = 1;
        }
    }
    return iRet;
}

static int detachDPFrontInfo(PPkgProcContext pPkgProcContext)
{
    if (pDpFrontList)
    {
        LHDelNode(pDpFrontList, DPFrontInfoMatch, pPkgProcContext->pkgType);
    }
    return 0;
}

static void* PkgProcThreadStart(void *args)
{
    PLNode curNode = NULL;
    PCoreContext pCoreContext = (PCoreContext)args;
    if (pCoreContext)
    {
        PPkgProcContext curPkgProcContext = NULL;
        while (pCoreContext->isPkgProcThreadRunning)
        {
            pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
            if (!LHListIsEmpty(pCoreContext->pkgProcContextList))
            {
                PLNode preNode = NULL;
                PLNode delNode = NULL;
                //int dpFrontFound = 0;
                curNode = pCoreContext->pkgProcContextList->head;
                while (curNode)
                {
                    curPkgProcContext = (PPkgProcContext)curNode->pUserData;
                    if (curPkgProcContext)
                    {
                        pthread_mutex_lock(&curPkgProcContext->dataMutex);
                        //Check the most in front task of list which could to deploy
                        if (curPkgProcContext->thenDP && isDPFrontWithType(curPkgProcContext))
                        {
                            curPkgProcContext->isDPFront = 1;
                        }
                        /*curPkgProcContext->isDPFront = 0;
                        if (curPkgProcContext->thenDP && !dpFrontFound)
                        {
                            curPkgProcContext->isDPFront = 1;
                            dpFrontFound = 1;
                        }*/
                        PkgDLAndDPProc(curPkgProcContext);

						// TASK_DP or TASK_DL
						if (curPkgProcContext->taskStatusInfo.statusCode == SC_ERROR ||
							curPkgProcContext->taskStatusInfo.statusCode == SC_FINISHED)
						{
							// detach DPFront, so other deploy which has the same pkgType can continue to process.
							detachDPFrontInfo(curPkgProcContext);
                            delNode = curNode;

							if (curPkgProcContext->taskStatusInfo.taskType == TASK_DL &&
								curPkgProcContext->taskStatusInfo.statusCode == SC_ERROR)
							{
								// delete package because package may not complete
								ClearPkgPath(curPkgProcContext);
							}

                            //dhl add
							//deploy error or finish, clear download files
                            //If config system startup deploy then don't clear package path
                            if (curPkgProcContext->taskStatusInfo.taskType == TASK_DP &&
							    curPkgProcContext->taskStatusInfo.errCode != SUEC_CORE_S_CFG_SYSSTARTUP_DP)
                            {
                                PPkgProcContext delProcContext = (PPkgProcContext)delNode->pUserData;
                                ClearPkgPath(delProcContext);
                                if (delProcContext->rbParam)
                                {
                                    PPkgProcContext delRBProcContext = (PPkgProcContext)delProcContext->rbParam;
                                    ClearPkgPath(delRBProcContext);
                                    DeleteOnePkgScene(delRBProcContext->pkgName);
                                }
                                DeleteOnePkgScene(curPkgProcContext->pkgName);
                            }
                        }

                        pthread_mutex_unlock(&curPkgProcContext->dataMutex);
                    }
                    if (delNode)
                    {
                        if (delNode == pCoreContext->pkgProcContextList->head)
                        {
                            pCoreContext->pkgProcContextList->head = delNode->next;
                            curNode = pCoreContext->pkgProcContextList->head;
                        }
                        else
                        {
                            preNode->next = curNode->next;
                            curNode = preNode->next;
                        }
                        if (delNode == pCoreContext->pkgProcContextList->trail)
                        {
                            pCoreContext->pkgProcContextList->trail = preNode;
                        }
                        if (pCoreContext->pkgProcContextList->freeUserDataCB)
                        {
                            pCoreContext->pkgProcContextList->freeUserDataCB(delNode->pUserData);
                            delNode->pUserData = NULL;
                        }
                        free(delNode);
                        delNode = NULL;
                    }
                    else
                    {
                        preNode = curNode;
                        curNode = preNode->next;
                    }
                }
            }
            pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
            usleep(1000*100);
        }
        pCoreContext->isPkgProcThreadRunning = 0;
    }
    return 0;
}

static void PkgDLAndDPProc(PPkgProcContext pPkgProcContext)
{
    if (pPkgProcContext)
    {
        //dhl modify
        PkgDLProc(pPkgProcContext);
        PkgDPProc(pPkgProcContext);
    }
}

static void PkgDLProc(PPkgProcContext pPkgProcContext)
{
    if (pPkgProcContext)
    {
        int curSC = SC_UNKNOW;
        TaskType curTaskTy = TASK_UNKNOW;
        curSC = pPkgProcContext->taskStatusInfo.statusCode;
        curTaskTy = pPkgProcContext->taskStatusInfo.taskType;
        if (curTaskTy == TASK_DL)
        {
            if (curSC >= SC_QUEUE && curSC <= SC_FINISHED) {
				SavePkgProcScene(pPkgProcContext, DEF_COMMON_RT);
			}
            switch (curSC)
            {
				case SC_QUEUE:
				{
					//add other code
					pPkgProcContext->taskStatusInfo.statusCode = SC_START;
					LOGI("Package %s download start!", pPkgProcContext->pkgName);
					NotifyTaskStatus(pPkgProcContext);
				}
				case SC_START:
				{
					if (ProcDLStartStatus(pPkgProcContext) != SUEC_CORE_SUCCESS)
						break;
				}
				case SC_DOING:
				{
					if (!pPkgProcContext->isDLThreadRunning && !pPkgProcContext->dlThreadT)
						ProcResumeDLDoing(pPkgProcContext);  //scene resume restart the processing thread.
					break;
				}
				case SC_SUSPEND:
				{
					break;
				}
				case SC_FINISHED:
				{
					ProcDLFinishedStatus(pPkgProcContext);
					break;
				}
				default:
					break;
            }
            if (pPkgProcContext->taskStatusInfo.taskType == TASK_DL &&
                pPkgProcContext->taskStatusInfo.statusCode == SC_ERROR)
            {
                LOGI("PkgDLProc: Package %s error! ErrCode %d, ErrMsg:%s", pPkgProcContext->pkgName,
                    pPkgProcContext->taskStatusInfo.errCode, SuecCoreGetErrorMsg(pPkgProcContext->taskStatusInfo.errCode));
            }
        }
    }
}

static int ProcDLStartStatus(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL)
		return SUEC_CORE_I_PARAMETER_ERROR;
    {
        retCode = StartPkgDLThread(pPkgProcContext);
        if (retCode != SUEC_CORE_SUCCESS)
        {
            pPkgProcContext->taskStatusInfo.statusCode = SC_ERROR;
            pPkgProcContext->taskStatusInfo.errCode = retCode;
        }
        else
        {
            pPkgProcContext->taskStatusInfo.statusCode = SC_DOING;
            LOGI("Package %s download doing!", pPkgProcContext->pkgName);
        }
        NotifyTaskStatus(pPkgProcContext);
    }
    return retCode;
}

static int ProcResumeDLDoing(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        retCode = StartPkgDLThread(pPkgProcContext);
        if (retCode != SUEC_CORE_SUCCESS)
        {
            pPkgProcContext->taskStatusInfo.statusCode = SC_ERROR;
            pPkgProcContext->taskStatusInfo.errCode = retCode;
            NotifyTaskStatus(pPkgProcContext);
        }
    }
    return retCode;
}

static int ProcDLFinishedStatus(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL)
		return SUEC_CORE_I_PARAMETER_ERROR;
    {
        if (CheckSysStartupDP())
        {
            LOGI("System startup deploy, Package %s save scene", pPkgProcContext->pkgName);
            pPkgProcContext->taskStatusInfo.taskType = TASK_DP;
            pPkgProcContext->taskStatusInfo.statusCode = SC_QUEUE;
            SavePkgProcScene(pPkgProcContext, DEF_SYS_STARTUP_RT);
            pPkgProcContext->taskStatusInfo.statusCode = SC_ERROR;
            pPkgProcContext->taskStatusInfo.errCode = SUEC_CORE_S_CFG_SYSSTARTUP_DP;
            if (!pPkgProcContext->thenDP) pPkgProcContext->taskStatusInfo.taskType = TASK_UNKNOW;
        }
        else if (pPkgProcContext->thenDP)
        {
            pPkgProcContext->taskStatusInfo.taskType = TASK_DP;
            pPkgProcContext->taskStatusInfo.statusCode = SC_QUEUE;
        }
        if (pPkgProcContext->thenDP) {
			NotifyTaskStatus(pPkgProcContext);
		}
    }
    return retCode;
}

static int StartPkgDLThread(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        pPkgProcContext->isDLThreadRunning = 1;
        if (pthread_create(&pPkgProcContext->dlThreadT, NULL, DLThreadStart, pPkgProcContext) != 0)
        {
            pPkgProcContext->isDLThreadRunning = 0;
            retCode = SUEC_CORE_S_DL_START_FAILED;
        }
    }
    return retCode;
}

static void* DLThreadStart(void *args)
{
    PPkgProcContext pPkgProcContext = (PPkgProcContext)(args);
    if (pPkgProcContext)
    {
        int retCode = SUEC_CORE_SUCCESS;
        retCode = PkgStepByStep(pPkgProcContext);
        if (pPkgProcContext->isDLThreadRunning)
        {
            pthread_mutex_lock(&pPkgProcContext->dataMutex);
            if (retCode != SUEC_CORE_SUCCESS)
            {
                pPkgProcContext->taskStatusInfo.statusCode = SC_ERROR;
                pPkgProcContext->taskStatusInfo.errCode = retCode;
            }
            else
            {
                pPkgProcContext->taskStatusInfo.statusCode = SC_FINISHED;
                LOGI("Package %s download finished!", pPkgProcContext->pkgName);
            }
            pPkgProcContext->taskStatusInfo.u.msg = NULL;
            NotifyTaskStatus(pPkgProcContext);
            pthread_mutex_unlock(&pPkgProcContext->dataMutex);
        }
        pPkgProcContext->isDLThreadRunning = 0;
    }
    return 0;
}

static int StopPkgDLThread(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        if (pPkgProcContext->isDLThreadRunning)
        {
            pPkgProcContext->isDLThreadRunning = 0;
        }
        if (pPkgProcContext->ftHandle)
        {
            FTAbortTransfer(pPkgProcContext->ftHandle);
        }
        if (pPkgProcContext->dlThreadT != 0)
        {
            pthread_join(pPkgProcContext->dlThreadT, NULL);
            pPkgProcContext->dlThreadT = 0;
        }
    }
    return retCode;
}

static void PkgDPProc(PPkgProcContext pPkgProcContext)
{
    //Deploying flag, ensure only one package deploy at same time
    static int isDPing = 0;
    if (pPkgProcContext)
    {
        int curSC = SC_UNKNOW;
        TaskType curTaskTy = TASK_UNKNOW;
        curSC = pPkgProcContext->taskStatusInfo.statusCode;
        curTaskTy = pPkgProcContext->taskStatusInfo.taskType;
        if (curTaskTy == TASK_DP)
        {
            if (curSC >= SC_QUEUE && curSC <= SC_FINISHED) {
				SavePkgProcScene(pPkgProcContext, DEF_COMMON_RT);
			}
            switch (curSC)
            {
            case SC_QUEUE:
            {
                //add other code
                if (pPkgProcContext->isDPFront && !isDPing)
                {
                    pPkgProcContext->taskStatusInfo.statusCode = SC_START;
                    LOGI("Package %s deploy start!", pPkgProcContext->pkgName);
                    NotifyTaskStatus(pPkgProcContext);
                }
                else break;
            }
            case SC_START:
            {
                if (ProcDPStartStatus(pPkgProcContext) != SUEC_CORE_SUCCESS) break;
            }
            case SC_DOING:
            {
                if (!isDPing)isDPing = 1;
                if (!pPkgProcContext->isDPThreadRunning && !pPkgProcContext->dpThreadT)
                    ProcResumeDPDoing(pPkgProcContext); //scene resume restart the processing thread.
                break;
            }
            case SC_SUSPEND:
            {
                break;
            }
            case SC_FINISHED:
            {
                isDPing = 0;
                ProcDPFinishedStatus(pPkgProcContext);
                break;
            }
            default: break;
            }
            if (pPkgProcContext->taskStatusInfo.statusCode == SC_ERROR)
            {
                LOGI("PkgDPProc: Package %s deploy error! ErrCode %d, ErrMsg:%s", pPkgProcContext->pkgName,
                    pPkgProcContext->taskStatusInfo.errCode, SuecCoreGetErrorMsg(pPkgProcContext->taskStatusInfo.errCode));
                //Don't allow the config of system startup deploy disrupt the deployment mechanism.
                if (pPkgProcContext->taskStatusInfo.errCode != SUEC_CORE_S_CFG_SYSSTARTUP_DP) isDPing = 0;
            }
        }
    }
}

static int ProcDPStartStatus(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL)
		return SUEC_CORE_I_PARAMETER_ERROR;
    {
        retCode = StartPkgDPThread(pPkgProcContext);
        if (retCode != SUEC_CORE_SUCCESS)
        {
            pPkgProcContext->taskStatusInfo.statusCode = SC_ERROR;
            pPkgProcContext->taskStatusInfo.errCode = retCode;
        }
        else
        {
            pPkgProcContext->taskStatusInfo.statusCode = SC_DOING;
            LOGI("Package %s deploy doing!", pPkgProcContext->pkgName);
        }
        NotifyTaskStatus(pPkgProcContext);
    }
    return retCode;
}

static int ProcResumeDPDoing(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        retCode = StartPkgDPThread(pPkgProcContext);
        if (retCode != SUEC_CORE_SUCCESS)
        {
            pPkgProcContext->taskStatusInfo.statusCode = SC_ERROR;
            pPkgProcContext->taskStatusInfo.errCode = retCode;
            NotifyTaskStatus(pPkgProcContext);
        }
    }
    return retCode;
}

static int ProcDPFinishedStatus(PPkgProcContext pPkgProcContext)
{
#define DEF_BK_FILE_WILDCARD  "*.bk"
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL)
		return SUEC_CORE_I_PARAMETER_ERROR;

	if (pPkgProcContext->pkgType && strcasecmp(pPkgProcContext->pkgType, "OTAClient") &&
		pPkgProcContext->taskStatusInfo.taskAction != TA_ROLLBACK && pPkgProcContext->isRB)
	{
		if (pPkgProcContext->pkgTypePath) {
			cp_del_match_file(pPkgProcContext->pkgTypePath, DEF_BK_FILE_WILDCARD);
		}
		if (pPkgProcContext->pkgPath && util_is_file_exist(pPkgProcContext->pkgPath) &&
			pPkgProcContext->pkgBKPath)
		{
			rename(pPkgProcContext->pkgPath, pPkgProcContext->pkgBKPath);
		}
	}
    return retCode;
}

static int StartPkgDPThread(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL)
		return SUEC_CORE_I_PARAMETER_ERROR;

	pPkgProcContext->isDPThreadRunning = 1;
	if (pthread_create(&pPkgProcContext->dpThreadT, NULL, DPThreadStart, pPkgProcContext) != 0)
	{
		pPkgProcContext->isDPThreadRunning = 0;
		retCode = SUEC_CORE_S_DP_START_FAILED;
	}

    return retCode;
}

static void* DPThreadStart(void *args)
{
    PPkgProcContext pPkgProcContext = (PPkgProcContext)(args);
    if (pPkgProcContext)
    {
        int retCode = SUEC_CORE_SUCCESS;
        retCode = PkgStepByStep(pPkgProcContext);
        if (pPkgProcContext->isDPCheckThreadRunning)
        {
            pPkgProcContext->isDPCheckThreadRunning = 0;
            pthread_join(pPkgProcContext->dpCheckThreadT, NULL);
            pPkgProcContext->dpCheckThreadT = 0;
        }
        if (pPkgProcContext->isDPThreadRunning)
        {
            pthread_mutex_lock(&pPkgProcContext->dataMutex);
            if (retCode == SUEC_CORE_SUCCESS)
            {
                pPkgProcContext->taskStatusInfo.statusCode = SC_FINISHED;
                LOGI("Package %s deploy finished!", pPkgProcContext->pkgName);
            }
            else
            {
                pPkgProcContext->taskStatusInfo.statusCode = SC_ERROR;
                pPkgProcContext->taskStatusInfo.errCode = retCode;
            }
            pPkgProcContext->taskStatusInfo.u.msg = NULL;
            NotifyTaskStatus(pPkgProcContext);
            pthread_mutex_unlock(&pPkgProcContext->dataMutex);
            pPkgProcContext->isDPThreadRunning = 0;
        }
    }
    return 0;
}

static int StopPkgDPThread(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        if (pPkgProcContext->isDPThreadRunning)
        {
            pPkgProcContext->isDPThreadRunning = 0;
        }
        if (pPkgProcContext->dpThreadT != 0)
        {
            pthread_join(pPkgProcContext->dpThreadT, NULL);
            pPkgProcContext->dpThreadT = 0;
        }
        if (pPkgProcContext->isDPCheckThreadRunning)
        {
            pPkgProcContext->isDPCheckThreadRunning = 0;
        }
        if (pPkgProcContext->dpCheckThreadT != 0)
        {
            pthread_join(pPkgProcContext->dpCheckThreadT, NULL);
            pPkgProcContext->dpCheckThreadT = 0;
        }
    }
    return retCode;
}

static int StopPkgThread(void * args, void * pUserData)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (NULL == pUserData) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        PPkgProcContext pPkgProcContext = (PPkgProcContext)pUserData;
        StopPkgDLThread(pPkgProcContext);
        StopPkgDPThread(pPkgProcContext);
    }
    return retCode;
}

//pkg step control
static int PkgStepByStep(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;

    if (pPkgProcContext == NULL)
		return SUEC_CORE_I_PARAMETER_ERROR;
    {
        int * runningFlag = NULL;
        PPkgProcContext pCurOprtPkgContext = pPkgProcContext;
        TaskType curTT = pPkgProcContext->taskStatusInfo.taskType;
        TaskAction curTA = pPkgProcContext->taskStatusInfo.taskAction;
        if (curTT == TASK_DL)
			runningFlag = &pPkgProcContext->isDLThreadRunning;//dl
        else
			runningFlag = &pPkgProcContext->isDPThreadRunning; //dp
        if (curTA == TA_ROLLBACK)
        {
            pCurOprtPkgContext = (PPkgProcContext)pPkgProcContext->rbParam;
        }

        if (pCurOprtPkgContext == NULL)
			retCode = SUEC_CORE_I_PARAMETER_ERROR;
        {
            PkgStep curPS = pCurOprtPkgContext->pkgStep;
            while (curPS != STEP_UNKNOW && *runningFlag)
            {
                PkgStepAction(pCurOprtPkgContext, runningFlag);
                if (*runningFlag)
                {
                    curPS = PkgGetNextStep(curTT, curTA, pCurOprtPkgContext);
                    if (curPS != STEP_UNKNOW)pCurOprtPkgContext->pkgStep = curPS;
                }
            }
            retCode = pCurOprtPkgContext->errCode;
        }
    }
    return retCode;
}

//pkg step action
static void PkgStepAction(PPkgProcContext pCurOprtPkgContext, int * runningFlag)
{
    if (pCurOprtPkgContext)
    {
		LOGI("PkgStepAction: step=%d", pCurOprtPkgContext->pkgStep);
        switch (pCurOprtPkgContext->pkgStep)
        {
        case STEP_INIT:
        {
            if (GetCurPkgFLPath(pCurOprtPkgContext) != 0)
				pCurOprtPkgContext->errCode = SUEC_CORE_S_PKG_PATH_ERROR;
            break;
        }
        case STEP_EXIST_CHECK:
        {
            if (!util_is_file_exist(pCurOprtPkgContext->pkgPath))
				pCurOprtPkgContext->errCode = SUEC_CORE_S_PKG_FILE_NOT_EXIST; //deploy but pkg exist check
            break;
        }
        case STEP_FT_DL:
        {
            pCurOprtPkgContext->errCode = PkgFTDownload(pCurOprtPkgContext, runningFlag);
            break;
        }
        case STEP_MD5_CHECK:
        {
            pCurOprtPkgContext->errCode = PkgMD5Check(pCurOprtPkgContext->pkgPath, runningFlag);
            break;
        }
        case STEP_UNZIP_FL:
        {
			if (CheckZipComment(pCurOprtPkgContext->pkgPath,NULL,0) == 1) {
				break;
			}
            if (MiniUnzip(pCurOprtPkgContext->pkgPath, pCurOprtPkgContext->pkgFirstUnzipPath, NULL) != 0) {
				pCurOprtPkgContext->errCode = SUEC_CORE_S_UNZIP_ERROR;
			}
            break;
        }
        case STEP_READXML_FL:
        {
            if (GetPkgInfoFromXml(pCurOprtPkgContext->pkgInfoXmlPath, &pCurOprtPkgContext->xmlPkgInfo, pCurOprtPkgContext->pkgPath) != 0) {
				pCurOprtPkgContext->errCode = SUEC_CORE_S_XML_PARSE_ERROR; //Read PackageInfo.xml
			}
            break;
        }
        case STEP_TAGS_CHECK:
        {
            pCurOprtPkgContext->errCode = PkgTagsCheck(pCurOprtPkgContext);
            break;
        }
        case STEP_UNZIP_SL:
        {
			if (CheckZipComment(pCurOprtPkgContext->pkgPath,NULL,0) == 1) {
				pCurOprtPkgContext->errCode = PkgUnzipSL2(pCurOprtPkgContext);
			} else {
				// legacy OTA package
				pCurOprtPkgContext->errCode = PkgUnzipSL(pCurOprtPkgContext);
			}
            break;
        }
        case STEP_READXML_SL:
        {
            if (GetDeployInfoFromXml(pCurOprtPkgContext->pkgDeployXmlPath, &pCurOprtPkgContext->xmlDeployInfo) != 0) {
                pCurOprtPkgContext->errCode = SUEC_CORE_S_XML_PARSE_ERROR; //Read Deploy.xml
            }
			if (GetCurPkgTLPath(pCurOprtPkgContext) != 0)
                pCurOprtPkgContext->errCode = SUEC_CORE_S_PKG_PATH_ERROR;
            break;
        }
        case STEP_CHECK_DPF:
        {
            if (!util_is_file_exist(pCurOprtPkgContext->pkgDeployFilePath))
                pCurOprtPkgContext->errCode = SUEC_CORE_S_DPFILE_NOT_EXIST;
            break;
        }
        case STEP_RUN_DPF:
        {
			// record current status before deploy
			PkgRecordSWInfo(pCurOprtPkgContext, runningFlag);
            pCurOprtPkgContext->errCode = PkgRunDeployFile(pCurOprtPkgContext, runningFlag);
            break;
        }
        case STEP_REBOOT_SYS:
        {
            PkgRebootSys(pCurOprtPkgContext);
            break;
        }
        case STEP_CHECK_DP_RET:
        {
            pCurOprtPkgContext->errCode = PkgCheckDPRet(pCurOprtPkgContext, runningFlag);
            break;
        }
        case STEP_RECORD_SW:
        {
            PkgRecordSWInfo(pCurOprtPkgContext, runningFlag);
            break;
        }
        case STEP_DP_RB:
        {
            pCurOprtPkgContext->errCode = PkgDPRollback(pCurOprtPkgContext, runningFlag);
            break;
        }
		default:
			LOGE("PkgStepAction: Error invalid pkgStep");
			break;
        }
    }
}

//Pkg step transfer
/*
download (TASK_DL):
	STEP_INIT -> STEP_FT_DL -> STEP_MD5_CHECK -> STEP_UNZIP_FL -> STEP_READXML_FL -> STEP_TAGS_CHECK ->
	STEP_UNZIP_SL -> STEP_READXML_SL -> STEP_CHECK_DPF -> STEP_RUN_DPF -> STEP_REBOOT_SYS ->
	STEP_CHECK_DP_RET -> STEP_RECORD_SW

rollback (TA_ROLLBACK):
	STEP_INIT -> STEP_UNZIP_FL -> STEP_READXML_FL ->
	STEP_UNZIP_SL -> STEP_READXML_SL -> STEP_CHECK_DPF -> STEP_RUN_DPF -> STEP_REBOOT_SYS ->
	STEP_CHECK_DP_RET -> STEP_RECORD_SW

error:
	STEP_FT_DL -> STEP_FT_DL
	STEP_RUN_DPF-> STEP_REBOOT_SYS -> STEP_CHECK_DP_RET -> STEP_RECORD_SW -> STEP_DP_RB
*/
static PkgStep PkgGetNextStep(TaskType curTT, TaskAction curTA, PPkgProcContext pPkgProcContext)
{
    PkgStep retPS = STEP_UNKNOW;
    if (pPkgProcContext != NULL)
    {
        PkgStep curPT = pPkgProcContext->pkgStep;
        if (pPkgProcContext->errCode == SUEC_CORE_SUCCESS)
        {
            if (curPT == STEP_INIT)
            {
                if (curTT == TASK_DL)
					retPS = STEP_FT_DL;
                else if (curTA == TA_ROLLBACK)
					retPS = STEP_UNZIP_FL;
                else
					retPS = STEP_EXIST_CHECK;
            }
            else if (curPT == STEP_EXIST_CHECK)
				retPS = STEP_MD5_CHECK;
            else if (curPT == STEP_FT_DL)
				retPS = STEP_MD5_CHECK;
            else if (curPT == STEP_MD5_CHECK)
				retPS = STEP_UNZIP_FL;
            else if (curPT == STEP_UNZIP_FL)
				retPS = STEP_READXML_FL;
            else if (curPT == STEP_READXML_FL)
            {
                if (curTA == TA_ROLLBACK)
					retPS = STEP_UNZIP_SL;
                else
					retPS = STEP_TAGS_CHECK;
            }
            else if (curPT == STEP_TAGS_CHECK)
				retPS = STEP_UNZIP_SL;
            else if (curPT == STEP_UNZIP_SL)
				retPS = STEP_READXML_SL;
            else if (curPT == STEP_READXML_SL)
				retPS = STEP_CHECK_DPF;
            else if (curPT == STEP_CHECK_DPF && curTT == TASK_DP)
				retPS = STEP_RUN_DPF;
            else if (curPT == STEP_RUN_DPF)
				retPS = STEP_REBOOT_SYS;
            else if (curPT == STEP_REBOOT_SYS)
				retPS = STEP_CHECK_DP_RET;
            else if (curPT == STEP_CHECK_DP_RET)
				retPS = STEP_RECORD_SW;
        }
        else
        {
            if (curPT == STEP_FT_DL && pPkgProcContext->dlRetryCnt > 0 &&
                pPkgProcContext->dlRetryCnt > pPkgProcContext->curDLRetryCnt)
            {
                retPS = STEP_FT_DL;
                pPkgProcContext->curDLRetryCnt++;
            }
            else if (curPT == STEP_RUN_DPF)
				retPS = STEP_REBOOT_SYS;
            else if (curPT == STEP_REBOOT_SYS)
				retPS = STEP_CHECK_DP_RET;
            else if (curPT == STEP_CHECK_DP_RET)
            {
                if (pPkgProcContext->dpRetryCnt > 0 &&
                    pPkgProcContext->dpRetryCnt > pPkgProcContext->curDPRetryCnt &&
                    curTA != TA_ROLLBACK)
                {
                    retPS = STEP_RUN_DPF;
                    pPkgProcContext->curDPRetryCnt++;
                }
                else
					retPS = STEP_RECORD_SW;
            }
            else if (curPT == STEP_RECORD_SW && curTA != TA_ROLLBACK)
            {
                if (pPkgProcContext->isRB)
					retPS = STEP_DP_RB;
            }
        }
    }
    return retPS;
}

static int GetCurPkgFLPath(PPkgProcContext pPkgProcContext)
{
#define DEF_BK_SUFFIX       "bk"
    int iRet = -1;
    PCoreContext pCoreContext = &CurCoreContext;
    if (pPkgProcContext != NULL && pPkgProcContext->pkgName != NULL &&
        pPkgProcContext->pkgType != NULL && pCoreContext->cfgParams.pkgRootPath != NULL)
    {
        char * pos = NULL;
        char * pkgType = pPkgProcContext->pkgType, *pkgName = pPkgProcContext->pkgName;
        int len = strlen(pCoreContext->cfgParams.pkgRootPath) + strlen(pkgType) + 2;
        char * curPath = (char *)malloc(len);

        memset(curPath, 0, len);
        sprintf(curPath, "%s%c%s", pCoreContext->cfgParams.pkgRootPath, FILE_SEPARATOR, pkgType);
        pPkgProcContext->pkgTypePath = curPath;

        if (!util_is_file_exist(pCoreContext->cfgParams.pkgRootPath))
			util_create_directory(pCoreContext->cfgParams.pkgRootPath);
        if (!util_is_file_exist(pPkgProcContext->pkgTypePath))
			util_create_directory(pPkgProcContext->pkgTypePath);
        len = strlen(pPkgProcContext->pkgTypePath) + strlen(pkgName) + 2;
        curPath = (char *)malloc(len);
        memset(curPath, 0, len);
        sprintf(curPath, "%s%c%s", pPkgProcContext->pkgTypePath, FILE_SEPARATOR, pkgName);
        pPkgProcContext->pkgPath = curPath;

        len = strlen(pPkgProcContext->pkgPath) + strlen(DEF_BK_SUFFIX) + 2;
        curPath = (char *)malloc(len);
        memset(curPath, 0, len);
        sprintf(curPath, "%s.%s", pPkgProcContext->pkgPath, DEF_BK_SUFFIX);
        pPkgProcContext->pkgBKPath = curPath;
        pos = strrchr(pPkgProcContext->pkgPath, '.');
        if (pos)
        {
            len = pos - pPkgProcContext->pkgPath + 1;
            curPath = (char *)malloc(len);
            memset(curPath, 0, len);
            strncpy(curPath, pPkgProcContext->pkgPath, len - 1);
            pPkgProcContext->pkgFirstUnzipPath = curPath;
			// mkdir folder if not exist
            if (!util_is_file_exist(pPkgProcContext->pkgFirstUnzipPath))
				util_create_directory(pPkgProcContext->pkgFirstUnzipPath);
        }
        if (pPkgProcContext->pkgFirstUnzipPath)
        {
            len = strlen(pPkgProcContext->pkgFirstUnzipPath) + strlen(DEF_PKG_INFO_XML_FILE_NAME) + 2;
            curPath = (char *)malloc(len);
            memset(curPath, 0, len);
            sprintf(curPath, "%s%c%s", pPkgProcContext->pkgFirstUnzipPath, FILE_SEPARATOR, DEF_PKG_INFO_XML_FILE_NAME);
            pPkgProcContext->pkgInfoXmlPath = curPath;
            iRet = 0;
        }
    }
    return iRet;
}

static int PkgFTDownload(PPkgProcContext pPkgProcContext, int * runningFlag)
{
    int retCode = SUEC_CORE_SUCCESS;
	int isDebug = 0;

    if (pPkgProcContext == NULL)
		return SUEC_CORE_I_PARAMETER_ERROR;

    {
        PCoreContext pCoreContext = &CurCoreContext;
        int iRet = 0;
        int len;
        char *realURL, *debugEnc;

		// if encode(pPkgProcContext->pkgURL) == downloadDebug, then enable debug mode
		if (pCoreContext->cfgParams.downloadDebug) {
			len = 2 * strlen(pPkgProcContext->pkgURL); // for exactly, 4*(n/3)
			debugEnc = (char*) malloc(len);
			memset(debugEnc, 0, len);
			DES_BASE64Encode(pPkgProcContext->pkgURL, debugEnc, len); //DES&BASE64 debugEnc
			if (memcmp(debugEnc, pCoreContext->cfgParams.downloadDebug, 16) == 0) {
				isDebug = 1;
			} else {
				isDebug = 0;
			}
			free(debugEnc);
		}

		// get realURL
		len = strlen(pPkgProcContext->pkgURL) + 1;
		realURL = (char*)malloc(len);
        memset(realURL, 0, len);
        pthread_mutex_lock(&pCoreContext->endecodeMutex);
        iRet = DES_BASE64Decode(pPkgProcContext->pkgURL, realURL, len); //DES&BASE64 parse url
        pthread_mutex_unlock(&pCoreContext->endecodeMutex);
        if (iRet != 0)
			retCode = SUEC_CORE_S_BS64DESDECODE_ERROR;
        else
        {
            int totalRetryCnt = 0, curRetryIndex = 0;
            unsigned retryIntervalMS = 0;
            int protocal = pPkgProcContext->dlProtocal ? pPkgProcContext->dlProtocal : FT_PROTOCOL_FTP;
            int sercurity = pPkgProcContext->dlSercurity ? pPkgProcContext->dlSercurity : FT_SECURITY_NONE;
            LOGI("Package %s download,local:%s", pPkgProcContext->pkgName, pPkgProcContext->pkgTypePath);
            totalRetryCnt = pPkgProcContext->dlRetryCnt;
            curRetryIndex = pPkgProcContext->curDLRetryCnt;
            retryIntervalMS = pCoreContext->cfgParams.dlRetryIntervalTS * 1000;
            if (curRetryIndex > 0) //Notify dl retry status
            {
                LOGI("Package %s, retry download current count:%d", pPkgProcContext->pkgName, curRetryIndex);
                pPkgProcContext->taskStatusInfo.taskAction = TA_RETRY;
                pPkgProcContext->taskStatusInfo.retryCnt = curRetryIndex;
                NotifyTaskStatus(pPkgProcContext);
                {//Per dl retry wait some time.
                    int perSleepMS = 10, i = 0, cnt = 0;
                    cnt = retryIntervalMS / 10;
                    for (i = 0; i < cnt && *runningFlag; i++) usleep(1000*perSleepMS);
                }
            }
            pPkgProcContext->ftHandle = FTNewHandle();
            if (pPkgProcContext->ftHandle == NULL)
				retCode = SUEC_CORE_S_PKG_FT_DL_ERROR;
            else
            {
                int ftRet = 0;
                ftRet = FTSet(pPkgProcContext->ftHandle, FT_OPT_DOWNLOADCALLBACK, FTTransCB, pPkgProcContext);
                if (ftRet < 0)
					retCode = SUEC_CORE_S_PKG_FT_DL_ERROR;
                else if (ftRet > 0)
					retCode = ftRet + SUEC_FT_THRPART_ERR_BASE;
                else
                {
                    ftRet = FTDownload(pPkgProcContext->ftHandle, protocal, sercurity,
                        realURL, pPkgProcContext->pkgTypePath, isDebug);
                    if (ftRet < 0)
						retCode = SUEC_CORE_S_PKG_FT_DL_ERROR;
                    else if (ftRet > 0)
						retCode = ftRet + SUEC_FT_THRPART_ERR_BASE;
                }
                FTCloseHandle(pPkgProcContext->ftHandle);
                pPkgProcContext->ftHandle = NULL;

            }
            if (curRetryIndex == totalRetryCnt) //recover taskAction
            {
                pPkgProcContext->taskStatusInfo.taskAction = TA_NORMAL;
                pPkgProcContext->taskStatusInfo.retryCnt = 0;
            }
        }
        free(realURL);
    }
    return retCode;
}

static int PkgMD5Check(char * pkgPath, int * isQuit)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pkgPath == NULL || isQuit == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        char retMd5[64] = { 0 };
        if (GetFileMD5(pkgPath, retMd5, isQuit) == 0) //calculate md5 code
        {
            char *tmpPkgPath = NULL;
            int len = strlen(pkgPath) + 1;
            tmpPkgPath = (char *)malloc(len);
            memset(tmpPkgPath, 0, len);
            strcpy(tmpPkgPath, pkgPath);
            if (strstr(cp_strlwr(tmpPkgPath), cp_strlwr(retMd5)) == NULL)
            {
                retCode = SUEC_CORE_S_MD5_NOT_MATCH;
            }
            free(tmpPkgPath);
        }
        else retCode = SUEC_CORE_S_CALC_MD5_ERROR;
    }
    return retCode;
}

static char *ltrim(char *str, const char *seps)
{
    size_t totrim;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    totrim = strspn(str, seps);
    if (totrim > 0) {
        size_t len = strlen(str);
        if (totrim == len) {
            str[0] = '\0';
        }
        else {
            memmove(str, str + totrim, len + 1 - totrim);
        }
    }
    return str;
}

static char *rtrim(char *str, const char *seps)
{
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

char *trim(char *str, const char *seps)
{
    return ltrim(rtrim(str, seps), seps);
}

static int PkgTagsCheck(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
	char *otaTags, *token, *saveptr = NULL;
	char *ptr, endchar;
	char *pkgTags;
    PCoreContext pCoreContext = &CurCoreContext;

    if (pPkgProcContext == NULL)
		return SUEC_CORE_I_PARAMETER_ERROR;

	if (pPkgProcContext->xmlPkgInfo.tags) { // has tags in xml, new policy
		otaTags = pCoreContext->tags;
		if (!otaTags)
			return SUEC_CORE_SUCCESS; // match, because we have no tags in agent

		// copy device tags string for strtok_r()
		pkgTags = (char*) malloc(strlen(pPkgProcContext->xmlPkgInfo.tags)+1);
		if (!pkgTags)
			return SUEC_CORE_S_TAGS_NOT_MATCH;
		strcpy(pkgTags, pPkgProcContext->xmlPkgInfo.tags);

		token = strtok_r(pkgTags, ",", &saveptr);
		while (token) {
			trim(token, NULL);
			// search
			ptr = strstr(otaTags, token);
			if (!ptr) { // not match, search next group
				retCode = SUEC_CORE_S_TAGS_NOT_MATCH;
				break;
			}

			// check previous char equal to ','
			if (ptr != otaTags && *(ptr-1) != ',') {
				retCode = SUEC_CORE_S_TAGS_NOT_MATCH;
				break;
			}

			// check end char equal to ',' or '\0'
			endchar = *(ptr+strlen(token));
			if (endchar != ',' && endchar != '\0') {
				retCode = SUEC_CORE_S_TAGS_NOT_MATCH;
				break;
			}

			// check next token
			token = strtok_r(NULL, ",", &saveptr);
		}
		free(pkgTags);
	}
	else  // legacy
	{
        if (pCoreContext->osName && pPkgProcContext->xmlPkgInfo.os && strlen(pPkgProcContext->xmlPkgInfo.os))//check OS
        {
            if (strcasecmp(pPkgProcContext->xmlPkgInfo.os, pCoreContext->osName) &&
                !(strstr(pPkgProcContext->xmlPkgInfo.os, pCoreContext->osName)))
            {
                retCode = SUEC_CORE_S_OS_NOT_MATCH;
            }
        }
        if (retCode == SUEC_CORE_SUCCESS)
        {
            if (pCoreContext->arch && pPkgProcContext->xmlPkgInfo.arch && strlen(pPkgProcContext->xmlPkgInfo.arch))//check Arch
            {
                if (strcasecmp(pPkgProcContext->xmlPkgInfo.arch, pCoreContext->arch) &&
                    !(strstr(pCoreContext->arch, pPkgProcContext->xmlPkgInfo.arch)))
                {
                    retCode = SUEC_CORE_S_ARCH_NOT_MATCH;
                }
            }
        }
    }

    return retCode;
}

static int GetCurPkgSLPath(PPkgProcContext pPkgProcContext)
{
    int iRet = -1;
    if (pPkgProcContext != NULL && pPkgProcContext->pkgFirstUnzipPath != NULL &&
        pPkgProcContext->xmlPkgInfo.secondZipName != NULL)
    {
        char * pos = NULL;
        int len = strlen(pPkgProcContext->pkgFirstUnzipPath) + strlen(pPkgProcContext->xmlPkgInfo.secondZipName) + 2;
        char * curPath = (char *)malloc(len);
        memset(curPath, 0, len);
        sprintf(curPath, "%s%c%s", pPkgProcContext->pkgFirstUnzipPath, FILE_SEPARATOR, pPkgProcContext->xmlPkgInfo.secondZipName);
        pPkgProcContext->pkgSecondZipPath = curPath;
        pos = strrchr(pPkgProcContext->pkgSecondZipPath, '.');
        if (pos)
        {
            len = pos - pPkgProcContext->pkgSecondZipPath + 1;
            curPath = (char *)malloc(len);
            memset(curPath, 0, len);
            strncpy(curPath, pPkgProcContext->pkgSecondZipPath, len - 1);
            pPkgProcContext->pkgSecondUnzipPath = curPath;
            if (!util_is_file_exist(pPkgProcContext->pkgSecondUnzipPath))util_create_directory(pPkgProcContext->pkgSecondUnzipPath);
        }
        if (pPkgProcContext->pkgSecondUnzipPath)
        {
            len = strlen(pPkgProcContext->pkgSecondUnzipPath) + strlen(DEF_DEPLOY_XML_FILE_NAME) + 2;
            curPath = (char *)malloc(len);
            memset(curPath, 0, len);
            sprintf(curPath, "%s%c%s", pPkgProcContext->pkgSecondUnzipPath, FILE_SEPARATOR, DEF_DEPLOY_XML_FILE_NAME);
            pPkgProcContext->pkgDeployXmlPath = curPath;
            iRet = 0;
        }
    }
    return iRet;
}

static int GetCurPkgSLPath2(PPkgProcContext pPkgProcContext)
{
	int iRet = -1;
	if (pPkgProcContext != NULL && pPkgProcContext->pkgFirstUnzipPath != NULL &&
		pPkgProcContext->xmlPkgInfo.secondZipName != NULL)
	{
		int len = strlen(pPkgProcContext->pkgFirstUnzipPath) + 1;
		char * curPath = (char *)malloc(len);
		memset(curPath, 0, len);
		sprintf(curPath, "%s",pPkgProcContext->pkgFirstUnzipPath);
		pPkgProcContext->pkgSecondUnzipPath = curPath;
		//pPkgProcContext->pkgSecondUnzipPath = pPkgProcContext->pkgFirstUnzipPath;
		if (!util_is_file_exist(pPkgProcContext->pkgSecondUnzipPath))util_create_directory(pPkgProcContext->pkgSecondUnzipPath);
		if (pPkgProcContext->pkgSecondUnzipPath)
		{
			len = strlen(pPkgProcContext->pkgSecondUnzipPath) + strlen(DEF_DEPLOY_XML_FILE_NAME) + 2;
			curPath = (char *)malloc(len);
			memset(curPath, 0, len);
			sprintf(curPath, "%s%c%s", pPkgProcContext->pkgSecondUnzipPath, FILE_SEPARATOR, DEF_DEPLOY_XML_FILE_NAME);
			pPkgProcContext->pkgDeployXmlPath = curPath;
			iRet = 0;
		}
	}
	return iRet;
}

static int PkgUnzipSL(PPkgProcContext pPkgProcContext)
{
    int retCode = SUEC_CORE_SUCCESS;
	/*
    ** fix bug(3626) realPwd is too small to be cpoied. memory overflow. 128->512
    */
    //char realPwd[128] = { 0 };
    char realPwd[512] = { 0 };
    char realPwd2[16] = { 0 };
	char* zipPwd = NULL;

    if (GetCurPkgSLPath(pPkgProcContext) != 0)
		retCode = SUEC_CORE_S_PKG_PATH_ERROR;
    else
    {   //DES&BASE64 parse unzip password
        int iRet = 0;
        PCoreContext pCoreContext = &CurCoreContext;
        pthread_mutex_lock(&pCoreContext->endecodeMutex);
		zipPwd = pPkgProcContext->xmlPkgInfo.zipPwd;
		if (zipPwd != NULL && strlen(zipPwd) > 0){
			iRet = DES_BASE64Decode(zipPwd, realPwd, sizeof(realPwd));
			if (iRet == 0) memcpy(realPwd2, realPwd, 8);
		}
        pthread_mutex_unlock(&pCoreContext->endecodeMutex);
        if (iRet != 0)
			retCode = SUEC_CORE_S_UNZIP_ERROR;
        else
        {
            //memcpy(realPwd2, realPwd, 8);
			if (strlen(realPwd2) > 0){
				if ((iRet = MiniUnzip(pPkgProcContext->pkgSecondZipPath, pPkgProcContext->pkgSecondUnzipPath, realPwd2)) != 0)
					retCode = SUEC_CORE_S_UNZIP_ERROR;
			}
			else{
				if ((iRet = MiniUnzip(pPkgProcContext->pkgSecondZipPath, pPkgProcContext->pkgSecondUnzipPath, NULL) != 0));
					retCode = SUEC_CORE_S_UNZIP_ERROR;
			}
        }
		if (iRet == 0)
		{
			char *argsFilePath = NULL;
			int argsFilePathLength = 0;
			char *start = NULL;
			char *end = NULL;
			char *temp = NULL;

			start = pPkgProcContext->xmlPkgInfo.secondZipName;
			end = strrchr(pPkgProcContext->xmlPkgInfo.secondZipName, '.');
			argsFilePathLength = end - start + 1;
			temp = (char *)malloc(argsFilePathLength);
			memset(temp, 0, argsFilePathLength);
			strncpy(temp, start, argsFilePathLength);
            temp[argsFilePathLength - 1] = '\0';

			argsFilePathLength += strlen(pPkgProcContext->pkgSecondUnzipPath);
            argsFilePathLength += 2;
			argsFilePath = (char *)malloc(argsFilePathLength);
			memset(argsFilePath, 0, argsFilePathLength);

			sprintf(argsFilePath, "%s%c%s", pPkgProcContext->pkgSecondUnzipPath, FILE_SEPARATOR, temp);
			free(temp);
			WriteSubDevicesArgs(pPkgProcContext->subDevices, pPkgProcContext->subDeviceSize, pPkgProcContext->pkgName, argsFilePath);
			free(argsFilePath);
		}
    }

    return retCode;
}

static int PkgUnzipSL2(PPkgProcContext pPkgProcContext)
{
	int retCode = SUEC_CORE_SUCCESS;

	/*
    ** fix bug(3626) realPwd is too small to be cpoied. memory overflow. 128->512
    */
    //char realPwd[128] = { 0 };
	char realPwd[512] = { 0 };
	char realPwd2[16] = { 0 };
	char* zipPwd = NULL;

	if (GetCurPkgSLPath2(pPkgProcContext) != 0) {
		retCode = SUEC_CORE_S_PKG_PATH_ERROR;
	}
	else
	{   //DES&BASE64 parse unzip password
		int iRet = 0;

		PCoreContext pCoreContext = &CurCoreContext;
		pthread_mutex_lock(&pCoreContext->endecodeMutex);
		zipPwd = pPkgProcContext->xmlPkgInfo.zipPwd;
		if (zipPwd != NULL && strlen(zipPwd) > 0){
			iRet = DES_BASE64Decode(zipPwd, realPwd, sizeof(realPwd));
			if (iRet == 0) memcpy(realPwd2, realPwd, 8);
		}
		pthread_mutex_unlock(&pCoreContext->endecodeMutex);
		if (iRet != 0) retCode = SUEC_CORE_S_UNZIP_ERROR;
		else
		{
			//memcpy(realPwd2, realPwd, 8);
			if (strlen(realPwd2) > 0){
				if ((iRet = MiniUnzip(pPkgProcContext->pkgPath, pPkgProcContext->pkgSecondUnzipPath, realPwd2)) != 0)
					retCode = SUEC_CORE_S_UNZIP_ERROR;
			}
			else{
				if ((iRet = MiniUnzip(pPkgProcContext->pkgPath, pPkgProcContext->pkgSecondUnzipPath, NULL)) != 0)
					retCode = SUEC_CORE_S_UNZIP_ERROR;
			}
		}

		if (iRet == 0)
		{
			char *argsFilePath = NULL;
			int argsFilePathLength = 0;
			char *start = NULL;
			char *end = NULL;
			char *temp = NULL;

			start = pPkgProcContext->xmlPkgInfo.secondZipName;
			end = strrchr(pPkgProcContext->xmlPkgInfo.secondZipName, '.');
			argsFilePathLength = end - start + 1;
			temp = (char *)malloc(argsFilePathLength);
			memset(temp, 0, argsFilePathLength);
			strncpy(temp, start, argsFilePathLength);
            temp[argsFilePathLength - 1] = '\0';

			argsFilePathLength += strlen(pPkgProcContext->pkgSecondUnzipPath);
            argsFilePathLength += 2;
			argsFilePath = (char *)malloc(argsFilePathLength);
			memset(argsFilePath, 0, argsFilePathLength);

			sprintf(argsFilePath, "%s%c%s", pPkgProcContext->pkgSecondUnzipPath, FILE_SEPARATOR, temp);
			free(temp);

			WriteSubDevicesArgs(pPkgProcContext->subDevices, pPkgProcContext->subDeviceSize, pPkgProcContext->pkgName, argsFilePath);
			free(argsFilePath);
		}
	}
	return retCode;
}

static int GetCurPkgTLPath(PPkgProcContext pPkgProcContext)
{
    int iRet = -1;
    if (pPkgProcContext != NULL && pPkgProcContext->pkgSecondUnzipPath != NULL &&
        pPkgProcContext->xmlDeployInfo.execFileName != NULL)
    {
        int len = strlen(pPkgProcContext->pkgSecondUnzipPath) + strlen(pPkgProcContext->xmlDeployInfo.execFileName) + 2;
        char * curPath = (char *)malloc(len);
        memset(curPath, 0, len);
        sprintf(curPath, "%s%c%s", pPkgProcContext->pkgSecondUnzipPath, FILE_SEPARATOR, pPkgProcContext->xmlDeployInfo.execFileName);
        if (pPkgProcContext->pkgDeployFilePath)
			free(pPkgProcContext->pkgDeployFilePath);
        cp_tidy_file_path(curPath);
        pPkgProcContext->pkgDeployFilePath = curPath;

        if (pPkgProcContext->xmlDeployInfo.retCheckScript != NULL && strlen(pPkgProcContext->xmlDeployInfo.retCheckScript))
        {
            len = strlen(pPkgProcContext->pkgSecondUnzipPath) + strlen(pPkgProcContext->xmlDeployInfo.retCheckScript) + 2;
            char * retCheckPath = (char *)malloc(len);
            memset(retCheckPath, 0, len);
            sprintf(retCheckPath, "%s%c%s", pPkgProcContext->pkgSecondUnzipPath, FILE_SEPARATOR, pPkgProcContext->xmlDeployInfo.retCheckScript);
            if (pPkgProcContext->pkgRetCheckScriptPath)
				free(pPkgProcContext->pkgRetCheckScriptPath);
            cp_tidy_file_path(retCheckPath);
            pPkgProcContext->pkgRetCheckScriptPath = retCheckPath;
        }
        iRet = 0;
    }
    return iRet;
}

/*
	launch deploy binary and wait, msi, exec ...
*/
static int PkgRunDeployFile(PPkgProcContext pPkgProcContext, int * runningFlag)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext->pkgDeployFilePath == NULL)
		retCode = SUEC_CORE_I_PARAMETER_ERROR;
    else
    {
        char * curDir = NULL;
        int totalRetryCnt = 0, curRetryIndex = 0;
        GetCurDirFromDPFile(pPkgProcContext->pkgDeployFilePath, &curDir);
        totalRetryCnt = pPkgProcContext->dpRetryCnt;
        curRetryIndex = pPkgProcContext->curDPRetryCnt;
        if (curRetryIndex > 0)
        {
            pPkgProcContext->taskStatusInfo.taskAction = TA_RETRY;
            pPkgProcContext->taskStatusInfo.retryCnt = curRetryIndex;
            NotifyTaskStatus(pPkgProcContext);
            {//Per dp retry wait some time.
                int perSleepMS = 10, i = 0, cnt = 0;
                cnt = 2000 / 10;
                for (i = 0; i < cnt && *runningFlag; i++) usleep(1000*perSleepMS);
            }
        }
        {
            PCoreContext pCoreContext = (PCoreContext)&CurCoreContext;
            PPkgDpCheckCBInfo pTmpDPCBInfo = NULL;
            CP_PID_T pidT = 0;

            retCode = StartDPCheckThread(pPkgProcContext, curDir, &pTmpDPCBInfo);
            if (retCode == SUEC_CORE_SUCCESS)
            {
                pidT = SUECLaunchExeFile(pPkgProcContext, curDir);
                //launch failed
                if (pidT == 0)
					retCode = SUEC_CORE_S_DP_EXEC_FAILED;
                else //launch success
                {
                    int processExitCode = 0;
                    int waitRet = 0;
                    unsigned int maxWaitMs = pCoreContext->cfgParams.deployWaitTM * 60 * 1000;
                    unsigned int curWaitMs = 0;
                    unsigned int perWaitMs = 200;
                    do
                    {
                        usleep(1000*10);
                        waitRet = cp_wait_pid(pidT, &processExitCode, perWaitMs);
                        if (waitRet < 0)
							retCode = SUEC_CORE_S_DP_EXEC_FAILED;
                        if (waitRet == 0 && processExitCode > 0)
                        {
                            retCode = SUEC_DP_THRPART_ERR_BASE + processExitCode;
                        }
                        if (waitRet > 0)
                        {
                            curWaitMs += perWaitMs;
                            if (curWaitMs >= maxWaitMs) //wait outtime kill process
                            {
                                cp_kill_pid(pidT);
                                retCode = SUEC_CORE_S_DP_TIMEOUT;
                            }
                        }
                    } while (retCode == SUEC_CORE_SUCCESS &&
                        waitRet && *runningFlag);
                    cp_close_pid(pidT);
                }

                while (pPkgProcContext->isDPCheckThreadRunning && *runningFlag)
					usleep(1000*10); //wait deploy procedure check thread quit.
                //dhl tmp comment
                //StopDPCheckThread(pPkgProcContext);
            }
            if (pTmpDPCBInfo)
				free(pTmpDPCBInfo);
        }
        if (curRetryIndex == totalRetryCnt)
        {
            pPkgProcContext->taskStatusInfo.taskAction = TA_NORMAL;
            pPkgProcContext->taskStatusInfo.retryCnt = 0;
        }

        free(curDir);
    }

    return retCode;
}

static void PkgRebootSys(PPkgProcContext pPkgProcContext)
{
#define TRIGER_REBOOT_ERR_CODE  (SUEC_THRPART_ERR_BASE + 99999)
    if (pPkgProcContext->xmlDeployInfo.rebootFlag || pPkgProcContext->errCode == TRIGER_REBOOT_ERR_CODE)
    {
        usleep(1000*2000);
        LOGD("Sys reboot and sue quit.");
        util_power_restart();
        exit(0);
	}
}

static int PkgCheckDPRet(PPkgProcContext pPkgProcContext, int * runningFlag)
{
    int retCode = pPkgProcContext->errCode;;
    if (pPkgProcContext->pkgRetCheckScriptPath && strlen(pPkgProcContext->pkgRetCheckScriptPath))
    {
        char * curDir = NULL;
        GetCurDirFromDPFile(pPkgProcContext->pkgRetCheckScriptPath, &curDir);
        retCode = RunRetCheckScript(pPkgProcContext->pkgRetCheckScriptPath, runningFlag, curDir);
        if (curDir) free(curDir);
    }
    return retCode;
}

static void PkgRecordSWInfo(PPkgProcContext pPkgProcContext, int * runningFlag)
{
    if (pPkgProcContext)
    {
        int iRet = 0;
        char * curDir = NULL;
        GetCurDirFromDPFile(pPkgProcContext->pkgDeployFilePath, &curDir);
        iRet = SUEPkgDeployCheckCB(pPkgProcContext, NotifyDPCheckMsg, runningFlag, curDir);
        if (iRet != 0) //Get failed then use package info instead of software info.
        {
            SWInfo swInfo;
            memset(&swInfo, 0, sizeof(SWInfo));
            swInfo.name = pPkgProcContext->pkgType;
            swInfo.version = pPkgProcContext->xmlPkgInfo.version;
            if (pPkgProcContext->errCode == SUEC_CORE_SUCCESS)
				swInfo.usable = 1;
            else
				swInfo.usable = 0;
            ReplaceSWInfo(&swInfo);
        }
        if (pPkgProcContext->errCode == SUEC_CORE_SUCCESS) {
			ReplacePkgInfo(pPkgProcContext->pkgType, pPkgProcContext->xmlPkgInfo.version);
		}
        free(curDir);
    }
}

static int PkgDPRollback(PPkgProcContext pPkgProcContext, int * runningFlag)
{
#define DEF_BK_FILE_WILDCARD   "*.bk"
    int retCode = SUEC_CORE_SUCCESS;
    {
        ClearPkgPath(pPkgProcContext);
        //when deploy failed and then rollback,report a status with deploying error code.
        pPkgProcContext->taskStatusInfo.errCode = pPkgProcContext->errCode;
        NotifyTaskStatus(pPkgProcContext);
        pPkgProcContext->taskStatusInfo.errCode = SUEC_CORE_SUCCESS;
    }
    {
        char * bkPkgFilePath = NULL;
        pPkgProcContext->taskStatusInfo.taskAction = TA_ROLLBACK;
        NotifyTaskStatus(pPkgProcContext); //Notify rollback start status
        if (pPkgProcContext->pkgTypePath && util_is_file_exist(pPkgProcContext->pkgTypePath))
        {
            //Find bk file
            util_file_iterate(pPkgProcContext->pkgTypePath, DEF_BK_FILE_WILDCARD, 1, 1,
                FindPkgBKFileCB, &bkPkgFilePath);
        }
        if (bkPkgFilePath == NULL)
			retCode = SUEC_CORE_S_BK_FILE_NOT_FOUND;
        else
        {
            char * bkZipFilePath = NULL;
            int len = strlen(bkPkgFilePath) + 1;
            bkZipFilePath = (char *)malloc(len);
            memset(bkZipFilePath, 0, len);
            //Get unzip target path
            memcpy(bkZipFilePath, bkPkgFilePath, len - 4);
            if (util_copy_file(bkPkgFilePath, bkZipFilePath) == 0) {
				retCode = SUEC_CORE_S_DP_ROLLBACK_ERROR;
            } else
            {
                char * pos = NULL;
                PPkgProcContext pRBPkgProcContext = NULL;
                pRBPkgProcContext = CreatePkgProcContext();
                len = strlen(pPkgProcContext->pkgType) + 1;
                pRBPkgProcContext->pkgType = (char*)malloc(len);
                memset(pRBPkgProcContext->pkgType, 0, len);
                strcpy(pRBPkgProcContext->pkgType, pPkgProcContext->pkgType);
                pos = strrchr(bkZipFilePath, FILE_SEPARATOR);
                if (pos == NULL) {
					retCode = SUEC_CORE_S_DP_ROLLBACK_ERROR;
                } else
                {
                    //Get rb pkg name
                    len = strlen(bkZipFilePath) - (pos - bkZipFilePath) + 1;
                    pRBPkgProcContext->pkgName = (char *)malloc(len);
                    memset(pRBPkgProcContext->pkgName, 0, len);
                    strcpy(pRBPkgProcContext->pkgName, pos + 1);
                    {
                        //Rollback report the target package name.
                        pPkgProcContext->taskStatusInfo.u.msg = pRBPkgProcContext->pkgName;
                        NotifyTaskStatus(pPkgProcContext);
                        pPkgProcContext->taskStatusInfo.u.msg = NULL;
                    }
                }
                pRBPkgProcContext->pkgStep = STEP_INIT;
                pPkgProcContext->rbParam = pRBPkgProcContext;
				LOGD("Do rollback [%s]", bkZipFilePath);
                retCode = PkgStepByStep(pPkgProcContext);
            }
            free(bkZipFilePath);
            free(bkPkgFilePath);
        }
    }
    return retCode;
}

static CP_PID_T SUECLaunchExeFile(PPkgProcContext pPkgProcContext, char * curDir)
{
    CP_PID_T pidT = 0;
    if (pPkgProcContext != NULL && curDir != NULL)
    {
        int len = 0;
        //launch deploy file
        LOGI("Package %s deploy launch:%s!", pPkgProcContext->pkgName,
            pPkgProcContext->pkgDeployFilePath);
        if (pPkgProcContext->xmlPkgInfo.installerTool &&
            strcasecmp(pPkgProcContext->xmlPkgInfo.installerTool, DEF_UNKNOW_INSTALLER_STR))
        {
            char * tmpArgvStr = NULL;
            char * realAppPath = NULL;
            char * argsStr = NULL;
            if (!strcasecmp(pPkgProcContext->xmlPkgInfo.installerTool, DEF_INNO_SETUP_STR))
            {
                tmpArgvStr = DEF_INNO_INSTALLER_ARGS;
            }
            else if (!strcasecmp(pPkgProcContext->xmlPkgInfo.installerTool, DEF_ADVANCED_INSTALLER_STR))
            {
                tmpArgvStr = DEF_ADVC_INSTALLER_ARGS;
            }
            else
            {
                //add other code
            }
            if (FileSuffixMatch(pPkgProcContext->pkgDeployFilePath, "msi"))
            {
                len = strlen(MSI_EXE_APP) + 1;
                realAppPath = (char *)malloc(len);
                memset(realAppPath, 0, len);
                strcpy(realAppPath, MSI_EXE_APP);

                len = strlen(pPkgProcContext->pkgDeployFilePath) + 8;
                argsStr = (char *)malloc(len);
                memset(argsStr, 0, len);
                sprintf(argsStr, " /i \"%s\" ", pPkgProcContext->pkgDeployFilePath);
                if (tmpArgvStr)
                {
                    len = strlen(tmpArgvStr) + strlen(argsStr) + 1;
                    argsStr = (char *)realloc(argsStr, len);
                    strcpy(argsStr + (len - strlen(tmpArgvStr) - 1), tmpArgvStr);
                }
            }
            else if (FileSuffixMatch(pPkgProcContext->pkgDeployFilePath, "exe"))
            {
                len = strlen(pPkgProcContext->pkgDeployFilePath) + 1;
                realAppPath = (char *)malloc(len);
                memset(realAppPath, 0, len);
                strcpy(realAppPath, pPkgProcContext->pkgDeployFilePath);
                if (tmpArgvStr)
                {
                    len = strlen(tmpArgvStr) + 1;
                    argsStr = (char *)malloc(len);
                    memset(argsStr, 0, len);
                    strcpy(argsStr, tmpArgvStr);
                }
            }
            else
            {
                //add other code
            }
            if (realAppPath)
            {
                //pidT = cp_process_launch(realAppPath, argsStr, curDir);
                pidT = cp_process_launch_as_user(realAppPath, argsStr, curDir);
                free(realAppPath);
            }
            if (argsStr) free(argsStr);
        }
        else
        {
            //pidT = cp_process_launch(pPkgProcContext->pkgDeployFilePath, NULL, curDir);
            pidT = cp_process_launch_as_user(pPkgProcContext->pkgDeployFilePath, NULL, curDir);
        }
    }
    return pidT;
}

static int RunRetCheckScript(char * retCheckScriptPath, int * isRunning, char * curDir)
{
#define DEF_TIMEOUT_MT   5
    int retCode = SUEC_CORE_SUCCESS;
    if (retCheckScriptPath && strlen(retCheckScriptPath))
    {
        CP_PID_T pidT = 0;
        pidT = cp_process_launch_as_user(retCheckScriptPath, NULL, curDir);
        //launch failed
        if (pidT == 0) {
			retCode = SUEC_CORE_S_RETCHECK_EXEC_ERROR;
			LOGD("cp_process_launch_as_user(%s) fail", retCheckScriptPath);
		}
        else //launch success
        {
            int processExitCode = 0;
            int waitRet = 0;
            unsigned int maxWaitMs = DEF_TIMEOUT_MT * 60 * 1000;
            unsigned int curWaitMs = 0;
            unsigned int perWaitMs = 200;
            do
            {
                usleep(1000*10);
                waitRet = cp_wait_pid(pidT, &processExitCode, perWaitMs);
                if (waitRet < 0) {
					retCode = SUEC_CORE_S_RETCHECK_EXEC_ERROR;
				}
                if (waitRet == 0 && processExitCode > 0)
                {
                    retCode = SUEC_DP_THRPART_ERR_BASE + processExitCode;
                }
                if (waitRet > 0)
                {
                    curWaitMs += perWaitMs;
                    if (curWaitMs >= maxWaitMs) //wait timeout kill process
                    {
                        cp_kill_pid(pidT);
                        retCode = SUEC_CORE_S_RETCHECK_EXEC_TIMEOUT;
                    }
                }
            } while (retCode == SUEC_CORE_SUCCESS &&
                waitRet && *isRunning);
            cp_close_pid(pidT);
        }
    }
    return retCode;
}

static int StartDPCheckThread(PPkgProcContext pPkgProcContext, char * curDir, PPkgDpCheckCBInfo * ppTmpCBInfo)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pPkgProcContext == NULL && curDir == NULL && ppTmpCBInfo != NULL) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        PCoreContext pCoreContext = (PCoreContext)&CurCoreContext;
        PPkgDpCheckCBInfo pTmpDPCBInfo = NULL;
        PPkgDpCheckCBInfo pCurDPCBInfo = NULL;
        PLNode pLNode = NULL;
        ClearDPLogFile(curDir);
        pLNode = LHFindNode(pCoreContext->pkgDpCheckCBList, MatchPkgDpCheckCBInfo, pPkgProcContext->pkgType);
        if (pLNode)
			pCurDPCBInfo = (PPkgDpCheckCBInfo)pLNode->pUserData;
        if (pCurDPCBInfo == NULL)//if not found then use default CB
        {
            /*pTmpDPCBInfo = (PPkgDpCheckCBInfo)malloc(sizeof(PkgDpCheckCBInfo));
            memset(pTmpDPCBInfo, 0, sizeof(PkgDpCheckCBInfo));
            pTmpDPCBInfo->dpCheckCB = DefDeployCheckCB;
            pTmpDPCBInfo->pkgType = pPkgProcContext->pkgType;
            pTmpDPCBInfo->dpCheckUserData = curDir;
            pCurDPCBInfo = pTmpDPCBInfo;*/
        }
        if (pCurDPCBInfo && pCurDPCBInfo->dpCheckCB)
        {
            pPkgProcContext->pPkgDpCheckCBInfo = pCurDPCBInfo;
            pPkgProcContext->isDPCheckThreadRunning = 1;
            // Create deploy status check thread
            if (pthread_create(&pPkgProcContext->dpCheckThreadT, NULL, DPCheckThreadStart, pPkgProcContext) != 0)
            {
                pPkgProcContext->isDPCheckThreadRunning = 0;
                retCode = SUEC_CORE_S_DP_EXEC_FAILED;
            }
        }
        *ppTmpCBInfo = pTmpDPCBInfo;
    }
    return retCode;
}

/*
static void StopDPCheckThread(PPkgProcContext pPkgProcContext)
{
    if (pPkgProcContext != NULL && pPkgProcContext->isDPCheckThreadRunning)
    {
        pPkgProcContext->isDPCheckThreadRunning = 0;
        pthread_join(pPkgProcContext->dpCheckThreadT);
        pPkgProcContext->dpCheckThreadT = 0;
    }
}
*/

static void* DPCheckThreadStart(void *args)
{
    PPkgProcContext pPkgProcContext = (PPkgProcContext)(args);
    if (pPkgProcContext)
    {
        if (pPkgProcContext->pPkgDpCheckCBInfo)
        {
            DeployCheckCB curDPCheckCB = (DeployCheckCB)pPkgProcContext->pPkgDpCheckCBInfo->dpCheckCB;
            void * chechHandle = pPkgProcContext;
            if (pPkgProcContext->isRB && pPkgProcContext->rbParam != NULL) chechHandle = pPkgProcContext->rbParam;
            if (curDPCheckCB(chechHandle, NotifyDPCheckMsg, &pPkgProcContext->isDPCheckThreadRunning,
                pPkgProcContext->pPkgDpCheckCBInfo->dpCheckUserData) != 0)
            {
                pthread_mutex_lock(&pPkgProcContext->dataMutex);
                pPkgProcContext->taskStatusInfo.statusCode = SC_ERROR;
                pPkgProcContext->taskStatusInfo.errCode = SUEC_CORE_S_DP_EXEC_FAILED;
                pthread_mutex_unlock(&pPkgProcContext->dataMutex);
            }
        }
        pPkgProcContext->isDPCheckThreadRunning = 0;
    }
    return 0;
}

static int NotifyDPCheckMsg(const void * const checkHandle, char * msg, unsigned int msgLen)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (checkHandle == NULL || msg == NULL || msgLen != strlen(msg) + 1) return SUEC_CORE_I_PARAMETER_ERROR;
    {
        PPkgProcContext pPkgProcContext = (PPkgProcContext)(checkHandle);
        pthread_mutex_lock(&pPkgProcContext->dataMutex);
        if (pPkgProcContext->taskStatusInfo.taskType == TASK_DP && pPkgProcContext->taskStatusInfo.statusCode == SC_DOING)
        {
            pPkgProcContext->taskStatusInfo.u.msg = (char *)malloc(msgLen + 1);
            memset(pPkgProcContext->taskStatusInfo.u.msg, 0, msgLen + 1);
            memcpy(pPkgProcContext->taskStatusInfo.u.msg, msg, msgLen);
            LOGI("Package %s deploy check msg:%s!", pPkgProcContext->pkgName,
                pPkgProcContext->taskStatusInfo.u.msg);
            NotifyTaskStatus(pPkgProcContext);
            UpgSWVersionInterceptor(pPkgProcContext->taskStatusInfo.u.msg, pPkgProcContext->pkgType);
            free(pPkgProcContext->taskStatusInfo.u.msg);
            pPkgProcContext->taskStatusInfo.u.msg = NULL;
        }
        else  retCode = SUEC_CORE_I_PKG_STATUS_NOT_FIT;
        pthread_mutex_unlock(&pPkgProcContext->dataMutex);
    }
    return retCode;
}

static void UpgSWVersionInterceptor(char * msg, char * pkgType)
{
#define DEF_VERSION_SUCCESS_INFO_MATCH_STR   "[result]Success"
#define DEF_VERSION_FAILED_INFO_MATCH_STR   "[result]Failed"
    if (msg != NULL)
    {
        int usable = -1;
        if (CheckIsDPSuccessMsg(msg, DEF_VERSION_SUCCESS_INFO_MATCH_STR)) usable = 1;
        if (CheckIsDPSuccessMsg(msg, DEF_VERSION_FAILED_INFO_MATCH_STR)) usable = 0;
        if (usable == 0 || usable == 1)
        {
            PSWInfo pSWInfo = NULL;
            ParseSWInfoFromDPRetMsg(msg, &pSWInfo);
            if (pSWInfo != NULL)
            {
                pSWInfo->usable = usable;
                ReplaceSWInfo(pSWInfo);
                //Try delete the package version info from swinfo table
                if (pkgType != NULL && strcasecmp(pkgType, pSWInfo->name))
                {
                    DeleteOneSWInfo(pkgType);
                }
                if (pSWInfo->name) free(pSWInfo->name);
                pSWInfo->name = NULL;
                if (pSWInfo->version) free(pSWInfo->version);
                pSWInfo->version = NULL;
                free(pSWInfo);
                pSWInfo = NULL;
            }
        }
    }
}

static int CheckIsDPSuccessMsg(const char * str, const char * subStr)
{
    int iRet = 0;
    if (str != NULL && subStr != NULL)
    {
        const char * curPos = NULL;
        char * tmpStr = NULL, *tmpSubSr = NULL, *ret = NULL;;
        int len = 0, i = 0, j = 0;
        len = strlen(str) + 1;
        tmpStr = (char *)malloc(len);
        memset(tmpStr, 0, len);
        curPos = str;
        for (i = 0; i < len; i++)
        {
            if (*(curPos + i) == '\x20') continue;
            *(tmpStr + j) = tolower(*(curPos + i));
            j++;
        }
        len = strlen(subStr) + 1;
        tmpSubSr = (char *)malloc(len);
        memset(tmpSubSr, 0, len);
        curPos = subStr;
        j = 0;
        for (i = 0; i < len; i++)
        {
            if (*(curPos + i) == '\x20') continue;
            *(tmpSubSr + j) = tolower(*(curPos + i));
            j++;
        }
        ret = strstr(tmpStr, tmpSubSr);
        if (ret != NULL) iRet = 1;
        free(tmpStr);
        free(tmpSubSr);
    }
    return iRet;
}

static void ParseSWInfoFromDPRetMsg(char * dpRetMsg, PSWInfo * ppSWInfo)
{
    if (dpRetMsg != NULL && ppSWInfo != NULL)
    {
        int len = 0;
        char * tmpName = NULL, *tmpVersion = NULL;
        char *posS = NULL, *posF = NULL, *posL = NULL;
        posS = dpRetMsg;
        posF = strchr(posS, '[');
        posL = strchr(posS, ']');
        if (posF != NULL && posL != NULL)
        {
            len = posL - posF;
            tmpName = (char *)malloc(len);
            memset(tmpName, 0, len);
            memcpy(tmpName, posF + 1, len - 1);
        }
        posS = posL + 1;
        posF = strchr(posS, '[');
        posL = strchr(posS, ']');
        if (posF != NULL && posL != NULL)
        {
            len = posL - posF;
            tmpVersion = (char *)malloc(len);
            memset(tmpVersion, 0, len);
            memcpy(tmpVersion, posF + 1, len - 1);
        }
        if (tmpVersion != NULL && tmpName != NULL)
        {
            PSWInfo pTmpSWInfo = (PSWInfo)malloc(sizeof(SWInfo));
            memset(pTmpSWInfo, 0, sizeof(SWInfo));
            pTmpSWInfo->name = tmpName;
            pTmpSWInfo->version = tmpVersion;
            *ppSWInfo = pTmpSWInfo;
        }
        else
        {
            if (tmpVersion)free(tmpVersion);
            if (tmpName) free(tmpName);
        }
    }
}

static int DES_BASE64Decode(char * srcBuf, char *destBuf, int destLen)
{
    int iRet = -1;
	char *plaintext = NULL;

    if (srcBuf == NULL || destBuf == NULL) return iRet;
    {
        char *base64Dec = NULL;
        int decLen = 0;
        int len = strlen(srcBuf);
        iRet = Base64Decode(srcBuf, len, &base64Dec, &decLen);
        if (iRet == 0)
        {
			plaintext = (char*) malloc(destLen);
            iRet = DESDecodeEx(DEF_DES_KEY, DEF_DES_KEY, base64Dec, decLen, plaintext);
            if (iRet == 0)
            {
                len = strlen(plaintext);
                /*
                ** 根据实际长度判断需要拷贝多少个字节
                */
                //memcpy(destBuf, plaintext, len + 1);
                if(len > 0 && destLen > len+1) {
                    memcpy(destBuf, plaintext, len + 1);
                }else{
                    LOGI("DES_BASE64Decode: buffer is not enough: %d, %d", len+1, destLen);
                }
            }
			if (plaintext)
				free(plaintext);
        }
        if (base64Dec)
			free(base64Dec);
    }

    return iRet;
}

static int DES_BASE64Encode(char * srcBuf, char *destBuf, int destLen)
{
    int iRet = -1;
    int cipherLen = 0;
	char* ciphertext;
    if (srcBuf == NULL || destBuf == NULL)
		return iRet;

	cipherLen = destLen;
	ciphertext = (char*) malloc(cipherLen);
    iRet = DESEncodeEx(DEF_DES_KEY, DEF_DES_KEY, srcBuf, ciphertext, &cipherLen);
    if (iRet == 0)
    {
        char *base64Enc = NULL;
        int encLen = 0;
        int len = cipherLen;
        iRet = Base64Encode(ciphertext, len, &base64Enc, &encLen);
        if (iRet == 0)
        {
            len = strlen(base64Enc);
            if (len > 0 && destLen > len+1) {
                memcpy(destBuf, base64Enc, len + 1);
            } else {
				LOGI("DES_BASE64Encode: buffer is not enough: %d, %d", len+1, destLen);
			}
        }
        if (base64Enc) free(base64Enc);
    }
	if (ciphertext) {
		free(ciphertext);
	}
    return iRet;
}

static int GetFileMD5(char * filePath, char * retMD5Str, int * runFlag)
{
#define  DEF_MD5_SIZE                16
#define  DEF_PER_MD5_DATA_SIZE       512
    int iRet = -1;
    if (NULL == filePath || NULL == retMD5Str || NULL == runFlag) return iRet;
    {
        FILE *fptr = NULL;
        fptr = fopen(filePath, "rb");
        if (fptr)
        {
            MD5_CTX context;
            unsigned char retMD5[DEF_MD5_SIZE] = { 0 };
            char dataBuf[DEF_PER_MD5_DATA_SIZE] = { 0 };
            unsigned int readLen = 0, realReadLen = 0;
            MD5Init(&context);
            readLen = sizeof(dataBuf);
            while ((realReadLen = fread(dataBuf, sizeof(char), readLen, fptr)) != 0 && *runFlag)
            {
                MD5Update(&context, dataBuf, realReadLen);
                memset(dataBuf, 0, sizeof(dataBuf));
                realReadLen = 0;
                readLen = sizeof(dataBuf);
            }
            MD5Final(retMD5, &context);

            if (*runFlag)
            {
                char md5str0x[DEF_MD5_SIZE * 2 + 1] = { 0 };
                int i = 0;
                for (i = 0; i < DEF_MD5_SIZE; i++)
                {
                    sprintf(&md5str0x[i * 2], "%.2x", retMD5[i]);
                }
                strcpy(retMD5Str, md5str0x);
                iRet = 0;
            }
            fclose(fptr);
        }
    }
    return iRet;
}

static int FileSuffixMatch(char * fileName, char * suffix)
{
    int iRet = 0;
    if (NULL == fileName || suffix == NULL) return iRet;
    {
        char * pos = strrchr(fileName, '.');
        if (pos)
        {
            if (!strcasecmp(pos + 1, suffix))
            {
                iRet = 1;
            }
        }
    }
    return iRet;
}

static void FTTransCB(void *userData, int64_t totalSize, int64_t nowSize, int64_t averageSpeed)
{
    if (userData != NULL && totalSize > 0 && nowSize >= 0)
    {
        PPkgProcContext pPkgProcContext = (PPkgProcContext)userData;
        if (pPkgProcContext->taskStatusInfo.taskType == TASK_DL &&
            pPkgProcContext->taskStatusInfo.statusCode == SC_DOING)
        {
            pPkgProcContext->taskStatusInfo.u.dlPercent = (int)(nowSize * 100 / totalSize);
            NotifyTaskStatus(pPkgProcContext);
        }
    }
}

static int ClearPkgPath(PPkgProcContext pPkgProcContext)
{
	LOGD("ClearPkgPath, pkgType=[%s], pkgFirstUnzipPath=[%s], pkgPath=[%s]", pPkgProcContext->pkgType, pPkgProcContext->pkgFirstUnzipPath, pPkgProcContext->pkgPath);
    if (pPkgProcContext && strcasecmp(pPkgProcContext->pkgType, DEF_SELF_PKG_TYPE))
    {
        usleep(1000*500);
        if (pPkgProcContext->pkgFirstUnzipPath)util_rm_dir(pPkgProcContext->pkgFirstUnzipPath);
        if (pPkgProcContext->pkgPath) {
			remove(pPkgProcContext->pkgPath);
		}
    }
    return SUEC_CORE_SUCCESS;
}

static void GetCurDirFromDPFile(char * dpFilePath, char ** ppCurDir)
{
    char * curDir = NULL;
    char * pos = NULL;
    int len = 0;
    char * pos1 = strrchr(dpFilePath, '\\');
    char * pos2 = strrchr(dpFilePath, '/');
    pos = pos2 > pos1 ? pos2 : pos1;
    len = pos - dpFilePath + 1;
    curDir = (char*)malloc(len);
    memset(curDir, 0, len);
    memcpy(curDir, dpFilePath, len - 1);
    *ppCurDir = curDir;
}

static int FindPkgBKFileCB(unsigned int depth, const char * filePath, void * userData)
{
    if (filePath != NULL && userData != NULL)
    {
        char ** userPath = (char **)userData;
        int len = strlen(filePath) + 1;
        *userPath = (char *)malloc(len);
        memset(*userPath, 0, len);
        strcpy(*userPath, filePath);
    }
    return 0;
}

/*
	send download/deploy task status or error code to server
*/
static void NotifyTaskStatus(PPkgProcContext pPkgProcContext)
{
    PCoreContext pCoreContext = &CurCoreContext;
    if (pCoreContext->taskStatusCB)
    {
        if (pPkgProcContext->taskStatusInfo.pkgName == NULL)
        {
            int len = strlen(pPkgProcContext->pkgName) + 1;
            pPkgProcContext->taskStatusInfo.pkgName = (char *)malloc(len);
            memset(pPkgProcContext->taskStatusInfo.pkgName, 0, len);
            strcpy(pPkgProcContext->taskStatusInfo.pkgName, pPkgProcContext->pkgName);
        }
        pCoreContext->taskStatusCB(&pPkgProcContext->taskStatusInfo, pCoreContext->taskUserData); // TaskStatusInfoCB
    }
}

static void LHFreeSWInfoCB(void * pUserData)
{
    if (pUserData != NULL)
    {
        PSWInfo pSWInfo = (PSWInfo)pUserData;
        if (pSWInfo->name) free(pSWInfo->name);
        pSWInfo->name = NULL;
        if (pSWInfo->version) free(pSWInfo->version);
        pSWInfo->version = NULL;
        free(pSWInfo);
    }
}

static int MatchPkgProcContextN(void * pUserData, void * key)
{
    int iRet = 0;
    if (pUserData != NULL && key != NULL)
    {
        char * curPkgName = (char*)key;
        PPkgProcContext curContext = (PPkgProcContext)pUserData;
        if (curContext->pkgName && !strcmp(curPkgName, curContext->pkgName))
        {
            iRet = 1;
        }
    }
    return iRet;
}

static int MatchPkgProcContextT(void * pUserData, void * key)
{
    int iRet = 0;
    if (pUserData != NULL && key != NULL)
    {
        char * curPkgName = (char*)key;
        PPkgProcContext curContext = (PPkgProcContext)pUserData;
        if (curContext->pkgType && !strcmp(curPkgName, curContext->pkgType))
        {
            iRet = 1;
        }
    }
    return iRet;
}

static int MatchPkgDpCheckCBInfo(void * pUserData, void * key)
{
	int iRet = 0;
	if(pUserData != NULL && key != NULL)
	{
		char * curPkgType = (char*)key;
		PPkgDpCheckCBInfo curInfo = (PPkgDpCheckCBInfo)pUserData;
		if(curInfo->pkgType && !strcmp(curPkgType, curInfo->pkgType))
		{
			iRet = 1;
		}
	}
	return iRet;
}

static void FreePkgDpCheckCBInfo(void * pUserData)
{
    if (pUserData)
    {
        PPkgDpCheckCBInfo curInfo = (PPkgDpCheckCBInfo)pUserData;
        if (curInfo)
        {
            if (curInfo->pkgType)
            {
                free(curInfo->pkgType);
            }
            curInfo->pkgType = NULL;
            curInfo->dpCheckCB = NULL;
            free(curInfo);
            curInfo = NULL;
        }
    }
}

static void FreePkgProcContext(void * pUserData)
{
    if (pUserData)
    {
        PPkgProcContext curContext = (PPkgProcContext)pUserData;
        DestroyPkgProcContext(curContext);
        pUserData = NULL;
    }
}

static void LHFreeDelPkgRetInfoCB(void * pUserData)
{
    if (pUserData != NULL)
    {
        PDelPkgRetInfo pDelPkgRetInfo = (PDelPkgRetInfo)pUserData;
        if (pDelPkgRetInfo->name) free(pDelPkgRetInfo->name);
        pDelPkgRetInfo->name = NULL;
        free(pDelPkgRetInfo);
    }
}

static void ClearDPLogFile(char * curDir)
{
    if (curDir != NULL)
    {
        char * logFilePath = NULL;
        int len = strlen(curDir) + strlen(DEF_DP_LOG_FILE_NAME) + 2;
        logFilePath = (char *)malloc(len);
        memset(logFilePath, 0, len);
        sprintf(logFilePath, "%s%c%s", curDir, FILE_SEPARATOR, DEF_DP_LOG_FILE_NAME);
        if (util_is_file_exist(logFilePath)) {
			remove(logFilePath);
		}
        free(logFilePath);
    }
}

static int CheckPkgRootPathObtainDPTask(unsigned int depth, const char * filePath, void * userData)
{
    PCoreContext pCoreContext = &CurCoreContext;
    if (filePath != NULL && userData != NULL)
    {
        PDPTaskInfo pDPTaskInfo = (PDPTaskInfo)userData;
        int isMatchFlag = 0;
        char * pkgName = NULL, *pkgType = NULL, *tmpStr = NULL;
        const char * curPos = filePath + strlen(filePath), *startPos = NULL, *endPos = curPos;
        int len = 0;
        while (curPos != filePath)
        {
            if (*curPos == FILE_SEPARATOR)
            {
                if (startPos != NULL) endPos = startPos - 2;
                startPos = curPos + 1;
                len = endPos - startPos + 2;
                tmpStr = (char *)malloc(len);
                memset(tmpStr, 0, len);
                memcpy(tmpStr, startPos, len - 1);
                if (pkgName == NULL) pkgName = tmpStr;
                else
                {
                    pkgType = tmpStr;
                    break;
                }
            }
            curPos--;
        }
        if (pkgName == NULL || pkgType == NULL) goto done1;

        if (pDPTaskInfo->pkgName)
        {
            if (strcasecmp(pDPTaskInfo->pkgName, pkgName) == 0) isMatchFlag = 1;
        }
        else if (pDPTaskInfo->pkgType)
        {
            if (strcasecmp(pDPTaskInfo->pkgType, pkgType) == 0) isMatchFlag = 1;
        }
        else
			isMatchFlag = 1;

        if (isMatchFlag)
        {
            PPkgProcContext pPkgProcContext = NULL;
            PPkgProcContext pFakeDLFPkgContext = NULL; // Fake download finished package context
            PLNode findNode = NULL;
            pthread_mutex_lock(&pCoreContext->pkgProcContextListMutex);
            findNode = LHFindNode(pCoreContext->pkgProcContextList, MatchPkgProcContextN, pkgName);
            if (findNode) //Related dp task object exist in context list
            {
                pPkgProcContext = (PPkgProcContext)findNode->pUserData;
                pthread_mutex_lock(&pPkgProcContext->dataMutex);
                if (pPkgProcContext->thenDP == 0) //if thenDP flag is 0, the set 1
                {
                    pPkgProcContext->thenDP = 1;
                    if (pPkgProcContext->dpRetryCnt == 0) pPkgProcContext->dpRetryCnt = pDPTaskInfo->dpRetry;
                    if (pPkgProcContext->isRB == 0) pPkgProcContext->isRB = pDPTaskInfo->isRollBack;
                    LOGI("Package %s deploy task add!", pPkgProcContext->pkgName);
                }
                if (pPkgProcContext->taskStatusInfo.taskType == TASK_DL && pPkgProcContext->taskStatusInfo.statusCode == SC_FINISHED)
                { //if current pkg download finished the change status to deploy queue;
                    pPkgProcContext->taskStatusInfo.u.msg = NULL;
                    pPkgProcContext->pkgStep = STEP_CHECK_DPF;
                    pFakeDLFPkgContext = pPkgProcContext;
                }
                if (pPkgProcContext->taskStatusInfo.taskType == TASK_DP &&
                    pPkgProcContext->taskStatusInfo.statusCode == SC_SUSPEND)
                {
                    pPkgProcContext->taskStatusInfo.statusCode = pPkgProcContext->suspendSCCode;
                    LOGI("Package %s deploy resume,status:%d!",
                        pPkgProcContext->pkgName, pPkgProcContext->taskStatusInfo.statusCode);
                    NotifyTaskStatus(pPkgProcContext);
                }
                pthread_mutex_unlock(&pPkgProcContext->dataMutex);
            }
            else //Related dp task object not in context list, then create dp task object and add to context list.
            {
                int len = 0;
                long long order = (long long)time((time_t *)NULL);
                pPkgProcContext = CreatePkgProcContext();
                len = strlen(pkgName) + 1;
                pPkgProcContext->pkgName = (char *)malloc(len);
                strcpy(pPkgProcContext->pkgName, pkgName);
                len = strlen(pkgType) + 1;
                pPkgProcContext->pkgType = (char *)malloc(len);
                strcpy(pPkgProcContext->pkgType, pkgType);
                pPkgProcContext->order = order;
                pPkgProcContext->pkgStep = STEP_INIT;
                pPkgProcContext->thenDP = 1;
                pPkgProcContext->taskStatusInfo.taskAction = TA_NORMAL;
                pPkgProcContext->dpRetryCnt = pDPTaskInfo->dpRetry;
                pPkgProcContext->isRB = pDPTaskInfo->isRollBack;
				pPkgProcContext->taskStatusInfo.pkgOwnerId = pDPTaskInfo->pkgOwnerId;
                LHAddNode(pCoreContext->pkgProcContextList, pPkgProcContext);
                pFakeDLFPkgContext = pPkgProcContext;
                LOGI("Package %s deploy task add!", pPkgProcContext->pkgName);
            }
            if (pFakeDLFPkgContext != NULL) ProcDLFinishedStatus(pFakeDLFPkgContext);
            pthread_mutex_unlock(&pCoreContext->pkgProcContextListMutex);
        }
    done1:
        if (pkgName) free(pkgName);
        if (pkgType) free(pkgType);
    }
    return 0;
}

/*
static int DefDeployCheckCB(const void * const checkHandle, NotifyDPCheckMsgCB notifyDpMsgCB, int * isQuit, void * userData)
{
    int iRet = -1;
    if (NULL == checkHandle || notifyDpMsgCB == NULL ||
        NULL == isQuit || NULL == userData) return iRet;
    {
        char * curDir = (char *)userData;
        char dpEvent[256] = { 0 };
        int curPercent = 0;
        while ((*isQuit) && curPercent <= 100)
        {
            memset(dpEvent, 0, sizeof(dpEvent));
            sprintf(dpEvent, "Deploy percent %d%%.", curPercent);
            notifyDpMsgCB(checkHandle, dpEvent, strlen(dpEvent) + 1);
            curPercent += 10;
            usleep(1000*1000);
        }
        memset(dpEvent, 0, sizeof(dpEvent));
        sprintf(dpEvent, "%s", "[OTATest][1.0.2][result]success");
        notifyDpMsgCB(checkHandle, dpEvent, strlen(dpEvent) + 1);
    }
    return 0;
}
*/

static int WriteSubDevicesArgs(char ** subDevices, int subDeviceSize, char * pkgName, char *filePath)
{
	int iRet = -1;
	char file[MAX_PATH];
	FILE *fp = NULL;
	int i = 0;

	if (subDevices == NULL) {
		return iRet;
	}

	//write args
	sprintf(file, "%s%c%s", filePath, FILE_SEPARATOR, "devicelist");
	if ((fp = fopen(file, "w")) == NULL)
	{
		LOGE("fopen(%s) failed", file);
		return iRet;
	}

	fputs(pkgName, fp);
	for (i = 0;	i < subDeviceSize; i++) {
		fputs(",", fp);
		fputs(subDevices[i], fp);
	}
	fclose(fp);
	iRet = 0;

	return iRet;
}
