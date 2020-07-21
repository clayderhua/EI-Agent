#ifndef _PROCESSLIST_H_
#define _PROCESSLIST_H_

#include "stdbool.h"

typedef struct msg_context_t{
	int procID;
	bool isActive;
	bool isShutdown;
	//char msg[1024];
}msg_context_t;

typedef struct{
	long long lastIdleTime;
	long long lastKernelTime;
	long long lastUserTime;
}sys_cpu_usage_time_t;

typedef struct{
	int cpuUsage;
	long totalPhysMemoryKB;
	long availPhysMemoryKB;
	double usagePhyMemory;
	sys_cpu_usage_time_t sysCpuUsageLastTimes;
}sys_mon_info_t;

typedef struct{
	long long lastKernelTime;
	long long lastUserTime;
	long long lastTime;
}prc_cpu_usage_time_t;

typedef struct{
	int isValid;
	char * prcName;
	char * ownerName;
	unsigned int prcID;
	int cpuUsage;
	long memUsage;
	long vmemUsage;
	int isActive;
	bool enGetStatus;
	prc_cpu_usage_time_t prcCpuUsageLastTimes;
}prc_mon_info_t;

typedef struct prc_mon_info_node_t{
	prc_mon_info_t prcMonInfo;
	struct prc_mon_info_node_t * next;
}prc_mon_info_node_t;

#ifdef WIN32
static char g_ProcessInfoHelperFile[260] = { 0 };
#define CMD_HELPER_FILE_NAME          "ProcessInfoHelper.exe"
#endif

#define CFG_FLAG_SEND_PRCINFO_ALL       0
#define CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS  1
#define CFG_FLAG_SEND_PRCINFO_BY_USER  2

#define MAX_LOGON_USER_CNT           32
#define MAX_LOGON_USER_NAME_LEN      32

#define DIV                       (1024)

#ifdef __cplusplus
extern "C" {
#endif

prc_mon_info_node_t * CreatePrcMonInfoList();

bool UpdateLogonUserPrcList(prc_mon_info_node_t * head, bool *isUserLogon, char* sysUserName, int gatherLevel, bool bGatherStatus, char* listProcStatus);

bool GetSysCPUUsage(int* cpuusage, sys_cpu_usage_time_t * pSysCpuUsageLastTimes);

bool GetSysMemoryUsageKB(long * totalPhysMemKB, long * availPhysMemKB);

int InsertPrcMonInfoNode(prc_mon_info_node_t * head, prc_mon_info_t * prcMonInfo);

prc_mon_info_node_t * FindPrcMonInfoNode(prc_mon_info_node_t * head, unsigned int prcID);

int DeleteAllPrcMonInfoNode(prc_mon_info_node_t * head);

void DestroyPrcMonInfoList(prc_mon_info_node_t * head);

bool RunProcessAsUser(char* cmdLine, bool isAppNameRun, bool isShowWindow, unsigned long* newPrcID, char* errmsg);

unsigned int RestartProcessWithID(unsigned int prcID);

bool KillProcessWithID(unsigned int prcID);

#ifdef __cplusplus
}
#endif

#endif