#include "ServiceHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "WISEPlatform.h"
#include "sys/time.h"
#include "pthread.h"
#include "cJSON.h"
#include "Service_API.h"

//-----------------------------------------------------------------------------
// Logger defines:
//-----------------------------------------------------------------------------
#define LOG_ENABLE

#define DEF_HANDLER_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

#ifdef HANDLER_LOG_ENABLE
#define Handler_Log(level, fmt, ...)  do { if (g_handler_log != NULL)   \
    WriteLog(g_handler_log, DEF_HANDLER_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define Handler_Log(level, fmt, ...)
#endif


//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct{
	Handler_info_ex *pluginfo_ex;
}handler_context_t;

typedef struct{
	char Name[SERVICE_NAME];
}ServiceArray;


static char* g_szAutoReportInfo = NULL;
static int g_iAutoReportInfoLength = 0;

int BroadcaseAction(char *pInBuffer, void* pUserdata );
int GetTotalServiceName(  ServiceArray *pOutService, int size );

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static Handler_info  g_HandlerInfo;
static handler_context_t g_HandlerContex;
static HANDLER_THREAD_STATUS g_status = handler_status_no_init;
static time_t g_monitortime;
static HandlerSendCbf g_sendcbf = NULL;                        // Client Send information (in JSON format) to Cloud Server
static HandlerSendCustCbf  g_sendcustcbf = NULL;                // Client Send information (in JSON format) to Cloud Server with custom topic
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerAutoReportCbf g_sendreportcbf = NULL;             // Client Send report (in JSON format) to Cloud Server with AutoReport topic
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;
static HandlerSendEventCbf g_sendeventcbf = NULL;

Handler_info_ex *pluginfo_ex;

LOGHANDLE g_handler_log = NULL;
//pthread_t g_reconnThreadHandler = 0;

static char init_capability[MAX_BUF] = {0};

//ServiceHandler* g_service = NULL;

void Handler_Uninitialize( );
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
    fprintf(stderr, "DllInitializer\n" );
}

__attribute__( ( destructor ) )
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer( )
{
    fprintf(stderr, "DllFinalizer\n" );
	Handler_Uninitialize( );
}
#endif

#ifdef _C_PLUS_PLUS
ServiceHandler::ServiceHandler(HANDLER_INFO* pHandler)
{
	g_pHandler = new PluginInfo();
	memcpy(&g_pHandler->plugin, pHandler, sizeof(HANDLER_INFO));
	
}

ServiceHandler::~ServiceHandler(void)
{
	if(g_pHandler != NULL)
		free(g_pHandler);

	g_pHandler = NULL;
}

void ServiceHandler::ServiceStart()
{
}

void ServiceHandler::ServiceStop()
{
}

void ServiceHandler::ServiceRecv(std::string payload)
{
	/*Parse payload, If has session ID then add to message queue*/
}

bool ServiceHandler::ReplaceDevID(std::string inBuf, std::string outBuf)
{
	return true;
}
#endif

long long GetTimeTick() 
{ 
        long long tick = 0; 
        struct timeval tv; 
        gettimeofday(&tv, NULL); 
        tick = (long long)tv.tv_sec*1000 + (long long)tv.tv_usec/1000; 
        return tick; 
}


// [susiCommData][infoSpec]
int GetInfoSpec(cJSON *root, const char *inServiceName, char * outData, int size )
{
	cJSON *susiCmd = NULL;
	cJSON *infoSpec = NULL;
	cJSON *service = NULL;
	int len = 0;

	if( root == NULL ) return len;

    susiCmd = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);     // susiCommData
    if(susiCmd) {
		infoSpec = cJSON_GetObjectItem(susiCmd, AGENTINFO_INFOSPEC);  // infoSpec
		if( infoSpec ) {
			char* buff = cJSON_PrintUnformatted( infoSpec );
			snprintf( outData, size, "%s",  buff);
			free(buff);
			len = strlen(outData);
		}
	}
	return len;
}

// [susiCommData][data]
int GetUpdateData(cJSON *root, const char *inServiceName,char * outData, int size )
{
	cJSON *susiCmd = NULL;
	cJSON *data = NULL;
	cJSON *service = NULL;
	int len = 0;

	if( root == NULL ) return len;

    susiCmd = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);     // susiCommData
    if(susiCmd) {
		data = cJSON_GetObjectItem(susiCmd, AGENTINFO_DATA);  // data
		if( data ) {
			char *buff = cJSON_PrintUnformatted( data );
			snprintf( outData, size, "%s",  buff);
			free(buff);
			len = strlen(outData);
		}
	}
	return len;
}


int GetEventData(cJSON *root, HANDLER_NOTIFY_SEVERITY *sev, char * outData, int size )
{
	cJSON *susiCmd = NULL;
	cJSON *eventnotify = NULL;
	cJSON *severity = NULL;
	int len = 0;

	if( root == NULL ) return len;


    susiCmd = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);     // susiCommData
    if(susiCmd) {
		eventnotify = cJSON_GetObjectItem(susiCmd, AGENT_EVNET);  // data
		if( eventnotify ) {
			char* buff = cJSON_PrintUnformatted(eventnotify);
			//snprintf( outData, size, "%s",  cJSON_PrintUnformatted(eventnotify) );
			snprintf( outData, size, "%s",  buff );
			free(buff);
			severity = cJSON_GetObjectItem(eventnotify, "severity");  // severity
			if( severity ) {
				*sev  =  (HANDLER_NOTIFY_SEVERITY) severity->valueint;
				len = strlen(outData);
			}
		}
	}

	return len;
}

// This parse only support 1.Get Capability, 2. Get / Set Sensor, 3. AutoReport replys's Result
/*
{
	"commCmd":526,
	"handlerName":"SenHub",
	"sessionID":"XXXXXWWOOWQQAQ",
	"sensorInfoList":{"e":[{}]}
}
*/
int GetReplyData( cJSON *root, int *cmd, char *outData, int size)
{
	cJSON *susiCmd = NULL;
	cJSON *handlerName = NULL;
	cJSON *commCmd = NULL;
	cJSON *sessionID = NULL;
	cJSON *capability = NULL;
	cJSON *sensorInfoList = NULL;
	
	int len = 0;

	if( root == NULL ) return len;

    susiCmd = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);     // susiCommData
    if(susiCmd) {
		handlerName = cJSON_GetObjectItem(susiCmd, "handlerName");  // handlerName
		commCmd = cJSON_GetObjectItem(susiCmd, AGENTINFO_CMDTYPE);  // commCmd
		sessionID = cJSON_GetObjectItem(susiCmd, AGENTINFO_SESSION_ID);  // sessionID
		sensorInfoList = cJSON_GetObjectItem(susiCmd, "sensorInfoList");  // sessionID
		if( commCmd )
			*cmd = commCmd->valueint;

		//Get/ Set Sensor Reply
		if( sessionID ) {
			char* buff = NULL;
			if( sensorInfoList )
			{
				buff = cJSON_Print(sensorInfoList);
				snprintf(outData, size, GET_SET_REPLY_FORMAT, sessionID->valuestring, buff);  //  	extra param: "commCmd":526, "handlerName":"SenHub",
			}
			else
			{
				buff = cJSON_Print(susiCmd);
				snprintf(outData, size, "%s", buff);  //  	extra param: "commCmd":526, "handlerName":"SenHub",
			}
			free(buff);
			len = strlen(outData);
		} 
		else if (( *cmd == 522 || *cmd == 534) && handlerName ) // reply capability id
		{
			capability = cJSON_GetObjectItem(susiCmd, handlerName->valuestring);  // handlerName

			if( capability )
			{
				char* buff = cJSON_Print(capability);
				snprintf(outData, size, "{\"%s\":%s}", handlerName->valuestring, buff);  // {"SenHub":{xxxx}}
				free(buff);
				len = strlen(outData);
			}
		} // End of if *cmd
		else // general reply
		{
			capability = cJSON_GetObjectItem(susiCmd, handlerName->valuestring);  // handlerName

			if( capability )
			{
				char* buff = cJSON_Print(capability);
				strcpy(outData, buff);
				free(buff);
				len = strlen(outData);
			}
		}
	} // End of susiCmd

	return len;
}


//-----------------------------------------------------------------------------
// Internal Function:
//-----------------------------------------------------------------------------
static SV_CODE SVCALL ProcService_Cb( SV_EVENT e, char *ServiceName, void *inData, int dataLen, void *pUserData )
{
	SV_CODE rc = SV_ER_FAILED;
	static char data[MAX_BUF]={0};
	static int size = MAX_BUF;
	handler_context_t  *phContex = ( handler_context_t *) pUserData;
	cJSON* root = NULL;
	int cmd = 522;
	Handler_info local_HandlerInfo;
	Handler_Log(Debug, "%s Event =%d   ServiceName=%s UserData=%d\n", HANDLER_NAME, e, ServiceName,  pUserData );

	int len = 0;

	//if( phContex == NULL ) return rc;

	if( dataLen > 0 )
		root = cJSON_Parse((char*)inData); // remain the cJSON root(pReqRoot), which is to be ergodic and will be free in end.

	if (!root)  {
		Handler_Log(Debug, "ProcService_Cb JSON Parse Error!\n");
		goto exit_proc;
	}

	switch( e )
	{
	case SV_E_JoinServiceSystem:
		Handler_Log(Debug, "Join Edge Sense System\n");
		break;	
	case SV_E_RegisterService:
	case SV_E_UpdateServiceCapability:
		{
			// send capability
			memset(data, 0, size);
			len = GetInfoSpec( root, ServiceName, data, sizeof(data));
			if( len > 0 ) {				
				if( e == SV_E_RegisterService ) {
					pluginfo_ex->addvirtualhandlercbf(ServiceName, pluginfo_ex->Name );
					if(g_szAutoReportInfo)
						pSVAPI_Interface->SV_AutoReportStart(ServiceName, g_szAutoReportInfo, g_iAutoReportInfoLength);
				}
	
				g_sendcapabilitycbf(&g_HandlerInfo, data, len, NULL, NULL);
			}
		}
		break;
	case SV_E_DeregisterService:
		{
			// XXX : WISE-PaaS Backend doesn't support remove Plug-in
		}
		break;
	case SV_E_UpdateData:
		{
			memset(data, 0, size);
			len = GetUpdateData( root, ServiceName, data, sizeof(data));
			if( len > 0 )
				g_sendreportcbf(&g_HandlerInfo, data, len, NULL, NULL);
		}
		break;
	case SV_E_ActionResult:
		{
			memcpy(&local_HandlerInfo, &g_HandlerInfo, sizeof(Handler_info));
			strncpy(local_HandlerInfo.Name, ServiceName, sizeof(local_HandlerInfo.Name));

			// requestID(i), commCmd(i), catalogID(i), handerName(s), result(s), sessionID(i), StatusCode(i)
			// Threadhold: setThrRep(j), thrCheckStatus(j), thrCheckMsg(j), delAllThrRep(s)
			// infoSpec(j)
			memset(data, 0, size);
			// sensorIDList(j)
			len = GetReplyData( root, &cmd, data, size );

			if( len > 0 )
			{
				//g_sendcbf(&g_HandlerInfo, cmd, data,  len +1, NULL, NULL );
				g_sendcbf(&local_HandlerInfo, cmd, data,  len +1, NULL, NULL );
			}
			else
			{
				cJSON* body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
				if(body)
				{
					cJSON* content = NULL;
					cJSON* target = body->child;
					while (target)
					{
						if(!strcmp(target->string, AGENTINFO_CMDTYPE))
							cmd = target->valueint;
						else if(!strcmp(target->string, AGENTINFO_REQID))
							local_HandlerInfo.ActionID = target->valueint;
						else if(!strcmp(target->string, AGENTINFO_AGENTID))
						{
						}
						else if(!strcmp(target->string, AGENTINFO_HANDLERNAME))
							strncpy(local_HandlerInfo.Name, target->valuestring, sizeof(local_HandlerInfo.Name));
						else if(!strcmp(target->string, AGENTINFO_CATALOG))
						{

						}
						else if(!strcmp(target->string, AGENTINFO_TIMESTAMP)) 
						{
						}
						else
						{
							if(!content)
								content = cJSON_CreateObject();

							cJSON_AddItemToObject(content, target->string, cJSON_Duplicate(target,true));
						}
						target = target->next;
					}

					if(content)
					{
						char* strcontent = cJSON_PrintUnformatted(content);
						cJSON_Delete(content);
						g_sendcbf(&local_HandlerInfo, cmd, strcontent,  strlen(strcontent)+1, NULL, NULL );
						free(strcontent);
					}
				}
				
			}

		}
		break;
	case SV_E_EventNotify:
		{
			HANDLER_NOTIFY_SEVERITY sev = Severity_Informational;
			memset(data, 0, size);
			len = GetEventData(root, &sev, data, sizeof(data) );
			if( len > 0 )
				g_sendeventcbf( &g_HandlerInfo, sev, data, strlen(data)+1, NULL, NULL );
		}
		break;
	default:
		Handler_Log(Debug, "%s Unknow Event =%d   \n", HANDLER_NAME, e );
	}

	if(root)
		cJSON_Delete(root);

	rc = SV_OK;

exit_proc:
	

	return rc;
}

static char buffer[MAX_BUF]={0};
static char syncdata[MAX_BUF]={0};
static pthread_t threadHandler = NULL;

static void* ThreadSyncService(void* args)
//void ThreadSyncService()
{
	// Get All Service & Send Cabability & data
	ServiceArray Service[MAX_SERVICE];



	cJSON *root = NULL;
	int count = 0, i = 0, len = 0;

	if( pSVAPI_Interface == NULL ) return 0;

	count = GetTotalServiceName(Service, MAX_SERVICE );

	for( i = 0; i < count; i++ ) {
		memset(buffer,0,MAX_BUF);
		memset(syncdata,0,MAX_BUF);
		if( pSVAPI_Interface->SV_GetCapability(Service[i].Name,buffer, MAX_BUF) == SV_OK )
		{
			root = cJSON_Parse(buffer);
			if( root )
			{
				len = GetInfoSpec( root, Service[i].Name, syncdata, sizeof(syncdata));
				if( len > 0 ) {
					pluginfo_ex->addvirtualhandlercbf(Service[i].Name, pluginfo_ex->Name );
					g_sendcapabilitycbf(&g_HandlerInfo, syncdata, len, NULL, NULL);
					g_sendreportcbf(&g_HandlerInfo, syncdata, len, NULL, NULL);
				}
			}
		} // End of SV_GetCapability
	} // End of For

	threadHandler = NULL;
	return 0;

}

int InitServiceSDKHandler()
{
	int rc = 0;
	int i = 0;
	char libpath[DEF_MAX_PATH]={0};

	snprintf(libpath,sizeof(libpath),"%s",SERVIVE_SDK_LIB_NAME);

	if( GetServiceSDKLibFn( libpath,(void**)&pSVAPI_Interface ) == 0 ) {
		Handler_Log(Error, " %s Failed: Get Service Function Point\r\n", HANDLER_NAME);
		return rc;
	}

	// 2. SN_Manager_Initialize
	if( pSVAPI_Interface->SV_Initialize( &ProcService_Cb, &g_HandlerContex ) != SV_OK ) {
		Handler_Log(Error, " %s Failed: to SV_Initialize\r\n", HANDLER_NAME);
		return rc;
	}

	rc = 1;

	Handler_Log(Debug, " %s Success: Initial Service SDK\r\n", HANDLER_NAME);
	return rc;
}


int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	//g_service = new ServiceHandler(handler);

	if( pluginfo == NULL ) {
        return handler_fail;
    }

    pluginfo_ex = (Handler_info_ex*)pluginfo;

    // 1. Topic of this handler
	snprintf(pluginfo->Name, sizeof(pluginfo->Name), "%s", HANDLER_NAME);
    g_handler_log = pluginfo->loghandle;
    Handler_Log(Debug, " %s> Initialize", HANDLER_NAME);
    // 2. Copy agent info
    memcpy(&g_HandlerInfo, pluginfo, sizeof(HANDLER_INFO));
    g_HandlerInfo.agentInfo = pluginfo->agentInfo;

    // 3. Callback function -> Send JSON Data by this callback function
    g_sendcbf = g_HandlerInfo.sendcbf = pluginfo->sendcbf;
    g_sendcustcbf = g_HandlerInfo.sendcustcbf = pluginfo->sendcustcbf;
    g_subscribecustcbf = g_HandlerInfo.subscribecustcbf = pluginfo->subscribecustcbf;
    g_sendreportcbf = g_HandlerInfo.sendreportcbf = pluginfo->sendreportcbf;
    g_sendcapabilitycbf =g_HandlerInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;
    g_sendeventcbf = g_HandlerInfo.sendeventcbf = pluginfo->sendeventcbf;

    //g_HandlerContex.threadHandler = NULL;
    //g_HandlerContex.isThreadRunning = false;
	g_HandlerContex.pluginfo_ex = (Handler_info_ex*)pluginfo;
    g_status = handler_status_no_init;

	//memset(g_szAutoReportInfo, 0, sizeof(g_szAutoReportInfo));

	
	return handler_success;
}

void Handler_Uninitialize( )
{
	if(g_szAutoReportInfo)
		free(g_szAutoReportInfo);
	g_szAutoReportInfo = NULL;
	g_iAutoReportInfoLength = 0;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
 *  Input :  None
 *  Output: char * : pOutStatus       // cagent handler status
 *  Return:  handler_success : Success Init Handler
 *              handler_fail : Fail Init Handler
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
    int iRet = handler_fail;
    //Handler_Log(Debug, " %s> Get Status", HANDLER_NAME);
    if(!pOutStatus) return iRet;

    *pOutStatus = g_status;

    iRet = handler_success;
    return iRet;
}

void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
    Handler_Log(Debug, " %s> Update Status", HANDLER_NAME);

    if(pluginfo) {
        memcpy(&g_HandlerInfo, pluginfo, sizeof(HANDLER_INFO));
    }

	if(pluginfo->agentInfo->status == 1) 
	{
		// Connected to WISE-PaaS RMM Send All Service's Capability and Data again

		if( threadHandler == NULL ) {
			if (pthread_create(&threadHandler, NULL, ThreadSyncService, NULL) == 0)
			{
				pthread_detach(threadHandler);
			}
		}

	}
}

int HANDLER_API Handler_Start( void )
{
    Handler_Log(Debug, "> %s Start", HANDLER_NAME);

	// Init Service SDK
	InitServiceSDKHandler();

    g_status = handler_status_start;

	return handler_success;
}

int HANDLER_API Handler_Stop( void )
{
    Handler_Log(Debug, "> %s Start", HANDLER_NAME);
	if( pSVAPI_Interface )
		pSVAPI_Interface->SV_Uninitialize();

    g_status = handler_status_stop;

	return handler_success;
}

void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
    static const char *str_handler_name     = "handlerName";
	int cmdID = 0;

	if( pSVAPI_Interface == NULL ) return;

	Handler_Log(Debug,"Recv Topic [%s] Data %s\n", topic, (char*) data);

    cJSON *root = cJSON_Parse((char*) data);
    if(root) 
	{
        cJSON *json_obj = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
        if(json_obj) {
            cJSON *handler = cJSON_GetObjectItem(json_obj, str_handler_name);      // receiver handler or virtual handler name
			cJSON *cmd = cJSON_GetObjectItem(json_obj, AGENTINFO_CMDTYPE); // cmd 
            if(handler) {
	               Handler_Log(Debug, "Recv to handler: %s\n", handler->valuestring);
				
				if( cmd )
					cmdID = cmd->valueint;

				if(strcmp(handler->valuestring, HANDLER_NAME)) // Only for Service Plugin
					if( strcmp(handler->valuestring,"general") == 0 )
						BroadcaseAction( (char *) data, NULL );
					else if( cmdID == 533 ) // Auto Start
						pSVAPI_Interface->SV_AutoReportStart(handler->valuestring, (char*)data, strlen((char*)data));
					else
					   pSVAPI_Interface->SV_Action(handler->valuestring, (char *) data, NULL );
             }
        }
        cJSON_Delete(root);
    }
}

int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
#if 1
	int size = 2048;
	char* buffer = (char*)calloc(1, size);
	if( pSVAPI_Interface == NULL ) return 0;
	/* { "Service":{"e":[{"n":"HDD_PMQ"},{"n":"Modebus"}] } */
	while(SV_OK != pSVAPI_Interface->SV_Query_Service(buffer, size))
	{
		size = size*2;
		free(buffer);
		buffer = (char*)calloc(1, size);
	}
	
	if(strlen(buffer) >0 )
	{
		cJSON* root = cJSON_Parse(buffer);
		free(buffer);
		if(root!= NULL)
		{
			cJSON* service = cJSON_GetObjectItem(root, "Service");
			if(service!= NULL)
			{
				cJSON* nodes = cJSON_GetObjectItem(service, "e");
				int size = cJSON_GetArraySize(nodes);
				cJSON* node = NULL;
				cJSON* name = NULL;
				int i=0;
				for(i=0; i<size; i++)
				{
					node = cJSON_GetArrayItem(nodes, i);
					if(node != NULL)
					{
						name = cJSON_GetObjectItem(node,"n");
						if(name != NULL && name->type == cJSON_String)
							pSVAPI_Interface->SV_ReSyncData(name->valuestring);
					}
				}
			}
			cJSON_Delete(root);
		}
	}
	return 0;
#else
    sprintf(init_capability, "{\"%s\":{\"plugin-init\":{\"bn\":\"handler init\",\"info\":{\"e\":[{\"n\":\"is_startup\",\"bv\":1}],\"bn\":\"info\"}}}}", HANDLER_NAME);
    int len_capability = strlen(init_capability);
    g_sendcapabilitycbf(&g_HandlerInfo, init_capability, len_capability, NULL, NULL);
    g_sendreportcbf(&g_HandlerInfo, init_capability, len_capability, NULL, NULL);

    *pOutReply = (char *)malloc(len_capability + 1);
    sprintf(*pOutReply, init_capability);

    return len_capability;
#endif
}

void HANDLER_API Handler_MemoryFree(char *pInData)
{

}




// {"Service":{"e":[{"n":"HDD_PMQ},{"n":"Modebus"}]}}
int GetTotalServiceName(  ServiceArray *pOutService, int size )
{
	int i=0, count = 0;
	char Data[TEMP_BUF]={0};
	cJSON* root = NULL;	
	cJSON* body = NULL;
	cJSON* pElement = NULL;
	cJSON* pSubItem = NULL;
	cJSON* jsName = NULL;
	if( pSVAPI_Interface == NULL ) return count;
	if(pSVAPI_Interface->SV_GetServiceStatus() == SV_JOINED ) 
	{
		if( pSVAPI_Interface->SV_Query_Service(Data, sizeof(Data)) != SV_OK ) 
			return count;

		root = cJSON_Parse((char *)Data);
		if(!root) { 
			Handler_Log(Debug, "Parse Error\n");
			return count;
		}

		body = cJSON_GetObjectItem( root, DEF_SERVICE_NAME ); // "Service"
		if( !body ) {
			cJSON_Delete(root);
			return count;
		}
		pElement = cJSON_GetObjectItem( body, SENML_E_FLAG ); // "e"

		if( !pElement ) {
			cJSON_Delete(root);
			return count;
		}
		count = cJSON_GetArraySize(pElement);
		for ( i = 0; i < count; i++ )
		{
			pSubItem = cJSON_GetArrayItem(pElement, i);
			if( pSubItem == NULL) break;
			jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG );                       // n
			snprintf(pOutService->Name,SERVICE_NAME,"%s",jsName->valuestring ); // => HDD_PMQ
			++pOutService;
		}
	}

	cJSON_Delete(root);

	return count;
}

int BroadcaseAction(char *pInBuffer, void* pUserdata )
{
	int i = 0, count = 0, rc=0;
	ServiceArray Service[MAX_SERVICE];

	if( pSVAPI_Interface == NULL ) return count;

	count = GetTotalServiceName(Service, MAX_SERVICE );
	for ( i = 0; i< count ; i ++ )
		pSVAPI_Interface->SV_Action(Service[i].Name, pInBuffer,pUserdata);
	return count;
}


void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	int i = 0, count = 0;
	int length = 0;
	ServiceArray Service[MAX_SERVICE];

	if( pSVAPI_Interface == NULL ) return;
	if( pInQuery == NULL ) return;
	length = strlen(pInQuery);

	if(length > g_iAutoReportInfoLength)
	{
		if(g_szAutoReportInfo)
			free(g_szAutoReportInfo);
		g_szAutoReportInfo = (char*)calloc(1, length+1);
		g_iAutoReportInfoLength = length;
	} else {
		memset(g_szAutoReportInfo, 0, g_iAutoReportInfoLength);
	}
	count = GetTotalServiceName(Service, MAX_SERVICE );

	strcpy(g_szAutoReportInfo, pInQuery);

	for ( i = 0; i< count ; i ++ )
		pSVAPI_Interface->SV_AutoReportStart(Service[i].Name, pInQuery, strlen(pInQuery));
}

void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	int i = 0, count = 0;
	ServiceArray Service[MAX_SERVICE];

	if( pSVAPI_Interface == NULL ) return;

	count = GetTotalServiceName(Service, MAX_SERVICE );

	if(g_szAutoReportInfo)
			free(g_szAutoReportInfo);
	g_szAutoReportInfo = NULL;
	g_iAutoReportInfoLength = 0;

	for ( i = 0; i< count ; i ++ )
		pSVAPI_Interface->SV_AutoReportStop(Service[i].Name, pInQuery, strlen(pInQuery));
}
