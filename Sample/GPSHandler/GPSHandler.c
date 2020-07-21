#include "srp/susiaccess_handler_api.h"
#include "GPSHandler.h"
#include "IoTMessageGenerate.h"
#include "GPSMessageGenerate.h"
#include "Log.h"
#include "ReadINI.h"
#include "gps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "WISEPlatform.h"
#include "util_path.h"
#include "HandlerKernel.h"
#include <pthread.h>

//-----------------------------------------------------------------------------
// Logger defines:
//-----------------------------------------------------------------------------
#define GPSHANDLER_LOG_ENABLE
//#define DEF_GPSHANDLER_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_GPSHANDLER_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_GPSHANDLER_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

LOGHANDLE g_gpshandlerlog = NULL;

#ifdef GPSHANDLER_LOG_ENABLE
#define GPSHLog(level, fmt, ...)  do { if (g_gpshandlerlog != NULL)   \
	WriteLog(g_gpshandlerlog, DEF_GPSHANDLER_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define GPSHLog(level, fmt, ...)
#endif

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define cagent_gps_request	2106
#define cagent_gps_action	3106

const char strHandlerName[MAX_TOPIC_LEN] = {"GPSHandler"};
const int iRequestID = cagent_gps_request;
const int iActionID = cagent_gps_action;

typedef struct{
	unsigned int SerialPortNum;
	unsigned int BaudRate;
}serial_port_t;
typedef struct{
	GPS_HANDLE pHandle;
	unsigned long major;
	unsigned long minor;
}gps_handle_t;

typedef struct{
   pthread_t threadHandler;
   int interval;
   bool isThreadRunning;
   serial_port_t serial;
   gps_handle_t gps;
}handler_context_t;

static Handler_info g_PluginInfo;
static handler_context_t g_HandlerContex;
static HANDLER_THREAD_STATUS g_status = handler_status_no_init;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;		
static HandlerSendEventCbf g_sendeventcbf = NULL;

MSG_CLASSIFY_T *g_Capability = NULL;

//-----------------------------------------------------------------------------
// API Function:
//-----------------------------------------------------------------------------
MSG_CLASSIFY_T * CreateCapability(bool bReport, serial_port_t* serial,gps_handle_t* pGPSHandle);
void* GPS_StartThread(void* args);

bool GPS_Initialize(gps_handle_t* pGPSHandle, serial_port_t* serial);
void GPS_Uninitialize(gps_handle_t* pGPSHandle);
bool GPS_Start(gps_handle_t* pGPSHandle);
void GPS_Stop(gps_handle_t* pGPSHandle);
bool GPS_GetCapability(MSG_CLASSIFY_T* root, serial_port_t* serial,gps_handle_t* pGPSHandle);

bool ReadINI_Platform(char *iniPath, serial_port_t* serial);

#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		printf("DllInitializer\r\n");
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL) // Dynamic load
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
		printf("DllFinalizer\r\n");
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
	sprintf_s(pluginfo->Name, sizeof(pluginfo->Name), "%s", strHandlerName);
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" %s> Initialize\n", strHandlerName);
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	memset(&g_HandlerContex, 0, sizeof(handler_context_t));
	g_gpshandlerlog = pluginfo->loghandle;
	if (!ReadINI_Platform(pluginfo->WorkDir, &g_HandlerContex.serial))
		return handler_init_error;
	GPS_Initialize(&g_HandlerContex.gps, &g_HandlerContex.serial);
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
	GPS_Uninitialize(&g_HandlerContex.gps);
	HandlerKernel_Uninitialize();

	if(g_Capability)
	{
		GPS_ReleaseGPS(g_Capability);
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
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	int iRet = handler_fail; 
	//printf(" %s> Get Status\n", strHandlerName);
	if(!pOutStatus) return iRet;

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
void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	printf(" %s> Update Status\n", strHandlerName);
	if(pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strHandlerName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
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
int HANDLER_API Handler_Start( void )
{
	printf("> %s Start\n", strHandlerName);
	g_HandlerContex.isThreadRunning = true;
	g_HandlerContex.interval = 1;

	if(!GPS_Start(&g_HandlerContex.gps))
		return handler_fail;

	if(pthread_create(&g_HandlerContex.threadHandler, NULL, GPS_StartThread, &g_HandlerContex) != 0)
	{
		g_HandlerContex.isThreadRunning = false;
		g_HandlerContex.threadHandler = 0;
		g_status = handler_status_start;
	}
	HandlerKernel_Start();
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
int HANDLER_API Handler_Stop( void )
{
	printf("> %s Stop\n", strHandlerName);
	if(g_HandlerContex.isThreadRunning)
	{
		g_HandlerContex.isThreadRunning = false;
		pthread_join(g_HandlerContex.threadHandler, NULL);
		
	}
	GPS_Stop(&g_HandlerContex.gps);
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
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int cmdID = 0;
	printf(" >Recv Topic [%s] Data %s\n", topic, (char*) data );
	
	if(HandlerKernel_ParseRecvCMD(data, &cmdID) != handler_success)
		return;
	switch(cmdID)
	{
	case hk_auto_upload_req:
		HandlerKernel_LiveReportStart(hk_auto_upload_rep, data);
		break;
	case hk_set_thr_req:
		HandlerKernel_StopThresholdCheck();
		HandlerKernel_SetThreshold(hk_set_thr_rep, data);
		HandlerKernel_StartThresholdCheck();
		break;
	case hk_del_thr_req:
		HandlerKernel_StopThresholdCheck();
		HandlerKernel_DeleteAllThreshold(hk_del_thr_rep);
		break;
	default:
		{
			char repMsg[32] = {0};
			int len = 0;
			sprintf( repMsg, "{\"errorRep\":\"Unknown cmd!\"}" );
			len= strlen( "{\"errorRep\":\"Unknown cmd!\"}" ) ;
			if ( g_sendcbf ) g_sendcbf( & g_PluginInfo, hk_error_rep, repMsg, len, NULL, NULL );
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
	printf("> %s Start Report\n", strHandlerName);
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
	printf("> %s Stop Report\n", strHandlerName);
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
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{
	char* result = NULL;
	int len = 0;

	printf("> %s Get Capability\n", strHandlerName);

	if(!pOutReply) return len;

	if(g_Capability == NULL)
		g_Capability = CreateCapability(true, &g_HandlerContex.serial, &g_HandlerContex.gps);

	result = IoT_PrintCapability(g_Capability);

	len = strlen(result);
	*pOutReply = (char *)calloc(1, len + 1);
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
	printf("> %s Free Allocated Memory\n", strHandlerName);

	if(pInData)
	{
		free(pInData);
		pInData = NULL;
	}
	return;
}

//-----------------------------------------------------------------------------
// Internal Function:
//-----------------------------------------------------------------------------


MSG_CLASSIFY_T * CreateCapability(bool bReport, serial_port_t* serial,gps_handle_t* pGPSHandle)
{
	MSG_CLASSIFY_T* myCapability = GPS_CreateGPS(strHandlerName);

	if(GPS_GetCapability(myCapability, serial, pGPSHandle))
		HandlerKernel_SetCapability(myCapability, bReport);

	return myCapability;
}

//-----------------------------------------------------------------------------
// GPS Access:
//-----------------------------------------------------------------------------
bool GPS_Initialize(gps_handle_t* pGPSHandle, serial_port_t* serial)
{
	if(pGPSHandle == NULL) return false;
	//pGPSHandle->pHandle = (GPS_HANDLE*)malloc(sizeof(GPS_HANDLE));
	if( EWM_InitializeGps(&pGPSHandle->pHandle) != 1)
	{
		GPSHLog(Error, "Init GPS Library Fail");
		return false;
	}
	EWM_GetGpsSDKVersion(pGPSHandle->pHandle, &pGPSHandle->major, &pGPSHandle->minor);

	if(EWM_SetComPortNum(pGPSHandle->pHandle, serial->SerialPortNum) != 1)
	{
		EWM_UnInitializeGps(pGPSHandle->pHandle);
		pGPSHandle->pHandle = NULL;
		GPSHLog(Error, "Set Com Port Fail");
		return false;
	}

	if(EWM_SetComPortBaudRate(pGPSHandle->pHandle,  serial->BaudRate) != 1)
	{
		EWM_UnInitializeGps(pGPSHandle->pHandle);
		pGPSHandle->pHandle = NULL;
		GPSHLog(Error, "Set Baud Rate Fail");
		return false;
	}

	if(EWM_InitializeComPort(pGPSHandle->pHandle) != 1)
	{
		EWM_UnInitializeGps(pGPSHandle->pHandle);
		pGPSHandle->pHandle = NULL;
		GPSHLog(Error, "Init COM Port Fail");
		return false;
	}

	return true;
}

void GPS_Uninitialize(gps_handle_t* pGPSHandle)
{
	if(pGPSHandle == NULL) return;
	if(pGPSHandle->pHandle == NULL) return;
	//EWM_UnInitializeComPort(pGPSHandle->pHandle);
	EWM_UnInitializeGps(pGPSHandle->pHandle);
	pGPSHandle->pHandle = NULL;
	return;
}

bool GPS_Start(gps_handle_t* pGPSHandle)
{
	if(pGPSHandle == NULL)
		return false;

	if(pGPSHandle->pHandle == NULL)
		return false;

	if(EWM_StartGpsParser(pGPSHandle->pHandle) != 1)
	{
		GPSHLog(Error, "Start Parse GPS Fail");
		return false;
	}

	return true;
}

void GPS_Stop(gps_handle_t* pGPSHandle)
{
	if(pGPSHandle == NULL) return;
	if(pGPSHandle->pHandle == NULL) return;

	EWM_StopGpsParser(pGPSHandle->pHandle);
}

bool GPS_GetCapability(MSG_CLASSIFY_T* root, serial_port_t* serial,gps_handle_t* pGPSHandle)
{
	char name[32]={0};
	MSG_CLASSIFY_T* dev = NULL;
	
	if(root == NULL) return false;
	if(pGPSHandle == NULL || serial == NULL)
		return false;
	//if(pGPSHandle->pHandle == NULL)
	//	return false;
	sprintf( (char*)name,"com%d", (char*)serial->SerialPortNum);
	dev = GPS_FindDevice(root, (const char*)name);
	if(dev == NULL)
	{
		char ver[32]={0};
		sprintf( (char*)ver,"%d.%d", pGPSHandle->major, pGPSHandle->minor);
		dev = GPS_AddDevice(root, (const char*)name, (const char*)ver);
	}
	GPS_SetModeAttribute(dev, 3);
	GPS_SetTimeAttribute(dev, "");
	GPS_SetLatitudeAttribute(dev, 0, 180, -180, NULL);
	GPS_SetLongitudeAttribute(dev, 0, 90, -90, NULL);
	GPS_SetAltitudeAttribute(dev, 0, 90, -90, NULL);
	return true;
}

bool GPS_TransferDateTimeStr(int date, double time, char* strdatetime)
{
	int dd,mm,yy,h,m,s,ms;
	if(strdatetime == NULL) return false;
	
	dd = date/10000;
	mm = date/100%100;
	yy = date%100;

	h = (int)(time*100)/1000000;
	m = (int)(time*100)/10000%100;
	s = (int)(time*100)/100%100;
	ms = (int)(time*100)%100;

	sprintf(strdatetime, "20%02d-%02d-%02dT%02d:%02d:%02d.%02d", 
		yy, mm, dd,
		h, m, s, ms
	);

	return true;
}

bool GPS_GetData(MSG_CLASSIFY_T* root, serial_port_t* serial, gps_handle_t* pGPSHandle)
{
	double lat = 0;
	double lon = 0;
	double alt = 0;
	int date = 0;
	double time = 0;
	char name[32]={0};
	MSG_CLASSIFY_T* dev = NULL;

	if(root == NULL) return false;
	if(pGPSHandle == NULL || serial == NULL)
		return false;
	if(pGPSHandle->pHandle == NULL)
		return false;

	sprintf( (char*)name,"com%d", serial->SerialPortNum);
	dev = GPS_FindDevice(root, (const char*)name);
	if(dev == NULL)
		return false;

	if(EWM_GetGpsUtcTime(pGPSHandle->pHandle, &time) == 1)
	{
		if(EWM_GetGpsDate(pGPSHandle->pHandle, &date) == 1)
		{
			char datetime[32] = {0};
			if(GPS_TransferDateTimeStr(date, time, datetime))
				GPS_SetTimeAttribute(dev, datetime);
		}
	}
	

	if(EWM_GetGpsLatitude(pGPSHandle->pHandle, &lat) == 1)
		GPS_SetLatitudeAttribute(dev, lat, 180, -180, NULL);
	if(EWM_GetGpsLongitude(pGPSHandle->pHandle, &lon) == 1)
		GPS_SetLongitudeAttribute(dev, lon, 90, -90, NULL);
	if(EWM_GetGpsAltitude(pGPSHandle->pHandle, &alt) == 1)
		GPS_SetAltitudeAttribute(dev, alt, 90, -90, NULL);
	return true;
}

void* GPS_StartThread(void* args)
{
	handler_context_t *pHandlerContex = (handler_context_t *)args;
	int count = 0, interval=1000;
	int mInterval = 0;
	if(!pHandlerContex)
	{
		pthread_exit(0);
		return 0;
	}
	mInterval = pHandlerContex->interval;
	if(g_Capability == NULL)
		g_Capability = CreateCapability(true, &pHandlerContex->serial, &pHandlerContex->gps);

	usleep(interval*1000);
	while(pHandlerContex->isThreadRunning)
	{
		/*update gps data*/
		GPS_GetData(g_Capability, &pHandlerContex->serial, &pHandlerContex->gps);

		if(pHandlerContex->interval!=mInterval)
		{
			mInterval = pHandlerContex->interval;
		}
		count = mInterval*1000/interval;

		while(count > 0)
		{
			if(pHandlerContex->interval!=mInterval)
				break;

			if(!pHandlerContex->isThreadRunning)
				break;

			count--;
			usleep(interval*1000);
		}
	}

	pthread_exit(0);
	return 0;
}
//-----------------------------------------------------------------------------
// INI File Access:
//-----------------------------------------------------------------------------

bool ReadINI_Platform(char *iniPath, serial_port_t* serial)
{
	char ininame[MAX_PATH]={0};
	char inifilepath[MAX_PATH]={0};
	
	sprintf(ininame, "%s.ini", strHandlerName);
	// Load ini file
	util_path_combine(inifilepath,iniPath,ininame);

    if (util_is_file_exist(inifilepath)) {
        GPSHLog(Debug, "INI Opened Successfully...\n[%s]", inifilepath);

		serial->SerialPortNum = GetIniKeyInt("Platform","SerialPortNum", inifilepath);
		serial->BaudRate = GetIniKeyInt("Platform","Baud", inifilepath);

		return true;
    }
    else {
        GPSHLog(Warning, "INI Opened Fail...\n[%s]", inifilepath);
		return false;
    }
}