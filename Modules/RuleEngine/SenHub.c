#include "SenHub.h"
#include "AdvWebClientAPI.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "RuleEngineLog.h"
#include "srp/susiaccess_handler_api.h"
#include "cJSON.h"
#include "WISEPlatform.h"

#if defined(WIN32) //windows
#define W_LIB_NAME_RULEENGINE "WAPI_RuleEngine.dll"
#else
#define W_LIB_NAME_RULEENGINE "libAdvWebClientAPI_RuleEngine.so"
#endif

#define BASICINFO_BODY_STRUCT	"susiCommData"
#define BASICINFO_HANDLERNAME	"handlerName"
#define BASICINFO_CMDTYPE		"commCmd"
#define BASICINFO_AGENTID		"agentID"
#define BASICINFO_TIMESTAMP		"sendTS"
#define BASICINFO_CONTENT		"content"
#define AGENTINFO_INFOSPEC		"infoSpec"
#define AGENTINFO_DATA			"data"


typedef struct _WEBClientObj{
    int id;
}WEBClientObj;

WAPI_Interface   *g_pWAPI_RuleEngine = NULL ;
SenHub_RecvCapability_Cb g_SenHub_recv_capability = NULL;
SenHub_RecvData_Cb g_SenHub_recv_data = NULL;
pthread_t g_SenHubThread = 0;
bool g_isSenHubThreadRunning = false;

void SenHubRecvCapability(char *devID, char *PluginName, void *inData, int dataLen)
{
	/*TODO extract the capability body inside of "content" or "infospec" for RMM 3.3*/
	cJSON *root = cJSON_Parse((char*) inData);

	if(root)
	{
		HANDLER_INFO handler;
		char* strcontent = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
		
		memset(&handler, 0, sizeof(HANDLER_INFO));
		handler.agentInfo = (cagent_agent_info_body_t*)calloc(1, sizeof(cagent_agent_info_body_t));
		strncpy(handler.agentInfo->devId, devID, sizeof(handler.agentInfo->devId));
		strncpy(handler.Name, PluginName, sizeof(handler.Name));

		if(g_SenHub_recv_capability)
			g_SenHub_recv_capability(&handler, strcontent, strlen(strcontent));
		free(handler.agentInfo);
		free(strcontent);
	}
}

void SenHubRecvData(char *devID, char *PluginName, void *inData, int dataLen)
{
	/*TODO extract the capability body inside of "content" or "infospec" for RMM 3.3*/
	cJSON *root = cJSON_Parse((char*) inData);

	if(root)
	{
		HANDLER_INFO handler;
		char* strcontent = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
		
		memset(&handler, 0, sizeof(HANDLER_INFO));
		handler.agentInfo = (cagent_agent_info_body_t*)calloc(1, sizeof(cagent_agent_info_body_t));
		strncpy(handler.agentInfo->devId, devID, sizeof(handler.agentInfo->devId));
		strncpy(handler.Name, PluginName, sizeof(handler.Name));

		if(g_SenHub_recv_data)
			g_SenHub_recv_data(&handler, strcontent, strlen(strcontent));
		free(handler.agentInfo);
		free(strcontent);
	}
}

int Proce_WebApi_Cb( WEBAPI_EVENT e, void *in, int len, void *user )
{
	char agentID[DEF_DEVID_LENGTH] = {0};
	char pluginname[MAX_TOPIC_LEN] = {0};
	
    RuleEngineLog(Warning, "RuleEngine-WAPI> event %d data=%s len=%d user=%d\n", e, in, len, user);

	if(e == WAPI_E_RECEIVE )
	{
		cJSON* root = cJSON_Parse((char*)in);
		if(root != NULL ) {
            cJSON* eventnode = cJSON_GetObjectItem(root,"event");
			cJSON* data = cJSON_GetObjectItem(root,"data");

			if( strcmp(eventnode->valuestring,"eSenHub_UpdateData") == 0 )
			{
				cJSON* target = data->child;
				cJSON* temp = NULL;	
				while(target)
				{
					if(strcmp(target->string, "agentID")==0)
						strncpy(agentID, target->valuestring, sizeof(agentID));
					else
					{
						if(temp == NULL)
							temp = cJSON_CreateObject();
						strncpy(pluginname, target->string, sizeof(pluginname));
						cJSON_AddItemToObject(temp, target->string, cJSON_Duplicate(target, 1));
					}
					target = target->next;
				}
				if(temp!=NULL)
				{
					char* reportdata = cJSON_PrintUnformatted(temp);
					cJSON_Delete(temp);
					SenHubRecvData(agentID, pluginname, reportdata, strlen(reportdata));
					free(reportdata);
				}
			}
			else if( strcmp(eventnode->valuestring,"eSenHub_Capability") == 0 )
			{
				cJSON* target = data->child;
				cJSON* temp = NULL;	
				while(target)
				{
					if(strcmp(target->string, "agentID")==0)
						strncpy(agentID, target->valuestring, sizeof(agentID));
					else
					{
						if(temp == NULL)
							temp = cJSON_CreateObject();
						strncpy(pluginname, target->string, sizeof(pluginname));
						cJSON_AddItemToObject(temp, target->string, cJSON_Duplicate(target, 1));
					}
					target = target->next;
				}
				if(temp)
				{
					char* reportdata = cJSON_PrintUnformatted(temp);
					cJSON_Delete(temp);
					SenHubRecvCapability(agentID, pluginname, reportdata, strlen(reportdata));
					free(reportdata);
				}
			}

			cJSON_Delete(root);
		}
	}

    return 1;
}

void ResyncSenHub()
{
	char buffer[1024]={0};
	cJSON* root = NULL;
	cJSON* target = NULL;

	if(g_pWAPI_RuleEngine == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-WAPI> AdvWAPI have not initialized yet!\r\n");
		return;
	}

	if(g_pWAPI_RuleEngine->AdvWAPI_Get != NULL)
	{
		g_pWAPI_RuleEngine->AdvWAPI_Get("/restapi/WSNManage/SenHub/AllSenHubList", buffer,  sizeof(buffer) );
		RuleEngineLog(Debug, "RuleEngine-WAPI> SenHub List=%s",buffer);
	}
	else
	{
		RuleEngineLog(Warning, "RuleEngine-WAPI> Access Failed on AdvWAPI_Get API!");
		return;
	}
	root = cJSON_Parse(buffer);
	if(root == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-WAPI> Response message incorrect!");
		return;
	}

	target = cJSON_GetObjectItem(root, "n");
	if(target == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-WAPI> Response message incorrect!");
		cJSON_Delete(root);
		return;
	}
	if(strcmp(target->valuestring,"AllSenHubList") != 0)
	{
		RuleEngineLog(Warning, "RuleEngine-WAPI> Response message incorrect!");
		cJSON_Delete(root);
		return;
	}
	target = cJSON_GetObjectItem(root, "sv");
	if(target == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-WAPI> Response message incorrect!");
		cJSON_Delete(root);
		return;
	}
	else
	{
		char* delim=",";
		char *p = NULL;
		char *token = NULL;
		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, target->valuestring, sizeof(buffer));
		p=strtok_r(buffer, delim, &token);
		while(p)
		{
			char buf[2048] = {0};
			char topic[260] = {0};
			char pluginname[MAX_TOPIC_LEN] = {0};
			sprintf(topic, "/restapi/WSNManage/SenHub/%s", p);
			g_pWAPI_RuleEngine->AdvWAPI_Get(topic, buf,  sizeof(buf) );
			if(buf != NULL)
			{
				char* reportdata = NULL;
				cJSON* root = cJSON_Parse(buf);
				if(root->child != NULL)
					strncpy(pluginname, root->child->string, sizeof(pluginname));
				reportdata = cJSON_PrintUnformatted(root);
				cJSON_Delete(root);
				SenHubRecvCapability(p, pluginname, reportdata, strlen(reportdata));
				free(reportdata);
			}
			RuleEngineLog(Debug, "RuleEngine-WAPI> %s Capability=%s", topic, buf);
			
			p=strtok_r(NULL, delim, &token);
		}
	}
	cJSON_Delete(root);
}

void* SenHubThread(void* args)
{
	//char libpath[256]={0};
	int nServiceHandle = 0;

    WEBClientObj wcb;

	if(g_pWAPI_RuleEngine == NULL)
	{
		RuleEngineLog(Warning, "RuleEngine-WAPI> AdvWAPI have not initialized yet!\r\n");
		goto STOP_WAPI;
	}

	RuleEngineLog(Normal, "RuleEngine-WAPI> Success: Initialize WAPI");
	g_isSenHubThreadRunning = true;

	/*Resync SenHub Capability*/
	ResyncSenHub();

	
	/*Set callback and Join*/
	if(g_pWAPI_RuleEngine->AdvWAPI_SetCallback != NULL)
		g_pWAPI_RuleEngine->AdvWAPI_SetCallback(&Proce_WebApi_Cb);
	else
		RuleEngineLog(Warning, "RuleEngine-WAPI> Access Failed on AdvWAPI_SetCallback API!");

    nServiceHandle = g_pWAPI_RuleEngine->AdvWAPI_Join_Service( "/WSNManage", &wcb);

	if(g_pWAPI_RuleEngine->AdvWAPI_GetStatus_Service)
		RuleEngineLog(Normal, "RuleEngine-WAPI> service status=%d", g_pWAPI_RuleEngine->AdvWAPI_GetStatus_Service(nServiceHandle));
	else
		RuleEngineLog(Warning, "RuleEngine-WAPI> Access Failed on AdvWAPI_GetStatus_Service API!");	

    while(g_isSenHubThreadRunning)
    {
        usleep(1000000);
    }    
	
STOP_WAPI:
	if(g_pWAPI_RuleEngine != NULL && g_pWAPI_RuleEngine->AdvWAPI_UnInitialize != NULL)
	{
		g_pWAPI_RuleEngine->AdvWAPI_UnInitialize();
		RuleEngineLog(Debug, "RuleEngine-WAPI> Uninitialize WAPI" );
	}
	else
	{
		RuleEngineLog(Warning, "RuleEngine-WAPI> Access Failed on AdvWAPI_UnInitialize API!");
	}

	pthread_exit(0);
	return 0;
}

int InitSenHubHandle(SenHub_RecvCapability_Cb recv_capability, SenHub_RecvData_Cb recv_data, void* loghandle)
{
	int rc = 0;

	g_SenHub_recv_capability = recv_capability;
	g_SenHub_recv_data = recv_data;
	g_ruleenginehandlerlog = loghandle;
	if(g_SenHubThread == 0)
	{
		char libpath[256]={0};
		sprintf(libpath,"%s",W_LIB_NAME_RULEENGINE);
		if(!util_is_file_exist(libpath))
		{
			RuleEngineLog(Warning, "RuleEngine-WAPI> Failed: File %s not exist!", libpath);
			g_pWAPI_RuleEngine = NULL;
			return rc;
		}
		if( GetAPILibFn( libpath,&g_pWAPI_RuleEngine ) == 0 ) {
			RuleEngineLog(Warning, "RuleEngine-WAPI> Failed: InitWHandler: GetWAPILibFn\r\n");
			g_pWAPI_RuleEngine = NULL;
			return rc;
		}

		if(g_pWAPI_RuleEngine->AdvWAPI_Initialize != NULL)
		{
			if( g_pWAPI_RuleEngine->AdvWAPI_Initialize() != WAPI_CODE_OK ) {
				RuleEngineLog(Warning, "RuleEngine-WAPI> Failed: to AdvWAPI_Initialize");
				return rc;
			}
		}
		else
		{
			RuleEngineLog(Warning, "RuleEngine-WAPI> Access Failed on AdvWAPI_Initialize API!");
			return rc;
		}

		if(pthread_create(&g_SenHubThread, NULL, SenHubThread, NULL) != 0)
		{
			g_isSenHubThreadRunning = false;
			RuleEngineLog(Warning, "RuleEngine-WAPI> start SenHub thread failed!");
		}
	}
	rc = 1;
    return rc;
}

void UninitSenHubHandle()
{
	g_SenHub_recv_capability = NULL;
	g_SenHub_recv_data = NULL;


	if(g_SenHubThread != 0)
	{
		g_isSenHubThreadRunning = false;
		pthread_join(g_SenHubThread, NULL);
		g_SenHubThread = 0;
	}
}


