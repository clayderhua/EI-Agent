// DiskPMQ.cpp : Defines the entry point for the console application.
//
#ifdef __linux__
#elif _WIN32
#pragma comment(lib, "Winmm.lib")
#include <windows.h>
#else
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "cJSON.h"
#include "HandlerKernel.h"
#include "Log.h"
#include "GenMessage.h"
#include "HDDPMQ.h"
#include "DiskPMQInfo.h"
#include "unistd.h"
#include "WISEPlatform.h"

//-----------------------------------------------------------------------------
// Logger defines:
//-----------------------------------------------------------------------------

#define PROPHET_LOG_ENABLE
#define DEF_PROPHET_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

LOGHANDLE ProphetLog = NULL;

#ifdef PROPHET_LOG_ENABLE
#define ProphetLog(level, fmt, ...)  do { if (ProphetLog != NULL)   \
	WriteLog(ProphetLog, DEF_PROPHET_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define ProphetLog(level, fmt, ...)
#endif

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

static HandlerSendCbf SendCBF = NULL;										// Client Send information (in JSON format) to Cloud Server	
static HandlerAutoReportCbf SendReport = NULL;								// Client Send information (in JSON format) to Cloud Server	
static HandlerSendEventCbf SendEvent = NULL;								// Client Send information (in JSON format) to Cloud Server
static HandlerSendCapabilityCbf SendCapbility = NULL;

static HANDLER_THREAD_STATUS g_status = handler_status_no_init;				// global status flag.
static Handler_info HandlerInfo;											// global Handler info structure
handler_context HandlerContext;

MSG_CLASSIFY_T* PMQCapability = NULL;
DiskPMQ DiskPMQInfo;
HandlerConfig PMQConfig;

bool ReSendEventFlag = true;

//-----------------------------------------------------------------------------
// Function:
//-----------------------------------------------------------------------------
bool SetSensor(set_data_t* objlist, void* pRev)
{
    set_data_t* current = objlist;
	char *reportStr = NULL;

	if (current == NULL)
	{
		current->errcode = STATUSCODE_FAIL;
		strcpy(current->errstring, STATUS_FAIL);
		return false;
	}

	if (strcmp(UPDATE_PERIOD_PATH, current->sensorname) == EQUAL)
	{
		if ((current->v >= 10) && (current->v <= 3600))
			PMQConfig.paramInfo.reportInterval = (unsigned int)current->v;
		else
		{
			current->errcode = STATUSCODE_OUTOF_RANGE;
			strcpy(current->errstring, STATUS_OUTOF_RANGE);
			return false;
		}
	}
	else if (strcmp(ENABLE_UPDATE_PATH, current->sensorname) == EQUAL)
	{
		PMQConfig.paramInfo.enableReport = current->bv;
	}
	else
	{
		current->errcode = STATUSCODE_NOT_FOUND;
		strcpy(current->errstring, STATUS_NOT_FOUND);
		return false;
	}

	if (UpdatePMQMsg(PMQCapability, &DiskPMQInfo, &PMQConfig) == GenSuccess)
	{
		if (SendReport && SendEvent)
		{
			reportStr = REPORT_STR(PMQCapability);
			SendReport(&HandlerInfo, reportStr, strlen(reportStr), NULL, NULL);
			DEBUG_PRINTF(reportStr);

			if (reportStr != NULL)
			{
				free(reportStr);
				reportStr = NULL;
			}
		}
	}

	SavePMQParameter(&PMQConfig);

    current->errcode = STATUSCODE_SUCCESS;
    strcpy(current->errstring, STATUS_SUCCESS);
    return true;
}

bool ParseRecvData(void *data, int datalen, int *cmdID, char *sessionId, int nLenSessionId)
{
	cJSON* root = NULL;
    cJSON* body = NULL;
    cJSON* target = NULL;

    if (!data)
	{
		printf("1\n");
        return false;
	}

    if (datalen <= 0)
	{
		printf("2\n");
        return false;
	}

    root = cJSON_Parse((char*)data);
    if (!root)
    {
        printf("Parse Error\n");
        return false;
    }

    body = cJSON_GetObjectItem(root, "susiCommData");
    if (!body)
    {
		printf("3\n");
        cJSON_Delete(root);
        return false;
    }

    target = cJSON_GetObjectItem(body, "commCmd");
    if (target)
        *cmdID = target->valueint;

    target = cJSON_GetObjectItem(body, "sessionID");
    if (target)
		snprintf(sessionId, nLenSessionId, "%s", target->valuestring);

	cJSON_Delete(root);
	return true;
}

void* HDDHandlerThreadStart(void* args)
{
    handler_context* pHandlerCtx = (handler_context*)args;
	pDiskPMQ pmqdiskTemp = NULL;
	char *reportStr = NULL;
	bool isSendEventMsg = false;
	unsigned int curTime = 0, nextTime = 0;
	struct timeval tv;
	struct timezone tz;
	
	if (pHandlerCtx == NULL)
    {
        pthread_exit(0);
        return 0;
	}

	if (pHandlerCtx->bHasSQFlash == false)
	{
		pthread_exit(0);
		return 0;
	}

	if (UpdatePMQInfoFromHDDInfo(&DiskPMQInfo, &pHandlerCtx->hddCtx.hddInfo))
		UpdatePMQMsg(PMQCapability, &DiskPMQInfo, &PMQConfig);

	HandlerKernel_SetCapability(PMQCapability, false);					// Report capability to agent.
	DEBUG_PRINTF(PMQCapability);

	while (HandlerInfo.agentInfo->status == 0)
		usleep(1000000);

	while (pHandlerCtx->bThreadRunning)
	{
		//curTime = timeGetTime();
		//curTime = app_os_GetTickCount();
		gettimeofday(&tv, &tz);
		curTime = tv.tv_sec;
		if ((curTime - nextTime) >= PMQConfig.paramInfo.reportInterval)
		{
			pthread_mutex_lock(&HandlerContext.hddCtx.hddMutex);
			hdd_GetHDDInfo(&HandlerContext.hddCtx.hddInfo);
			pthread_mutex_unlock(&HandlerContext.hddCtx.hddMutex);

			if (UpdatePMQInfoFromHDDInfo(&DiskPMQInfo, &pHandlerCtx->hddCtx.hddInfo))
			{
				if (ReSendEventFlag == true)
				{
					ReSendEventFlag = false;

					pmqdiskTemp = &DiskPMQInfo;
					while (pmqdiskTemp)
					{
						pmqdiskTemp->state = isChange;
						pmqdiskTemp = pmqdiskTemp->next;
					}
				}

				if (UpdatePMQMsg(PMQCapability, &DiskPMQInfo, &PMQConfig) == GenSuccess)
				{
					if (SendReport && SendEvent)
					{
						pmqdiskTemp = &DiskPMQInfo;
						while (pmqdiskTemp)
						{
							if (pmqdiskTemp->state == isChange)
							{
								reportStr = CreateEventMsg(pmqdiskTemp);

								if (reportStr != NULL)
								{
									SendEvent(&HandlerInfo, Severity_Warning, reportStr, strlen(reportStr), NULL, NULL);
									DEBUG_PRINTF(reportStr);

									if (reportStr != NULL)
									{
										free(reportStr);
										reportStr = NULL;
									}
								}
								
								isSendEventMsg = true;
							}

							pmqdiskTemp = pmqdiskTemp->next;
						}
						/*
						reportStr = REPORT_STR(PMQCapability);

						if (PMQConfig.paramInfo.enableReport == true)
							SendReport(&HandlerInfo, reportStr, strlen(reportStr), NULL, NULL);
						else if (isSendEventMsg == true)
							SendReport(&HandlerInfo, reportStr, strlen(reportStr), NULL, NULL);

						isSendEventMsg = false;
						DEBUG_PRINTF(reportStr);

						if (reportStr != NULL)
						{
							free(reportStr);
							reportStr = NULL;
						}
						*/
					}
				}
			}

			//nextTime = timeGetTime();
			gettimeofday(&tv, &tz);
			nextTime = tv.tv_sec;
		}

		usleep(1000000);
	}

    pthread_exit(0);

    return 0;
}


/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
 *  Input :  None
 *  Output: char * : pOutStatus       // cagent handler status
 *  Return:  handler_success  : Success Init Handler
 *			 handler_fail : Fail Init Handler
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status(HANDLER_THREAD_STATUS * pOutStatus)
{
	int iRet = handler_fail; 
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
	if (pluginfo)
	{
		memcpy(&HandlerInfo, pluginfo, sizeof(HANDLER_INFO));
	}
	else
	{
		memset(&HandlerInfo, 0, sizeof(HANDLER_INFO));
		sprintf(HandlerInfo.Name, "%s", PLUGIN_NAME);
		HandlerInfo.RequestID = CAGENT_REQUEST_CUSTOM;
		HandlerInfo.ActionID = CAGENT_CUSTOM_ACTION;
	}
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
void HANDLER_API Handler_Recv(char * const topic, void * const data, const size_t datalen, void * pRev1, void * pRev2)
{
	int cmdID = 0;
	char sessionID[64] = {0};

	printf(" >Recv Topic [%s] Data %s\n", topic, (char *)data);
	
	/*Parse Received Command*/
	if(HandlerKernel_ParseRecvCMDWithSessionID((char *)data, &cmdID, sessionID) == handler_fail)
		return;

	switch (cmdID)
	{
	case hk_auto_upload_req:				// start live report
		HandlerKernel_LiveReportStart(hk_auto_upload_rep, (char *)data);
		break;

	case hk_set_thr_req:					// setup threshold
		HandlerKernel_StopThresholdCheck();
		HandlerKernel_SetThreshold(hk_set_thr_rep,(char *) data);

		HandlerKernel_StartThresholdCheck();
		break;

	case hk_del_thr_req:
		HandlerKernel_StopThresholdCheck();
		HandlerKernel_DeleteAllThreshold(hk_del_thr_rep);
		break;

	case hk_get_sensors_data_req:
		HandlerKernel_GetSensorData(hk_get_sensors_data_rep, sessionID, (char *)data, NULL);
		break;

	case hk_set_sensors_data_req:
		HandlerKernel_SetSensorData(hk_set_sensors_data_rep, sessionID, (char *)data, SetSensor);
		//system("C:\\WINDOWS\\System32\\shutdown /r /f /t 0");
		break;

	default:
	{
		/* Send command not support reply message*/
		char repMsg[32] = {0};
		int len = 0;
		sprintf(repMsg, "{\"errorRep\":\"Unknown cmd!\"}");
		len = strlen("{\"errorRep\":\"Unknown cmd!\"}") ;
		if (SendCBF) SendCBF(&HandlerInfo, hk_error_rep, repMsg, len, NULL, NULL);
	}
		break;
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Stop
 *  Description: Stop the handler
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	if (HandlerContext.bThreadRunning)
	{	
		HandlerContext.bThreadRunning = false;
		
		pthread_join(HandlerContext.threadHandler, NULL);
		HandlerContext.threadHandler = NULL;
	}

	if (HandlerContext.bHasSQFlash)
		hdd_CleanupSQFlashLib();
	
	if (&HandlerContext.hddCtx.hddInfo)
		hdd_DestroyHDDInfoList(HandlerContext.hddCtx.hddInfo.hddMonInfoList);
	
	g_status = handler_status_stop;

	return HandlerKernel_Stop();
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
	//ProphetLog(Debug, "> %s Start Report", strHandlerName);
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
	//ProphetLog(Debug, "> %s Stop Report", strHandlerName);
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
int HANDLER_API Handler_Get_Capability(char ** pOutReply) // JSON Format
{
	char *result = NULL;
	char *reportStr;
	int len = 0;

	if (!pOutReply)
		return len;
	
	reportStr = REPORT_STR(PMQCapability);

	len = strlen(reportStr);								// create buffer to store the string
	*pOutReply = (char *)malloc(len + 1);
	memset(*pOutReply, 0, len + 1);
	strcpy(*pOutReply, reportStr);

	if (reportStr != NULL)
	{
		free(reportStr);
		reportStr = NULL;
	}

	ReSendEventFlag = true;

	return len;
}


/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	// ========== Load Handler Parameter ==========
	LoadPMQParameter(&PMQConfig);

	if (PMQCapability == NULL)
		PMQCapability = CreatePMQMsg();

	// ========== Init SQFlash ==========
	pthread_mutex_lock(&HandlerContext.hddCtx.hddMutex);
	if (hdd_IsExistSQFlashLib())
	{
		HandlerContext.bHasSQFlash = hdd_StartupSQFlashLib();

		if (HandlerContext.bHasSQFlash)
			hdd_GetHDDInfo(&HandlerContext.hddCtx.hddInfo);

		// ========== Create Thread ==========
		//if (app_os_thread_create(&HandlerContext.threadHandler, HDDHandlerThreadStart, &HandlerContext) != 0)
		if (pthread_create(&HandlerContext.threadHandler, NULL, HDDHandlerThreadStart,  &HandlerContext) != 0)
		{
			HandlerContext.bThreadRunning = false;
			return handler_fail;
		}
	}
	pthread_mutex_unlock(&HandlerContext.hddCtx.hddMutex);
	
	HandlerContext.bThreadRunning = true;
	g_status = handler_status_start;
	return HandlerKernel_Start();
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
	HandlerKernel_Uninitialize();

	/*Release Capability Message Structure*/
	if (PMQCapability != NULL)
	{
		IoT_ReleaseAll(PMQCapability);
		PMQCapability = NULL;
		UnInitPMQParameter();
	}

	if (HandlerContext.bThreadRunning)
	{
		HandlerContext.bThreadRunning = false;
		pthread_cancel(HandlerContext.threadHandler);
		pthread_join(HandlerContext.threadHandler, NULL);
		HandlerContext.threadHandler = NULL;
	}

	if (HandlerContext.bHasSQFlash)
		hdd_CleanupSQFlashLib();

	pthread_mutex_destroy(&HandlerContext.hddCtx.hddMutex);
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize(HANDLER_INFO* pluginfo)
{
    if (pluginfo == NULL)
        return handler_fail;

    // 1. Topic of this handler			
	sprintf(pluginfo->Name, "%s", PLUGIN_NAME);											// Set your plug-in name in pluginfo content.
    pluginfo->RequestID = CAGENT_REQUEST_CUSTOM;										// Set request ID in pluginfo content.
    pluginfo->ActionID = CAGENT_CUSTOM_ACTION;											// Set action ID in pluginfo content.
    ProphetLog = pluginfo->loghandle;
	sprintf(PMQConfig.iniDir, "%s", pluginfo->WorkDir);
	sprintf(PMQConfig.iniFileName, "%s.ini", PLUGIN_NAME);

    // 2. Copy agent info
    memcpy(&HandlerInfo, pluginfo, sizeof(HANDLER_INFO));								// Copy plug-in information from OS service.
    HandlerInfo.agentInfo = pluginfo->agentInfo;

    // 3. Callback function -> Send JSON Data by this callback function
    g_status = handler_status_no_init;
	SendCBF = HandlerInfo.sendcbf;
	SendReport = HandlerInfo.sendreportcbf;
	SendEvent = HandlerInfo.sendeventcbf;
	SendCapbility = HandlerInfo.sendcapabilitycbf;

    // 4. Customized plug-in feature
    memset(&HandlerContext, 0, sizeof(handler_context));
    if (pthread_mutex_init(&HandlerContext.hddCtx.hddMutex, NULL) != 0)
        return handler_fail;
    return HandlerKernel_Initialize(pluginfo);
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
	if (pInData)
	{
		free(pInData);
		pInData = NULL;
	}
}
#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH)							// Self-explanatory
	{
		DisableThreadLibraryCalls(module_handle);						// Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL)											// Dynamic load
		{
			// Initialize your stuff or whatever
			// Return FALSE if you don't want your module to be dynamically loaded
		}
		else															// Static load
		{
			// Return FALSE if you don't want your module to be statically loaded
			return FALSE;
		}
	}

	if (reason_for_call == DLL_PROCESS_DETACH)							// Self-explanatory
	{
		if (reserved == NULL)											// Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize();
		}
		else															// Process is terminating
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
static void Initializer(int argc, char** argv, char** envp)
{
    printf("DllInitializer\r\n");
}

__attribute__((destructor))
/**
 * It is called when shared lib is being unloaded.
 *
 */
static void Finalizer()
{
    printf("DllFinalizer\r\n");
        Handler_Uninitialize();
}
#endif
