/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2016/07/14 by Scott Chang									*/
/* Modified Date: 2016/07/14 by Scott Chang									*/
/* Abstract     : Sample Handler to parse json string in c:/test.txt file	*/
/*                and report to server.										*/
/* Reference    : None														*/
/****************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "WISEPlatform.h"
#include "srp/susiaccess_handler_api.h"
#include "DeviceMessageGenerate.h"
#include "IoTMessageGenerate.h"
#include <cJSON.h>
#include "HandlerKernel.h"
#include "util_path.h"
#include <time.h>
#include "ProcessMonitorLog.h"
#include "ReadINI.h"
#include "ProcessList.h"
#include "process_config.h"
#include "ProcessParser.h"
#include "wrapper.h"

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define cagent_request_custom 2102 /*define the request ID for V3.0, not used on V3.1 or later*/
#define cagent_custom_action 31002 /*define the action ID for V3.0, not used on V3.1 or later*/
#define SPEC_INFO_TIME_OUT_MS 5000
#ifdef OLDFMT
const char strHandlerName[MAX_TOPIC_LEN] = {"ProcessMonitor"}; /*declare the handler name*/
#else
const char strHandlerName[MAX_TOPIC_LEN] = {"ProcessMonitorV2"}; /*declare the handler name*/
#endif
//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct
{
	pthread_t threadHandler; // thread handle
	int interval;			 // time interval for file read
	bool isThreadRunning;	//thread running flag

	MSG_CLASSIFY_T *pCapability;
	prc_mon_info_node_t *prcMonInfoList;
	sys_mon_info_t sysMonInfo;

	bool isUserLogon;
	int gatherLevel;
	char sysUserName[33];

	int iRetrieveInterval; //10 sec.
	bool bEnProcessStatus;
	char* strProcessList;

	bool getCapability;

} swm_handler_context_t;

typedef enum
{
	swm_restart_prc_req = 167,
	swm_restart_prc_rep,
	swm_kill_prc_req,
	swm_kill_prc_rep,
	swm_exec_prc_req,
	swm_exec_prc_rep,
	swm_error_rep = 600,
} process_comm_cmd_t;
//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static Handler_info g_PluginInfo; //global Handler info structure
static swm_handler_context_t g_HandlerContex;
static HANDLER_THREAD_STATUS g_status = handler_status_no_init; // global status flag.
static HandlerSendCbf g_sendcbf = NULL;							// Client Send information (in JSON format) to Cloud Server

#define PROCESSMONIOTR_INI_COTENT "[Platform]\nInterval=10\n#Interval: The time delay between two access round in second.\nEnableProcessStatus=0\n#EnableProcessStatus: Disabling (0) or Enabling (1) process status gather.\nProcessStatusList=CAgent.exe\n#List the process name to get status."
#define PROCESSMONIOTR_INI_FORMAT "[Platform]\nInterval=%d\n#Interval: The time delay between two access round in second.\nEnableProcessStatus=%d\n#EnableProcessStatus: Disabling (0) or Enabling (1) process status gather.\nProcessStatusList=%s\n#List the process name to get status."

//-----------------------------------------------------------------------------
// Function:
//-----------------------------------------------------------------------------
void Handler_Uninitialize();
void GetPrcMonInfo(prc_mon_info_node_t *head);

#ifdef ANDROID
typedef pthread_t sp_pthread_t;
static int pthread_cancel(sp_pthread_t thread) {
        return (kill(thread, SIGTERM));
}
#endif

#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL)					  // Dynamic load
		{
			// Initialize your stuff or whatever
			// Return FALSE if you don't want your module to be dynamically loaded
		}
		else // Static load
		{
			// Return FALSE if you don't want your module to be statically loaded
			return FALSE;
		}
	}

	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
	{
		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize();
		}
		else // Process is terminating
		{
			// Cleanup
			Handler_Uninitialize();
		}
	}
	return TRUE;
}
#else
__attribute__((constructor))
/**
 * initializer of the shared lib.
 */
static void
Initializer(int argc, char **argv, char **envp)
{
	printf("DllInitializer\r\n");
}

__attribute__((destructor))
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void
Finalizer()
{
	printf("DllFinalizer\r\n");
	Handler_Uninitialize();
}
#endif

void SWMKillPrc(prc_mon_info_node_t *prcMonInfoList, char *killPrcInfo, char *sessionID)
{
	if (killPrcInfo == NULL || prcMonInfoList == NULL)
		return;
	{
		unsigned int *temp = NULL;
		unsigned int pid_size;
		char repMsg[2 * 1024] = {0};
		char errorStr[128] = {0};

		if (Parse_swm_kill_prc_req(killPrcInfo, &pid_size, &temp))
		{
			unsigned int *pPrcID = temp;
			unsigned int i = 0;

			for (i = 0; i < pid_size; i++)
			{
				if (pPrcID[i])
				{
					prc_mon_info_node_t *findPrcMonInfoNode = NULL;
					findPrcMonInfoNode = FindPrcMonInfoNode(prcMonInfoList, pPrcID[i]);
					if (findPrcMonInfoNode)
					{
						bool bRet = false;
						bRet = KillProcessWithID(pPrcID[i]);
						if (!bRet)
						{
							snprintf(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#kill failed#tk#!", pPrcID[i]);
						}
						else
						{
							snprintf(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#kill success#tk#!", pPrcID[i]);

							HandlerKernel_LockCapability();
							if (UpdateLogonUserPrcList(g_HandlerContex.prcMonInfoList, &g_HandlerContex.isUserLogon, g_HandlerContex.sysUserName, g_HandlerContex.gatherLevel, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList))
							{
								if (g_HandlerContex.isUserLogon)
									GetPrcMonInfo(g_HandlerContex.prcMonInfoList);
								UpdateCapability(g_HandlerContex.pCapability, g_HandlerContex.prcMonInfoList, &g_HandlerContex.sysMonInfo, g_HandlerContex.iRetrieveInterval, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList, g_HandlerContex.isUserLogon, true);
							}
							HandlerKernel_UnlockCapability();
							HandlerKernel_SetCapability(g_HandlerContex.pCapability, true);
						}
					}
				}
				else
				{
					memset(errorStr, 0, sizeof(errorStr));
					sprintf(errorStr, "Command(%d) params error!", swm_kill_prc_req);
					{
						char *uploadRepJsonStr = NULL;
						char *str = errorStr;
						int jsonStrlen = Parser_string(str, sessionID, &uploadRepJsonStr);
						if (jsonStrlen > 0 && uploadRepJsonStr != NULL)
						{
							g_sendcbf(&g_PluginInfo, swm_kill_prc_rep, uploadRepJsonStr, strlen(uploadRepJsonStr) + 1, NULL, NULL);
						}
						if (uploadRepJsonStr)
							free(uploadRepJsonStr);
					}
				}
			}
		}
		else
		{
			memset(errorStr, 0, sizeof(errorStr));
			sprintf(errorStr, "Command(%d) parse error!", swm_kill_prc_req);
			{
				char *uploadRepJsonStr = NULL;
				char *str = errorStr;
				int jsonStrlen = Parser_string(str, sessionID, &uploadRepJsonStr);
				if (jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_kill_prc_rep, uploadRepJsonStr, strlen(uploadRepJsonStr) + 1, NULL, NULL);
				}
				if (uploadRepJsonStr)
					free(uploadRepJsonStr);
			}
		}
		if (temp)
		{
			free(temp);
		}
		if (strlen(repMsg) == 0)
		{
			char *uploadRepJsonStr = NULL;
			char *str = errorStr;
			int jsonStrlen = Parser_string(str, sessionID, &uploadRepJsonStr);
			if (jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, swm_kill_prc_rep, uploadRepJsonStr, strlen(uploadRepJsonStr) + 1, NULL, NULL);
			}
			if (uploadRepJsonStr)
				free(uploadRepJsonStr);
		}
	}
}

void SWMRestartPrc(prc_mon_info_node_t *prcMonInfoList, char *restartPrcInfo, char *sessionID)
{
	if (restartPrcInfo == NULL || prcMonInfoList == NULL)
		return;
	{
		unsigned int *temp = NULL;
		unsigned int pid_size = 0;
		char repMsg[2 * 1024] = {0};
		char errorStr[128] = {0};

		if (Parse_swm_restart_prc_req(restartPrcInfo, &pid_size, &temp))
		{
			unsigned int *pPrcID = temp;
			unsigned int i = 0;

			for (i = 0; i < pid_size; i++)
			{
				if (pPrcID)
				{
					prc_mon_info_node_t *findPrcMonInfoNode = NULL;
					findPrcMonInfoNode = FindPrcMonInfoNode(prcMonInfoList, pPrcID[i]);

					if (findPrcMonInfoNode)
					{
						unsigned int newPrcID = 0;
						newPrcID = RestartProcessWithID(pPrcID[i]);
						if (newPrcID > 0)
						{
							snprintf(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#restart success#tk#!", pPrcID[i]);

							HandlerKernel_LockCapability();
							if (UpdateLogonUserPrcList(g_HandlerContex.prcMonInfoList, &g_HandlerContex.isUserLogon, g_HandlerContex.sysUserName, g_HandlerContex.gatherLevel, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList))
							{
								if (g_HandlerContex.isUserLogon)
									GetPrcMonInfo(g_HandlerContex.prcMonInfoList);
								UpdateCapability(g_HandlerContex.pCapability, g_HandlerContex.prcMonInfoList, &g_HandlerContex.sysMonInfo, g_HandlerContex.iRetrieveInterval, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList, g_HandlerContex.isUserLogon, true);
							}
							HandlerKernel_UnlockCapability();
							HandlerKernel_SetCapability(g_HandlerContex.pCapability, true);
						}
						else
						{
							snprintf(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#restart failed#tk#!", pPrcID[i]);
						}
					}
				}
				else
				{
					memset(errorStr, 0, sizeof(errorStr));
					sprintf(errorStr, "Command(%d) params error!", swm_restart_prc_req);
					{
						char *uploadRepJsonStr = NULL;
						char *str = errorStr;
						int jsonStrlen = Parser_string(str, sessionID, &uploadRepJsonStr);
						if (jsonStrlen > 0 && uploadRepJsonStr != NULL)
						{
							g_sendcbf(&g_PluginInfo, swm_restart_prc_rep, uploadRepJsonStr, strlen(uploadRepJsonStr) + 1, NULL, NULL);
						}
						if (uploadRepJsonStr)
							free(uploadRepJsonStr);
					}
				}
			}
		}
		else
		{
			memset(errorStr, 0, sizeof(errorStr));
			sprintf(errorStr, "Command(%d) parse error!", swm_restart_prc_req);
			{
				char *uploadRepJsonStr = NULL;
				char *str = errorStr;
				int jsonStrlen = Parser_string(str, sessionID, &uploadRepJsonStr);
				if (jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_restart_prc_rep, uploadRepJsonStr, strlen(uploadRepJsonStr) + 1, NULL, NULL);
				}
				if (uploadRepJsonStr)
					free(uploadRepJsonStr);
			}
		}
		if (temp)
		{
			free(temp);
		}
		if (strlen(repMsg) > 0)
		{
			{
				char *uploadRepJsonStr = NULL;
				char *str = repMsg;
				int jsonStrlen = Parser_string(str, sessionID, &uploadRepJsonStr);
				if (jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_restart_prc_rep, uploadRepJsonStr, strlen(uploadRepJsonStr) + 1, NULL, NULL);
				}
				if (uploadRepJsonStr)
					free(uploadRepJsonStr);
			}
		}
	}
}

void SWMExecPrc(char* execPrcInfo, char* sessionID)
{
	char prcPath[DEF_MAX_PATH] = { 0 };
	char prcArg[DEF_MAX_PATH] = { 0 };
	bool bHide = true;
	
	char repMsg[2 * 1024] = { 0 };
	

	if (execPrcInfo == NULL)
		return;
	
	if (Parse_swm_exec_prc_req(execPrcInfo, prcPath, prcArg, &bHide))
	{
		char cmdline[DEF_MAX_PATH] = { 0 };
		char errmsg[2 * 1024] = { 0 };
		unsigned long newPrcID = 0;
		unsigned long errcode = -1;
		snprintf(cmdline, sizeof(cmdline), "%s %s", prcPath, prcArg);
		if (RunProcessAsUser(cmdline, false, !bHide, &newPrcID, errmsg))
		{
			errcode = 0;
			snprintf(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#execute success#tk#!", newPrcID);
		}
		else
		{
			errcode = newPrcID;
			snprintf(repMsg, sizeof(repMsg), "Process #tk#execute failed#tk#, %s!", errmsg);
		}

		if (strlen(repMsg) > 0)
		{
			char* uploadRepJsonStr = NULL;
			char* str = repMsg;
			int jsonStrlen = Parser_errstring(str, errcode, sessionID, &uploadRepJsonStr);
			if (jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, swm_exec_prc_rep, uploadRepJsonStr, strlen(uploadRepJsonStr) + 1, NULL, NULL);
			}
			if (uploadRepJsonStr)
				free(uploadRepJsonStr);
		}
	}
	else
	{
		char errorStr[128] = { 0 };
		char* execRepJsonStr = NULL;
		int jsonStrlen = 0;
		sprintf(errorStr, "Command(%d) parse error!", swm_exec_prc_req);
		jsonStrlen = Parser_errstring(errorStr, -1, sessionID, &execRepJsonStr);
		if (jsonStrlen > 0 && execRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, swm_exec_prc_rep, execRepJsonStr, strlen(execRepJsonStr) + 1, NULL, NULL);
		}
		if (execRepJsonStr)
			free(execRepJsonStr);
	}
}

/*Create Capability Message Structure to describe sensor data*/
MSG_CLASSIFY_T *CreateCapability()
{
	long sysTotalPhysMemKB = 0, sysAvailPhysMemKB = 0;
	int cpuUsage = 0;
	MSG_CLASSIFY_T *myCapability = IoT_CreateRoot((char *)strHandlerName);

	if (GetSysCPUUsage(&cpuUsage, &g_HandlerContex.sysMonInfo.sysCpuUsageLastTimes))
	{
		g_HandlerContex.sysMonInfo.cpuUsage = cpuUsage;
	}

	if (GetSysMemoryUsageKB(&sysTotalPhysMemKB, &sysAvailPhysMemKB))
	{
		g_HandlerContex.sysMonInfo.totalPhysMemoryKB = sysTotalPhysMemKB;
		g_HandlerContex.sysMonInfo.availPhysMemoryKB = sysAvailPhysMemKB;
		if (sysTotalPhysMemKB > 0)
			g_HandlerContex.sysMonInfo.usagePhyMemory = (double)(sysTotalPhysMemKB - sysAvailPhysMemKB) * 100 / (double)sysTotalPhysMemKB;
	}

	g_HandlerContex.prcMonInfoList = CreatePrcMonInfoList();

	UpdateLogonUserPrcList(g_HandlerContex.prcMonInfoList, &g_HandlerContex.isUserLogon, g_HandlerContex.sysUserName, g_HandlerContex.gatherLevel, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList);

	/*TODO: Upload capability*/
	UpdateCapability(myCapability, g_HandlerContex.prcMonInfoList, &g_HandlerContex.sysMonInfo, g_HandlerContex.iRetrieveInterval, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList, g_HandlerContex.isUserLogon, true);
	return myCapability;
}

void *ProcessRetrieveThread(void *args)
{
	swm_handler_context_t *pSoftwareMonHandlerContext = (swm_handler_context_t *)args;
	long long startTime = MSG_GetTimeTick();

	if (!pSoftwareMonHandlerContext->pCapability)
	{
		pSoftwareMonHandlerContext->pCapability = CreateCapability();
		HandlerKernel_SetCapability(pSoftwareMonHandlerContext->pCapability, pSoftwareMonHandlerContext->getCapability);
	}

	while (pSoftwareMonHandlerContext->isThreadRunning)
	{
		long sysTotalPhysMemKB = 0, sysAvailPhysMemKB = 0;
		int cpuUsage = 0;
		bool bUpdateCapability = false;

		if (GetSysCPUUsage(&cpuUsage, &pSoftwareMonHandlerContext->sysMonInfo.sysCpuUsageLastTimes))
		{
			pSoftwareMonHandlerContext->sysMonInfo.cpuUsage = cpuUsage;
		}

		if (GetSysMemoryUsageKB(&sysTotalPhysMemKB, &sysAvailPhysMemKB))
		{
			pSoftwareMonHandlerContext->sysMonInfo.totalPhysMemoryKB = sysTotalPhysMemKB;
			pSoftwareMonHandlerContext->sysMonInfo.availPhysMemoryKB = sysAvailPhysMemKB;
			if (sysTotalPhysMemKB > 0)
				pSoftwareMonHandlerContext->sysMonInfo.usagePhyMemory = (double)(sysTotalPhysMemKB - sysAvailPhysMemKB) * 100 / (double)sysTotalPhysMemKB;
		}

		HandlerKernel_LockCapability();
		{
			long long tmpTimeNow = MSG_GetTimeTick();
			if (tmpTimeNow - startTime > SPEC_INFO_TIME_OUT_MS)
			{
				if (UpdateLogonUserPrcList(pSoftwareMonHandlerContext->prcMonInfoList, &pSoftwareMonHandlerContext->isUserLogon, pSoftwareMonHandlerContext->sysUserName, pSoftwareMonHandlerContext->gatherLevel, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList))
				{
					bUpdateCapability = true;
				}
				startTime = MSG_GetTimeTick();
			}
		}

		if (pSoftwareMonHandlerContext->isUserLogon)
		{
			GetPrcMonInfo(pSoftwareMonHandlerContext->prcMonInfoList);
		}

		UpdateCapability(pSoftwareMonHandlerContext->pCapability, pSoftwareMonHandlerContext->prcMonInfoList, &pSoftwareMonHandlerContext->sysMonInfo, g_HandlerContex.iRetrieveInterval, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList, g_HandlerContex.isUserLogon, bUpdateCapability);
		HandlerKernel_UnlockCapability();

		if (bUpdateCapability)
		{
			HandlerKernel_SetCapability(pSoftwareMonHandlerContext->pCapability, pSoftwareMonHandlerContext->getCapability);
		}

		if (!pSoftwareMonHandlerContext->isThreadRunning)
			break;
		{
			int i = 0;
			for (i = 0; pSoftwareMonHandlerContext->isThreadRunning && i < pSoftwareMonHandlerContext->iRetrieveInterval; i++)
			{
				usleep(1000000);
			}
		}
	}
	pthread_exit(0);
	return 0;
}

/*callback function to handle threshold rule check event*/
void on_threshold_triggered(threshold_event_type type, char *sensorname, double value, MSG_ATTRIBUTE_T *attr, void *pRev)
{
	//printf(" %s> threshold triggered:[%d, %s, %f]", g_PluginInfo.Name, type, sensorname, value);
}

void loadSWMINIFile(swm_handler_context_t *contex)
{
	char inifile[256] = {0};
	char filename[64] = {0};
	int value = 0;
	long bufsize = 0;

	if (contex == NULL)
		return;
	sprintf(filename, "%s.ini", strHandlerName);
	util_path_combine(inifile, g_PluginInfo.WorkDir, filename);

	if (!util_is_file_exist(inifile))
	{
		FILE *iniFD = fopen(inifile, "w");
		fwrite(PROCESSMONIOTR_INI_COTENT, strlen(PROCESSMONIOTR_INI_COTENT), 1, iniFD);
		fclose(iniFD);
		ProcessMonitorLog(Debug, " %s> Create %s", strHandlerName, inifile);
	}

	bufsize = util_file_size_get(inifile);
	if(bufsize == 0)
		return;

	contex->iRetrieveInterval = GetIniKeyIntDef("Platform", "Interval", inifile, 10);
	if (contex->iRetrieveInterval < 1)
		contex->iRetrieveInterval = 10;


	value = GetIniKeyIntDef("Platform", "EnableProcessStatus", inifile, 0);
	if (value == 1)
		contex->bEnProcessStatus = true;
	else
		contex->bEnProcessStatus = false;

	contex->strProcessList = calloc(1, bufsize);

	GetIniKeyStringDef("Platform", "ProcessStatusList", inifile, contex->strProcessList, bufsize, "");
}

void saveINIFile(swm_handler_context_t* contex)
{
	char inifile[256] = { 0 };
	char filename[64] = { 0 };
	int value = 0;
	char* buffer = NULL;
	int size = 0;
	FILE* iniFD = NULL;

	if (contex == NULL)
		return;

	sprintf(filename, "%s.ini", strHandlerName);
	util_path_combine(inifile, g_PluginInfo.WorkDir, filename);

	size = strlen(PROCESSMONIOTR_INI_FORMAT) + strlen(contex->strProcessList) + 16;
	buffer = calloc(1, size);

	sprintf(buffer, PROCESSMONIOTR_INI_FORMAT, contex->iRetrieveInterval, contex->bEnProcessStatus ? 1 : 0, contex->strProcessList ? contex->strProcessList : "");

	iniFD = fopen(inifile, "w");
	fwrite(buffer, strlen(buffer), 1, iniFD);
	fclose(iniFD);
	free(buffer);
}

void loadCfg(swm_handler_context_t *contex)
{
	char tmpCfg_gatherLev[4] = {0};
	char agentConfigFilePath[MAX_PATH] = {0};
	if (contex == NULL)
		return;

	util_path_combine(agentConfigFilePath, g_PluginInfo.WorkDir, DEF_CONFIG_FILE_NAME);
	if (proc_config_get(agentConfigFilePath, CFG_PROCESS_GATHER_LEVEL, tmpCfg_gatherLev, sizeof(tmpCfg_gatherLev)) == false)
	{
		contex->gatherLevel = CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS; //1 indicate gather logon user processes, 0 gather all processes.
	}
	else
	{
		contex->gatherLevel = atoi(tmpCfg_gatherLev);
		if (contex->gatherLevel > CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS)
		{
			memset(contex->sysUserName, 0, sizeof(contex->sysUserName));
			proc_config_get(agentConfigFilePath, CFG_SYSTEM_LOGON_USER, contex->sysUserName, sizeof(contex->sysUserName));
		}
	}
}

bool SetSensor(set_data_t* objlist, void* pRev)
{
	set_data_t* current = objlist;
	bool bChanged = false;
	if (current == NULL)
	{
		return false;
	}

	while (current)
	{
		MSG_ATTRIBUTE_T* attr = IoT_FindSensorNodeWithPath(g_HandlerContex.pCapability, current->sensorname);
		if (attr)
		{
			if (strcmp(current->sensorname, "ProcessMonitor/Param/Interval") == 0)
			{
				if (current->newtype == attr_type_numeric)
				{
					current->errcode = STATUSCODE_SUCCESS;
					strcpy(current->errstring, STATUS_SUCCESS);
					if (g_HandlerContex.iRetrieveInterval != current->v)
					{
						g_HandlerContex.iRetrieveInterval = current->v;
						bChanged = true;
					}
				}
				else
				{
					current->errcode = STATUSCODE_FORMAT_ERROR;
					strcpy(current->errstring, STATUS_FORMAT_ERROR);
				}
			}
			else if (strcmp(current->sensorname, "ProcessMonitor/Param/EnableProcessStatus") == 0) 
			{
				if (current->newtype == attr_type_boolean)
				{
					current->errcode = STATUSCODE_SUCCESS;
					strcpy(current->errstring, STATUS_SUCCESS);
					if(g_HandlerContex.bEnProcessStatus != current->bv)
					{
						g_HandlerContex.bEnProcessStatus = current->bv;
						bChanged = true;
					}
					
				}
				else
				{
					current->errcode = STATUSCODE_FORMAT_ERROR;
					strcpy(current->errstring, STATUS_FORMAT_ERROR);
				}
			}
			else if (strcmp(current->sensorname, "ProcessMonitor/Param/ProcessList") == 0)
			{
				if (current->newtype == attr_type_string)
				{
					current->errcode = STATUSCODE_SUCCESS;
					strcpy(current->errstring, STATUS_SUCCESS);
					if (strcmp(g_HandlerContex.strProcessList, current->sv))
					{
						if (g_HandlerContex.strProcessList)
						{
							char* tmp = g_HandlerContex.strProcessList;
							g_HandlerContex.strProcessList = calloc(1, strlen(current->sv) + 1);
							strcpy(g_HandlerContex.strProcessList, current->sv);
							free(tmp);
						}
						else
						{
							g_HandlerContex.strProcessList = calloc(1, strlen(current->sv) + 1);
							strcpy(g_HandlerContex.strProcessList, current->sv);
						}
						bChanged = true;
					}
				}
				else
				{
					current->errcode = STATUSCODE_FORMAT_ERROR;
					strcpy(current->errstring, STATUS_FORMAT_ERROR);
				}
			}
			else
			{
				current->errcode = STATUSCODE_NOT_IMPLEMENT;
				strcpy(current->errstring, STATUS_NOT_IMPLEMENT);
			}
		}
		else
		{
			current->errcode = STATUSCODE_NOT_FOUND;
			strcpy(current->errstring, STATUS_NOT_FOUND);
		}
		current = current->next;
	}

	if (bChanged)
	{
		saveINIFile(&g_HandlerContex);
		HandlerKernel_LockCapability();
		if (UpdateLogonUserPrcList(g_HandlerContex.prcMonInfoList, &g_HandlerContex.isUserLogon, g_HandlerContex.sysUserName, g_HandlerContex.gatherLevel, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList))
		{
			if (g_HandlerContex.isUserLogon)
				GetPrcMonInfo(g_HandlerContex.prcMonInfoList);
			UpdateCapability(g_HandlerContex.pCapability, g_HandlerContex.prcMonInfoList, &g_HandlerContex.sysMonInfo, g_HandlerContex.iRetrieveInterval, g_HandlerContex.bEnProcessStatus, g_HandlerContex.strProcessList, g_HandlerContex.isUserLogon, true);
		}
		HandlerKernel_UnlockCapability();
		HandlerKernel_SetCapability(g_HandlerContex.pCapability, true);
	}
	return true;
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize(HANDLER_INFO *pluginfo)
{
	if (pluginfo == NULL)
		return handler_fail;
	srand((int)time(0));
	// 1. Topic of this handler
	snprintf(pluginfo->Name, sizeof(pluginfo->Name), "%s", strHandlerName);
	pluginfo->RequestID = cagent_request_custom;
	pluginfo->ActionID = cagent_custom_action;

	// 2. Copy agent info
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	// 3. Callback function -> Send JSON Data by this callback function
	memset(&g_HandlerContex, 0, sizeof(swm_handler_context_t));
	g_HandlerContex.threadHandler = (pthread_t)0;
	g_HandlerContex.isThreadRunning = false;
	g_HandlerContex.iRetrieveInterval = 10;
	g_HandlerContex.gatherLevel = CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS;
	g_status = handler_status_no_init;
	g_processloghandle = g_PluginInfo.loghandle;
	ProcessMonitorLog(Debug, " %s> Initialize", strHandlerName);

	loadSWMINIFile(&g_HandlerContex);
	loadCfg(&g_HandlerContex);

	g_sendcbf = pluginfo->sendcbf;

	return HandlerKernel_Initialize(pluginfo);
}

/* **************************************************************************************
 *  Function Name: Handler_Uninitialize
 *  Description: Release the objects or variables used in this handler
 *  Input :  None
 *  Output: None
 *  Return:  void
 * ***************************************************************************************/
void Handler_Uninitialize()
{
	g_sendcbf = NULL;
	/*Stop read text file thread*/
	if (g_HandlerContex.threadHandler)
	{
		g_HandlerContex.isThreadRunning = false;
		pthread_cancel(g_HandlerContex.threadHandler);
		pthread_join(g_HandlerContex.threadHandler, NULL);
		g_HandlerContex.threadHandler = (pthread_t)0;
	}
	HandlerKernel_Uninitialize();
	/*Release Capability Message Structure*/
	if (g_HandlerContex.pCapability)
	{
		IoT_ReleaseAll(g_HandlerContex.pCapability);
		g_HandlerContex.pCapability = NULL;
	}

	if (g_HandlerContex.prcMonInfoList)
	{
		DestroyPrcMonInfoList(g_HandlerContex.prcMonInfoList);
		g_HandlerContex.prcMonInfoList = NULL;
	}

	if (g_HandlerContex.strProcessList)
	{
		free(g_HandlerContex.strProcessList);
		g_HandlerContex.strProcessList = NULL;
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
 *  Input :  None
 *  Output: char * : pOutStatus       // cagent handler status
 *  Return:  handler_success  : Success Init Handler
 *			 handler_fail : Fail Init Handler
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status(HANDLER_THREAD_STATUS *pOutStatus)
{
	int iRet = handler_fail;
	//printf(" %s> Get Status", strHandlerName);
	if (!pOutStatus)
		return iRet;
	/*user need to implement their thread status check function*/
	*pOutStatus = g_status;

	iRet = handler_success;
	return iRet;
}

/* **************************************************************************************
 *  Function Name: Handler_OnStatusChange
 *  Description: Agent can notify handler the status is changed.
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  None
 * ***************************************************************************************/
void HANDLER_API Handler_OnStatusChange(HANDLER_INFO *pluginfo)
{
	//printf(" %s> Update Status\n", strHandlerName);
	if (pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf(g_PluginInfo.Name, sizeof(g_PluginInfo.Name), "%s", strHandlerName);
		g_PluginInfo.RequestID = cagent_request_custom;
		g_PluginInfo.ActionID = cagent_custom_action;
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Start(void)
{
	//printf("> %s Start", strHandlerName);
	/*Create thread to read text file*/
	g_HandlerContex.interval = 1;
	g_HandlerContex.isThreadRunning = true;
	if (pthread_create(&g_HandlerContex.threadHandler, NULL, ProcessRetrieveThread, &g_HandlerContex) != 0)
	{
		g_HandlerContex.isThreadRunning = false;
		ProcessMonitorLog(Error, " %s > start thread failed!", strHandlerName);
	}

	/*Start HandlerKernel thread*/
	HandlerKernel_Start();
	g_status = handler_status_start;
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Stop
 *  Description: Stop the handler
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Stop(void)
{
	//printf("> %s Stop", strHandlerName);

	/*Stop text file read thread*/
	if (g_HandlerContex.threadHandler)
	{
		g_HandlerContex.isThreadRunning = false;
		pthread_join(g_HandlerContex.threadHandler, NULL);
		g_HandlerContex.threadHandler = (pthread_t)0;
	}

	/*Stop HandlerKernel thread*/
	HandlerKernel_Stop();

	g_status = handler_status_stop;
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input : char * const topic, 
 *			void* const data, 
 *			const size_t datalen
 *  Output: void *pRev1, 
 *			void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv(char *const topic, void *const data, const size_t datalen, void *pRev1, void *pRev2)
{
	int cmdID = 0;
	char sessionID[33] = {0};
	ProcessMonitorLog(Error, " >Recv Topic [%s] Data %s\n", topic, (char *)data);

	/*Parse Received Command*/
	if (HandlerKernel_ParseRecvCMDWithSessionID((char *)data, &cmdID, sessionID) != handler_success)
		return;
	switch (cmdID)
	{
	case hk_get_capability_req:
		if (!g_HandlerContex.pCapability)
			g_HandlerContex.pCapability = CreateCapability();
		HandlerKernel_SetCapability(g_HandlerContex.pCapability, true);
		break;
	case hk_auto_upload_req:
		/*start live report*/
		HandlerKernel_LiveReportStart(hk_auto_upload_rep, (char *)data);
		break;
	case hk_set_thr_req:
		/*Stop threshold check thread*/
		HandlerKernel_StopThresholdCheck();
		/*setup threshold rule*/
		HandlerKernel_SetThreshold(hk_set_thr_rep, (char *)data);
		/*register the threshold check callback function to handle trigger event*/
		HandlerKernel_SetThresholdTrigger(on_threshold_triggered);
		/*Restart threshold check thread*/
		HandlerKernel_StartThresholdCheck();
		break;
	case hk_del_thr_req:
		/*Stop threshold check thread*/
		HandlerKernel_StopThresholdCheck();
		/*clear threshold check callback function*/
		HandlerKernel_SetThresholdTrigger(NULL);
		/*Delete all threshold rules*/
		HandlerKernel_DeleteAllThreshold(hk_del_thr_rep);
		break;
	case hk_get_sensors_data_req:
		/*Get Sensor Data with callback function*/
		HandlerKernel_GetSensorData(hk_get_sensors_data_rep, sessionID, (char *)data, NULL);
		break;
	case hk_set_sensors_data_req:
		/*Set Sensor Data with callback function*/
		HandlerKernel_SetSensorData(hk_set_sensors_data_rep, sessionID, (char *)data, SetSensor);
		break;
	case swm_restart_prc_req:
		//{"commCmd":167,"handlerName":"ProcessMonitor","content":{"prcID":31281}}
		SWMRestartPrc(g_HandlerContex.prcMonInfoList, (char *)data, sessionID);
		break;
	case swm_kill_prc_req:
		//{"commCmd":169,"handlerName":"ProcessMonitor","content":{"prcID":21420}}
		SWMKillPrc(g_HandlerContex.prcMonInfoList, (char *)data, sessionID);
		break;
	case swm_exec_prc_req:
		//{"commCmd":171,"handlerName":"ProcessMonitor","content":{"prcPath":"c:/sample.exe","prcArg":"-?","prcHidden":false}}
		SWMExecPrc((char*)data, sessionID);
		break;
	default:
	{
		/* Send command not support reply message*/
		char repMsg[32] = {0};
		int len = 0;
		sprintf(repMsg, "{\"errorRep\":\"Unknown cmd!\"}");
		len = strlen("{\"errorRep\":\"Unknown cmd!\"}");
		if (g_sendcbf)
			g_sendcbf(&g_PluginInfo, hk_error_rep, repMsg, len, NULL, NULL);
	}
	break;
	}
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	/*TODO: Parsing received command
	*input data format: 
	* {"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["all"],"commCmd":2053}}
	*
	* "autoUploadIntervalSec":30 means report sensor data every 30 sec.
	* "requestItems":["all"] defined which handler or sensor data to report. 
	*/
	//printf("> %s Start Report", strHandlerName);
	/*create thread to report sensor data*/
	HandlerKernel_AutoReportStart(pInQuery);
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : None
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	/*TODO: Parsing received command*/
	//printf("> %s Stop Report", strHandlerName);

	//comment to keep report sensro data!!
	HandlerKernel_AutoReportStop(pInQuery);
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char ** : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability(char **pOutReply) // JSON Format
{
	char *result = NULL;
	int len = 0;

	//printf("> %s Get Capability", strHandlerName);

	if (!pOutReply)
		return len;

	/*Create Capability Message Structure to describe sensor data*/
	g_HandlerContex.getCapability = true;
	if (!g_HandlerContex.pCapability)
	{
		/*g_HandlerContex.pCapability = CreateCapability();
		HandlerKernel_SetCapability(g_HandlerContex.pCapability, false);*/
		return len;
	}
	
	/*generate capability JSON string*/
	result = IoT_PrintCapability(g_HandlerContex.pCapability);

	/*create buffer to store the string*/
	len = strlen(result);
	*pOutReply = (char *)malloc(len + 1);
	memset(*pOutReply, 0, len + 1);
	strcpy(*pOutReply, result);
	free(result);

	return len;
}

/* **************************************************************************************
 *  Function Name: Handler_MemoryFree
 *  Description: free the memory allocated for Handler_Get_Capability
 *  Input : char *pInData.
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_MemoryFree(char *pInData)
{
	//printf("> %s Free Allocated Memory", strHandlerName);

	if (pInData)
	{
		free(pInData);
		pInData = NULL;
	}
	return;
}
