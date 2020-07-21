#include <stdio.h>
#include <string.h>   //strncpy

#include "AdvElsAPI.h"
#include "AdvWebClientAPI.h"

// Structure
typedef struct ElsServerInfo_
{
	char szAddress[256];
	int    nPort;
	int    nServerAlive; // 0: unavailable, 1: available
	int    nStopService;
} ElsServerInfo, *ElsServerInfo_Ptr;


#define INIT       0
#define RUNNING    1
#define STOPPING   2
#define LEAVED        3
#define CHECK_INTERVAL 2000 // 2 sec
#define MAX_LEAVE_TIMEOUT  CHECK_INTERVAL*2 // max waiting time
#define RETRY_SERVER_UNAVAILABLE 2

// Global variable
int           gInitELS = INIT;
ElsServerInfo gElsServerInfo;




static void *pthread_checkalive(void *in)
{
    pthread_detach(pthread_self());
    ElsServerInfo *pElsInfo = (ElsServerInfo*) in;
	int ret = 0;
	char buffer[1024]={0};
	int retry = 0;
    while( pElsInfo->nStopService == RUNNING ) {

			ret = g_pWAPI_Interface->WAPI_Get( (const char*)pElsInfo->szAddress, pElsInfo->nPort, (const char*)"", buffer, 1024, (char*)"",  1 /*sec*/);
			if( ret == WAPI_CODE_OK ) {
				retry = 0;
				pElsInfo->nServerAlive = 1;
			} else {
				++retry;
				if( retry == RETRY_SERVER_UNAVAILABLE )
					pElsInfo->nServerAlive = 0;
			}

			TaskSleep(CHECK_INTERVAL);
    }
	pElsInfo->nStopService = LEAVED;


    printf("levae websocket thread routine\n");
	return 0;
}

 ELSAPI_CODE  AdvEls_Init( const char* address, int port )
 {
	ELSAPI_CODE rc = ELS_CODE_FAILED;
	
	if( gInitELS == RUNNING ) {
		rc = ELS_CODE_OK;
		 return rc;		
	}
	memset(&gElsServerInfo,0,sizeof(ElsServerInfo));

		
	// Load WAPI.dll & Check does support "WAPI_Post" API ?
	if( GetAPILibFn( W_LIB_NAME, (void**)&g_pWAPI_Interface ) == 0 ) {
		printf("Failed: InitWHandler: GetWAPILibFn\r\n");
		return rc;
	}

    if( g_pWAPI_Interface->AdvWAPI_Initialize() != WAPI_CODE_OK )
        return rc;

	if( strlen(address) <=0 || port < 1 || port > 65535 ) 
		return rc;

	sprintf(gElsServerInfo.szAddress,"%s",address);
	gElsServerInfo.nPort = port;
	gElsServerInfo.nStopService = RUNNING;

	// create a thread to check is Elastic Server is alive
    pthread_t pid;
    pthread_create(&pid, NULL, pthread_checkalive, &gElsServerInfo);

	rc = ELS_CODE_OK;
	gInitELS = RUNNING;
	return rc;
	
 }

 void AdvEls_Uninit()
 {
	 int count = 0;
	 
     if( gInitELS != RUNNING ) return;
     
	 // Stop check server is alive thread
	 gElsServerInfo.nStopService = STOPPING;
	
	 while(gElsServerInfo.nStopService != LEAVED || count >  MAX_LEAVE_TIMEOUT /*sec*/) {
		 ++count;
		TaskSleep(100); // 100 ms
	 }
	 // To release resource
	 if( g_pWAPI_Interface != NULL )
		g_pWAPI_Interface->AdvWAPI_UnInitialize();
 }

 // uri must be lowercase
  ELSAPI_CODE AdvEls_Write( const char *uri, const char* Content, char *Buffer, int BufSize )
  {
    ELSAPI_CODE rc = ELS_CODE_FAILED;
	int i = 0;
	int ret = WAPI_CODE_FAILED;
	char uri_lowercase[1024] ="";

	  if( gElsServerInfo.nServerAlive == 0 || gElsServerInfo.nStopService != RUNNING ) {
		  rc = ELS_CODE_SERVICE_UNABAILABLE;
		  return rc;
	  }

	  if( uri == NULL || strlen(uri) <= 0 ) // logContent ?
		return rc;

	  // Do we need to check the uri vaild

	strcpy(uri_lowercase, uri);
	ret = strlen(uri_lowercase);

    // Elasticsearch's index supports lowercase only: Do we need to check ( performance issue ? )
	for ( i=0; i<=ret; i++)
		uri_lowercase[i] = tolower(uri_lowercase[i]);

	// Add log
	 if( g_pWAPI_Interface != NULL )
		ret = g_pWAPI_Interface->WAPI_Post((const char*)gElsServerInfo.szAddress, gElsServerInfo.nPort, (const char*)uri_lowercase, (const char*)Content, strlen(Content), Buffer, BufSize, (char*)"", 1 );

	if( ret == WAPI_CODE_OK )
		rc = ELS_CODE_OK;

    return rc;
  }

