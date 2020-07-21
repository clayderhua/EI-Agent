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
#include "HandlerKernel.h"
#include "util_path.h"
#include <time.h>
#include "NetMonitorLog.h"
#include "ReadINI.h"
#include "NetInfoList.h"
#include "NetInfoParser.h"
//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define cagent_request_custom 2102 /*define the request ID for V3.0, not used on V3.1 or later*/
#define cagent_custom_action 31002 /*define the action ID for V3.0, not used on V3.1 or later*/

const char strHandlerName[MAX_TOPIC_LEN] = {"NetMonitor"}; /*declare the handler name*/

#define NETMONIOTR_INI_COTENT "[Platform]\nInterval=10\n#Interval: The time delay between two access round in second."
int g_iRetrieveInterval = 10; //10 sec.
//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct
{
	pthread_t threadHandler; // thread handle
	int interval;			 // time interval for file read
	bool isThreadRunning;	//thread running flag

	int iRetrieveInterval;

	net_info_list netInfoList;
	MSG_CLASSIFY_T *pCapability;
	bool getCapability;
} handler_context_t;

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static Handler_info g_HandlerInfo; //global Handler info structure
static handler_context_t g_HandlerContex;
static HANDLER_THREAD_STATUS g_status = handler_status_no_init; // global status flag.
static HandlerSendCbf g_sendcbf = NULL;							// Client Send information (in JSON format) to Cloud Server
//-----------------------------------------------------------------------------
// Function:
//-----------------------------------------------------------------------------
void Handler_Uninitialize();

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

bool UpdateNetInfoList(MSG_CLASSIFY_T *capability, net_info_list netInfoList)
{
	net_info_node_t* head = NULL;
	bool bChanged = false;
	GetNetInfo(netInfoList, &bChanged);

	UpdateNetInfo(capability, netInfoList, bChanged);


	return bChanged;
}

MSG_CLASSIFY_T *CreateCapability(net_info_list netInfoList)
{
	MSG_CLASSIFY_T *myCapability = IoT_CreateRoot((char *)strHandlerName);

	UpdateNetInfoList(myCapability, netInfoList);

	return myCapability;
}

void *NetDataRetrieveThread(void *args)
{
	/*thread to read text file repeatedly.*/
	handler_context_t *pHandlerContex = (handler_context_t *)args;
	int mInterval = pHandlerContex->interval * 1000;
	if (!pHandlerContex->pCapability)
	{
		pHandlerContex->pCapability = CreateCapability(pHandlerContex->netInfoList);
		HandlerKernel_SetCapability(pHandlerContex->pCapability, pHandlerContex->getCapability);
	}

	while (pHandlerContex->isThreadRunning)
	{
		bool bUpdateCapability = false;
		/*Retrieve data by calling ParseReceivedData, and protect the global resource: myGroup by critical section lock(HandlerKernel_LockCapability and HandlerKernel_UnlockCapability)*/
		if (pHandlerContex->pCapability)
		{
			HandlerKernel_LockCapability();
			if(UpdateNetInfoList(pHandlerContex->pCapability, pHandlerContex->netInfoList))
			{
				bUpdateCapability = true;
			}
			HandlerKernel_UnlockCapability();
		}
		if(bUpdateCapability)
		{
			HandlerKernel_SetCapability(pHandlerContex->pCapability, pHandlerContex->getCapability);
		}
		if(!pHandlerContex->isThreadRunning) break;
		{
			int i = 0;
			for(i = 0; pHandlerContex->isThreadRunning && i<pHandlerContex->iRetrieveInterval; i++)
			{
				usleep(1000000);
			}
		}
	}
	pthread_exit(0);
	return 0;
}

void loadINIFile(handler_context_t *contex)
{
	char inifile[256] = {0};
	char filename[64] = {0};
	int bProcessStatus = 0;
	if (contex == NULL)
		return;

	snprintf(filename, sizeof(filename), "%s.ini", strHandlerName);
	util_path_combine(inifile, g_HandlerInfo.WorkDir, filename);
	if (!util_is_file_exist(inifile))
	{
		FILE *iniFD = fopen(inifile, "w");
		fwrite(NETMONIOTR_INI_COTENT, strlen(NETMONIOTR_INI_COTENT), 1, iniFD);
		fclose(iniFD);
	}
	contex->iRetrieveInterval = GetIniKeyInt("Platform", "Interval", inifile);
	if (contex->iRetrieveInterval < 1)
		contex->iRetrieveInterval = 10;
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

	// 1. Topic of this handler
	snprintf(pluginfo->Name, sizeof(pluginfo->Name), "%s", strHandlerName);
	pluginfo->RequestID = cagent_request_custom;
	pluginfo->ActionID = cagent_custom_action;

	// 2. Copy agent info
	memcpy(&g_HandlerInfo, pluginfo, sizeof(HANDLER_INFO));
	g_HandlerInfo.agentInfo = pluginfo->agentInfo;

	// 3. Callback function -> Send JSON Data by this callback function

	g_HandlerContex.threadHandler = NULL;
	g_HandlerContex.isThreadRunning = false;

	if(g_HandlerContex.netInfoList)
	{
		DeleteAllNetInfoNode(g_HandlerContex.netInfoList);
	}
	else
	{
		g_HandlerContex.netInfoList = CreateNetInfoList();
	}

	loadINIFile(&g_HandlerContex);

	g_status = handler_status_no_init;

	NETWORKLog(Debug, " %s> Initialize", strHandlerName);

	SetWorkDir(g_HandlerInfo.WorkDir);
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
	/*Stop read text file thread*/
	if (g_HandlerContex.threadHandler)
	{
		g_HandlerContex.isThreadRunning = false;
		pthread_cancel(g_HandlerContex.threadHandler);
		pthread_join(g_HandlerContex.threadHandler, NULL);
		g_HandlerContex.threadHandler = NULL;
	}
	HandlerKernel_Uninitialize();
	/*Release Capability Message Structure*/
	if (g_HandlerContex.pCapability)
	{
		IoT_ReleaseAll(g_HandlerContex.pCapability);
		g_HandlerContex.pCapability = NULL;
	}

	if(g_HandlerContex.netInfoList)
	{
		DestroyNetInfoList(g_HandlerContex.netInfoList);
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
	printf(" %s> Update Status\n", strHandlerName);
	if (pluginfo)
		memcpy(&g_HandlerInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_HandlerInfo, 0, sizeof(HANDLER_INFO));
		snprintf(g_HandlerInfo.Name, sizeof(g_HandlerInfo.Name), "%s", strHandlerName);
		g_HandlerInfo.RequestID = cagent_request_custom;
		g_HandlerInfo.ActionID = cagent_custom_action;
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
	if (pthread_create(&g_HandlerContex.threadHandler, NULL, NetDataRetrieveThread, &g_HandlerContex) != 0)
	{
		g_HandlerContex.isThreadRunning = false;
		//printf("start thread failed!\n");
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
		g_HandlerContex.threadHandler = NULL;
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
	//printf(" >Recv Topic [%s] Data %s\n", topic, (char*) data );

	/*Parse Received Command*/
	if (HandlerKernel_ParseRecvCMDWithSessionID((char *)data, &cmdID, sessionID) != handler_success)
		return;
	switch (cmdID)
	{
	case hk_get_capability_req:
		if (!g_HandlerContex.pCapability)
			g_HandlerContex.pCapability = CreateCapability(g_HandlerContex.netInfoList);
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
		HandlerKernel_SetThresholdTrigger(NULL);
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
		HandlerKernel_SetSensorData(hk_set_sensors_data_rep, sessionID, (char *)data, NULL);
		break;
	default:
	{
		/* Send command not support reply message*/
		char repMsg[32] = {0};
		int len = 0;
		sprintf(repMsg, "{\"errorRep\":\"Unknown cmd!\"}");
		len = strlen("{\"errorRep\":\"Unknown cmd!\"}");
		if (g_sendcbf)
			g_sendcbf(&g_HandlerInfo, hk_error_rep, repMsg, len, NULL, NULL);
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
	if (!g_HandlerContex.pCapability)
	{
		g_HandlerContex.getCapability = true;
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
