#ifndef _SUE_CLIENT_CORE_
#define _SUE_CLIENT_CORE_

#include "Log.h"
#include "SUEClientCoreData.h"
#include "ListHelper.h"

/*
SUEC£ºSUEClient
CB£ºCallback
DL£ºDownload
DP£ºDeploy
TR£ºTask Request
Del£ºDelete*/

/**
 * Success code: No error,indicates successful completion of an SUE client core operation.
 */
#define SUEC_CORE_SUCCESS    0

/**
 * Error code: Third party error code base!
 */
#define SUEC_THRPART_ERR_BASE       100000

/**
* Error code: Deploy third party error code base!(100000~199999)
*/
#define SUEC_DP_THRPART_ERR_BASE       SUEC_THRPART_ERR_BASE

/**
* Error code: File transfer third party error code base!(200000~299999)
*/
#define SUEC_FT_THRPART_ERR_BASE       (SUEC_THRPART_ERR_BASE + 100000)

/**
 * Error code: Interface parameter error!
 */
#define SUEC_CORE_I_PARAMETER_ERROR              2001

/**
 * Error code: SUECCore library not init!
 */
#define SUEC_CORE_I_NOT_INIT                     2002

/**
 * Error code: SUECCore library already start!
 */
#define SUEC_CORE_I_ALREADY_INIT                 2003

/**
 * Error code: SUECCore library not start!
 */
#define SUEC_CORE_I_NOT_START                    2004

/**
 * Error code: SUECCore library already start!
 */
#define SUEC_CORE_I_ALREADY_START                 2005

/**
 * Error code: Task process startup failed!
 */
#define SUEC_CORE_I_TASK_PROCESS_START_FAILED     2006

/**
 * Error code: Task status not checked!
 */
#define SUEC_CORE_I_PKG_TASK_STATUS_UNKNOW       2007

/**
 * Error code: Package task already exist!
 */
#define SUEC_CORE_I_PKG_TASK_EXIST             2008

/**
 * Error code: Operation cannot be performed at the current state of the package!
 */
#define SUEC_CORE_I_PKG_STATUS_NOT_FIT            2009

/**
 * Error code: Package suspend failed!
 */
#define SUEC_CORE_I_PKG_SUSPEND_FAILED            2010

/**
 * Error code: Package is not suspend!
 */
#define SUEC_CORE_I_PKG_NOT_SUSPEND               2011

/**
 * Error code: Package resume failed!
 */
#define SUEC_CORE_I_PKG_RESUME_FAILED             2012

/**
 * Error code: FT init failed!
 */
#define SUEC_CORE_I_FT_INIT_FAILED                 2013

/**
 * Error code: The package object to be operated has not been found!
 */
#define SUEC_CORE_I_OBJECT_NOT_FOUND              2014

/**
 * Error code: Config read failed!
 */
#define SUEC_CORE_I_CFG_READ_FAILED              2015

/**
 * Error code: Package file exist!
 */
#define SUEC_CORE_I_PKG_FILE_EXIST                2016

/**
 * Error code: Pkg Downloading!
 */
#define SUEC_CORE_I_DLING                            2017

/**
 * Error code: Package start download failed!
 */
#define SUEC_CORE_S_DL_START_FAILED                2501

/**
 * Error code: Package start deploy failed!
 */
#define SUEC_CORE_S_DP_START_FAILED                2502

/**
 * Error code: Package path error!
 */
#define SUEC_CORE_S_PKG_PATH_ERROR                2503

/**
 * Error code: Package file exist!
 */
#define SUEC_CORE_S_PKG_FILE_EXIST                2504

/**
 * Error code: Package file transfer error!
 */
#define SUEC_CORE_S_PKG_FT_DL_ERROR               2505

/**
 * Error code: Calculate MD5 error!
 */
#define SUEC_CORE_S_CALC_MD5_ERROR                2506

/**
 * Error code: MD5 not match!
 */
#define SUEC_CORE_S_MD5_NOT_MATCH                 2507

/**
 * Error code: Unzip error!
 */
#define SUEC_CORE_S_UNZIP_ERROR                  2508

/**
 * Error code: XML parse error!
 */
#define SUEC_CORE_S_XML_PARSE_ERROR               2509

/**
 * Error code: OS not match!
 */
#define SUEC_CORE_S_OS_NOT_MATCH                 2510

/**
 * Error code: Arch not match!
 */
#define SUEC_CORE_S_ARCH_NOT_MATCH                 2511

/**
 * Error code: Deploy file not exist!
 */
#define SUEC_CORE_S_DPFILE_NOT_EXIST             2512

/**
 * Error code: Deploy exec failed!
 */
#define SUEC_CORE_S_DP_EXEC_FAILED                2513

/**
 * Error code: Package file not exist!
 */
#define SUEC_CORE_S_PKG_FILE_NOT_EXIST             2514

/**
 * Error code: Base64 or DES decode error!
 */
#define SUEC_CORE_S_BS64DESDECODE_ERROR              2515

/**
 * Error code: Deploy wait process exit failed!
 */
#define SUEC_CORE_S_DP_WAIT_FAILED                   2516

/**
* Error code: Already config system startup deploy!
*/
#define SUEC_CORE_S_CFG_SYSSTARTUP_DP                2517

/**
* Error code: Package back file not found!
*/
#define SUEC_CORE_S_BK_FILE_NOT_FOUND                2518

/**
* Error code: Package deploy rollback error!
*/
#define SUEC_CORE_S_DP_ROLLBACK_ERROR                2519

/**
* Error code: Package deploy timeout!
*/
#define SUEC_CORE_S_DP_TIMEOUT                       2520

/**
* Error code: Package result check script exec failed!
*/
#define SUEC_CORE_S_RETCHECK_EXEC_ERROR              2521

/**
* Error code: Package result check script exec timeout!
*/
#define SUEC_CORE_S_RETCHECK_EXEC_TIMEOUT            2522

/**
* Error code: Package Download abort!
*/
#define SUEC_CORE_S_DL_ABORT                         2523

/**
 * Error code: Tags not match!
 */
#define SUEC_CORE_S_TAGS_NOT_MATCH                   2524

#ifdef __cplusplus
extern "C" {
#endif

	typedef int (*TaskStatusCB)(PTaskStatusInfo pTaskStatusInfo, void * userData);

	typedef int (*NotifyDPCheckMsgCB)(const void * const checkHandle, char * msg, unsigned int msgLen);

	typedef int (*DeployCheckCB)(const void * const checkHandle, NotifyDPCheckMsgCB notifyDpMsgCB, int * isQuit, void * userData);

	int SUECCoreInit(char * cfgFile, char* tags);

	int SUECCoreUninit();

	int SUECCoreStart(void * rsvParam);

	int SUECCoreStop();

	int SUECCoreSetTaskStatusCB(TaskStatusCB taskStatusCB, void * userData);

	int SUECCoreGetTaskStatus(char* pkgName, PTaskStatusInfo * ppTaskStatusInfo);

	int SUECCoreSetDpCheckCB(char* pkgType, DeployCheckCB dpCheckCB, void * userData);

	int SUECCoreAddDLTask(PDLTaskInfo pDLTaskInfo);

	int SUECCoreSuspendDLTask(char* pkgType,char * pkgName);

	int SUECCoreResumeDLTask(char* pkgType,char * pkgName);

	int SUECCoreDelDLTask(char* pkgType, char * pkgName);

	int SUECCoreCheckPkgExist(PDPTaskInfo pDPTaskInfo);

	int SUECCoreAddDPTask(PDPTaskInfo pDPTaskInfo);

	int SUECCoreSuspendDPTask(char* pkgType,char * pkgName);

	int SUECCoreResumeDPTask(char* pkgType, char * pkgName);

	int SUECCoreDelDPTask(char* pkgType, char * pkgName);

	int SUECCoreCheckTaskExist(char * pkgName, int taskType);

	void SUECCoreFreeTSInfo(PTaskStatusInfo pTaskStatusInfo);

	void SUECCoreFreeDLTaskInfo(PDLTaskInfo pDLTaskInfo);

    PLHList SUECCoreGetSWInfos(char ** swNames, int cnt);

    void SUECCoreGetRecordPkgVersion(char * pkgType, char ** version);

    PLHList SUECCoreDelDLPkgs(char ** pkgNames, int cnt, long pkgOwnerId);

    int SUECCoreSetTaskNorms(char * pkgName, int dlRetryCnt, int dpRetryCnt, int isRB);

	char * SUECCoreGetErrMsg(int errCode);

#ifdef __cplusplus
};
#endif

#endif
