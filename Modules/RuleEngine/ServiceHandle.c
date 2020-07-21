#include "ServiceHandle.h"
#include "Service_API.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "RuleEngineLog.h"
#include "srp/susiaccess_def.h"
#include "cJSON.h"

#if defined(WIN32) //windows
#define SERVIVE_SDK_LIB_NAME "EIServiceSDK_RuleEngine.dll"
#elif defined(__linux)//linux
#define SERVIVE_SDK_LIB_NAME "libEIServiceSDK_RuleEngine.so"
#endif

#define BASICINFO_BODY_STRUCT	"susiCommData"
#define BASICINFO_HANDLERNAME	"handlerName"
#define BASICINFO_CMDTYPE		"commCmd"
#define BASICINFO_AGENTID		"agentID"
#define BASICINFO_TIMESTAMP		"sendTS"
#define BASICINFO_CONTENT		"content"
#define AGENTINFO_INFOSPEC		"infoSpec"
#define AGENTINFO_DATA			"data"

SVAPI_Interface *g_pSVAPI_Interface = NULL ;

ServiceSDK_RecvCapability_Cb g_cb_recv_capability = NULL;
ServiceSDK_RecvData_Cb g_cb_recv_data = NULL;
pthread_t g_ServiceSDKThread = 0;
bool g_isThreadRunning = false;

void ServiceSDKRecvCapability(char *ServiceName, void *inData, int dataLen)
{
	/*TODO extract the capability body inside of "content" or "infospec" for RMM 3.3*/
	cJSON *body = NULL, *target = NULL, *content = NULL;
	cJSON *root = cJSON_Parse((char*) inData);
	bool bValid = true;
	if(root == NULL)
		return;

	body = cJSON_GetObjectItem(root, BASICINFO_BODY_STRUCT);
	if(!body)
	{
		body = root;
	}

	target = body->child;
	while (target)
	{
		if(!strcmp(target->string, BASICINFO_CMDTYPE))
		{
			if(target->valueint != wise_get_capability_rep)
				bValid = false;
		}
		else if(!strcmp(target->string, BASICINFO_AGENTID))
		{
			
		}
		else if(!strcmp(target->string, BASICINFO_HANDLERNAME))
		{
			
		}
		else if(!strcmp(target->string, BASICINFO_CONTENT))
		{
			if(!content)
				cJSON_Delete(content);
	
			{/*not verified*/
				cJSON* child = target->child;
				content = cJSON_CreateObject();
				while(child)
				{
					cJSON_AddItemToObject(content, child->string, cJSON_Duplicate(child,true));
					child = child->next;
				}
			}
		}
		else if(!strcmp(target->string, AGENTINFO_INFOSPEC))
		{
			if(!content)
				cJSON_Delete(content);

			content = cJSON_Duplicate(target,true);
		}
		target = target->next;
	}

	if(content)
	{
		char* strcontent = cJSON_PrintUnformatted(content);
		cJSON_Delete(content);
		if(g_cb_recv_capability)
			g_cb_recv_capability(NULL, strcontent, strlen(strcontent));
		free(strcontent);
	}

	cJSON_Delete(root);
}

void ServiceSDKRecvData(char *ServiceName, void *inData, int dataLen)
{
	/*TODO extract the capability body inside of "content" or "infospec" for RMM 3.3*/
	cJSON *body = NULL, *target = NULL, *content = NULL;
	cJSON *root = cJSON_Parse((char*) inData);
	bool bValid = true;
	if(root == NULL)
		return;

	body = cJSON_GetObjectItem(root, BASICINFO_BODY_STRUCT);
	if(!body)
	{
		body = root;
	}

	target = body->child;
	while (target)
	{
		if(!strcmp(target->string, BASICINFO_CMDTYPE))
		{
			if(target->valueint != wise_report_data_rep)
				bValid = false;
		}
		else if(!strcmp(target->string, BASICINFO_AGENTID))
		{
			
		}
		else if(!strcmp(target->string, BASICINFO_HANDLERNAME))
		{
			
		}
		else if(!strcmp(target->string, BASICINFO_CONTENT))
		{
			if(!content)
				cJSON_Delete(content);
	
			{/*not verified*/
				cJSON* child = target->child;
				content = cJSON_CreateObject();
				while(child)
				{
					cJSON_AddItemToObject(content, child->string, cJSON_Duplicate(child,true));
					child = child->next;
				}
			}
		}
		else if(!strcmp(target->string, AGENTINFO_DATA))
		{
			if(!content)
				cJSON_Delete(content);

			content = cJSON_Duplicate(target,true);
		}
		target = target->next;
	}

	if(content)
	{
		char* strcontent = cJSON_PrintUnformatted(content);
		cJSON_Delete(content);
		if(g_cb_recv_data)
			g_cb_recv_data(NULL, strcontent, strlen(strcontent));
		free(strcontent);
	}

	cJSON_Delete(root);
}

static SV_CODE SVCALL ProcService_Cb( SV_EVENT e, char *ServiceName, void *inData, int dataLen, void *pUserData )
{
	SV_CODE rc = SV_OK;

	char eventType[32]={0};

	switch(e)
	{
	case SV_E_LeaveServiceSystem:
			sprintf(eventType,"Leave");
		break;
	case SV_E_JoinServiceSystem:
			sprintf(eventType,"Join");
		break;
	case SV_E_RegisterService:
			sprintf(eventType,"Register");
		break;
	case SV_E_DeregisterService:
			sprintf(eventType,"Deregister");
		break;
	case SV_E_UpdateServiceCapability:
			sprintf(eventType,"UpdateCapability");
			ServiceSDKRecvCapability(ServiceName, inData, dataLen);
		break;
	case SV_E_UpdateData:
			sprintf(eventType,"UpdateData");
			ServiceSDKRecvData(ServiceName, inData, dataLen);
		break;
	case SV_E_ActionResult:
			sprintf(eventType,"ActionResult");
		break;
	case SV_E_EventNotify:
			sprintf(eventType,"EventNotify");
		break;
	}
	RuleEngineLog(Debug, "RuleEngine-ServiceSDK> Event= %s  ServiceName= %s  Data=(\n%s\n) Len= %d UserData= %d", eventType, ServiceName,  (char*)inData, dataLen, pUserData );

	return rc;
}

void ServiceSDKResync()
{
	int size = 2048;
	char* buffer = (char*)calloc(1, size);
	/* { "Service":{"e":[{"n":"HDD_PMQ"},{"n":"Modebus"}] } */
	while(SV_OK != g_pSVAPI_Interface->SV_Query_Service(buffer, size))
	{
		size = size*2;
		free(buffer);
		buffer = (char*)calloc(1, size);
	}
	RuleEngineLog(Debug, "RuleEngine-ServiceSDK> All of Service Name:   %s\n",  buffer );
	
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
							g_pSVAPI_Interface->SV_ReSyncData(name->valuestring);
					}
				}
			}
			cJSON_Delete(root);
		}
	}
}

void* ServiceSDKThread(void* args)
{
	/*char libpath[256]={0};
	sprintf(libpath,"%s",SERVIVE_SDK_LIB_NAME);
	if( GetServiceSDKLibFn( libpath,&g_pSVAPI_Interface ) == 0 ) {
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Failed: Get Service Function Point error");
		pthread_exit(0);
		return 0;
	}*/
	if(g_pSVAPI_Interface == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> ServiceSDK have not initialized yet!");
		goto STOP_SERVICESDK;
	}

	/*if( g_pSVAPI_Interface->SV_Initialize( ProcService_Cb, NULL ) != SV_OK ) {
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Failed: to SV_Initialize");
		goto STOP_SERVICESDK;
	}*/
	RuleEngineLog(Normal, "RuleEngine-ServiceSDK> Success: Initialize Service SDK");
	g_isThreadRunning = true;

	if(g_pSVAPI_Interface->SV_GetVersion)
	{
		char version[32] = {0};
		if ( g_pSVAPI_Interface->SV_GetVersion(version, sizeof(version)) == SV_OK )
		{
			RuleEngineLog(Debug, "RuleEngine-ServiceSDK> Service SDK Version: %s",  version );
		}
	}

	ServiceSDKResync();

	while(g_isThreadRunning)
	{
		usleep(1000000);
	}
	
STOP_SERVICESDK:
	if(g_pSVAPI_Interface && g_pSVAPI_Interface->SV_Uninitialize != NULL)
	{
		g_pSVAPI_Interface->SV_Uninitialize();
		RuleEngineLog(Debug, "RuleEngine-ServiceSDK> Uninitialize Service SDK" );
	}
	else
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Access Failed on SV_Uninitialize API!");
	}

	pthread_exit(0);
	return 0;
}

int InitServiceSDKHandler(ServiceSDK_RecvCapability_Cb recv_capability, ServiceSDK_RecvData_Cb recv_data, void* loghandle)
{
	int rc = 0;
	g_cb_recv_capability = recv_capability;
	g_cb_recv_data = recv_data;
	g_ruleenginehandlerlog = loghandle;
	if(g_ServiceSDKThread == 0)
	{
		char libpath[256]={0};
		sprintf(libpath,"%s",SERVIVE_SDK_LIB_NAME);
		if(!util_is_file_exist(libpath))
		{
			RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Failed: File %s not exist!", libpath);
			g_pSVAPI_Interface = NULL;
			return rc;
		}
		if( GetServiceSDKLibFn( libpath,&g_pSVAPI_Interface ) == 0 ) 
		{
			RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Failed: Get Service Function Point error");
			g_pSVAPI_Interface = NULL;
			return rc;
		}

		if( g_pSVAPI_Interface->SV_Initialize( ProcService_Cb, NULL ) != SV_OK ) 
		{
			RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Failed: to SV_Initialize");
			return rc;
		}

		if(pthread_create(&g_ServiceSDKThread, NULL, ServiceSDKThread, NULL) != 0)
		{
			g_isThreadRunning = false;
			RuleEngineLog(Warning, "RuleEngine-ServiceSDK> start ServiceSDK thread failed!");
		}
	}
	rc = 1;
	return rc;
}

void UninitServiceSDKHandler()
{
	g_cb_recv_capability = NULL;
	g_cb_recv_data = NULL;


	if(g_ServiceSDKThread != 0)
	{
		g_isThreadRunning = false;
		pthread_join(g_ServiceSDKThread, NULL);
		g_ServiceSDKThread = 0;
	}
}

int GetServiceSDKVersion(char* szResult, int iResultLength) 
{
	int rc = 0;
	if(g_pSVAPI_Interface == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> ServiceSDK have not initialized yet!");
		return rc;
	}

	if(g_pSVAPI_Interface->SV_GetVersion == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Access Failed on SV_GetVersion API!");
		return rc;
	}
	if ( g_pSVAPI_Interface->SV_GetVersion(szResult, iResultLength) == SV_OK )
	{
		RuleEngineLog(Debug, "RuleEngine-ServiceSDK> Service SDK Version: %s",  szResult );
		rc = 1; 
	}
	else
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Failed to get Service SDK Version!");
	return rc;
}

int GetServiceSDKStatus()
{
	int rc = 0;
	if(g_pSVAPI_Interface == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> ServiceSDK have not initialized yet!");
		return rc;
	}
	if(g_pSVAPI_Interface->SV_GetServiceStatus == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Access Failed on SV_GetServiceStatus!");
	} else {
		rc = g_pSVAPI_Interface->SV_GetServiceStatus( );
		RuleEngineLog(Debug, "RuleEngine-ServiceSDK> Status of Service SDK is %d\n",  rc );
	}
	return rc;
}

int QueryServiceSDKAllservices(char* szResult, int iResultLength)
{
	int rc = 0;
	if(g_pSVAPI_Interface == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> ServiceSDK have not initialized yet!");
		return rc;
	}
	if(g_pSVAPI_Interface->SV_Query_Service == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Access Failed on SV_Query_Service!");
	}

	if ( g_pSVAPI_Interface->SV_Query_Service( szResult, iResultLength ) == SV_OK )
	{
		RuleEngineLog(Debug, "RuleEngine-ServiceSDK> All of Service Name:   %s\n",  szResult );
		rc = 1; 
	}
	else
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Failed to get all of Service Name!");
	return rc;
}

int GetServiceSDKServiceCapability(char* ServiceName, char* szResult, int iResultLength)
{
	int rc = 0;
	if(g_pSVAPI_Interface == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> ServiceSDK have not initialized yet!");
		return rc;
	}
	if(g_pSVAPI_Interface->SV_GetCapability == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Access Failed on SV_GetCapability!");
	}

	if( pSVAPI_Interface->SV_GetCapability( ServiceName, szResult, iResultLength ) == SV_OK )
	{
		RuleEngineLog(Debug, "RuleEngine-ServiceSDK> %s Capability: %s", ServiceName, szResult );
		rc = 1;
	}
	else
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Failed to get Capability of %s!", ServiceName);
	return rc;
}

int SendServiceSDKAction(char* ServiceName, char* action, void* userdata)
{
	int rc = 0;
	if(g_pSVAPI_Interface == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> ServiceSDK have not initialized yet!");
		return rc;
	}
	if(g_pSVAPI_Interface->SV_Action == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Access Failed on SV_Action!");
	}

	if( pSVAPI_Interface->SV_Action( ServiceName, action, userdata ) == SV_OK )
	{
		RuleEngineLog(Debug, "RuleEngine-ServiceSDK> %s Send Action: %s", ServiceName, action );
		rc = 1;
	}
	else
		RuleEngineLog(Warning, "RuleEngine-ServiceSDK> Failed to send action %s to %s!", action, ServiceName);
	return rc;
}