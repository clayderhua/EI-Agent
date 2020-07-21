#ifndef _INTERNAL_DATA_
#define _INTERNAL_DATA_

#include <stdbool.h>

#include "pthread.h"
#include "SUEClientCore.h"
#include "ListHelper.h"

#ifndef NULL
#define NULL 0
#endif

#define DEF_DES_KEY                  "#otasue*"
#define DEF_PKG_FOLDER_NAME          "Pkg"
#define CORE_CONTEXT_INITIALIZE      {NULL, 0, 0, {NULL, 0}, \
                                      NULL, NULL, NULL, NULL, \
									  NULL, NULL, 0, NULL,\
                                      NULL,NULL}

#define DEF_OS_ALL_WIN            "win"
#define DEF_OS_WINXP              "winxp"
#define DEF_OS_WIN7               "win7"
#define DEF_OS_WIN8               "win8"
#define DEF_OS_WIN10              "win10"

#define DEF_IS_SYS_STARTUP_DP_ENABLE      1
#define DEF_IS_SYS_STARTUP_DP_DISENABLE   0
#define DEF_RESUME_PERIOD_MAXTS           90
#define DEF_DEPLOY_WAIT_TM                60
#define DEF_DL_RETRY_INTERVAL_TS          10

typedef struct CfgParams{
	char * pkgRootPath;
    int isSysStartupDP;
    unsigned int resumePeriodMaxTS;
    unsigned int deployWaitTM;
    unsigned int dlRetryIntervalTS;
    char * envFileDir;
	char * webSocketUrl;
	char* downloadDebug;
}CfgParams, *PCfgParams;

typedef struct CoreContext{
	char * cfgFile;
	int isInited;
	int isStarted;
	CfgParams cfgParams;

	char * osName;
	char * arch;
	char * tags;

	PLHList pkgDpCheckCBList;
	pthread_mutex_t endecodeMutex;

	PLHList pkgProcContextList;
	pthread_mutex_t pkgProcContextListMutex;
	int isPkgProcThreadRunning;
	pthread_t pkgProcThreadT;

	TaskStatusCB taskStatusCB;
	void * taskUserData;
}CoreContext, *PCoreContext;

typedef struct PkgDpCheckCBInfo{
	char * pkgType;
	void * dpCheckCB;
	void * dpCheckUserData;
}PkgDpCheckCBInfo,*PPkgDpCheckCBInfo;

typedef struct XMLPkgInfo{
	char * secondZipName;
	char * os;
	char * arch;
	char * tags;
    char * version;
	char * zipPwd;
	char * installerTool;
}XMLPkgInfo, *PXMLPkgInfo;

typedef struct DeployInfo{
	char * execFileName;
    char * retCheckScript;
    int rebootFlag;
}XMLDeployInfo, *PXMLDeployInfo;

typedef enum{
	STEP_UNKNOW = 0,
	STEP_INIT = 1,
	STEP_EXIST_CHECK = 2,
	STEP_FT_DL = 3,
	STEP_MD5_CHECK = 4,
	STEP_UNZIP_FL = 5,
	STEP_READXML_FL = 6,
	STEP_TAGS_CHECK = 7,
	STEP_UNZIP_SL = 8,
	STEP_READXML_SL=9,
	STEP_CHECK_DPF = 10,
	STEP_RUN_DPF = 11,
    STEP_REBOOT_SYS = 12,
	STEP_CHECK_DP_RET = 13,
    STEP_RECORD_SW = 14,
    STEP_DP_RB = 15,
}PkgStep;

typedef struct PkgProcContext{
	pthread_mutex_t dataMutex;

	TaskStatusInfo taskStatusInfo;

	char * pkgType;
	char * pkgName; // include ".zip"
	char * pkgURL;
	char * * subDevices;
	int subDeviceSize;
	int thenDP;
	int isDPFront;

	int suspendSCCode;
	PkgStep pkgStep;

	void * ftHandle;
	int isDLThreadRunning;
	pthread_t dlThreadT;

	int isDPThreadRunning;
	pthread_t dpThreadT;

	int isDPCheckThreadRunning;
	PPkgDpCheckCBInfo pPkgDpCheckCBInfo;
	pthread_t dpCheckThreadT;

	char * pkgPath;
	char * pkgTypePath;
	char * pkgBKPath;
	char * pkgFirstUnzipPath;
	char * pkgInfoXmlPath;
	char * pkgSecondZipPath;
	char * pkgSecondUnzipPath;
	char * pkgDeployXmlPath;
	char * pkgDeployFilePath; // setup exe file in OTA zip
    char * pkgRetCheckScriptPath; // check.sh in OTA zip
	XMLPkgInfo xmlPkgInfo;
	XMLDeployInfo xmlDeployInfo;

    long long order;
    int dlRetryCnt;
    int curDLRetryCnt;
    int dpRetryCnt;
    int curDPRetryCnt;
    int isRB;
    void * rbParam;
    int dlProtocal;
    int dlSercurity;
    int errCode;
}PkgProcContext, *PPkgProcContext;

#endif
