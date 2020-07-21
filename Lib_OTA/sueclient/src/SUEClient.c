#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>

#include "util_path.h"

#define LOG_TAG "OTA"
#include "Log.h"
#include "InternalData.h"
#include "SUEClient.h"
#include "ErrorDef.h"
#include "SUEClientCore.h"
#include "MsgParser.h"
#include "iniparser.h"
#include "Log.h"
#include "SUESchedule.h"
#include "cJSON.h"
#ifdef linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <sys/types.h>
#else
#include <windows.h>
#include <lm.h>

#include "cp_fun.h"
#include "wrapper.h"

#pragma comment(lib, "netapi32.lib")
#endif

#define WEBSOCKET_CONNECT_TIMEOUT_MS	10*1000
#define WEBSOCKET_WAIT_RESPOND_TIMEOUT_MS (20*1000 + WEBSOCKET_CONNECT_TIMEOUT_MS)

SUECContext CurSUECContext; // = SUEC_CONTEXT_INITIALIZE;

static int ErrCodeMap(int errCode);
static void InitSysInfo(PSUECContext pSUECContext);
static void CleanContext(PSUECContext pSUECContext);
static void FreeOutputMsgCB(void * userData);
static void* OutputMsgThreadStart(void *args);
static int OutputMsgProcStart(PSUECContext pSUECContext);
static int OutputMsgProcStop(PSUECContext pSUECContext);
static void TaskStatusMsgOutput(char * pkgName,
							    int taskType,
								int statusCode,
								int errCode,
								char * msg,
								int percent,
								int pkgOwnerId,
								PSUECContext pSUECContext);
static int TaskStatusInfoCB(PTaskStatusInfo pTaskStatusInfo, void * userData);
static int ProcDLInfo(PDLTaskInfo pDLTaskInfo, PSUECContext pSUECContext);
static int ProcReqDownloadRet(int reqDLRet, PDLTaskInfo curRepDLTaskInfo, PSUECContext pSUECContext);
static int PorcDLReq(char * msg, PSUECContext pSUECContext);
static int ProcDPReq(char * msg, PSUECContext pSUECContext);
static int PorcReqDLRep(char * msg, PSUECContext pSUECContext);
static int ProcStatusReq(char * msg, PSUECContext pSUECContext);
static int ProcSWInfoReq(char * msg, PSUECContext pSUECContext);
static int ProcDelPkgReq(char * msg, PSUECContext pSUECContext);
static void ScheDLRetInterceptor(PTaskStatusInfo pTaskStatusInfo, PSUECContext pSUECContext);
static int SUEScheOutputMsgCB(char * msg, int msgLen, void * userData);
static void ReportSWInfos(PSUECContext pSUECContext);
static void CheckAndSendDelScheCmd(PSUECContext pSUECContext);
static char * GetEnvFileDirFromCfg(char * cfgFile);
char * GetOS();
char * GetDevID();
char * GetArch();
extern char * SuecGetErrorMsg(unsigned int errorCode);

//arch: openwrt
#ifdef linux
static void InitWSParams(char * cfgFile, PSUECContext pSUECContext);
static void* MonitorSubDevThreadStart(void *args);
static int MonitorSubDevProcStop(PSUECContext pSUECContext);
static int MonitorSubDevProcStart(PSUECContext pSUECContext);
static void SubDevTaskStatusMsgOutput(char * pkgName, char * devID, int taskType, int statusCode, int errCode,char * msg, int percent, PSUECContext pSUECContext);
static int WSGetUrl(char * cfgFile, PSUECContext pSUECContext);
#endif


#ifdef linux
int cp_get_origin_arch(char * pbuf, int buflen)
{
	int ret = -1;
	struct utsname buf;

	if (!pbuf || buflen < 1)
		return ret;

	if (0 == uname(&buf) && strlen(buf.machine) < buflen) {
		strcpy(pbuf, buf.machine);
		ret = 0;
	}

	return ret;
}

static char *trimwhitespace(char *str)
{
  char *end = str + strlen(str) - 1;

  // Trim leading space
  while(str < end && isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

int cp_get_os_name(char *pbuf, int buflen) {
	int ret = -1;
	FILE *fp = NULL;
	char osname[128] = {0};

	if (!pbuf || buflen < 1)
		return ret;

#ifndef ANDROID
	fp = popen("echo `lsb_release -is`-`lsb_release -rs`", "r");
	if (fp) {
		int len = 0;

		if (fgets(osname, sizeof(osname), fp) == NULL) {
			osname[0] = '\0';
		}
		pclose(fp);

		len = strlen(osname);
		if (len > 0 && len <= buflen) {
			strcpy(pbuf, trimwhitespace(osname));
			*pbuf = toupper(*pbuf);
			ret = 0;
		}
	}
#else
	strcpy(pbuf, "Android");
	ret = 0;
#endif

	return ret;
}

int cp_get_architecture(char * pbuf, int buflen)
{
	int ret = cp_get_origin_arch(pbuf, buflen);

	if (0 == ret) {
		if (NULL != strstr(pbuf, "arm") || NULL != strstr(pbuf, "aarch64")) {
			sprintf(pbuf, "arm");
		} else if (*pbuf == 'i' && NULL != strstr(pbuf, "86")) {
			sprintf(pbuf, "x86_32");
		} else {
			/* keep origin value, is x86_64 commonly */
		}
	}

	return ret;
}

#else // windows

static int cp_get_os_name_reg(char * pOSNameBuf, int bufLen)
{
	int iRet = -1;
	if (pOSNameBuf != NULL && bufLen > 0)
	{
		HKEY hk;
		char regName[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
		char valueName[] = "ProductName";
		char valueData[256] = { 0 };
		unsigned long  valueDataSize = sizeof(valueData);
		if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk)) return iRet;
		else
		{
			if (ERROR_SUCCESS != RegQueryValueEx(hk, valueName, 0, NULL, (LPBYTE)valueData, &valueDataSize))
			{
				RegCloseKey(hk);
				return iRet;
			}
			RegCloseKey(hk);
		}
		if (valueDataSize > 0)
		{
			if (bufLen <= valueDataSize)
			{
				iRet = valueDataSize + 1;
			}
			else
			{
				memcpy(pOSNameBuf, valueData, valueDataSize);
				iRet = 0;
			}
		}
	}
	return iRet;
}

int cp_get_os_name(char * pOSNameBuf, int bufLen)
{
	int iRet = -1;
	OSVERSIONINFOEX osvInfo;
	char *tmpOSName = NULL;
    WKSTA_INFO_100 *wkstaInfo = NULL;
    if(pOSNameBuf == NULL || bufLen<=0) return iRet;
	memset(&osvInfo, 0, sizeof(OSVERSIONINFOEX));
	osvInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if(GetVersionEx((OSVERSIONINFO *)&osvInfo)==0) return iRet;
    if (osvInfo.dwMajorVersion >= 6 && osvInfo.dwMinorVersion >= 2) {
        if (NetWkstaGetInfo(NULL, 100, (LPBYTE *)&wkstaInfo) == NERR_Success)
        {
            osvInfo.dwMajorVersion = wkstaInfo->wki100_ver_major;
            osvInfo.dwMinorVersion = wkstaInfo->wki100_ver_minor;
            NetApiBufferFree(wkstaInfo);
        }
    }
	switch(osvInfo.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_WINDOWS:
		{
			if( 4 == osvInfo.dwMajorVersion )
			{
				switch(osvInfo.dwMinorVersion)
				{
				case 0:
					{
						tmpOSName = OS_WINDOWS_95;
						break;
					}
				case 10:
					{
						tmpOSName = OS_WINDOWS_98;
						break;
					}
				case 90:
					{
						tmpOSName = OS_WINDOWS_ME;
						break;
					}
				default:
					{
						break;
					}
				}
			}
			break;
		}
	case VER_PLATFORM_WIN32_NT:
		{
			switch(osvInfo.dwMajorVersion)
			{
			case 3:
				{
					tmpOSName = OS_WINDOWS_NT_3_51;
					break;
				}
			case 4:
				{
					tmpOSName = OS_WINDOWS_NT_4;
					break;
				}
			case 5:
				{
					switch(osvInfo.dwMinorVersion)
					{
					case 0:
						{
							tmpOSName = OS_WINDOWS_2000;
							break;
						}
					case 1:
						{
							tmpOSName = OS_WINDOWS_XP;
							break;
						}
					case 2:
						{
							tmpOSName = OS_WINDOWS_SERVER_2003;
							break;
						}
					default:
						{
							break;
						}
					}
					break;
				}
			case 6:
				{
					switch(osvInfo.dwMinorVersion)
					{
					case 0:
						{
							tmpOSName = OS_WINDOWS_VISTA;
							break;
						}
					case 1:
						{
							tmpOSName = OS_WINDOWS_7;
							break;
						}
					case 2:
						{
							if(osvInfo.wProductType == VER_NT_WORKSTATION)
							{
								tmpOSName = OS_WINDOWS_8;
							}
							else
							{
								tmpOSName = OS_WINDOWS_SERVER_2012;
							}
							break;
						}
					case 3:
						{
							if(osvInfo.wProductType == VER_NT_WORKSTATION)
							{
								tmpOSName = OS_WINDOWS_8_1;
							}
							else
							{
								tmpOSName = OS_WINDOWS_SERVER_2012_R2;
							}
							break;
						}
					default:
						{
							break;
						}
					}
					break;
				}
			case 10:
				{
					switch(osvInfo.dwMinorVersion)
					{
					case 0:
						{
							if(osvInfo.wProductType == VER_NT_WORKSTATION)
							{
								tmpOSName = OS_WINDOWS_10;
							}
							else
							{
								tmpOSName = OS_WINDOWS_SERVER_2016;
							}
							break;
						}
					default:
						{
							break;
						}
					}
					break;
				}
			default:
				{
					break;
				}
			}
			break;
		}
	default:
		{
			break;
		}
	}
	if(tmpOSName != NULL)
	{
		if(bufLen <= strlen(tmpOSName))
		{
			iRet = strlen(tmpOSName)+1;
		}
		else
		{
			strcpy(pOSNameBuf, tmpOSName);
			iRet = 0;
		}
	}
	else
	{
		iRet = cp_get_os_name_reg(pOSNameBuf, bufLen);
	}
	return iRet;
}

int cp_get_architecture(char * pArchBuf, int bufLen)
{
	int iRet = -1;
	if (pArchBuf != NULL && bufLen > 0)
	{
		char * tmpArch = NULL;
		SYSTEM_INFO siSysInfo;
		typedef void (WINAPI *LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO);
		LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandle("kernel32"), "GetNativeSystemInfo");
		if (NULL != fnGetNativeSystemInfo)
		{
			fnGetNativeSystemInfo(&siSysInfo);
		}
		else
		{
			GetSystemInfo(&siSysInfo);
		}

		switch(siSysInfo.wProcessorArchitecture)
		{
		case PROCESSOR_ARCHITECTURE_AMD64:
			{
				tmpArch = ARCH_X64;
				break;
			}
		case PROCESSOR_ARCHITECTURE_ARM:
			{
				tmpArch = ARCH_ARM;
				break;
			}
		case PROCESSOR_ARCHITECTURE_IA64:
			{
				tmpArch = ARCH_IA64;
				break;
			}
		case PROCESSOR_ARCHITECTURE_INTEL:
			{
				tmpArch = ARCH_X86;
				break;
			}
		case PROCESSOR_ARCHITECTURE_UNKNOWN:
			{
				tmpArch = ARCH_UNKNOW;
				break;
			}
		default:
			break;
		}
		if(tmpArch)
		{
			if(bufLen <= strlen(tmpArch))
			{
				iRet = strlen(tmpArch)+1;
			}
			else
			{
				strcpy(pArchBuf, tmpArch);
				iRet = 0;
			}
		}
	}
	return iRet;
}


#endif

//----------------------------SUEClient interface define S---------------------------------------
int SUECInit(char *devID, char *cfgFile, char* tags)
{
	int retCode = SUEC_SUCCESS;
	PSUECContext pSUECContext;
	LOGI("SUEClient init...");

	memset(&CurSUECContext, 0, sizeof(CurSUECContext));
	CurSUECContext.opActType = OPAT_CONTINUE;
	pSUECContext = &CurSUECContext;

	if(devID == NULL)
		return SUEC_I_PARAMETER_ERROR;
	if(!pSUECContext->isInited)
	{
		int len = 0;
		pSUECContext->isInited = 1;
		// init pSUECContext
        len = strlen(devID) + 1;
		pSUECContext->devID = (char *) malloc(len);
		strcpy(pSUECContext->devID, devID);
		InitSysInfo(pSUECContext);
		pSUECContext->tags = tags;
		pthread_mutex_init(&pSUECContext->dataMutex, NULL);
        retCode = SUECCoreInit(cfgFile, tags);
		if(retCode != SUEC_SUCCESS)
		{
			retCode = ErrCodeMap(retCode);
			CleanContext(pSUECContext);
			pSUECContext->isInited = 0;
		}
        else
        {
            char *envFileDir = GetEnvFileDirFromCfg(cfgFile);
            if (envFileDir)
            {
                if (!util_is_file_exist(envFileDir))
					util_create_directory(envFileDir);
                if (!util_is_file_exist(envFileDir))
                {
                    if (envFileDir) free(envFileDir);
                    envFileDir = NULL;
                }
            }
            pSUECContext->sueScheHandle = InitSUESche(envFileDir);
            SetSUESheOutPutMsgCB(pSUECContext->sueScheHandle, SUEScheOutputMsgCB, pSUECContext);
            if (envFileDir) free(envFileDir);

        }

		//arch: openwrt
#ifdef linux
        if (cfgFile != NULL && strlen(cfgFile) > 0) {
            InitWSParams(cfgFile, pSUECContext);
        }
#endif
	}
	else
		retCode = SUEC_I_ALREADY_INIT;
	if(retCode == SUEC_SUCCESS)
		LOGI("SUEClient init success!");
	else
		LOGE("SUEClient init failed!ErrCode:%d, ErrMsg:%s", retCode, SuecGetErrorMsg(retCode));
   return retCode;
}

int SUECUninit()
{
	int retCode = SUEC_CORE_SUCCESS;
	PSUECContext pSUECContext = &CurSUECContext;
	LOGI("SUEClient uninit...");
	if(pSUECContext->isInited)
	{
		if(pSUECContext->isStarted)
		{
			SUECStop();
			pSUECContext->isStarted = 0;
		}
		SUECCoreUninit();
		CleanContext(pSUECContext);
		pSUECContext->isInited = 0;
	}
	LOGI("SUEClient uninit success!");
	return retCode;
}

int SUECStart(void * rsvParam)
{
	int retCode = SUEC_SUCCESS;
	PSUECContext pSUECContext = &CurSUECContext;
	LOGI("SUEClient startup...");
	if(pSUECContext->isInited)
	{
		if(!pSUECContext->isStarted)
		{
			pSUECContext->isStarted = 1;
			if(pSUECContext->outputMsgCB)
			{
				retCode = OutputMsgProcStart(pSUECContext);
			}
			if(retCode == SUEC_SUCCESS)
			{
				retCode = SUECCoreSetTaskStatusCB(TaskStatusInfoCB, pSUECContext);
				if(retCode == SUEC_SUCCESS) {
					retCode = SUECCoreStart(NULL);
				}
				if(retCode == SUEC_SUCCESS) {
					retCode = StartSUESche();
				}
				if(retCode != SUEC_SUCCESS)
				{
					retCode = ErrCodeMap(retCode);
				}
			}
			if(retCode != SUEC_SUCCESS)
			{
				pSUECContext->isStarted = 0;
				OutputMsgProcStop(pSUECContext);
			}
			//arch: openwrt
#ifdef linux
            if (pSUECContext->wsParams.ip != NULL) {
                /* code */
                retCode = MonitorSubDevProcStart(pSUECContext);
            }
            if (retCode != SUEC_SUCCESS) {
                /* code */
                MonitorSubDevProcStop(pSUECContext);
            }
#endif
		}
		else
			retCode = SUEC_I_ALREADY_START;
	}
	else
		retCode = SUEC_I_NOT_INIT;
	if(retCode == SUEC_SUCCESS)
		LOGI("SUEClient startup success!");
	else
		LOGE("SUEClient startup failed!ErrCode:%d, ErrMsg:%s", retCode, SuecGetErrorMsg(retCode));

    if (retCode == SUEC_SUCCESS)
    {
        ReportSWInfos(pSUECContext);
    }

    return retCode;
}

int SUECStop()
{
	int retCode = SUEC_SUCCESS;
	PSUECContext pSUECContext = &CurSUECContext;
	LOGI("SUEClient Stop...");
	if(pSUECContext->isInited)
	{
		if(pSUECContext->isStarted)
		{
			SUECCoreStop();
			OutputMsgProcStop(pSUECContext);
			//arch: openwrt
#ifdef linux
            MonitorSubDevProcStop(pSUECContext);
#endif
			pSUECContext->isStarted = 0;
		}
		else retCode = SUEC_I_NOT_START;
	}
	else retCode = SUEC_I_NOT_INIT;
	if(retCode == SUEC_SUCCESS) LOGI("SUEClient Stop success!");
	else LOGE("SUEClient Stop failed!ErrCode:%d, ErrMsg:%s", retCode, SuecGetErrorMsg(retCode));
	return retCode;
}

int SUECSetOutputMsgCB(SUECOutputMsgCB outputMsgCB, void * userData)
{
	int retCode = SUEC_SUCCESS;
	PSUECContext pSUECContext = &CurSUECContext;
	if(pSUECContext->isInited)
	{
		if(!pSUECContext->isStarted)
		{
			pSUECContext->outputMsgCB = outputMsgCB;
			pSUECContext->opMsgCBUserData = userData;
		}
		else retCode = SUEC_I_ALREADY_START;
	}
	else retCode = SUEC_I_NOT_INIT;
	if(retCode != SUEC_SUCCESS) LOGE("SUEClient set output message callback failed!ErrCode:%d, ErrMsg:%s",
		retCode, SuecGetErrorMsg(retCode));
	//Wei.Gang add
	if (retCode == SUEC_SUCCESS && pSUECContext->isInited && pSUECContext->opActType == OPAT_CONTINUE)
    {
        ReportSWInfos(pSUECContext);
		// if there is not Schedule.cfg file. Send delete all schedule cmd
		CheckAndSendDelScheCmd(pSUECContext);
    }
	//Wei.Gang add end
	return retCode;
}

int SUECSetTaskStatusCB(SUECTaskStatusCB taskStatusCB, void * userData)
{
	int retCode = SUEC_SUCCESS;
	PSUECContext pSUECContext = &CurSUECContext;
	if(pSUECContext->isInited)
	{
		if(!pSUECContext->isStarted)
		{
			pSUECContext->taskStatusCB = taskStatusCB;
			pSUECContext->tsCBUserData = userData;
		}
		else retCode = SUEC_I_ALREADY_START;
	}
	else retCode = SUEC_I_NOT_INIT;
	if(retCode != SUEC_SUCCESS) LOGE("SUEClient set task status callback failed!ErrCode:%d, ErrMsg:%s",
		retCode, SuecGetErrorMsg(retCode));
	return retCode;
}

int SUECSetDpCheckCB(char* pkgType, SUECDeployCheckCB dpCheckCB,void * userData)
{
	int retCode = SUEC_SUCCESS;
	PSUECContext pSUECContext = &CurSUECContext;
	if(pSUECContext->isInited)
	{
		if(!pSUECContext->isStarted)
		{
			retCode = SUECCoreSetDpCheckCB(pkgType, dpCheckCB, userData);
			if(retCode != SUEC_SUCCESS)
			{
				retCode = ErrCodeMap(retCode);
			}
		}
		else retCode = SUEC_I_ALREADY_START;
	}
	else retCode = SUEC_I_NOT_INIT;
	if(retCode != SUEC_SUCCESS) LOGE("SUEClient set deploy check callback failed!ErrCode:%d, ErrMsg:%s",
		retCode, SuecGetErrorMsg(retCode));
	return retCode;
}

int SUECInputMsg(char * msg, int msgLen)
{
	int retCode = SUEC_SUCCESS;
	PSUECContext pSUECContext = &CurSUECContext;
	if(NULL == msg || msgLen == 0)
		return SUEC_I_PARAMETER_ERROR;
    if(pSUECContext->isInited)
	{
		int cmdID = CMD_INVALID;
		LOGI("Input message:%s,%d", msg, msgLen);
		cmdID = ParseCmdID(msg);
		switch(cmdID)
		{
		case CMD_DOWNLOAD_REQ:
		    {
			    retCode = PorcDLReq(msg, pSUECContext);
			    break;
		    }
		case CMD_DEPLOY_REQ:
			{
				retCode = ProcDPReq(msg, pSUECContext);
				break;
			}
		case CMD_REQ_DOWNLOAD_REP:
			{
				retCode = PorcReqDLRep(msg, pSUECContext);
				break;
			}
		case CMD_STATUS_REQ:
			{
				retCode = ProcStatusReq(msg, pSUECContext);
				break;
			}
		case CMD_SET_SCHE_REQ:
		case CMD_DEL_SCHE_REQ:
		case CMD_CHECK_PKGINFO_REP:
		case CMD_SCHE_INFO_REQ:
			{
				retCode = ProcSUEScheCmd(pSUECContext->sueScheHandle, msg);
				break;
			}
        case CMD_SWINFO_REQ:
            {
                retCode = ProcSWInfoReq(msg, pSUECContext);
                break;
            }
        case CMD_DEL_PKG_REQ:
            {
                retCode = ProcDelPkgReq(msg, pSUECContext);
                break;
            }
		default:
			{
				retCode = SUEC_I_MSG_PARSE_FAILED;
				break;
			}
		}
	}
	else
		retCode = SUEC_I_NOT_INIT;
	if(retCode != SUEC_SUCCESS)
		LOGE("SUEClient Input message error!ErrCode:%d, ErrMsg:%s",  retCode, SuecGetErrorMsg(retCode));
	return retCode;
}

int SUECSetOutputAct(OutputActType actType)
{
	int retCode = SUEC_SUCCESS;
	PSUECContext pSUECContext = &CurSUECContext;
	if(actType<OPAT_SUSPEND || actType>OPAT_CONTINUE) return SUEC_I_PARAMETER_ERROR;
    pthread_mutex_lock(&pSUECContext->dataMutex);
    pSUECContext->opActType = actType;
    pthread_mutex_unlock(&pSUECContext->dataMutex);
    if (pSUECContext->isInited && actType == OPAT_CONTINUE)
    {
        ReportSWInfos(pSUECContext);
    }
	/*if(pSUECContext->isInited)
	{
		pthread_mutex_lock(&pSUECContext->dataMutex);
		pSUECContext->opActType = actType;
		pthread_mutex_unlock(&pSUECContext->dataMutex);
	}
	else retCode = SUEC_I_NOT_INIT;*/
	if(retCode == SUEC_SUCCESS)
	{
		if(actType == OPAT_CONTINUE) LOGI("SUEClient current output message act type: CONTINUE");
		else LOGI("SUEClient current output message act type: SUSPEND");
	}
	else LOGE("SUEClient set output act failed!ErrCode:%d, ErrMsg:%s",
		retCode, SuecGetErrorMsg(retCode));
	return retCode;
}

int SUECRequestDownload(char * pkgName, unsigned int timeoutMS)
{
    return SUECRequestDownloadWithNorm(pkgName, timeoutMS, 0);
    //return SUECRequestDownloadWithNorm(pkgName, timeoutMS, 3);
}

int SUECRequestDeploy(char * pkgName)
{
    return SUECRequestDeployWithNorm(pkgName, 0, 0);
    //return SUECRequestDeployWithNorm(pkgName, 3, 1);
}

char * SUECGetErrMsg(int errCode)
{
	return SuecGetErrorMsg(errCode);
}

int SUEGetSysOS(char * osBuf, int bufLen)
{
	return cp_get_os_name(osBuf, bufLen);
}

int SUEGetSysArch(char * archBuf, int bufLen)
{
	return cp_get_architecture(archBuf, bufLen);
}

int SUECRequestDownloadWithNorm(char * pkgName, unsigned int timeoutMS, unsigned int retryCnt)
{
    int retCode = SUEC_SUCCESS;
    PSUECContext pSUECContext = &CurSUECContext;
    if (NULL == pkgName) return SUEC_I_PARAMETER_ERROR;
    if (pSUECContext->isInited)
    {
        if (SUECCoreCheckTaskExist(pkgName, TASK_DL) != SUEC_SUCCESS)  //check exist
        {
            OutputActType curOPAT = OPAT_CONTINUE;
            pthread_mutex_lock(&pSUECContext->dataMutex);
            curOPAT = pSUECContext->opActType;
            pthread_mutex_unlock(&pSUECContext->dataMutex);
            if (pSUECContext->outputMsgCB && curOPAT == OPAT_CONTINUE) //judge can output msg
            {
                pthread_mutex_lock(&pSUECContext->dataMutex);
                if (pSUECContext->dlReqPkgName == NULL)  //judge current not download request doing
                {
                    char * msg = NULL;
                    int len = strlen(pkgName) + 1;
                    pSUECContext->dlReqPkgName = (char*)malloc(len);
                    memset(pSUECContext->dlReqPkgName, 0, len);
                    strcpy(pSUECContext->dlReqPkgName, pkgName);
                    pSUECContext->dlReqRet = 0;
                    pthread_mutex_unlock(&pSUECContext->dataMutex);
                    PackReqDLTaskInfo(pkgName, pSUECContext->devID, &msg); //pack request download msg
                    if (msg != NULL)
                    {
                        int iRet = 0;
                        LOGI("SUEClient output message: %s", msg);
                        iRet = pSUECContext->outputMsgCB(msg, strlen(msg) + 1, pSUECContext->opMsgCBUserData); //output msg
                        if (iRet == 0)
                        {
                            PDLTaskInfo curRepDLTaskInfo = NULL;
                            int dlReqRet = 0;
                            int cnt = timeoutMS / 10 + 1, i = 0;
                            for (i = 0; i < cnt; i++)   //wait result,once check interval 10ms
                            {
                                usleep(1000*10);
                                pthread_mutex_lock(&pSUECContext->dataMutex);
                                if (pSUECContext->dlReqRet)
                                {
                                    dlReqRet = pSUECContext->dlReqRet;
                                    curRepDLTaskInfo = pSUECContext->repDLTaskInfo;
                                    curRepDLTaskInfo->dlRetry = retryCnt;
                                }
                                pthread_mutex_unlock(&pSUECContext->dataMutex);
                                if (dlReqRet) break;
                            }
                            retCode = ProcReqDownloadRet(dlReqRet, curRepDLTaskInfo, pSUECContext); //process result
                        }
                        else retCode = SUEC_I_REQ_DL_OUTPUT_ERROR;
                        free(msg);
                    }
                    else retCode = SUEC_I_REQ_DL_OUTPUT_ERROR;
                    pthread_mutex_lock(&pSUECContext->dataMutex);
                    free(pSUECContext->dlReqPkgName);
                    pSUECContext->dlReqPkgName = NULL;
                    pthread_mutex_unlock(&pSUECContext->dataMutex);
                }
                else
                {
                    pthread_mutex_unlock(&pSUECContext->dataMutex);
                    retCode = SUEC_I_REQ_DL_TASK_DOING;
                }
            }
            else retCode = SUEC_I_MSG_CANNOT_OUTPUT;
        }
        else retCode = SUEC_I_REQ_DL_TASK_ALRADY_EXIST;
    }
    else retCode = SUEC_I_NOT_INIT;
    if (retCode == SUEC_SUCCESS) LOGI("SUEClient actively request download %s success!", pkgName);
    else LOGE("SUEClient actively request download %s failed!ErrCode:%d, ErrMsg:%s",
        pkgName, retCode, SuecGetErrorMsg(retCode));
    return retCode;
}

int SUECRequestDeployWithNorm(char * pkgName, unsigned int retryCnt, unsigned int isRollback)
{
    int retCode = SUEC_SUCCESS;
    PSUECContext pSUECContext = &CurSUECContext;
    if (NULL == pkgName) return SUEC_I_PARAMETER_ERROR;
    if (pSUECContext->isInited)
    {
        if (SUECCoreCheckTaskExist(pkgName, TASK_DP) != SUEC_SUCCESS)
        {
            DPTaskInfo dpTaskInfo;
            int len = strlen(pkgName) + 1;
            memset(&dpTaskInfo, 0, sizeof(DPTaskInfo));
            dpTaskInfo.pkgName = (char *)malloc(len);
            memset(dpTaskInfo.pkgName, 0, len);
            strcpy(dpTaskInfo.pkgName, pkgName);
            dpTaskInfo.dpRetry = retryCnt;
            dpTaskInfo.isRollBack = isRollback;
            retCode = SUECCoreAddDPTask(&dpTaskInfo);
            if (retCode != SUEC_SUCCESS)
            {
                retCode = ErrCodeMap(retCode);
            }
            free(dpTaskInfo.pkgName);
        }
        else retCode = SUEC_I_REQ_DP_TASK_ALRADY_EXIST;
    }
    else retCode = SUEC_I_NOT_INIT;
    if (retCode == SUEC_SUCCESS) LOGI("SUEClient actively request deploy %s success!", pkgName);
    else LOGE("SUEClient actively request deploy %s failed!ErrCode:%d, ErrMsg:%s",
        pkgName, retCode, SuecGetErrorMsg(retCode));
    return retCode;
}
//----------------------------SUEClient interface define E---------------------------------------

static int ErrCodeMap(int errCode)
{
    switch (errCode)
    {
    case SUEC_CORE_I_PARAMETER_ERROR:
    {
        errCode = SUEC_I_PARAMETER_ERROR;
        break;
    }
    case SUEC_CORE_I_NOT_INIT:
    {
        errCode = SUEC_I_NOT_INIT;
        break;
    }
    case SUEC_CORE_I_ALREADY_INIT:
    {
        errCode = SUEC_I_ALREADY_INIT;
        break;
    }
    case SUEC_CORE_I_NOT_START:
    {
        errCode = SUEC_I_NOT_START;
        break;
    }
    case SUEC_CORE_I_ALREADY_START:
    {
        errCode = SUEC_I_ALREADY_START;
        break;
    }
    case SUEC_CORE_I_TASK_PROCESS_START_FAILED:
    {
        errCode = SUEC_I_TASK_PROCESS_START_FAILED;
        break;
    }
    case SUEC_CORE_I_PKG_TASK_EXIST:
    {
        errCode = SUEC_I_REQ_DL_TASK_ALRADY_EXIST;
        break;
    }
    case SUEC_CORE_I_PKG_STATUS_NOT_FIT:
    {
        errCode = SUEC_I_PKG_STATUS_NOT_FIT;
        break;
    }
    case SUEC_CORE_I_FT_INIT_FAILED:
    {
        errCode = SUEC_I_FT_INIT_FAILED;
        break;
    }
    case SUEC_CORE_I_OBJECT_NOT_FOUND:
    {
        errCode = SUEC_I_OBJECT_NOT_FOUND;
        break;
    }
    case SUEC_CORE_I_CFG_READ_FAILED:
    {
        errCode = SUEC_I_CFG_READ_FAILED;
        break;
    }
    case SUEC_CORE_I_DLING:
    {
        errCode = SUEC_S_PKG_DLING;
        break;
    }
    case SUEC_CORE_I_PKG_FILE_EXIST:
    {
        errCode = SUEC_S_PKG_FILE_EXIST;
        break;
    }
    }
    return errCode;
}

char * GetOS()
{
    return CurSUECContext.osName;
}

char * GetArch()
{
    return CurSUECContext.arch;
}

char * GetDevID()
{
    return CurSUECContext.devID;
}

static void InitSysInfo(PSUECContext pSUECContext)
{
    int len = 0;
    char tmpBuf[64] = { 0 };

    if (pSUECContext == NULL)
		return;

	if (pSUECContext->osName == NULL) {
		cp_get_os_name(tmpBuf, sizeof(tmpBuf));
		len = strlen(tmpBuf) + 1;
		pSUECContext->osName = (char*)malloc(len);
		strcpy(pSUECContext->osName, tmpBuf);
	}
	if (pSUECContext->arch == NULL) {
		cp_get_architecture(tmpBuf, sizeof(tmpBuf));
		len = strlen(tmpBuf) + 1;
		pSUECContext->arch = (char*)malloc(len);
		strcpy(pSUECContext->arch, tmpBuf);
	}
}

static void CleanContext(PSUECContext pSUECContext)
{
    if (pSUECContext)
    {
        pSUECContext->isStarted = 0;
        pSUECContext->isInited = 0;
        pSUECContext->outputMsgCB = NULL;
        pSUECContext->opMsgCBUserData = NULL;
        UninitSUESche(pSUECContext->sueScheHandle);
        pSUECContext->sueScheHandle = NULL;
        if (pSUECContext->osName) {
			free(pSUECContext->osName);
			pSUECContext->osName = NULL;
		}
        if (pSUECContext->arch) {
			free(pSUECContext->arch);
			pSUECContext->arch = NULL;
		}
		// tags is allocate from OTA handler, so we don't need to free
		if (pSUECContext->tags) {
			pSUECContext->tags = NULL;
		}
        pthread_mutex_destroy(&pSUECContext->dataMutex);
        if (pSUECContext->devID)
        {
            free(pSUECContext->devID);
            pSUECContext->devID = NULL;
        }
#ifdef linux
        if(pSUECContext->wsUrl){
            free(pSUECContext->wsUrl);
        }
        if (pSUECContext->wsParams.interfacePath) {
            free(pSUECContext->wsParams.interfacePath);
        }
        if (pSUECContext->wsParams.connection) {
            free(pSUECContext->wsParams.connection);
        }
        if(pSUECContext->wsParams.ip){
            free(pSUECContext->wsParams.ip);
        }
        if(pSUECContext->wsParams.accept){
            free(pSUECContext->wsParams.accept);
        }
        if (pSUECContext->wsParams.secWebsocketKey) {
            free(pSUECContext->wsParams.secWebsocketKey);
        }
        if(pSUECContext->wsParams.upgrade){
            free(pSUECContext->wsParams.upgrade);
        }
        if(pSUECContext->wsParams.secWebSocketAccept){
            free(pSUECContext->wsParams.secWebSocketAccept);
        }
        pSUECContext->wsParams.port = 0;
        pSUECContext->wsParams.secWebSocketVersion = 0;
#endif
    }
}

static void FreeOutputMsgCB(void * userData)
{
    char * opMsg = (char *)userData;
    if (opMsg)
    {
        free(opMsg);
    }
}

static void* OutputMsgThreadStart(void *args)
{
    PSUECContext pSUECContext = (PSUECContext)args;
    if (pSUECContext)
    {
        OutputActType curOPAT = OPAT_CONTINUE;
        while (pSUECContext->isOutputMsgThreadRunning)
        {
            pthread_mutex_lock(&pSUECContext->dataMutex);
            curOPAT = pSUECContext->opActType;
            pthread_mutex_unlock(&pSUECContext->dataMutex);
            if (curOPAT == OPAT_CONTINUE)
            {
                pthread_mutex_lock(&pSUECContext->opMsgQueueMutex);
                {
                    PQNode pQHNode = QHDequeue(pSUECContext->opMsgQueue);
                    if (pQHNode)
                    {
                        char * msg = (char *)pQHNode->pUserData;
                        if (msg)
                        {
                            LOGD("SUEClient output message: %s", msg);
                            pSUECContext->outputMsgCB(msg, strlen(msg) + 1, pSUECContext->opMsgCBUserData);
                            free(msg);
                        }
                        free(pQHNode);
                    }
                }
                pthread_mutex_unlock(&pSUECContext->opMsgQueueMutex);
            }
            usleep(1000*100);
        }
    }
    return 0;
}

static int OutputMsgProcStart(PSUECContext pSUECContext)
{
    int retCode = SUEC_SUCCESS;
    if (pSUECContext == NULL) return SUEC_I_PARAMETER_ERROR;
    if (!pSUECContext->isOutputMsgThreadRunning)
    {
        pSUECContext->isOutputMsgThreadRunning = 1;
        pthread_mutex_init(&pSUECContext->opMsgQueueMutex, NULL);
        pSUECContext->opMsgQueue = QHInitQueue(0, FreeOutputMsgCB);
        if (pthread_create(&pSUECContext->outputMsgThreadT, NULL, OutputMsgThreadStart, pSUECContext) != 0)
        {
            pSUECContext->isOutputMsgThreadRunning = 0;
            retCode = SUEC_I_OUT_MSG_PROCESS_START_FAILED;
        }
    }
    return retCode;
}

static int OutputMsgProcStop(PSUECContext pSUECContext)
{
    int retCode = SUEC_CORE_SUCCESS;
    if (pSUECContext == NULL) return SUEC_I_PARAMETER_ERROR;;
    if (pSUECContext->isOutputMsgThreadRunning)
    {
        pSUECContext->isOutputMsgThreadRunning = 0;
        pthread_join(pSUECContext->outputMsgThreadT, NULL);
        pSUECContext->outputMsgThreadT = 0;
    }
    pthread_mutex_lock(&pSUECContext->opMsgQueueMutex);
    QHDestroyQueue(pSUECContext->opMsgQueue);
    pSUECContext->opMsgQueue = NULL;
    pthread_mutex_unlock(&pSUECContext->opMsgQueueMutex);
    pthread_mutex_destroy(&pSUECContext->opMsgQueueMutex);
    return retCode;
}

static void TaskStatusMsgOutput(char * pkgName,
							    int taskType,
								int statusCode,
								int errCode,
								char * msg,
								int percent,
								int pkgOwnerId,
								PSUECContext pSUECContext)
{
    if (NULL != pkgName && NULL != pSUECContext)
    {
        PTaskStatusInfo pTaskStatusInfo = NULL;
        int len = sizeof(TaskStatusInfo);
        pTaskStatusInfo = (PTaskStatusInfo)malloc(len);
        memset(pTaskStatusInfo, 0, len);
        len = strlen(pkgName) + 1;
        pTaskStatusInfo->pkgName = (char *)malloc(len);
        memset(pTaskStatusInfo->pkgName, 0, len);
        strcpy(pTaskStatusInfo->pkgName, pkgName);
        pTaskStatusInfo->statusCode = statusCode;
        pTaskStatusInfo->taskType = taskType;
        pTaskStatusInfo->errCode = errCode;
		pTaskStatusInfo->pkgOwnerId = pkgOwnerId;
        if (taskType == TASK_DL && statusCode == SC_DOING && percent >= 0)
        {
            pTaskStatusInfo->u.dlPercent = percent;
        }
        else if (msg != NULL)
        {
            len = strlen(msg) + 1;
            pTaskStatusInfo->u.msg = (char *)malloc(len);
            memset(pTaskStatusInfo->u.msg, 0, len);
            strcpy(pTaskStatusInfo->u.msg, msg);
        }
        {
            char * outMsg = NULL;
            if (PackAutoTaskStatusInfo(pTaskStatusInfo, pSUECContext->devID, &outMsg) == 0)
            {
                LOGD("SUEClient output message: %s", outMsg);
                pSUECContext->outputMsgCB(outMsg, strlen(outMsg) + 1, pSUECContext->opMsgCBUserData);
                if (outMsg) free(outMsg);
            }
            SUECCoreFreeTSInfo(pTaskStatusInfo);
            pTaskStatusInfo = NULL;
        }
    }
}

static void SubDevTaskStatusMsgOutput(char * pkgName,
									  char * devID,
									  int taskType,
									  int statusCode,
									  int errCode,
									  char * msg,
									  int percent,
									  PSUECContext pSUECContext)
{
	PTaskStatusInfo pTaskStatusInfo = NULL;
	int len;
	char * outMsg = NULL;

	if (NULL == pkgName || NULL == pSUECContext) {
		return;
	}

	len = sizeof(TaskStatusInfo);
	pTaskStatusInfo = (PTaskStatusInfo) malloc(len);
	memset(pTaskStatusInfo, 0, len);

	len = strlen(pkgName) + 1;
	pTaskStatusInfo->pkgName = (char *) malloc(len);
	strcpy(pTaskStatusInfo->pkgName, pkgName);

	pTaskStatusInfo->statusCode = statusCode;
	pTaskStatusInfo->taskType = taskType;
	pTaskStatusInfo->errCode = errCode;
	if (taskType == TASK_DL && statusCode == SC_DOING && percent >= 0) {
		pTaskStatusInfo->u.dlPercent = percent;
	} else if (msg != NULL) {
		len = strlen(msg) + 1;
		pTaskStatusInfo->u.msg = (char *)malloc(len);
		strcpy(pTaskStatusInfo->u.msg, msg);
	}

	if (PackAutoTaskStatusInfo(pTaskStatusInfo, devID, &outMsg) == 0)
	{
		LOGD("SUEClient output message: %s", outMsg);
		pSUECContext->outputMsgCB(outMsg, strlen(outMsg) + 1, pSUECContext->opMsgCBUserData);
		if (outMsg)
			free(outMsg);
	}
	SUECCoreFreeTSInfo(pTaskStatusInfo);
	pTaskStatusInfo = NULL;
}

static int TaskStatusInfoCB(PTaskStatusInfo pTaskStatusInfo, void * userData)
{
    PSUECContext pSUECContext = (PSUECContext)userData;
    if (NULL != pTaskStatusInfo && NULL != pSUECContext)
    {
        OutputActType curOPAT = OPAT_CONTINUE;
        pthread_mutex_lock(&pSUECContext->dataMutex);
        curOPAT = pSUECContext->opActType;
        pthread_mutex_unlock(&pSUECContext->dataMutex);
        if (curOPAT == OPAT_CONTINUE || (curOPAT == OPAT_SUSPEND &&
            pTaskStatusInfo->statusCode != SC_DOING))
        {
            char * outMsg = NULL;
            if (PackAutoTaskStatusInfo(pTaskStatusInfo, pSUECContext->devID, &outMsg) == 0)
            {
                pthread_mutex_lock(&pSUECContext->opMsgQueueMutex);
                {
                    PQNode enNode = (PQNode)malloc(sizeof(QNode));
                    enNode->pUserData = outMsg;
                    if (QHEnqueue(pSUECContext->opMsgQueue, enNode) != 0)
                    {
                        free(outMsg);
                        free(enNode);
                    }
                }
                pthread_mutex_unlock(&pSUECContext->opMsgQueueMutex);
            }
        }
        ScheDLRetInterceptor(pTaskStatusInfo, pSUECContext);
        if (pTaskStatusInfo->taskType == TASK_DP &&
            (pTaskStatusInfo->statusCode == SC_FINISHED || pTaskStatusInfo->statusCode == SC_ERROR))
            ReportSWInfos(pSUECContext);
        if (pSUECContext->taskStatusCB)
        {
            pSUECContext->taskStatusCB(pTaskStatusInfo, pSUECContext->tsCBUserData);
        }
    }
    return 0;
}

static int ProcReqDownloadRet(int reqDLRet, PDLTaskInfo curRepDLTaskInfo, PSUECContext pSUECContext)
{
    int retCode = SUEC_SUCCESS;
    if (curRepDLTaskInfo == NULL || pSUECContext == NULL) return SUEC_I_PARAMETER_ERROR;
    if (reqDLRet)
    {
        if (curRepDLTaskInfo && curRepDLTaskInfo->pkgType != NULL &&
            curRepDLTaskInfo->url != NULL)  //if download task info complete and effective then add download task, else return not found
        {
            retCode = ProcDLInfo(curRepDLTaskInfo, pSUECContext);
            if (curRepDLTaskInfo->pkgName)
            {
                free(curRepDLTaskInfo->pkgName);
                curRepDLTaskInfo->pkgName = NULL;
            }
            if (curRepDLTaskInfo->pkgType)
            {
                free(curRepDLTaskInfo->pkgType);
                curRepDLTaskInfo->pkgType = NULL;
            }
            if (curRepDLTaskInfo->url)
            {
                free(curRepDLTaskInfo->url);
                curRepDLTaskInfo->url = NULL;
            }
            free(curRepDLTaskInfo);
        }
        else retCode = SUEC_I_REQ_DL_TASK_NOT_FOUND;
    }
    else retCode = SUEC_I_REQ_DL_TIMEOUT;
    return retCode;
}

static int ProcDLInfo(PDLTaskInfo pDLTaskInfo, PSUECContext pSUECContext)
{
    int retCode = SUEC_SUCCESS;
    if (pDLTaskInfo == NULL || pSUECContext == NULL)
		return SUEC_I_PARAMETER_ERROR;

	retCode = SUECCoreAddDLTask(pDLTaskInfo);
	if (retCode != SUEC_SUCCESS)
	{
		retCode = ErrCodeMap(retCode);
		if (retCode == SUEC_I_REQ_DL_TASK_ALRADY_EXIST) {
			retCode = SUEC_S_PKG_DL_TASK_EXIST;
		} else if (retCode == SUEC_S_PKG_FILE_EXIST) { // ignore file exist error
			retCode = SUEC_SUCCESS;
		}
	}
	if (retCode != SUEC_SUCCESS && pSUECContext->outputMsgCB)
	{
		LOGE("SUEClient process download request message error!PkgName:%s,ErrCode:%d,ErrMsg:%s",
			pDLTaskInfo->pkgName, retCode, SuecGetErrorMsg(retCode));
		TaskStatusMsgOutput(pDLTaskInfo->pkgName, TASK_DL, SC_ERROR, retCode, NULL, 0, pDLTaskInfo->pkgOwnerId, pSUECContext);
	}
    return retCode;
}

static int PorcDLReq(char * msg, PSUECContext pSUECContext)
{
	PDLTaskInfo pDLTaskInfo = NULL;
    int retCode = SUEC_SUCCESS;

    if (msg == NULL || pSUECContext == NULL)
		return SUEC_I_PARAMETER_ERROR;

	pDLTaskInfo = (PDLTaskInfo)malloc(sizeof(DLTaskInfo));
	memset(pDLTaskInfo, 0, sizeof(DLTaskInfo));
	if (ParseDLTaskInfo(msg, pDLTaskInfo) == 0)
	{
		retCode = ProcDLInfo(pDLTaskInfo, pSUECContext);
		if (pDLTaskInfo->pkgName) free(pDLTaskInfo->pkgName);
		if (pDLTaskInfo->pkgType) free(pDLTaskInfo->pkgType);
		if (pDLTaskInfo->url) free(pDLTaskInfo->url);

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
	else
	{
		retCode = SUEC_I_MSG_PARSE_FAILED;
	}
	free(pDLTaskInfo);

    return retCode;
}

static int ProcDPReq(char * msg, PSUECContext pSUECContext)
{
	PDPTaskInfo pDPTaskInfo = NULL;
    int retCode = SUEC_SUCCESS;
    if (msg == NULL || pSUECContext == NULL)
		return SUEC_I_PARAMETER_ERROR;

	pDPTaskInfo = (PDPTaskInfo)malloc(sizeof(DPTaskInfo));
	memset(pDPTaskInfo, 0, sizeof(DPTaskInfo));
	if (ParseDPTaskInfo(msg, pDPTaskInfo) == 0)
	{
		//Wei.Gang add
		int iRet = SUECCoreCheckPkgExist(pDPTaskInfo);
		if (iRet == SUEC_CORE_SUCCESS){
		//Wei.Gang add end
			iRet = SUECCoreAddDPTask(pDPTaskInfo);
		}
		if (iRet == SUEC_CORE_I_OBJECT_NOT_FOUND && pSUECContext->outputMsgCB)
		{
			LOGE("SUEClient process deploy request message error!PkgName:%s,ErrCode:%d,ErrMsg:%s",
				pDPTaskInfo->pkgName, SUEC_S_OBJECT_NOT_FOUND, SuecGetErrorMsg(SUEC_S_OBJECT_NOT_FOUND));
			TaskStatusMsgOutput(pDPTaskInfo->pkgName, TASK_DP, SC_ERROR, SUEC_S_OBJECT_NOT_FOUND, NULL, 0, pDPTaskInfo->pkgOwnerId, pSUECContext);
		}
		if (pDPTaskInfo->pkgName)free(pDPTaskInfo->pkgName);
		if (pDPTaskInfo->pkgType)free(pDPTaskInfo->pkgType);
	}
	else retCode = SUEC_I_MSG_PARSE_FAILED;
	free(pDPTaskInfo);

    return retCode;
}

static int PorcReqDLRep(char * msg, PSUECContext pSUECContext)
{
	PDLTaskInfo pDLTaskInfo = NULL;
    int retCode = SUEC_SUCCESS;
    if (msg == NULL || pSUECContext == NULL)
		return SUEC_I_PARAMETER_ERROR;

	pDLTaskInfo = (PDLTaskInfo)malloc(sizeof(DLTaskInfo));
	memset(pDLTaskInfo, 0, sizeof(DLTaskInfo));
	if (ParseReqDLRetInfo(msg, pDLTaskInfo) == 0)
	{
		pthread_mutex_lock(&pSUECContext->dataMutex);
		if (pSUECContext->dlReqPkgName != NULL && strcasecmp(pDLTaskInfo->pkgName, pSUECContext->dlReqPkgName) == 0) //judge the info whether or not match current request package name
		{
			pSUECContext->dlReqRet = 1;
			pSUECContext->repDLTaskInfo = pDLTaskInfo;
		}
		else   //if not match, give up this info
		{
			if (pDLTaskInfo->pkgName) free(pDLTaskInfo->pkgName);
			if (pDLTaskInfo->pkgType) free(pDLTaskInfo->pkgType);
			if (pDLTaskInfo->url) free(pDLTaskInfo->url);
			free(pDLTaskInfo);
		}
		pthread_mutex_unlock(&pSUECContext->dataMutex);
	}
	else
	{
		free(pDLTaskInfo);
		retCode = SUEC_I_MSG_PARSE_FAILED;
	}
    return retCode;
}

static int ProcStatusReq(char * msg, PSUECContext pSUECContext)
{
    int retCode = SUEC_SUCCESS;
    if (msg == NULL || pSUECContext == NULL) return SUEC_I_PARAMETER_ERROR;
    {
        char * pkgName = NULL;
        if (ParseReqTaskStatusInfo(msg, &pkgName) == 0)
        {
            PTaskStatusInfo pTaskStatusInfo = NULL;
            int iRet = SUECCoreGetTaskStatus(pkgName, &pTaskStatusInfo);
            if (iRet != SUEC_SUCCESS)
            {
                int len = sizeof(TaskStatusInfo);
                pTaskStatusInfo = (PTaskStatusInfo)malloc(len);
                memset(pTaskStatusInfo, 0, len);
                len = strlen(pkgName) + 1;
                pTaskStatusInfo->pkgName = (char *)malloc(len);
                memset(pTaskStatusInfo->pkgName, 0, len);
                strcpy(pTaskStatusInfo->pkgName, pkgName);
                pTaskStatusInfo->statusCode = SC_UNKNOW;
                pTaskStatusInfo->taskType = TASK_UNKNOW;
                LOGI("SUEClient get package status, %s status unknow!", pTaskStatusInfo->pkgName);
            }
            if (pSUECContext->outputMsgCB)
            {
                char * outMsg = NULL;
                if (PackReqpTaskStatusInfo(pTaskStatusInfo, pSUECContext->devID, &outMsg) == 0)
                {
                    LOGD("SUEClient output message: %s", outMsg);
                    pSUECContext->outputMsgCB(outMsg, strlen(outMsg) + 1, pSUECContext->opMsgCBUserData);
                    if (outMsg) free(outMsg);
                }
            }
            SUECCoreFreeTSInfo(pTaskStatusInfo);
            pTaskStatusInfo = NULL;

        }
        else retCode = SUEC_I_MSG_PARSE_FAILED;
        if (pkgName) free(pkgName);
    }
    return retCode;
}

static int ProcSWInfoReq(char * msg, PSUECContext pSUECContext)
{
    int retCode = SUEC_SUCCESS;
    if (msg == NULL || pSUECContext == NULL) return SUEC_I_PARAMETER_ERROR;
    {
        char ** swNames = NULL;
        int cnt = 0;
        if (ParseReqSWInfo(msg, &swNames, &cnt) == 0)
        {
            PLHList pSWInfoList = SUECCoreGetSWInfos(swNames, cnt);
            {
                char * outMsg = NULL;
                if (PackSWInfoList(pSWInfoList, pSUECContext->devID, &outMsg) == 0)
                {
                    if (pSUECContext->outputMsgCB)
                    {
                        LOGD("SUEClient output message: %s", outMsg);
                        pSUECContext->outputMsgCB(outMsg, strlen(outMsg) + 1, pSUECContext->opMsgCBUserData);
                    }
                    if (outMsg) free(outMsg);
                }
            }
            if (pSWInfoList)
            {
                LHDestroyList(pSWInfoList);
                pSWInfoList = NULL;
            }
        }
        else retCode = SUEC_I_MSG_PARSE_FAILED;
    }
    return retCode;
}

static int ProcDelPkgReq(char * msg, PSUECContext pSUECContext)
{
    int retCode = SUEC_SUCCESS;
    if (msg == NULL || pSUECContext == NULL) return SUEC_I_PARAMETER_ERROR;
    {
        char ** pkgNames = NULL;
		long pkgOwnerId;
        int cnt = 0;
		char * outMsg = NULL;

        if (ParseReqDelPkgInfo(msg, &pkgNames, &cnt, &pkgOwnerId) == 0)
        {
            PLHList pDelPkgRetInfoList = SUECCoreDelDLPkgs(pkgNames, cnt, pkgOwnerId);
			if (PackDelPkgRetInfoList(pDelPkgRetInfoList, pSUECContext->devID, &outMsg) == 0)
			{
				if (pSUECContext->outputMsgCB)
				{
					LOGD("SUEClient output message: %s", outMsg);
					pSUECContext->outputMsgCB(outMsg, strlen(outMsg) + 1, pSUECContext->opMsgCBUserData);
				}
				if (outMsg) free(outMsg);
			}
            if (pDelPkgRetInfoList)
            {
                LHDestroyList(pDelPkgRetInfoList);
                pDelPkgRetInfoList = NULL;
            }
            if (cnt > 0 && pkgNames != NULL)
            {
                int i = 0;
                for (i = 0; i < cnt; i++)
                {
                    free(pkgNames[i]);
                    pkgNames[i] = NULL;
                }
                free(pkgNames);
                pkgNames = NULL;
            }
        }
        else retCode = SUEC_I_MSG_PARSE_FAILED;
    }
    return retCode;
}

static void ScheDLRetInterceptor(PTaskStatusInfo pTaskStatusInfo, PSUECContext pSUECContext)
{
    if (NULL != pTaskStatusInfo && NULL != pSUECContext)
    {
        if (pTaskStatusInfo != NULL && pTaskStatusInfo->taskType == TASK_DL &&
            (pTaskStatusInfo->statusCode == SC_ERROR || pTaskStatusInfo->statusCode == SC_FINISHED))
        {
            InterceptorSUEScheDLRet(pSUECContext->sueScheHandle, pTaskStatusInfo->pkgName, pTaskStatusInfo->errCode);
        }
    }
}

static int SUEScheOutputMsgCB(char * msg, int msgLen, void * userData)
{
    int iRet = 0;
    PSUECContext pSUECContext = (PSUECContext)userData;
    if (pSUECContext != NULL && pSUECContext->outputMsgCB)
    {
        iRet = pSUECContext->outputMsgCB(msg, strlen(msg) + 1, pSUECContext->opMsgCBUserData); //output msg
    }
    return iRet;
}

static void ReportSWInfos(PSUECContext pSUECContext)
{
    if (pSUECContext != NULL)
    {
        PLHList pSWInfoList = SUECCoreGetSWInfos(NULL, 0);
        {
            char * outMsg = NULL;
            if (PackSWInfoList(pSWInfoList, pSUECContext->devID, &outMsg) == 0)
            {
                if (pSUECContext->outputMsgCB)
                {
                    LOGD("SUEClient output message: %s", outMsg);
                    pSUECContext->outputMsgCB(outMsg, strlen(outMsg) + 1, pSUECContext->opMsgCBUserData);
                }
                if (outMsg) free(outMsg);
            }
        }
        if (pSWInfoList)
        {
            LHDestroyList(pSWInfoList);
            pSWInfoList = NULL;
        }
    }
}

static char * GetEnvFileDirFromCfg(char * cfgFile)
{
#define ENV_FILE_DIR_INI_ENTRY                   "Other:EnvFileDir"
    char * retStr = NULL;
    if (cfgFile)
    {
        char * realCfgPath = NULL, *tmpCfgPath = NULL;
        dictionary * dic = NULL;
        char modulePath[MAX_PATH] = { 0 };
        util_module_path_get(modulePath);
        if (*cfgFile != '/' && (strstr(cfgFile, ":\\") == NULL) &&
            (strstr(cfgFile, ":/") == NULL)) //relative path
        {
            int len = 0;
            len = strlen(modulePath) + strlen(cfgFile) + 1;
            tmpCfgPath = (char *)malloc(len);
            memset(tmpCfgPath, 0, len);
            sprintf(tmpCfgPath, "%s%s", modulePath, cfgFile);
            realCfgPath = tmpCfgPath;
        }
        else
			realCfgPath = cfgFile; //absolute path

        if (util_is_file_exist(realCfgPath))
        {
            dic = iniparser_load(realCfgPath);
            if (dic)
            {
                int len = 0;
                const char * envFileDir = NULL;
                envFileDir = iniparser_getstring(dic, ENV_FILE_DIR_INI_ENTRY, NULL);
                if (envFileDir != NULL && strlen(envFileDir) > 0)
                {
                    char * tmpPath = NULL;
                    if (*envFileDir != '/' && (strstr(envFileDir, ":\\") == NULL) &&
                        (strstr(envFileDir, ":/") == NULL))
                    {
                        len = strlen(envFileDir) + strlen(modulePath) + 1;
                        tmpPath = (char *)malloc(len);
                        memset(tmpPath, 0, len);
                        sprintf(tmpPath, "%s%s", modulePath, envFileDir);
                    }
                    else
                    {
                        len = strlen(envFileDir) + 1;
                        tmpPath = (char *)malloc(len);
                        memset(tmpPath, 0, len);
                        strcpy(tmpPath, envFileDir);
                    }
                    if (util_is_file_exist(tmpPath))
                    {
                        retStr = tmpPath;
                    }
                    else free(tmpPath);
                }
            }
        }
        if (dic) iniparser_freedict(dic);
        if (tmpCfgPath) free(tmpCfgPath);
    }
    return retStr;
}
// Wei.Gang add. If there is not Sche.cfg file, send cmd to delete all schedule.
static void CheckAndSendDelScheCmd(PSUECContext pSUECContext)
{
	FILE * fp = NULL;
	int len = 0;
	char * realCfgPath = NULL;
	char modulePath[MAX_PATH] = { 0 };
	util_module_path_get(modulePath);
	len = strlen(modulePath) + strlen(SCHE_CFG_FILE) + 1;
	realCfgPath = (char *)malloc(len);
	memset(realCfgPath, 0, len);
	sprintf(realCfgPath, "%s%s", modulePath, SCHE_CFG_FILE);

	if (!util_is_file_exist(realCfgPath)){
		//printf("sche.cfg is not exist");
		if (pSUECContext != NULL)
		{
			int retCode = SUEC_SUCCESS;
			char * outMsg = NULL;
			char * msg = "{\"Data\":{\"ScheInfo\":{\"PkgType\":\"\",\"ActType\":15},\"DevID\":\"aaa\",\"CmdID\":111},\"SendTS\":1526028147867}";
			if (PackDelScheRepInfo(msg, retCode, GetDevID(), &outMsg) == 0)
			{
				if (pSUECContext->outputMsgCB){
					LOGD("SUEClient output message: %s", outMsg);
					pSUECContext->outputMsgCB(outMsg, strlen(outMsg) + 1, pSUECContext->opMsgCBUserData);
				}
				if (outMsg) free(outMsg);
				//Create empty file.
				fp = fopen(realCfgPath, "wb");
				if (fp) fclose(fp);
			}
		}
	}
	//else{
	//	printf("sche.cfg is exist");
	//}
	if (realCfgPath) free(realCfgPath);
}

//arch: openwrt
#ifdef linux
static char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }

    return s;
}

static void InitWSParams(char * cfgFile, PSUECContext pSUECContext){
	char *start, *end, *wsUrl, *path = NULL;
	char port[12];
	char domainName[128];
	int length, rv;
	struct addrinfo hints, *servinfo;

    if (cfgFile != NULL && (strlen(cfgFile) > 0)) {
        WSGetUrl(cfgFile, pSUECContext);
		do {
			if (pSUECContext->wsUrl == NULL || strlen(pSUECContext->wsUrl) <= 0) {
				break;
			}

			wsUrl = pSUECContext->wsUrl;

			// 1) get domain name
			start = strstr(wsUrl, "//");
			if (start == NULL) { // no "protocol://index"
				start = wsUrl;
			} else {
				start += 2; // skip "//"
			}
			end = strchr(start, ':');
			if (end == NULL) { // invalid url format
				LOGE("Invalid url format 1! [%s]", wsUrl);
				break;
			}
			// copy domain name
			if (sizeof(domainName) <  end-start) { // buffer is not enough
				LOGE("domainName buffer is not enough");
				break;
			}
			memcpy(domainName, start, end-start);
			domainName[end-start] = '\0';

			// 2) get port
			start = strchr(end, ':');
			if (start == NULL) { // invalid url format
				LOGE("Invalid url format 2! [%s]", wsUrl);
				break;
			}
			start += 1; // skip ":"
			end = strchr(start, '/');
			if (end == NULL) { // no index url
				end = start + strlen(start);
			}
			// copy port
			if (sizeof(port) <  end-start) { // buffer is not enough
				LOGE("port buffer is not enough");
				break;
			}
			memcpy(port, start, end-start);
			port[end-start] = '\0';

			// 3) get index page
			if (*end == '/') { // has index url
				start = end;
				end = start + strlen(start);
				length = end - start;
				// copy path
				if (length > 0) {
					// fill path
					path = (char *) malloc(length);
					memcpy(path, start, length);
					path[length] = '\0';
				}
			}

			// 4) Convert domain name to IP
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			if ((rv = getaddrinfo(domainName, port, &hints, &servinfo)) != 0) {
				LOGE("getaddrinfo: %s\n", gai_strerror(rv));
				break;
			}

			// fill parameters
			// treat first serverinfo as result
			if (servinfo) {
				pSUECContext->wsParams.ip = (char *) malloc(40); // 40 can save ipv4 or ipv6
				get_ip_str(servinfo->ai_addr, pSUECContext->wsParams.ip, 40);
				pSUECContext->wsParams.port = strtol(port, NULL, 10);
				pSUECContext->wsParams.interfacePath = path;
				freeaddrinfo(servinfo);
			}
		} while (0);
    }

	if (pSUECContext->wsParams.ip == NULL) {
        length = 12;
        pSUECContext->wsParams.interfacePath = (char*)malloc(length);
        strcpy(pSUECContext->wsParams.interfacePath, "/OTAService");

        pSUECContext->wsParams.ip = (char *)malloc(length);
        strcpy(pSUECContext->wsParams.ip, "127.0.0.1");

        pSUECContext->wsParams.port = 3000;
	}

	// print debug
	LOGI("wsParams.ip=[%s]", pSUECContext->wsParams.ip);
	LOGI("wsParams.port=%d", pSUECContext->wsParams.port);
	if (pSUECContext->wsParams.interfacePath) {
		LOGI("wsParams.interfacePath=[%s]", pSUECContext->wsParams.interfacePath);
	}

    length = 20;
    pSUECContext->wsParams.connection = (char *)malloc(length);
    strcpy(pSUECContext->wsParams.connection, "keep-alive,Upgrade");

    pSUECContext->wsParams.accept = (char *)malloc(length);
    strcpy(pSUECContext->wsParams.accept, "*/*");

    pSUECContext->wsParams.secWebsocketKey = (char *)malloc(length);
    strcpy(pSUECContext->wsParams.secWebsocketKey, "websocket");

    pSUECContext->wsParams.secWebSocketVersion = 13;

    pSUECContext->wsParams.upgrade = (char *)malloc(length);
    strcpy(pSUECContext->wsParams.upgrade, "websocket");

	length = 32;
    pSUECContext->wsParams.secWebSocketAccept = (char *)malloc(length);
    strcpy(pSUECContext->wsParams.secWebSocketAccept, "qVby4apnn2tTYtB1nPPVYUn68gY=");
}

static void WSDelayms(unsigned int ms)
{
	struct timeval tim;
	tim.tv_sec = ms / 1000;
	tim.tv_usec = (ms % 1000) * 1000;
	select(0, NULL, NULL, NULL, &tim);
}

//连接正常，返回连接句柄
///连接失败返回-1；
static int WSConnectToServer(PSUECContext pSUECContext){
	int iRet = -1;

	int connfd, timeout;

	char headBuf[1024] = { 0 };
	char recvBuf[1024] = { 0 };
	char *p = NULL;
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(pSUECContext->wsParams.port);
	if ((serverAddr.sin_addr.s_addr = inet_addr(pSUECContext->wsParams.ip)) == INADDR_NONE){
        LOGD("OTAService sub-device connecting ip is invalid!");
		return iRet;
	}
	if ((connfd = socket(AF_INET, SOCK_STREAM, 0))<0){
        LOGD("creat socket of connecting to OTAService sub-device error!");
		return iRet;
    }
	timeout = 0;
	while ((connect(connfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)){
		if (++timeout>WEBSOCKET_CONNECT_TIMEOUT_MS){
            LOGD("connecting to OTAService sub-device time out!");
			close(connfd);
			return iRet;
		}
		WSDelayms(1);
	}

	const char httpDemo[] = "GET %s HTTP/1.1\r\n"
							"Connection: %s\r\n"
							"Host: %s:%d\r\n"
							"Accept: %s\r\n"
							"Sec-WebSocket-Key: %s\r\n"
							"Sec-WebSocket-Version: %d\r\n"
							"Upgrade: %s\r\n\r\n";

	sprintf(headBuf, httpDemo, pSUECContext->wsParams.interfacePath, pSUECContext->wsParams.connection, pSUECContext->wsParams.ip, pSUECContext->wsParams.port, pSUECContext->wsParams.accept, pSUECContext->wsParams.secWebsocketKey, pSUECContext->wsParams.secWebSocketVersion, pSUECContext->wsParams.upgrade);
	send(connfd, headBuf, strlen(headBuf), MSG_NOSIGNAL);

	while (1){
		int ret = recv(connfd, recvBuf, sizeof(recvBuf), MSG_NOSIGNAL);
		if (ret>0)
		{
			if (strncmp((const char *)recvBuf,(const char *)"HTTP",strlen((const char *)"HTTP"))==0)
			{
				if ((p = strstr(recvBuf, (const char *)"Sec-WebSocket-Accept: ")) != NULL)
				{
					p += strlen((const char *)"Sec-WebSocket-Accept: ");
					if (strncmp((const char *)pSUECContext->wsParams.secWebSocketAccept, (const char *)p,strlen(pSUECContext->wsParams.secWebSocketAccept)) == 0)
					{
                        return connfd;
					}
					else{
						send(connfd, headBuf, strlen(headBuf), MSG_NOSIGNAL);
					}
				}
				else{
					send(connfd, headBuf, strlen(headBuf), MSG_NOSIGNAL);
				}
			}
		}else{
			send(connfd, headBuf, strlen(headBuf), MSG_NOSIGNAL);
        }
		if (++timeout>=WEBSOCKET_WAIT_RESPOND_TIMEOUT_MS)
		{
			close(connfd);
			return iRet;
		}
		WSDelayms(1);
	}
	close(connfd);
	return iRet;
}

/*
return data start address for success
return NULL for fail
*/
static int WSRecvData(int connfd, char *data, int dataLen){

	int msglen = 0;
	unsigned char header[4];

	// waiting for websocket start
	do {
		if (recv(connfd, header, 1, MSG_NOSIGNAL) <= 0) {
			LOGD("WSRecvData: errno=%d", errno);
			return -1;
		}

		if (header[0] == 0x81)	{
			break;
		}
	} while (1);

	// check message length
	if (recv(connfd, header, 1, MSG_NOSIGNAL) <= 0) {
		LOGD("WSRecvData: errno=%d", errno);
		return -1;
	}
	if (header[0] == 0x7E) { // two bytes length
		// check message length
		if (recv(connfd, header, 2, MSG_NOSIGNAL) <= 0) {
			LOGD("WSRecvData: errno=%d", errno);
			return -1;
		}
		msglen = (((int) header[0] << 8) | ((int) header[1]));
	} else { // one byte length
		msglen = (int) header[0];
	}

	// read data
	if ((msglen+1) > dataLen) {
		LOGD("WSRecvData: dataLen is not engouth %d < %d", msglen+1, dataLen);
		return -1;
	}

	if (recv(connfd, data, msglen, MSG_NOSIGNAL) <= 0) {
		LOGD("WSRecvData: errno=%d", errno);
		return -1;
	}
	data[msglen] = '\0';

	LOGD("WSRecvData data=%d[%s]", msglen, data);

	return 0;
}

static void* MonitorSubDevThreadStart(void *args)
{
#define MAX_WS_SERVER_RETRY	3
	PSUECContext pSUECContext = (PSUECContext)args;
	int connfd = -1, retry = 0;
	char buffer[512];
	char *pkgName = NULL, *devID = NULL, *status = NULL, *description = NULL;
	cJSON *root = NULL, *event = NULL, *data = NULL, *item = NULL;
	int statusCode;
	char *msg = NULL;
	int msgLen = 256;

	if (pSUECContext != NULL)
	{
        //connected OTAService successfully!
		while (pSUECContext->isMonitorSubDevThreadRunning)
		{
			if (connfd == -1) {
				LOGI("MonitorSubDevThreadStart: WS Connect to server...");
				connfd = WSConnectToServer(pSUECContext);
				if (connfd == -1) {
					LOGE("MonitorSubDevThreadStart: Connect to server fail!");
					retry++;
					if (retry > MAX_WS_SERVER_RETRY) {
						LOGE("MonitorSubDevThreadStart: Reach MAX retry time!");
						break;
					}

					sleep(retry);
					continue;
				}
				retry = 0; // reset retry
			}

			if ( WSRecvData(connfd, buffer, sizeof(buffer)) != 0 ) { // receive error
				LOGE("MonitorSubDevThreadStart: WSRecvData fail!");
				close(connfd);
				sleep(1);
				connfd = -1;
				continue;
			}

			pkgName = devID = status = description = NULL;

			//parse input message
			do {
				root = cJSON_Parse(buffer);
				if (root == NULL) {
					break; // skip if parsing fail.
				}

				event = cJSON_GetObjectItem(root, "event");
				if (event == NULL || event->type == cJSON_NULL || strcmp(event->valuestring, "eOTA_StatusUpdate")) {
					break; // skip if invalid event
				}

				data = cJSON_GetObjectItem(root, "data");
				if (data == NULL) {
					break; // skip if no data
				}

				item = cJSON_GetObjectItem(data, "deviceID");
				if (item != NULL && item->type != cJSON_NULL && (strlen(item->valuestring) > 0)) {
					devID = (char *) malloc(strlen(item->valuestring) + 1);
					strcpy(devID, item->valuestring);
				}

				item = cJSON_GetObjectItem(data, "package");
				if(item != NULL && item->type != cJSON_NULL && (strlen(item->valuestring) > 0)){
					pkgName = (char *) malloc(strlen(item->valuestring) + 1);
					strcpy(pkgName, item->valuestring);
				}

				//OTA_status
				item = cJSON_GetObjectItem(data,"OTA_status");
				if (item != NULL && item->type != cJSON_NULL && (strlen(item->valuestring)) > 0) {
					status = (char *) malloc(strlen(item->valuestring) + 1);
					strcpy(status, item->valuestring);
				}

				//description
				item = cJSON_GetObjectItem(data,"description");
				if (item != NULL && item->type != cJSON_NULL && (strlen(item->valuestring)) > 0) {
					description = (char *)malloc(strlen(item->valuestring) + 1);
					strcpy(description, item->valuestring);
				}

				//report msg
				if (pkgName == NULL || devID == NULL || status == NULL) {
					break; // skip if input is invalid
				}

				msg = (char *) malloc(msgLen);
				if (msg != NULL) {
					if (!strcmp(status, "idle")) {
						statusCode = SC_FINISHED;
					}else if (!strcmp(status, "processing")) {
						statusCode = SC_START;
					}else if(!strcmp(status,"success")){
						statusCode = SC_FINISHED;
					}else{
						statusCode = SC_ERROR;
					}
					strncpy(msg, description, strlen(description));
					msg[strlen(description)] = '\0';
					/* code */
					SubDevTaskStatusMsgOutput(pkgName, devID, TASK_DP, statusCode, 0, msg, 0, pSUECContext);
					free(msg);
				}
			} while (0);

			if (pkgName != NULL) {
				free(pkgName);
				pkgName = NULL;
			}
			if(devID != NULL){
				free(devID);
				devID = NULL;
			}
			if(status != NULL){
				free(status);
				status = NULL;
			}
			if(description != NULL){
				free(description);
				description = NULL;
			}
			if (root) {
				cJSON_Delete(root);
				root = NULL;
			}

			usleep(100*1000);
		}
	}
	return 0;
}

static int MonitorSubDevProcStart(PSUECContext pSUECContext){
	int retCode = SUEC_CORE_SUCCESS;
	if (pSUECContext == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
	{
		if (!pSUECContext->isMonitorSubDevThreadRunning)
		{
			pSUECContext->isMonitorSubDevThreadRunning = 1;
			if (pthread_create(&pSUECContext->MonitorSubDevThreadT, NULL, MonitorSubDevThreadStart, pSUECContext) != 0)
			{
				pSUECContext->isMonitorSubDevThreadRunning = 0;
				retCode = SUEC_CORE_I_TASK_PROCESS_START_FAILED;
			}
		}
	}
	return retCode;
}

static int MonitorSubDevProcStop(PSUECContext pSUECContext)
{
	int retCode = SUEC_CORE_SUCCESS;
	if (pSUECContext == NULL) return SUEC_CORE_I_PARAMETER_ERROR;
	{
		if (pSUECContext->isMonitorSubDevThreadRunning)
		{
			pSUECContext->isMonitorSubDevThreadRunning = 0;
		}
		pthread_join(pSUECContext->MonitorSubDevThreadT, NULL);
		pSUECContext->MonitorSubDevThreadT = 0;
		//add other thread stop
	}
	return retCode;
}

static int WSGetUrl(char * cfgFile, PSUECContext pSUECContext)
{
#define WEB_SOCKET_URL_ENTRY                     "SUECCore:WebSocketURL"
    int retCode = SUEC_CORE_SUCCESS;
    if (cfgFile)
    {
        char * realCfgPath = NULL, *tmpCfgPath = NULL;
        dictionary * dic = NULL;
        char modulePath[MAX_PATH] = { 0 };
        util_module_path_get(modulePath);
        if (*cfgFile != '/' && (strstr(cfgFile, ":\\") == NULL) &&
            (strstr(cfgFile, ":/") == NULL))
        {
            int len = 0;
            len = strlen(modulePath) + strlen(cfgFile) + 1;
            tmpCfgPath = (char *)malloc(len);
            memset(tmpCfgPath, 0, len);
            sprintf(tmpCfgPath, "%s%s", modulePath, cfgFile);
            realCfgPath = tmpCfgPath;
        }
        else realCfgPath = cfgFile;

        if (util_is_file_exist(realCfgPath))
        {
            dic = iniparser_load(realCfgPath);
            if (dic)
            {
                int len = 0;
				{
					const char * webSocketUrl = NULL;
					webSocketUrl = iniparser_getstring(dic, WEB_SOCKET_URL_ENTRY, NULL);
					if (webSocketUrl != NULL && strlen(webSocketUrl) > 0)
					{
						len = strlen(webSocketUrl) + 1;
						pSUECContext->wsUrl = (char *)malloc(len);
						memset(pSUECContext->wsUrl, 0, len);
						strcpy(pSUECContext->wsUrl, webSocketUrl);
					}
				}
            }
        }
        if (dic) iniparser_freedict(dic);
        if (tmpCfgPath) free(tmpCfgPath);
    }
    return retCode;
}
#endif