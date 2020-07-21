/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								       */
/* Create Date  : 2018/6/12 by jin.xin							          */
/* Modified Date: 2018/6/12 by jin.xin								          */
/* Abstract     : DroidRoot Handler API                                     			 */
/* Reference    : None														             */
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include "susiaccess_handler_api.h"
#include "DroidRootHandler.h"
#include "DroidRootLog.h"
#include <configuration.h>
#include "cJSON.h"
#include "WISEPlatform.h"
#include "util_os.h"
#include "util_path.h"
#include "Parser.h"

const char strPluginName[MAX_TOPIC_LEN] = {"DroidRoot"};
static void* g_loghandle = NULL;
static bool g_bEnableLog = true;
static bool g_bRunningAndroidService = false;
const int iRequestID = cagent_request_droid_root;
const int iActionID = cagent_reply_droid_root;
static Handler_info  g_PluginInfo;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;		//not used
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;

//----------------------sensor info item list function define------------------

int Parser_PackCpbInfo(char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *root = NULL, *pSUSICommDataItem = NULL;
	if(outputStr == NULL) return outLen;

	root = cJSON_CreateObject();

	pSUSICommDataItem = cJSON_CreateObject();
	cJSON_AddItemToObject(root, AGENTINFO_BODY_STRUCT, pSUSICommDataItem);
	if(pSUSICommDataItem)
	{
		cJSON *handlerName = cJSON_CreateObject();
		if(handlerName)
		{
			cJSON *rootctrl = cJSON_CreateObject();

			cJSON_AddItemToObject(pSUSICommDataItem, HANDLER_NAME, handlerName);
			//first
			if(rootctrl){
				cJSON *eItem = cJSON_CreateArray();
				cJSON_AddItemToObject(handlerName, DROID_ROOTCTRL, rootctrl);
				if(eItem)
				{
					cJSON_AddItemToObject(rootctrl, DROID_E_FLAG, eItem);
					if(true)
					{
						cJSON *subItem = cJSON_CreateObject();
						cJSON_AddStringToObject(subItem, DROID_N_FLAG, "stop-some-app");
						cJSON_AddStringToObject(subItem, DROID_ASM_FLAG, "rw");
						cJSON_AddBoolToObject(subItem, DROID_V_FLAG, 0);
						cJSON_AddItemToArray(eItem, subItem);
					}
					if(true)
                    {
                        cJSON *subItem = cJSON_CreateObject();
                        cJSON_AddStringToObject(subItem, DROID_N_FLAG, "run-shell-command");
                        cJSON_AddStringToObject(subItem, DROID_ASM_FLAG, "rw");
                        cJSON_AddBoolToObject(subItem, DROID_V_FLAG, 0);
                        cJSON_AddItemToArray(eItem, subItem);
                    }
					/*
					if(true)
					{
						cJSON *subItem = cJSON_CreateObject();
						cJSON_AddStringToObject(subItem, DROID_N_FLAG, "brightness");
						cJSON_AddStringToObject(subItem, DROID_ASM_FLAG, "rw");
						cJSON_AddNumberToObject(subItem, DROID_V_FLAG, 0);
						cJSON_AddItemToArray(eItem, subItem);
					}
					*/
				}
				cJSON_AddStringToObject(rootctrl, DROID_BU_FLAG, DROID_BU_VALUE_BU);
               	cJSON_AddStringToObject(rootctrl, DROID_BN_FLAG, DROID_ROOTCTRL);
				cJSON_AddNumberToObject(rootctrl, DROID_VER_FLAG, DROID_VER_VALUE_1);
			}
		}
	}

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(root);
	printf("%s capability: %s\n", strPluginName, out);	
	free(out);
	return outLen;
}

void GetCapability()
{
    char * cpbStr = NULL;
    int jsonStrlen = 0;
    jsonStrlen = Parser_PackCpbInfo(&cpbStr);
    if(jsonStrlen > 0 && cpbStr != NULL)
    {
        g_sendcbf(&g_PluginInfo, droidroot_get_capability_rep, cpbStr, jsonStrlen+1, NULL, NULL);
        if(cpbStr)free(cpbStr);
    }
    else
    {
        char * errorRepJsonStr = NULL;
        char errorStr[128];
        int jsonStrlen = 0;
        sprintf(errorStr, "Command(%d), Get capability error!", droidroot_get_capability_req);
        jsonStrlen = Parser_PackDroidRootErrorRep(errorStr, &errorRepJsonStr);
        if(jsonStrlen > 0 && errorRepJsonStr != NULL)
        {
            g_sendcbf(&g_PluginInfo, droidroot_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
        }
        if(errorRepJsonStr)free(errorRepJsonStr);
    }
}

/* **************************************************************************************
*  Function Name: Handler_Initialize
*  Description: Init any objects or variables of this handler
*  Input :  PLUGIN_INFO *pluginfo
*  Output: None
*  Return:  handler_success  : Success Init Handler
*           handler_fail : Fail Init Handler
* ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	if(pluginfo == NULL)
	{
		return handler_fail;
	}

	if(g_bEnableLog)
	{
		g_loghandle = pluginfo->loghandle;
	}

	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", strPluginName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" >Name: %s\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf; 
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
	return;
}

static bool  android_system(char* cmdline){
	int status = -1;
	printf("android_system: cmdline=%s\n", cmdline);
	status = system(cmdline);
	if(status < 0)
  	{
  		DroidRootLog(g_loghandle, Warning, " %s> run android_system failed : %s\n", strPluginName, strerror(errno));
		return false;  
	}
	else
    {

        if (WIFEXITED(status))
        {
            if (0 == WEXITSTATUS(status))
            {
            	DroidRootLog(g_loghandle, Warning, " %s> run android_system successfully\n", strPluginName);
				return true;
			}
            else
            {
				DroidRootLog(g_loghandle, Warning, " %s> run android_system failed, script exit code:  %d\n", strPluginName, WEXITSTATUS(status));
            	return false;
			}
        }
        else
        {
        	DroidRootLog(g_loghandle, Warning, " %s> run android_system failed,  exit code:  %d\n", strPluginName, WEXITSTATUS(status));
			return false;
		}
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
	return handler_success;
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
	printf(" %s> Update Status", strPluginName);
	if(pluginfo)
	{
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	}
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strPluginName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
	}
	if(pluginfo->agentInfo->status == AGENT_STATUS_ONLINE)
	{
		;//
	}
}


/* **************************************************************************************
*  Function Name: Handler_Start
*  Description: Start Running
*  Input :  None
*  Output: None
*  Return:  handler_success : Success to Start Handler
*           handler_fail : Fail to Start Handler
* ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	printf("%s Handler_Start\n", strPluginName);
	return handler_success;
}


/* **************************************************************************************
*  Function Name: Handler_Stop
*  Description: Stop the handler
*  Input :  None
*  Output: None
*  Return:  handler_success : Success to Stop
*           handler_fail: Fail to Stop handler
* ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	printf("%s Handler_Stop\n", strPluginName);
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
void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int commCmd = unknown_cmd;
	char errorStr[128] = {0};

	DroidRootLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );
	
	if(!ParseReceivedData(data, datalen, &commCmd))
        return;
	switch(commCmd)
    {
    case droidroot_get_capability_req:
        {
            GetCapability();
            break;
        }
	case droidroot_set_sensors_data_req:
		{
			cJSON* root = NULL;
    		cJSON* body = NULL;
    		cJSON* sessionJson = NULL;
			cJSON * sensorIDListItem = NULL;
			char curSessionID[256] = {0};
			char *cmdRep;
			char statusCode = -1;
			char cmdline[1024] = {0};
			root = cJSON_Parse((char *)data);
    		if(root)
    		{
        		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
        		if(body)
       	 		{
            		int iRet = 0;
					//get "sessionID"
                	sessionJson = cJSON_GetObjectItem(body, DROID_SESSION_ID);
                	if(sessionJson)
                	{
                    	strcpy(curSessionID, sessionJson->valuestring);
                	}
					
					//get "sensorIDList"
					sensorIDListItem = cJSON_GetObjectItem(body, DROID_SENSOR_ID_LIST);
					if(sensorIDListItem){
						cJSON * eItem = NULL;
						//get "e"
            			eItem = cJSON_GetObjectItem(sensorIDListItem, DROID_E_FLAG);
            			if(eItem)
            			{
							cJSON *itemJson = cJSON_GetArrayItem(eItem, 0); // only deal the first one 
							cJSON *nameJson = cJSON_GetObjectItem(itemJson, DROID_N_FLAG);
							cJSON *valueJson = cJSON_GetObjectItem(itemJson, DROID_SV_FLAG);	
							
							printf("->name=%s, value=%s\n", nameJson->valuestring, valueJson->valuestring);
							//invoke system
							if(strstr(nameJson->valuestring, "stop-some-app") != NULL)
								sprintf(cmdline, "am force-stop %s", valueJson->valuestring);
							else if(strstr(nameJson->valuestring, "run-shell-command") != NULL){
								sprintf(cmdline, "%s", valueJson->valuestring);	
							}	
							else{
								sprintf(cmdline, ":"); // nop cmd
							}
							bool bret = android_system(cmdline);
							if(bret = true){	
								cmdRep = STATUS_SUCCESS;
								statusCode = STATUSCODE_SUCCESS;
							} else{
								cmdRep = STATUS_FAIL;
								statusCode = STATUSCODE_FAIL;
							}

							//send proper response data to server
							char * repJsonStr = NULL;
            				int jsonStrlen = Parser_PackSetSensorDataRepEx(nameJson->valuestring, cmdRep, statusCode, curSessionID, &repJsonStr);
							if(jsonStrlen > 0 && repJsonStr != NULL)
            				{
                				g_sendcbf(&g_PluginInfo, droidroot_set_sensors_data_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
            				}
							if(repJsonStr)
                            	free(repJsonStr);
						}
						else{
							goto fail;
						}
					}
        		}
        		cJSON_Delete(root);
				break;
    		}
fail:
			cJSON_Delete(root);
			g_sendcbf(&g_PluginInfo, droidroot_error_rep, "parse json fail", strlen("parse json fail")+1, NULL, NULL);
            break;
		}
	case droidroot_get_sensors_data_req:
		{
			//break;
		}
	default:
        {
            g_sendcbf(&g_PluginInfo, droidroot_error_rep, "Unknown cmd!", strlen("Unknown cmd!")+1, NULL, NULL);
            break;
        }
	}
}

/* **************************************************************************************
*  Function Name: Handler_Get_Capability
*  Description: Get Handler Information specification. 
*  Input :  None
*  Output: char * : pOutReply       // JSON Format
*  Return:  int  : Length of the status information in JSON format
*                :  0 : no data need to trans
* **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
    int len = 0; // Data length of the pOutReply 
    char * cpbStr = NULL;
    int jsonStrlen = 0;
    /*power_capability_info_t cpbInfo;
    memset((char*)&cpbInfo, 0, sizeof(power_capability_info_t));
    CollectCpbInfo(&cpbInfo);*/
    jsonStrlen = Parser_PackCpbInfo(&cpbStr);
    if(jsonStrlen > 0 && cpbStr != NULL)
    {
        len = strlen(cpbStr);
        *pOutReply = (char *)malloc(len + 1);
        memset(*pOutReply, 0, len + 1);
        strcpy(*pOutReply, cpbStr);
    }
    if(cpbStr)free(cpbStr);

    return len;
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
	DroidRootLog(g_loghandle, Normal, "%s Handler_AutoReportStart\n", strPluginName);
	return ;
}

/* **************************************************************************************
*  Function Name: Handler_AutoReportStop
*  Description: Stop Auto Report
*  Input : char *pInQuery
*  Output: None
*  Return: None
* ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	DroidRootLog(g_loghandle, Normal, "%s Handler_AutoReportStop\n", strPluginName);
	return ;
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
