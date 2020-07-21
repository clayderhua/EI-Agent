#ifndef _PROCESS_PARSER_H_
#define _PROCESS_PARSER_H_

#include "ProcessList.h"
#include "IoTMessageGenerate.h"

#define SWM_SYS_MON_INFO              "System Monitor Info"
#define SWM_PRC_MON_INFO              "Process Monitor Info"
#define SWM_PRC_NAME                  "Name"
#define SWM_PID                       "PID"
#define SWM_OWNER                     "Owner"
#define SWM_PRC_CPU_USAGE             "CPU Usage"
#define SWM_PRC_MEM_USAGE             "Mem Usage"
#define SWM_PRC_VMEM_USAGE            "Virtual Mem Usage"
#define SWM_PRC_IS_ACTIVE             "IsActive"
#define SWM_SYS_TOTAL_PHYS_MEM        "totalPhysMemKB"
#define SWM_SYS_AVAIL_PHYS_MEM        "availPhysMemKB"
#define SWM_SYS_USAGE_PHYS_MEM        "usagePhysMem"
#define SWM_PRC_GET_STATUS		      "GetStatus"

#define SWM_PARAM_INFO	              "Param"
#define SWM_PARAM_INTERVAL		      "Interval"
#define SWM_PARAM_STATUS_EN		      "EnableProcessStatus"
#define SWM_PARAM_PROCESS_LIST	      "ProcessList"

#define SWM_INFO_INFO	              "info"
#define SWM_INFO_ERRMSG				  "ErrorMessage"
#define SWM_INFO_ERRCODE			  "ErrorCode"

#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_CONTENT_STRUCT		"content"

#define SWM_ERROR_REP                   "errorRep"
#define SWM_ERROR_CODE                  "errorCode"
#define SWM_SESSION_ID                  "sessionID"

#ifdef __cplusplus
extern "C" {
#endif

	void UpdateCapability(MSG_CLASSIFY_T* capability, prc_mon_info_node_t * prcMonInfoList, sys_mon_info_t * sysMonInfo, int iInterval, bool bGatherStatus, char* listProcStatus, bool bLogon, bool bReset);

	bool Parse_swm_kill_prc_req(char *inputstr, unsigned int *size, unsigned int **outputVal);

	bool Parse_swm_restart_prc_req(char *inputstr, unsigned int *size, unsigned int **outputVal);

	int Parser_string(char* pCommData, char* sessionID, char** outputStr);
	int Parser_errstring(char *pCommData, unsigned long errcode, char* sessionID, char **outputStr);

	bool Parse_swm_exec_prc_req(char* inputstr, char* prcPath, char* prcArg, bool* bHide);

#ifdef __cplusplus
}
#endif

#endif