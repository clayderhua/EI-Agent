/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/06 by Eric Liang															     */
/* Modified Date: 2015/03/06 by Eric Liang															 */
/* Abstract     : IoTSensors_Handler                                     										*/
/* Reference    : None																									 */
/****************************************************************************/
//#include <stdbool.h>
#include <stddef.h>
#include "platform.h"
#include "susiaccess_handler_ex_api.h"
#include "wisepaas_02_def.h"
// IoTSensorHandler Header Files
#include "IoTGWHandler.h"
#include "IoTGWFunction.h"
#include "inc/IoTGW_Def.h"
#include "inc/SensorNetwork_Manager_API.h"
#include "inc/SensorNetwork_BaseDef.h"
#include "inc/SensorNetwork_API.h"
#include "inc/AdvWebClientAPI.h"
#include "common.h"
#include "SnGwParser.h"
#include "SnMJSONParser.h"
// include lib Header Files
#include "BasicFun_Tool.h"
#include "DataTransform.h"
#include "cJSON.h"
#include "HashTable.h"

// include AdvApiMux
#include "inc/AdvApiMux/AdvAPIMuxServer.h"

#include <AdvJSON.h>


//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define cagent_request_custom 2002
#define cagent_custom_action 30002
const char strPluginName[MAX_TOPIC_LEN] = {"IoTGW"};
const int iRequestID = cagent_request_custom;
const int iActionID = cagent_custom_action;

//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct{
   void* threadHandler;
   bool isThreadRunning;
}handler_context_t;

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static int  g_bAPIVer = 1;  // 1: C API, 2: WAPI
static void* g_loghandle = NULL;
static bool g_bEnableLog = TRUE;
static char agentConfigFilePath[MAX_PATH] = {0};

static Handler_info_ex  g_PluginInfo;
static handler_context_t g_HandlerContex;
//static HANDLER_THREAD_STATUS g_status = handler_no_init;
static bool m_bAutoReprot = false;
static time_t g_monitortime;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerUnsubscribeCustCbf g_unsubscribecustcbf = NULL;
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
static HandlerSendCapabilityCbf g_sendinfospeccbf = NULL;		


// WAPI
static char *g_pWAP_IoTGWInfoBuffer = NULL;						// Storage Connectivity Capability buffer : size: MAX_DATA_SIZE
static int     g_hWebSocket = 0;                                  // Handler of websocket
static int     g_nAPIGwRunning = 0;

CRITICAL_SECTION g_SenHubSendCriticalSec;
CRITICAL_SECTION g_SendIoTGWCriticalSec;
//static CAGENT_PTHREAD_ENTRY(ThreadSetValue, args);

// SenHub

HashTable          *g_SenHubHashTable;

static Handler_info  g_SenPluginInfo[MAX_SENNODES];  
static cagent_agent_info_body_t g_SenHubAgentInfo[MAX_SENNODES];
static cagent_agent_info_body_t g_gw;



// IoTGW 
static char* GetCapability( );

int runing = 0;


//-----------------------------------------------------------------------------
// Function:
//-----------------------------------------------------------------------------

void update_sendata(void *arg);
static int SendMsgToSUSIAccess(  const char* Data, unsigned int const DataLen, void *pRev1, void* pRev2 );

//#ifdef _MSC_VER
//BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
//{
//	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
//	{
//		printf("DllInitializer\r\n");
//		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
//		if (reserved == NULL) // Dynamic load
//		{
//			// Initialize your stuff or whatever
//			// Return FALSE if you don't want your module to be dynamically loaded
//		}
//		else // Static load
//		{
//			// Return FALSE if you don't want your module to be statically loaded
//			return FALSE;
//		}
//	}
//
//	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
//	{
//		printf("DllFinalizer\r\n");
//		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
//		{
//			// Cleanup
//			Handler_Uninitialize();
//		}
//		else // Process is terminating
//		{
//			// Cleanup
//			Handler_Uninitialize();
//		}
//	}
//	return TRUE;
//}
//#else
//__attribute__((constructor))
///**
// * initializer of the shared lib.
// */
//static void Initializer(int argc, char** argv, char** envp)
//{
//    fprintf(stderr, "DllInitializer\r\n");
//}
//
//__attribute__((destructor))
///** 
// * It is called when shared lib is being unloaded.
// * 
// */
//static void Finalizer()
//{
//    fprintf(stderr, "DllFinalizer\r\n");
//	Handler_Uninitialize();
//}
//#endif

static CAGENT_PTHREAD_ENTRY(ThreadSendSenHubConnect, args)
{
	int i=0;
	int len = 0;
	Handler_info  *pSenHander = NULL;
	char buffer[MAX_BUFFER_SIZE]={0};
	//cagent_agent_info_body_t *pSenAgentInfo = NULL;

	//PRINTF("Start send SenHub list message \r\n" );			
	len = ProcGetTotalInterface(buffer, sizeof(buffer));
	if( len > 0 )
	{
		//PRINTF("Send SenHub list: %s\r\n", buffer);
		SendMsgToSUSIAccess(buffer, sizeof(buffer), NULL, NULL);
	}


	PRINTF("Start send SenHub connect message\r\n");
	for( i = 0; i< MAX_SENNODES ; i ++ ) {
		if(g_SenHubAgentInfo[i].status == 1)
		{
			pSenHander = &g_SenPluginInfo[i];
			//pSenAgentInfo = &g_SenHubAgentInfo[i];

			// 2. Prepare Sensor Node Handler_info data
			//PackSenHubPlugInfo( pSenHander, &g_PluginInfo, pSenAgentInfo->mac, pSenAgentInfo->hostname, pSenAgentInfo->product, 1 );

			// 3. Register to WISECloud
			PRINTF("Send SenHub %s\r\n", pSenHander->agentInfo->hostname);
			SenHubConnectToWISECloud( pSenHander );
		}
	}
	PRINTF("Finish send SenHub connect message\r\n");
	app_os_thread_exit(0);
	return 0;
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	HANDLER_INFO_EX* plugininfoex = NULL;
	if( pluginfo == NULL )
		return handler_fail;
	plugininfoex = (HANDLER_INFO_EX*)pluginfo;
	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", strPluginName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" >Name: %s\n\r\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, plugininfoex, sizeof(HANDLER_INFO_EX));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	if(g_bEnableLog)
	{
		g_loghandle = pluginfo->loghandle;
	}

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;


	g_sendinfospeccbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;

	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_unsubscribecustcbf = g_PluginInfo.unsubscribecustcbf = plugininfoex->unsubscribecustcbf;

	g_HandlerContex.threadHandler = NULL;
	g_HandlerContex.isThreadRunning = false;
	//g_status = handler_init;


	// <Eric>
	if( InitSNGWHandler() < 0 )
		return handler_fail;

	return handler_success;
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
	g_sendcustcbf = NULL;
	g_sendreportcbf = NULL;
	g_sendinfospeccbf = NULL;
	g_subscribecustcbf = NULL;
	g_unsubscribecustcbf = NULL;
	Handler_Stop();
	
	//<Eric>
	UnInitSNGWHandler();
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
	*pOutStatus = handler_status_start;
	return handler_success;
/*
	int iRet = handler_fail; 

	if(!pOutStatus) return iRet;

	switch (g_status)
	{
	default:
	case handler_no_init:
	case handler_init:
	case handler_stop:
		*pOutStatus = g_status;
		break;
	case handler_start:
	case handler_busy:
		{
			time_t tv;
			time(&tv);
			if(difftime(tv, g_monitortime)>5)
				g_status = handler_busy;
			else
				g_status = handler_busy;
			*pOutStatus = g_status;
		}
		break;
	}
	
	iRet = handler_success;
	return iRet;
*/
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
	int i=0;
	Handler_info  *pSenHander = NULL;
        void* threadHandler;
	//cagent_agent_info_body_t *pSenAgentInfo = NULL;

	printf(" %s> Update Status\r\n", strPluginName);
	if(pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strPluginName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
	}
	/*if(pluginfo->agentInfo->status == 1)
	{
		if (app_os_thread_create(&threadHandler, ThreadSendSenHubConnect, NULL) == 0)
		{
			app_os_thread_detach(threadHandler);
		}
	}*/
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
	printf("> %s Handler_Start\r\n", strPluginName);
	//if (app_os_thread_create(&g_HandlerContex.threadHandler, SampleHandlerThreadStart, &g_HandlerContex) != 0)
	//{
	//	g_HandlerContex.isThreadRunning = false;
	//	printf("> start handler thread failed!\r\n");	
	//	return handler_fail;
 //   }
	g_HandlerContex.isThreadRunning = true;
	//g_status = handler_start;
	time(&g_monitortime);

	// <Eric>
	StartSNGWHandler();
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
	if(g_HandlerContex.isThreadRunning == true)
	{
		g_HandlerContex.isThreadRunning = false;
		if(g_HandlerContex.threadHandler) {
			app_os_thread_join(g_HandlerContex.threadHandler);
		}
		g_HandlerContex.threadHandler = NULL;
	}
	//g_status = handler_stop;

	// <Eric>
	StopSNGWHandler();
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input : char * const topic, 
 *			void* const data, 
 *			const size_t datalen
 *  Output: void *pRev1, 
 *					void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int cmdID = 0;
	int len = 0;
	char szSessionId[MAX_SIZE_SESSIONID]={0};
	char buffer[MAX_BUFFER_SIZE]={0};
	// Recv Remot Server Command to process
	PRINTF("%s>Recv Topic [%s] Data %s\r\n", strPluginName, topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &cmdID,szSessionId,sizeof(szSessionId)))
		return;

	PRINTF("Cmd ID=%d\r\n",cmdID);
	switch (cmdID)
	{
	case IOTGW_GET_CAPABILITY_REQUEST:
	case IOTGW_HANDLER_GET_CAPABILITY_REQUEST:
		{			
			char *pszCapability = NULL;
			pszCapability = GetCapability();
			if( pszCapability != NULL ) {
				g_sendcbf(&g_PluginInfo, cmdID+1, pszCapability, strlen( pszCapability )+1, NULL, NULL);
				{
					void* threadHandler;
					if (app_os_thread_create(&threadHandler, ThreadSendSenHubConnect, NULL) == 0)
					{
						app_os_thread_detach(threadHandler);
					}
				}
			} else {
				g_sendcbf(&g_PluginInfo, IOTGW_ERROR_REPLY, g_szErrorCapability, strlen( g_szErrorCapability )+1, NULL, NULL);
			}
		}
		break;
	case IOTGW_GET_SENSOR_REQUEST:
		{
			// { "sessionID":"XXX", "sensorInfoList":{"e":[{"n":"WSN/0000852CF4B7B0E8/Info/Health","v":80,"StatusCode":200}]} }
			PRINTF("IOTGW_GET_SENSOR_REQUEST data=%s\r\n",data );			
			len = ProcGetInterfaceValue(szSessionId, data, buffer, sizeof(buffer), g_nAPIGwRunning);
			PRINTF("len=%d Ret=%s\r\n",len,buffer);
			if( len > 0 )
				g_sendcbf(&g_PluginInfo,IOTGW_GET_SENSOR_REPLY, buffer, len+1, NULL, NULL);
		}
		break;
	case IOTGW_SET_SENSOR_REQUEST:
		{
			// { "sessionID":"XXX", "sensorInfoList":{"e":[{"n":"WSN/0000852CF4B7B0E8/Info/Health","v":80,"StatusCode":200}]} }
			PRINTF("IOTGW_SET_SENSOR_REQUEST data=%s\r\n",data );			
			len = ProcSetInterfaceValue(szSessionId, data, buffer, sizeof(buffer));
			PRINTF("len=%d Ret=%s\r\n",len,buffer);
			if( len > 0 )
				g_sendcbf(&g_PluginInfo,IOTGW_SET_SENSOR_REPLY, buffer, len+1, NULL, NULL);
		}
		break;
	case IOTGW_MONITOR_REQUEST:
		{
			char *pszCapability = GetCapability();
			if( pszCapability != NULL ) 
				g_sendcbf(&g_PluginInfo, cmdID+1, pszCapability, strlen( pszCapability )+1, NULL, NULL);
		}
		break;
	default:
		{
			PRINTF("Unknow CMD ID=%d\r\n", cmdID );
			/*  {"sessionID":"1234","errorRep":"Unknown cmd!"}  */
			if(strlen(szSessionId)>0)
				snprintf(buffer, sizeof(buffer), "{\"%s\":\"%s\",\"sessionID\":\"%s\"}", SN_ERROR_REP, "Unknown cmd!", szSessionId);
			else
				snprintf(buffer, sizeof(buffer), "{\"%s\":\"%s\"}", SN_ERROR_REP, "Unknown cmd!");
			g_sendcbf(&g_PluginInfo, IOTGW_ERROR_REPLY, buffer, strlen(buffer)+1, NULL, NULL);
			break;
		}
		break;
	}
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: char * pOutReply
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["all"],"commCmd":2053,"type":"WSN"}}*/
	/*TODO: Parsing received command*/
	m_bAutoReprot = true;
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
	m_bAutoReprot = false;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char * : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{
	int len = 0; // Data length of the pOutReply 
	char *pBuffer = NULL;
	//char result[MAX_DATA_SIZE]={0};
	PRINTF("\r\n\r\n");
	PRINTF("IoTGW Handler_Get_Capability\r\n");
	//GetIoTGWCapability(data,sizeof(data));

	if( pSNManagerAPI || g_pWAPI_Interface ) {
		if( pSNManagerAPI)
			pBuffer = pSNManagerAPI->SN_Manager_GetCapability( );
		else
			pBuffer = g_pWAP_IoTGWInfoBuffer;
		if( pBuffer ) {
			len = strlen( pBuffer );
			*pOutReply = (char *)malloc(len + 1);
			memset(*pOutReply, 0, len + 1);
			strcpy(*pOutReply, pBuffer);
			{
				void* threadHandler;
				if (app_os_thread_create(&threadHandler, ThreadSendSenHubConnect, NULL) == 0)
				{
					app_os_thread_detach(threadHandler);
				}
			}
		} // End of pBuffer
	} else {
		PRINTF("SN_Manager_GetCapability Not Found\r\n");
	}

	return len;
}

/* **************************************************************************************
 *  Function Name: Handler_MemoryFree
 *  Description: free the mamory allocated for Handler_Get_Capability
 *  Input : char *pInData.
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_MemoryFree(char *pInData)
{
	if(pInData)
	{
		free(pInData);
		pInData = NULL;
	}
}

static char* GetCapability( )
{
	if( pSNManagerAPI ) {
		return pSNManagerAPI->SN_Manager_GetCapability();
	} else if ( g_pWAP_IoTGWInfoBuffer ) {
		return g_pWAP_IoTGWInfoBuffer;
	} else
		return NULL;

}




/****************************************************************************/
/* Below Functions are IoTGW Handler source code                   			 */
/****************************************************************************/

static int UpdateInterfaceData(  const char *pInJson, const int InDatalen );

static int Register_SenHub( const char *pJSON, const int nLen, void **pOutParam, void *pRev1, void *pRev2 );

static int Disconnect_SenHub( void *pOutParam );

static int SendInfoSpec_SenHub( const char *pInJson, const int InDataLen, void *pOutParam, void *pRev1 );

static int AutoReportSenData_SenHub( const char *pInJson, const int InDataLen, void *pOutParam, void *pRev1 );

int ProcSet_Result( const char *pInJson, const int InDataLen, void *pInParam, void *pRev1 );

static SN_CODE SNManagerAPI ProceSNManagerDataCbf ( const int cmdId, const char *pInJson, const int InDatalen, void **pOutParam, void *pRev1, void *pRev2 )
{
	int rc = 0;

	switch( cmdId)
	{
	case SN_Inf_UpdateInterface_Data:
		{
			rc = UpdateInterfaceData( pInJson, InDatalen );
		}
		break;
	case SN_SenHub_Register:
		{
			rc = Register_SenHub( pInJson, InDatalen, pOutParam, pRev1, pRev2 );
		}
		break;
	case SN_SenHub_SendInfoSpec:
		{
			rc = SendInfoSpec_SenHub( pInJson, InDatalen, *pOutParam, pRev1 );
		}
		break;
	case SN_SenHub_AutoReportData:
		{
			rc = AutoReportSenData_SenHub( pInJson, InDatalen, *pOutParam, pRev1 );
		}
		break;
	case SN_SetResult:
		{
			rc = ProcSet_Result( pInJson, InDatalen, *pOutParam, pRev1 );
		}
		break;
	case SN_SenHub_Disconnect:
		{
			rc = Disconnect_SenHub( *pOutParam );
		}
		break;	
	default:
		PRINTF("ReportSNManagerDtatCbf Cmd Not Support = %d\r\n", cmdId);
	}
	return rc;
}

static int SendMsgToSUSIAccess(  const char* Data, unsigned int const DataLen, void *pRev1, void* pRev2 )
{
	int iRet = 0;
	char* newData = ReplaceSenHubList(Data);
	if(newData)
	{
		if( g_sendreportcbf ) {
			g_sendreportcbf( &g_PluginInfo, newData, strlen(newData), pRev1, pRev2);
			iRet = 1;
		}
		free(newData);
	}
	return iRet;
}


// topic: /cagent/admin/%s/agentcallbackreq
static void HandlerCustMessageRecv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2)
{
	int cmdID = 0;
	char newSenHubUID[MAX_SN_UID]={0};
	char oldSenHubUID[MAX_SN_UID]={0};
	char buffer[MAX_BUFFER_SIZE]={0};
	char Topic[MAX_TOPIC_SIZE]={0};
	char szSessionId[MAX_SIZE_SESSIONID]={0};
	int index = -1;
	int len = 0;
	Handler_info  *pSenHander = NULL;
	Handler_info  SenHubHandler;

	int i = 0;
	// Recv Remot Server Command to process
	//PRINTF(" %s>Recv Cust Topic [%s] Data %s\r\n", strPluginName, topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &cmdID,szSessionId,sizeof(szSessionId))) {
		PRINTF(" Can't find cmdID\r\n" );
		return;
	}
	/*if( GetSenHubUIDfromTopic( transTopic, SenHubUID, sizeof(SenHubUID) ) == 0 ) {
		PRINTF(" Can't find SenHubID Topic=%s\r\n",topic );
		return;
	}*/
	if(!GetNewIDFromTopic(topic, newSenHubUID))
	{
		PRINTF(" Can't find SenHubID Topic=%s\r\n", topic );
		return;
	}
	GetOldID(newSenHubUID, oldSenHubUID);
	if( ( index = GetSenHubAgentInfobyUID(&g_SenHubAgentInfo, MAX_SENNODES, newSenHubUID) ) == -1 ) {
		PRINTF(" Can't find SenHub UID in Table =%s\r\n",newSenHubUID );
		return;
	}

	pSenHander = &g_SenPluginInfo[index];

	if( pSenHander ) {
		memset(&SenHubHandler,0,sizeof(SenHubHandler));
		memcpy(&SenHubHandler,pSenHander, sizeof(SenHubHandler));
		snprintf(SenHubHandler.Name,MAX_TOPIC_LEN, "%s", SENHUB_HANDLER_NAME); // "SenHub"
		pSenHander = &SenHubHandler;
	} else 
		return;
	// "/cagent/admin/%s/agentinfoack"
	memset(Topic,0,sizeof(Topic));
	snprintf(Topic,sizeof(Topic), DEF_AGENTACT_TOPIC, g_PluginInfo.agentInfo->productId, newSenHubUID );
		
	PRINTF("HandlerCustMessageRecv Cmd =%d\r\n",cmdID);

	switch (cmdID)
	{
	case IOTGW_HANDLER_GET_CAPABILITY_REQUEST:
		{
			// { "sessionID":"XXX", "StatusCode":200, "SenHub": { xxxx_JSON_Object } }
			//PRINTF("IOTGW_HANDLER_GET_CAPABILITY_REQUEST: %s\r\n", oldSenHubUID);
			len = ProcGetSenHubCapability(oldSenHubUID, data, buffer, sizeof(buffer));
			//PRINTF("len=%d Ret=%s\n",len,buffer);
			if( len > 0 )
			{
				char* newData = NULL;
				newData = Trans2NewFrom(buffer);
				if(newData)
				{
					Handler_info tmpHandler;
					memcpy(&tmpHandler, pSenHander, sizeof(Handler_info));					
					strcpy(tmpHandler.Name, "general");
					tmpHandler.RequestID=1001;
					tmpHandler.ActionID=2001;
					//PRINTF("Handler Name=%s RequestID=%d ActionID=%d\n",tmpHandler.Name ,tmpHandler.RequestID,tmpHandler.ActionID);
					g_sendcustcbf(&tmpHandler,IOTGW_HANDLER_GET_CAPABILITY_REPLY, Topic, newData, strlen(newData)+1, NULL, NULL);
					free(newData);
				}
				else
				{
					PRINTF("IOTGW_HANDLER_GET_CAPABILITY_REQUEST: Data Transform failed!\r\n");
				}
			}
		}
	case IOTGW_GET_CAPABILITY_REQUEST:
		{
			// { "sessionID":"XXX", "StatusCode":200, "SenHub": { xxxx_JSON_Object } }
			//PRINTF("IOTGW_GET_CAPABILITY_REQUEST\r\n" );
			len = ProcGetSenHubCapability(oldSenHubUID, data, buffer, sizeof(buffer));
			//PRINTF("len=%d Ret=%s\n",len,buffer);
			if( len > 0 )
			{
				char* newData = NULL;
				newData = Trans2NewFrom(buffer);
				if(newData)
				{
					if( len > 0 )
						g_sendcustcbf(pSenHander,IOTGW_GET_CAPABILITY_REPLY, Topic, newData, strlen(newData)+1 , NULL, NULL);
				
					free(newData);
				}
				else
				{
					PRINTF("IOTGW_GET_CAPABILITY_REQUEST: Data Transform failed!\r\n");
				}
			}
		}
		break;
	case IOTGW_GET_SENSOR_REQUEST:
		{
			// { "sessionID":"XXX", "sensorInfoList":{"e":[{"n":"SenData/dout","bv":1,"StatusCode":200}]} }
			PRINTF("IOTGW_GET_SENSOR_REQUEST data=%s\r\n",data );			
			len = ProcGetSenHubValue(oldSenHubUID, szSessionId, data, buffer, sizeof(buffer));
			//PRINTF("len=%d Ret=%s\n",len,buffer);
			if( len > 0 )
			{
				char* newData = NULL;
				newData = Trans2NewFrom(buffer);
				if(newData)
				{
					if( len > 0 )
						g_sendcustcbf(pSenHander,IOTGW_GET_SENSOR_REPLY, Topic, newData, strlen(newData)+1 , NULL, NULL);
				
					free(newData);
				}
				else
				{
					PRINTF("IOTGW_GET_SENSOR_REQUEST: Data Transform failed!\r\n");
				}
			}
		}
		break;
	case IOTGW_SET_SENSOR_REQUEST:
		{
			// { "sessionID":"XXX", "sensorInfoList":{"e":[ {"n":"SenData/dout", "sv":"Setting", "StatusCode": 202 } ] } }
			// { "sessionID":"XXX", "sensorInfoList":{"e":[{"n":"SenData/din","sv":"Read Only","StatusCode":405} ] } }
			//PRINTF("IOTGW_SET_SENSOR_REQUEST data=%s\r\n",data );
			len = ProcSetSenHubValue(oldSenHubUID, szSessionId, data, index, Topic, buffer, sizeof(buffer));
			//PRINTF("len=%d Ret=%s\n",len,buffer);
			if( len > 0 )
			{
				char* newData = NULL;
				newData = Trans2NewFrom(buffer);
				if(newData)
				{
					if( len > 0 )
						g_sendcustcbf(pSenHander,IOTGW_SET_SENSOR_REPLY, Topic, newData, strlen(newData)+1 , NULL, NULL);
					
					free(newData);
				}
				else
				{
					PRINTF("IOTGW_SET_SENSOR_REQUEST: Data Transform failed!\r\n");
				}
			}
		}
		break;
	default:
		{
			char* newData = NULL;
			PRINTF("Unknow CMD ID=%d\r\n", cmdID );
			/*  {"sessionID":"1234","errorRep":"Unknown cmd!"}  */
			if(strlen(szSessionId)>0)
				snprintf(buffer, sizeof(buffer), "{\"%s\":\"%s\",\"sessionID\":\"%s\"}", SN_ERROR_REP, "Unknown cmd!", szSessionId);
			else
				snprintf(buffer, sizeof(buffer), "{\"%s\":\"%s\"}", SN_ERROR_REP, "Unknown cmd!");
			
			newData = Trans2NewFrom(buffer);
			if(newData)
			{
				g_sendcustcbf(pSenHander,IOTGW_ERROR_REPLY, Topic, newData, strlen(newData)+1 , NULL, NULL);			
				free(newData);
			}
			else
			{
				PRINTF("Unknow CMD: Data Transform failed!\r\n");
			}
		}
		break;
	}
}

// topic: /cagent/admin/%s/agentinfo
static void HandlerCustConnectRecv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2)
{
	int cmdID = 0;
	char szParentId[MAX_SN_UID]={0};
	char newSenHubUID[MAX_SN_UID]={0};
	int status = 0;
	if(!ParseAgentInfo(data, datalen, &status, szParentId,sizeof(szParentId))) {
		PRINTF(" Can't find parentID\r\n" );
		return;
	}
	
	if(!GetNewIDFromTopic(topic, newSenHubUID))
	{
		PRINTF(" Can't find SenHubID Topic=%s\r\n", topic );
		return;
	}

	//if(strcmp(szParentId, g_PluginInfo.agentInfo->devId) != 0 && status == 1)
	//{
	//	char Topic[DEF_MAX_STRING_LENGTH]={0};
	//	// 1. Unsubscribe Topic
	//	snprintf( Topic, sizeof(Topic), DEF_CALLBACKREQ_TOPIC,g_PluginInfo.agentInfo->productId, newSenHubUID);

	//	if( g_unsubscribecustcbf )
	//		g_unsubscribecustcbf( Topic);		
	//	PRINTF("Unsubscribe SenHub Topic:%s \n", Topic);

	//	snprintf( Topic, sizeof(Topic), DEF_INFOACK_TOPIC, newSenHubUID);
	//	if( g_unsubscribecustcbf )
	//		g_unsubscribecustcbf( Topic);		
	//	PRINTF("Unsubscribe SenHub Topic:%s \n", Topic);
	//}
}


int SenHubConnectToWISECloud( Handler_info *pSenHubHandler)
{
	char Topic[DEF_MAX_STRING_LENGTH]={0};
	char JSONData[MAX_FUNSET_DATA_SIZE]={0};
	int   datalen = 0;
	int   rc = 1;
	char newID[DEF_DEVID_LENGTH] = {0};
	char parentId[DEF_DEVID_LENGTH] = {0};

	app_os_EnterCriticalSection(&g_SenHubSendCriticalSec);

	if(strlen(pSenHubHandler->WorkDir) > 0 )// FOR ParentID
		snprintf(parentId,sizeof(parentId),"%s",pSenHubHandler->WorkDir);
	else
		snprintf(parentId,sizeof(parentId),"%s",g_PluginInfo.agentInfo->devId);
	
	datalen = GetAgentInfoData(JSONData,sizeof(JSONData),pSenHubHandler, parentId, g_PluginInfo.agentInfo->devId);

	if(datalen <20 )
		rc = 0;

	// 1. Subscribe Topic2 -> Create SenHub <-> WISECloud communication channel
	snprintf( Topic, sizeof(Topic), DEF_CALLBACKREQ_TOPIC, pSenHubHandler->agentInfo->productId, pSenHubHandler->agentInfo->devId);

	if( g_subscribecustcbf )
		g_subscribecustcbf( Topic, &HandlerCustMessageRecv );		
	PRINTF("Subscribe SenHub Topic:%s \n", Topic);

	snprintf( Topic, sizeof(Topic), DEF_INFOACK_TOPIC, pSenHubHandler->agentInfo->devId);

	if( g_subscribecustcbf )
		g_subscribecustcbf( Topic, &HandlerCustConnectRecv );		
	PRINTF("Subscribe SenHub Topic:%s \n", Topic);

	

	// 2. SendAgentInfo online
	memset(Topic,0,sizeof(Topic));
	snprintf(Topic,sizeof(Topic),DEF_INFOACK_TOPIC, pSenHubHandler->agentInfo->devId);
	if( g_sendcustcbf )
	{
		if( g_sendcustcbf )
			g_sendcustcbf(pSenHubHandler,1,Topic,JSONData, datalen, NULL, NULL);
	}
	PRINTF("Send SenHub AgentInfo:%s \n", JSONData);
	app_os_LeaveCriticalSection(&g_SenHubSendCriticalSec);
	//PRINTF("SenHubConnectToWISECloud Leave\n");
	return rc;

}
int SenHubDisconnectWISECloud( Handler_info *pSenHubHandler)
{
	char Topic[MAX_TOPIC_SIZE]={0};
	char JSONData[MAX_FUNSET_DATA_SIZE]={0};
	int   datalen = 0;
	int   rc = 1;
	char newID[DEF_DEVID_LENGTH] = {0};

	// 1. Unsubscribe Topic
	snprintf( Topic, sizeof(Topic), DEF_CALLBACKREQ_TOPIC, pSenHubHandler->agentInfo->productId, pSenHubHandler->agentInfo->devId);

	if( g_unsubscribecustcbf )
		g_unsubscribecustcbf( Topic);		
	PRINTF("Unsubscribe SenHub Topic:%s \n", Topic);

	snprintf( Topic, sizeof(Topic), DEF_INFOACK_TOPIC, pSenHubHandler->agentInfo->devId);
	if( g_unsubscribecustcbf )
		g_unsubscribecustcbf( Topic);		
	PRINTF("Unsubscribe SenHub Topic:%s \n", Topic);


	// 2. Send disconnect message to WISECloud
	memset(Topic,0,sizeof(Topic));
	snprintf(Topic,sizeof(Topic),DEF_INFOACK_TOPIC, pSenHubHandler->agentInfo->devId);

	datalen = GetAgentInfoData(JSONData,sizeof(JSONData),pSenHubHandler, g_PluginInfo.agentInfo->devId, g_PluginInfo.agentInfo->devId);

	if(datalen <20 )
		rc = 0;

	if( g_sendcustcbf )
		g_sendcustcbf(pSenHubHandler,1,Topic,JSONData, datalen, NULL, NULL);
	else 
		rc = 0;

	return rc;
}

static int wapiID= 0;

static CAGENT_PTHREAD_ENTRY(ThreadSynWSNInfo, args)
{
	int i=0;
    int j=0;
	int len = 0;
    char id[64]={0};
	char *buffer = NULL;
	char *tmp = NULL;
	char name[128]={0};
	char type[32]={0};
	char parentid[64]={0};
    char idlist[MAX_SENNODES][64]={0};
	char* dev = NULL;
	char* token = NULL;
	SenHubInfo senHubInfo;
	SenHubExInfo senHubExInfo; // ESL
	Handler_info *pHandler_info = NULL;
	JSONode *jsonde = NULL;


	buffer = (char*)malloc(MAX_SET_DATA);
	tmp = (char*)malloc(MAX_SET_DATA);

	g_nAPIGwRunning = 0;

	if( buffer != NULL && tmp != NULL ) 
	{
			memset(buffer,0,MAX_SET_DATA);
			memset(tmp,0,MAX_SET_DATA);
	}
	else
		return -1;


	if( args != NULL ) { // API-GW service is not availabled
		// send null IoTGW Capability
		app_os_EnterCriticalSection(&g_SendIoTGWCriticalSec);
		snprintf(g_pWAP_IoTGWInfoBuffer, MAX_BUFFER_SIZE, "%s", g_szErrorCapability );
		if( g_sendinfospeccbf ) {
			char* newData = Trans2NewFrom(g_pWAP_IoTGWInfoBuffer);
			if(newData)
			{
				g_sendinfospeccbf(&g_PluginInfo, newData, strlen(newData), NULL, NULL);
				free(newData);
			}
		}
		//g_sendinfospeccbf(&g_PluginInfo, g_pWAP_IoTGWInfoBuffer, strlen(g_pWAP_IoTGWInfoBuffer), NULL, NULL);
		
		app_os_LeaveCriticalSection(&g_SendIoTGWCriticalSec);

		// send disconnect for each senhub
		for( i = 0; i< MAX_SENNODES ; i ++ ) {
			if(g_SenHubAgentInfo[i].status == 1)
			{
				pHandler_info = &g_SenPluginInfo[i];

				// 3. Register to WISECloud
				PRINTF("Send SenHub %s\r\n", pHandler_info->agentInfo->hostname);
				Disconnect_SenHub( pHandler_info );
			}
		}

	}

	TaskSleep(5000);
	while( g_HandlerContex.isThreadRunning ) {
		// Get Capability -> fill in tmp buffer		
		if( g_pWAPI_Interface->AdvWAPI_Get( CONNECTIVITY_PREFIX, buffer,  MAX_SET_DATA )  == WAPI_CODE_OK ) {
		        if( strlen ( buffer ) < 20 ) {
				TaskSleep(1000);
				continue;
			}
			app_os_EnterCriticalSection(&g_SendIoTGWCriticalSec);

			memset(g_pWAP_IoTGWInfoBuffer,0,MAX_BUFFER_SIZE);
			snprintf(g_pWAP_IoTGWInfoBuffer, MAX_BUFFER_SIZE, "%s", buffer );	
			
			g_nAPIGwRunning = 1;
			if( g_sendinfospeccbf ) {
				char* newData = Trans2NewFrom(g_pWAP_IoTGWInfoBuffer);
				if(newData)
				{
					//PRINTF("WSN Interface =%s\n", newData );
					g_sendinfospeccbf(&g_PluginInfo, newData, strlen(newData), NULL, NULL);
					free(newData);
				}
			}
			//g_sendinfospeccbf(&g_PluginInfo, g_pWAP_IoTGWInfoBuffer, strlen(g_pWAP_IoTGWInfoBuffer), NULL, NULL);

			app_os_LeaveCriticalSection(&g_SendIoTGWCriticalSec);
			break;

		} else { // End of AdvWAPI_Get
			TaskSleep(1000);
		}
	}


	memset(buffer, 0, MAX_SET_DATA);

	// 0. Update Connectivity with Data 
	len = ProcGetTotalInterface(buffer, MAX_SET_DATA);

	if( len > 0 )
	{
		PRINTF("Get WSN Capability: %s\r\n", buffer);
		UpdateInterfaceData( buffer, strlen(buffer) );
	}

	memset(buffer,0,MAX_SET_DATA);
	memset(&senHubInfo,0,sizeof(senHubInfo));
	memset(&senHubExInfo,0,sizeof(senHubExInfo));

	// 1. Get AllSenHublist
	if( WProcGetAllSenHubList(buffer, MAX_SET_DATA) <= 0 ) {
                PRINTF("Not get senhub list\n");
		//goto exit_wsn;
        }else {
		PRINTF("Get SenHub List=%s\n", buffer);
	}
	i = 0;
	dev = strtok_r(buffer,",", &token);
	while (dev != NULL)
	{
                sprintf( idlist[i], "%s", dev);
		i++;
		dev = strtok_r(NULL, ",", &token);
	}

	PRINTF("Total SenHub is =%d\n", i );
        
        for( j = 0; j < i; j++ )
	{
		memset( id, 0, sizeof(id));
        	sprintf( id, "%s", idlist[j]);
                
                if( WProcGetSenHubDeviceInfo(id, tmp, MAX_SET_DATA) > 0 ){
                        jsonde = JSON_Parser((const char*) tmp);

                        snprintf(senHubInfo.sUID, MAX_SN_UID, "%s",  id );

                        if(jsonde != NULL )
                         JSON_Get(jsonde,"[hostname]",name,sizeof(name));

                        snprintf (senHubInfo.sHostName, MAX_SN_HOSTNAME, "%s",  name );

                        memset(name,0, sizeof(name));										

                        if(jsonde != NULL )
                         JSON_Get(jsonde,"[product]",name,sizeof(name));

                        snprintf (senHubInfo.sProduct, MAX_SN_PRODUCT_NAME, "%s", name );

						// ESL
						senHubInfo.pExtened = (void*)&senHubExInfo;

						memset(type,0,sizeof(type));
						if(jsonde != NULL )
							JSON_Get(jsonde,"[type]", type, sizeof(type));

						if(!memcmp(type,"Router",6)){
							memset(type,0,sizeof(type));
							snprintf(type,sizeof(type),"IoTGW");
							snprintf(senHubExInfo.szHandlerName,sizeof(senHubExInfo.szHandlerName),type);
						}
						else
						{
							memset(parentid,0,sizeof(parentid));
							memset(type,0,sizeof(type));
							snprintf(type,sizeof(type),"SenHub");							
							snprintf(senHubExInfo.szHandlerName,sizeof(senHubExInfo.szHandlerName),type);							

							if(jsonde != NULL )
								JSON_Get(jsonde,"[parentID]", parentid, sizeof(parentid));

							if(strcasecmp(parentid,"null"))
								snprintf(senHubExInfo.szParentID,sizeof(senHubExInfo.szParentID),parentid);
							else {															
								snprintf(senHubExInfo.szParentID,sizeof(senHubExInfo.szParentID),"");
								senHubInfo.pExtened = NULL;
                            }	
															
						}								

                        Register_SenHub( NULL, 0, &pHandler_info, &senHubInfo, NULL );

                        memset(&senHubInfo,0,sizeof(senHubInfo));
						memset(&senHubExInfo,0,sizeof(senHubExInfo));
                        memset(name,0, sizeof(name));

                        if( jsonde )
                                JSON_Destory(&jsonde);

                        // 5. Get / Send Capability
                        if( len = WProcGetSenHubCapability( id, tmp, MAX_SET_DATA) > 0 ) {
                                SendInfoSpec_SenHub( tmp, len, pHandler_info, NULL );

								memset(tmp, 0, MAX_SET_DATA );
                                if( len = WProcGetSenHubLastValue( id, tmp, MAX_SET_DATA, type ) > 0) {
										snprintf(buffer,MAX_SET_DATA,"{\"%s\":%s}", type, tmp );
                                        AutoReportSenData_SenHub( buffer, strlen(buffer), pHandler_info, NULL );
                                }
                        }
                        pHandler_info = NULL;
                }else{
                    PRINTF("Get dev Info Error=%s\n", id);
                }
        }
	g_hWebSocket = g_pWAPI_Interface->AdvWAPI_Join_Service( "/WSNManage", NULL);

exit_wsn:
	free(buffer);
	free(tmp);

	app_os_thread_exit(0);
	return 0;
}


static int UpdateInterfaceData(  const char *pInJson, const int InDatalen )
{
	return SendMsgToSUSIAccess( pInJson, InDatalen, NULL, NULL );
}

static int Register_SenHub( const char *pJSON, const int nLen, void **pOutParam, void *pRev1, void *pRev2 )
{
	int rc = 0;
	int index = 0;
	char parentID[64]={0};
	char handlerName[32]={0};
	SenHubInfo *pSenHubInfo = (SenHubInfo*)pRev1;
	SenHubExInfo *pSenHubExInfo = (SenHubExInfo*)pSenHubInfo->pExtened;	
	Handler_info  *pSenHander = NULL;


	if( pSenHubInfo == NULL ) {
		PRINTF("SenHub is NULL\r\n");
		return rc;
	}

	// 1. Find empty SenHub Array
	if(  (index = GetUsableIndex(&g_SenHubAgentInfo, MAX_SENNODES ) )  == -1 ) 
	{
		PRINTF("GW Handler is Full \r\n");
		return rc;
	}

	pSenHander = &g_SenPluginInfo[index];
	*pOutParam    = &g_SenPluginInfo[index];

	// ESL
	if( pSenHubExInfo != NULL )
	{
		snprintf(parentID, sizeof(parentID),"%s",pSenHubExInfo->szParentID);
		GetNewID(pSenHubExInfo->szParentID, parentID);
		snprintf(handlerName, sizeof(handlerName),"%s",pSenHubExInfo->szHandlerName);
	}
	else // default
	{
		snprintf(parentID, sizeof(parentID),"");
		snprintf(handlerName, sizeof(handlerName),"SenHub");		
	}

	// 2. Prepare Sensor Node Handler_info data
	PackSenHubPlugInfo( pSenHander, &g_PluginInfo, pSenHubInfo->sUID, pSenHubInfo->sHostName, pSenHubInfo->sProduct, 1 ,handlerName, parentID );

	// 3. Register to WISECloud
	rc = SenHubConnectToWISECloud( pSenHander );
	

	return rc;
}

static int Disconnect_SenHub( void *pInParam )
{
	int rc = 0;
	Handler_info *pHandler_info = (Handler_info*)pInParam;
	if( pHandler_info == NULL )  {
		PRINTF("[Disconnect_SenHub]: pOutParam is NULL\r\n");
		return rc;
	}

	pHandler_info->agentInfo->status = 0;
	rc = SenHubDisconnectWISECloud( pHandler_info );
	return rc;
}


static int SendInfoSpec_SenHub( const char *pInJson, const int InDataLen, void *pInParam, void *pRev1 )
{
	int rc = 0;

	Handler_info *pHandler_info = (Handler_info*)pInParam;

	if( pHandler_info == NULL ) return rc;

	EnableSenHubAutoReport( pHandler_info->agentInfo->devId, 1 );

	if( g_sendinfospeccbf ) {
		g_sendinfospeccbf( pHandler_info, pInJson, InDataLen, NULL, NULL );
		rc = 1;
	}

	return rc;
}

static int AutoReportSenData_SenHub( const char *pInJson, const int InDataLen, void *pInParam, void *pRev1 )
{
	int rc = 0;

	Handler_info *pHandler_info = (Handler_info*)pInParam;

	if( pHandler_info == NULL ) return rc;

	if( g_sendreportcbf ) {
		g_sendreportcbf( pHandler_info, pInJson, InDataLen, NULL, NULL );
		rc = 1;
	}

	return rc;	
}



// pInJson: 
int ProcSet_Result( const char *pInJson, const int InDataLen, void *pInParam, void *pRev1 )
{
	int rc = 0;	
	int len = 0;
	int index = 0;
	char buffer[MAX_BUFFER_SIZE]={0};
	Handler_info  *pSenHander = NULL;
	Handler_info   SenHubHandler;

	AsynSetParam *pAsynSetParam = (AsynSetParam*)pInParam;
	API_MUX_ASYNC_DATA *pAsynApiMux = (API_MUX_ASYNC_DATA*)pInParam;

	PRINTF("RESTful Set Result\r\n"); // To Proc API MUX set result
	if( pAsynApiMux && !memcmp( pAsynApiMux->magickey, ASYNC_MAGIC_KEY, sizeof( ASYNC_MAGIC_KEY )) ) {
		if( pAsynApiMux->nDone == 0 ) {
			snprintf(pAsynApiMux->buf, pAsynApiMux->bufsize,"%s",pInJson);
			pAsynApiMux->nDone = 1;
			PRINTF("RESTful Set success\n");
			rc = 1;
			return rc;
		} else if ( pAsynApiMux->nDone == -1 )
			FreeMemory(pAsynApiMux);
			PRINTF("RESTful Set Time Out\r\n");
		return rc;
	} 	

	if( pAsynSetParam == NULL ) return rc;


	// To Process C API & WAPI ( REST API-GW ) 
	index = pAsynSetParam->index;

	if( index ==IOTGW_INFTERFACE_INDEX ) {

		memset(buffer, 0, sizeof(buffer));

		len = ProcAsynSetSenHubValueEvent( pInJson,  InDataLen, pAsynSetParam, buffer, sizeof( buffer ) );

		if( len > 0 ) {
			char* newData = Trans2NewFrom(buffer);
			int length = strlen(newData);
			if(newData)
			{			
				g_sendcbf(&g_PluginInfo,IOTGW_SET_SENSOR_REPLY, newData, length, NULL, NULL);
				rc = 1;
				free(newData);				
			}
		}


	} else if( index < 0 || index > MAX_SENNODES)
		goto Exit_ProcSet;
	else {

		pSenHander = &g_SenPluginInfo[index];

		if( pSenHander ) {
			memset(&SenHubHandler,0,sizeof(SenHubHandler));
			memcpy(&SenHubHandler,pSenHander, sizeof(SenHubHandler));
			snprintf(SenHubHandler.Name,MAX_TOPIC_LEN, "%s", SENHUB_HANDLER_NAME); // "SenHub"
			pSenHander = &SenHubHandler;
		} else 
			goto Exit_ProcSet;

		// need to check UID
		// pHandler_info->

		memset(buffer, 0, sizeof(buffer));

		len = ProcAsynSetSenHubValueEvent( pInJson,  InDataLen, pAsynSetParam, buffer, sizeof( buffer ) );

		if( g_sendcustcbf && len > 0 ) { 

			char* newData = Trans2NewFrom(buffer);
			if(newData)
			{
				//g_sendcustcbf(&g_PluginInfo, IOTGW_SET_SENSOR_REPLY, newTopic, newData, strlen(newData) , NULL, NULL);
				g_sendcustcbf(pSenHander, IOTGW_SET_SENSOR_REPLY, pAsynSetParam->szTopic, newData, strlen(newData) , NULL, NULL);
				rc = 1;
				free(newData);
			}
		}
	}
Exit_ProcSet:

	if( pInParam )
		FreeMemory( pInParam );

	return rc;
}


static int Proce_WebApi_Cb( WEBAPI_EVENT e, void *in, int len, void *user )
{

    JSONode *jsonde = NULL;
	char data[MAX_BUFFER_SIZE]={0};
	char tmp[MAX_BUFFER_SIZE]={0};	
	char jsontag[64]={0};
    char event[MAX_TOPIC_SIZE]={0};
	char SenHubUID[MAX_SN_UID]={0};
	char newSenHubUID[MAX_SN_UID]={0};
	char parentid[DEF_DEVID_LENGTH]={0};
	char type[32]={0};
	SenHubExInfo senHubExInfo; // ESL
	int index = -1;
	Handler_info *pSenHander = NULL;
	SenHubInfo senHubInfo;
    void* threadHandler;

	if(e != WAPI_E_RECEIVE )
	{
		if( e == WAPI_E_CLOSE_CONNECTION ) {
	        if (app_os_thread_create(&threadHandler, ThreadSynWSNInfo, &index) == 0)
                {
                        app_os_thread_detach(threadHandler);
                }
		}
	}
	else{
        jsonde = JSON_Parser((const char*) in);
        if(jsonde != NULL ) {

            JSON_Get(jsonde,"[event]",event,64);
   	     	JSON_Get(jsonde,"[data]",data, MAX_BUFFER_SIZE);


			 if( memcmp(event,"eSenHub_UpdateData",18) == 0 ){
			 	 //PRINTF("Update SenHub Data data=%s\n",data);
				 JSON_Get(jsonde,"[data][agentID]",SenHubUID, MAX_SN_UID);
				 GetNewID(SenHubUID, newSenHubUID);
				if( ( index = GetSenHubAgentInfobyUID(&g_SenHubAgentInfo, MAX_SENNODES, newSenHubUID) ) == -1 ) {
					PRINTF(" Can't find SenHub UID in Table =%s\r\n",newSenHubUID );
					//return -1;
                    goto exit_json;
				}
				pSenHander = &g_SenPluginInfo[index];

				snprintf(jsontag,sizeof(jsontag),"[data][%s]",pSenHander->Name);
				JSON_Get(jsonde,jsontag,tmp, MAX_BUFFER_SIZE);
				snprintf(data,MAX_BUFFER_SIZE,"{\"%s\":%s}", pSenHander->Name, tmp );
			
                            //printf("id=%s data=%s\n",SenHubUID, data);
			   AutoReportSenData_SenHub( data, strlen(data) , pSenHander, NULL );

			 } else if( memcmp(event,"eSenHub_Connected",17 ) == 0) {
				//PRINTF("eSenHub_Connected data=%s\n",data); 
				JSON_Get(jsonde,"[data][susiCommData][devID]",SenHubUID, MAX_SN_UID);

				memset(&senHubInfo,0,sizeof(senHubInfo));
				memset(&senHubExInfo,0,sizeof(senHubExInfo));

				snprintf(senHubInfo.sUID, MAX_SN_UID, "%s",  SenHubUID );

				memset(event,0, sizeof(event));
           		JSON_Get(jsonde,"[data][susiCommData][hostname]",event,sizeof(event));
				snprintf (senHubInfo.sHostName, MAX_SN_HOSTNAME, "%s",  event );

				memset(event,0, sizeof(event));
            	JSON_Get(jsonde,"[data][susiCommData][product]",event,sizeof(event));
				snprintf (senHubInfo.sProduct, MAX_SN_PRODUCT_NAME, "%s", event );

				// ESL
				senHubInfo.pExtened = (void*)&senHubExInfo;

				memset(type,0,sizeof(type));
				if(jsonde != NULL )
					JSON_Get(jsonde,"[data][susiCommData][type]", type, sizeof(type));

				if(!memcmp(type,"Router",6)){ // ESL
					memset(type,0,sizeof(type));
					snprintf(type,sizeof(type),"IoTGW");
					snprintf(senHubExInfo.szHandlerName,sizeof(senHubExInfo.szHandlerName),type);
				}
				else // SenHub
				{
					memset(parentid,0,sizeof(parentid));
					memset(type,0,sizeof(type));
					snprintf(type,sizeof(type),"SenHub");							
					snprintf(senHubExInfo.szHandlerName,sizeof(senHubExInfo.szHandlerName),type);							

					if(jsonde != NULL )
						JSON_Get(jsonde,"[data][susiCommData][parentID]", parentid, sizeof(parentid));

					if(strcasecmp(parentid,"null"))
						snprintf(senHubExInfo.szParentID,sizeof(senHubExInfo.szParentID),parentid);
					else{																
						snprintf(senHubExInfo.szParentID,sizeof(senHubExInfo.szParentID),"");
						senHubInfo.pExtened = NULL;
                                        }
				}	


				Register_SenHub( NULL, 0, &pSenHander, &senHubInfo, NULL );


			 } else if( memcmp(event,"eSenHub_Disconnect",18) == 0 ) {
				//PRINTF("eSenHub_Disconnect data=%s\n",data); 

				JSON_Get(jsonde,"[data][susiCommData][devID]",SenHubUID, MAX_SN_UID);
				GetNewID(SenHubUID, newSenHubUID);
				if( ( index = GetSenHubAgentInfobyUID(&g_SenHubAgentInfo, MAX_SENNODES, newSenHubUID) ) == -1 ) {
					PRINTF(" Can't find SenHub UID in Table =%s\r\n",newSenHubUID );
					//return -1;
                                        goto exit_json;
				}

				pSenHander = &g_SenPluginInfo[index];

				Disconnect_SenHub(pSenHander);

			 } else if(  memcmp(event,"eSenHub_Capability",18) == 0 ) {
				//PRINTF("eSenHub_Capability data=%s\n",data); 

				JSON_Get(jsonde,"[data][agentID]",SenHubUID, MAX_SN_UID);
				GetNewID(SenHubUID, newSenHubUID);

				if( ( index = GetSenHubAgentInfobyUID(&g_SenHubAgentInfo, MAX_SENNODES, newSenHubUID) ) == -1 ) {
					PRINTF(" Can't find SenHub UID in Table =%s\r\n",newSenHubUID );
					//return -1;
                                        goto exit_json;
				}

				pSenHander = &g_SenPluginInfo[index];

				snprintf(jsontag,sizeof(jsontag),"[data][%s]",pSenHander->Name);
				JSON_Get(jsonde,jsontag,tmp, MAX_BUFFER_SIZE);
				snprintf(data,MAX_BUFFER_SIZE,"{\"%s\":%s}", pSenHander->Name, tmp );
				SendInfoSpec_SenHub( data, strlen(data), pSenHander, NULL );

			 }	else if ( memcmp(event,"eConnectivity_Capability",24) == 0 ) {
				//PRINTF("eConnectivity_Capability data=%s\n",data); 
				if( strlen(data) < MAX_BUFFER_SIZE) {
					app_os_EnterCriticalSection(&g_SendIoTGWCriticalSec);

					memset(g_pWAP_IoTGWInfoBuffer,0,MAX_BUFFER_SIZE);
					snprintf(g_pWAP_IoTGWInfoBuffer, MAX_BUFFER_SIZE, "%s", data);
					if( g_sendinfospeccbf ) {
						char* newData = Trans2NewFrom(g_pWAP_IoTGWInfoBuffer);
						if(newData)
						{
							g_sendinfospeccbf(&g_PluginInfo, newData, strlen(newData), NULL, NULL);
							free(newData);
						}
					}
					//g_sendinfospeccbf(&g_PluginInfo, g_pWAP_IoTGWInfoBuffer, strlen(g_pWAP_IoTGWInfoBuffer), NULL, NULL);
					app_os_LeaveCriticalSection(&g_SendIoTGWCriticalSec);
					//SendMsgToSUSIAccess(g_pWAP_IoTGWInfoBuffer, strlen(g_pWAP_IoTGWInfoBuffer), NULL, NULL);
				}else
				  PRINTF("Buffer is overflow\n\n\n\n");

			 } else if ( memcmp(event,"eConnectivity_UpdateData",24) == 0 ) {
				//PRINTF("eConnectivity_UpdateData data=%s\n",data); 
				UpdateInterfaceData( data, strlen(data) );
			 }
exit_json:
			JSON_Destory(&jsonde);				
		}		 

	}

    return 1;
}


// <IoTGW>
int InitSNGWHandler()
{
	int rc = 0;
	int i = 0;
	char libpath[MAX_PATH]={0};

	//snprintf(libpath,sizeof(libpath),"%s/%s",GetLibaryDir(IoTGW_HANDLE_NAME),SN_MANAGER_LIB_NAME);
	snprintf(libpath,sizeof(libpath),"%s",SN_MANAGER_LIB_NAME);

	PRINTF("LibPath=%s\n",libpath);

	// 1. Load Sensor Network Manager Libary  & Get Library function point
	if( GetSNManagerAPILibFn( libpath,&pSNManagerAPI ) == 0 ) {
		PRINTF("Failed: InitSNGWHandler: GetSNManagerAPILibFn\r\n");		
		snprintf(libpath,sizeof(libpath),"%s",W_LIB_NAME);
		if( GetAPILibFn( libpath,&g_pWAPI_Interface ) == 0 ) {
			PRINTF("Failed: InitWHandler: GetWAPILibFn\r\n");
			return rc;
		} else{
			PRINTF("Load WAPI Success!\r\n");
		}			
		g_bAPIVer = 2;	
			
	}else
		PRINTF("Load WSN C API Success =%d  psn=%d!\r\n",g_pWAPI_Interface, pSNManagerAPI);

	// 2. Allocate Handler_info array buffer point
	for( i = 0; i< MAX_SENNODES ; i ++ ) {
		memcpy(&g_SenPluginInfo[i],&g_PluginInfo,sizeof(Handler_info));
		memcpy(&g_SenHubAgentInfo[i],&g_PluginInfo.agentInfo,sizeof(cagent_agent_info_body_t));
		g_SenHubAgentInfo[i].status = 0;
		g_SenPluginInfo[i].agentInfo = &g_SenHubAgentInfo[i];
	}

	// Critical
	app_os_InitializeCriticalSection(&g_SenHubSendCriticalSec);
	app_os_InitializeCriticalSection(&g_SendIoTGWCriticalSec);

	if( g_bAPIVer == 1 ) {

		// 3. Set ProcSNManagerCbf
		if( pSNManagerAPI->SN_Manager_ActionProc( SN_Set_ReportSNManagerDataCbf, ProceSNManagerDataCbf,0,0,0) == SN_ER_FAILED ){
			PRINTF("Failed: InitSNGWHandler: SN_Set_ReportSNManagerDataCbf\r\n");
			return rc;
		}

		// 4. SN_Manager_Initialize
		if( pSNManagerAPI->SN_Manager_Initialize( ) == SN_ER_FAILED ) {
			PRINTF("Failed: InitSNGWHandler: SN_Manager_Initialize\r\n");
			return rc;
		}

		pSNManagerAPI->SN_Manager_GetCapability( );
		//5. Init AdvApiMux Server
		InitAdvAPIMux_Server();
	} else {   // WAPI
		//void* threadHandler;

		g_pWAPI_Interface->AdvWAPI_SetCallback(&Proce_WebApi_Cb);

    	if( g_pWAPI_Interface->AdvWAPI_Initialize() != WAPI_CODE_OK )
        	return rc;	

        if( g_pWAP_IoTGWInfoBuffer == NULL ) {
			g_pWAP_IoTGWInfoBuffer = (char *)malloc( MAX_DATA_SIZE + 1);
			memset(g_pWAP_IoTGWInfoBuffer, 0, MAX_DATA_SIZE + 1);
		}			

		if( g_pWAP_IoTGWInfoBuffer == NULL ) {
			PRINTF("Can not alloc engiore memory for IoTGW Capability");
			return rc;
		}

		snprintf(g_pWAP_IoTGWInfoBuffer, MAX_DATA_SIZE, "%s", g_szErrorCapability);		

	}
	rc = 1; 
	PRINTF("Success: Initial Sensor Network Handler\r\n");


	return rc;
}

int StartSNGWHandler()
{
	int rc = 0;
    void* threadHandler;
	
	if( g_bAPIVer == 2 ) {
	        if (app_os_thread_create(&threadHandler, ThreadSynWSNInfo, NULL) == 0) {
				g_HandlerContex.threadHandler = threadHandler;
                //app_os_thread_detach(threadHandler);
			}
	}

	return rc;
}

int StopSNGWHandler()
{
	int rc = 0;

	while( runing )
		TaskSleep(100);

	return rc;
}

void UnInitSNGWHandler()
{
	// 1. Send Disconnect Msg
	
	if( g_bAPIVer == 1 ) {	
		if(pSNManagerAPI)
			pSNManagerAPI->SN_Manager_Uninitialize( );

		UnInitAdvAPIMux_Server();
	} else {
		if(g_pWAPI_Interface) {
			g_pWAPI_Interface->AdvWAPI_Leave_Service( g_hWebSocket );
			g_pWAPI_Interface->AdvWAPI_UnInitialize();
		}
		if( g_pWAP_IoTGWInfoBuffer ) {
			free(g_pWAP_IoTGWInfoBuffer);
			g_pWAP_IoTGWInfoBuffer = NULL;
		}
	}
	app_os_DeleteCriticalSection(&g_SenHubSendCriticalSec);
	app_os_DeleteCriticalSection(&g_SendIoTGWCriticalSec);

}


//-----------------------------------------------------------------------------
// UTIL Function:
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
BOOL WINAPI DllMain( HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved )
{
	if ( reason_for_call == DLL_PROCESS_ATTACH ) // Self-explanatory
	{
		printf( "DllInitializer\n" );
		DisableThreadLibraryCalls( module_handle ); // Disable DllMain calls for DLL_THREAD_*
		if ( reserved == NULL ) // Dynamic load
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

	if ( reason_for_call == DLL_PROCESS_DETACH ) // Self-explanatory
	{
		printf( "DllFinalizer\n" );
		if ( reserved == NULL ) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize( );
		}
		else // Process is terminating
		{
			// Cleanup
			Handler_Uninitialize( );
		}
	}
	return TRUE;
}
#else
__attribute__( ( constructor ) )
/**
 * initializer of the shared lib.
 */
static void Initializer( int argc, char** argv, char** envp )
{
    fprintf(stderr, "[IOTGW] DllInitializer\n" );
}

__attribute__( ( destructor ) )
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer( )
{
    fprintf(stderr, "[IOTGW] DllFinalizer\n" );
	Handler_Uninitialize( );
}
#endif






