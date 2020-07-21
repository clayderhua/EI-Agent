#include "RuleEngine.h"
#include "HandlerKernelEx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "WISEPlatform.h"
#include "IPSOParser.h"
#include "RuleParser.h"
#include "cJSON.h"
#include "version.h"
#include "ServiceHandle.h"
#include "SenHub.h"
#include "RuleEngineLog.h"
#include "ActionTrigger.h"
#ifdef MULIT_THREAD
#else
#include <pthread.h>
#include "ThresholdCheck.h"
#endif
#define CAPAB_INFORMATION                           "Information"
#define CAPAB_INFO		                            "info"
#define CAPAB_DATA		                            "data"
#define CAPAB_ACTION	                            "action"
#define CAPAB_E_FLAG                                "e"
#define CAPAB_N_FLAG                                "n"
#define CAPAB_V_FLAG                                "v"
#define CAPAB_SV_FLAG                               "sv"
#define CAPAB_BV_FLAG                               "bv"
#define CAPAB_BN_FLAG							    "bn"
#define CAPAB_NONSENSORDATA_FLAG					"nonSensorData"
#define CAPAB_FUNCTION_LIST                         "functionList"
#define CAPAB_FUNCTION_CODE                         "functionCode"
#define CAPAB_NAME									"name"
#define CAPAB_DESCRIPTION							"description"
#define CAPAB_VERSION								"version"
#define CAPAB_MONITOR_LIST							"sensorpluginlist"
#define CAPAB_ACTION_LIST							"actionpluginlist"

static bool g_bEIService = false;
static bool g_bWAPI = false;
const char strHandlerName[MAX_TOPIC_LEN] = {"RuleEngine"}; /*declare the handler name*/
Handler_info_ex g_HandlerInfo;
HANDLER_THREAD_STATUS g_status = handler_status_no_init; // global status flag.
MSG_CLASSIFY_T *g_Capability = NULL; /*the global message structure to describe the sensor data as the handelr capability*/
void* g_CurHandler = NULL;
#ifdef MULIT_THREAD
Handler_info_ex g_thrHandlerInfo;
#else
pthread_t g_thrThresholdCheck = 0;
bool g_bThresholdCheck = false;
#endif

typedef struct HANDLER_CAPAB
{	
	char Name[MAX_TOPIC_LEN];
	char strDevID[DEF_DEVID_LENGTH];
	bool bSensing;
	bool bAction;
	MSG_CLASSIFY_T *pCapability;
	MSG_CLASSIFY_T* pAction;
#ifdef MULIT_THREAD
#else
	thr_item_list pThresholdList;
#endif
	void* pHandlerKernel;
	struct HANDLER_CAPAB* next;
}Handler_Capability;

Handler_Capability* g_pCapablityList = NULL;

pthread_mutex_t g_HandlerListMux;


int g_SendCapabilityDelay = 0;
pthread_mutex_t g_SendCapablityMux;

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
    fprintf(stderr, "DllInitializer\r\n");
}

__attribute__((destructor))
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer()
{
    fprintf(stderr, "DllFinalizer\r\n");
	Handler_Uninitialize();
}
#endif

bool SendCapability();
#ifndef MULIT_THREAD
void* RuleEngine_ThresholdCheckThread(void *args)
{
	int interval=1000;
	long long curTime = 0;
	long long nextTime = 0;

	while(g_bThresholdCheck)
	{
		//unsigned int diffTime = 0;
		HANDLER_NOTIFY_SEVERITY severity = Severity_Debug;

		if(g_pCapablityList)
		{
			Handler_Capability* curCapa = g_pCapablityList;
			while(curCapa)
			{
				if(curCapa->pThresholdList != NULL)
				{
					char* buffer = (char*)calloc(1,1024);
					char* buff = NULL;
					int buffsize = 0;
					bool bNormal = true;
					bool bResult = false;
					
					bResult = ThresholdCheck_CheckThr(curCapa->pThresholdList, &buffer, 1024, &bNormal);
				
					if(bResult)
					{
						if(strlen(buffer) > 0)
						{
							if(bNormal)
							{
								severity = Severity_Informational;
								buffsize = strlen("{\"subtype\":\"THRESHOLD_CHECK_INFO\",\"thrCheckStatus\":\"%s\",\"msg\":\"%s\"}") + 5 + strlen(buffer);
								buff = (char*)calloc(1, buffsize+1);
								sprintf(buff,"{\"subtype\":\"THRESHOLD_CHECK_INFO\",\"thrCheckStatus\":\"%s\",\"msg\":\"%s\"}", bNormal?"True":"False", buffer); /*for custom handler*/
							}
							else
							{
								severity = Severity_Error;
								buffsize = strlen("{\"subtype\":\"THRESHOLD_CHECK_ERROR\",\"thrCheckStatus\":\"%s\",\"msg\":\"%s\"}") + 5 + strlen(buffer);
								buff = (char*)calloc(1, buffsize+1);
								sprintf(buff,"{\"subtype\":\"THRESHOLD_CHECK_ERROR\",\"thrCheckStatus\":\"%s\",\"msg\":\"%s\"}", bNormal?"True":"False", buffer); /*for custom handler*/
							}
						}
					}
					free(buffer);

					if(buff)
					{
						Handler_info_ex tmpHandlerInfo;
						memcpy(&tmpHandlerInfo, &g_HandlerInfo, sizeof(Handler_info_ex));
						strcpy(tmpHandlerInfo.Name, curCapa->Name);
					
						if(g_HandlerInfo.sendeventcbf)
							g_HandlerInfo.sendeventcbf(/*Handler Info*/&tmpHandlerInfo, severity, /*message data*/buff, /*message length*/strlen(buff), /*preserved*/NULL, /*preserved*/NULL);

						free(buff);
					}

					ThresholdCheck_QueueingAction(curCapa->pThresholdList);
				}

				curCapa = curCapa->next;
			}
		}
		//nextTime = curTime + 1000;
		
		usleep(100000);
	}

	pthread_exit(0);
	return 0;
}
#endif

Handler_Capability* FindHandlerCapabilityEx(char* devID, char* name, Handler_Capability* pList)
{
	Handler_Capability *pHandlerCap = NULL;
	Handler_Capability *pLast = pList, *pTarget = NULL;
	//RuleEngineLog(Debug, " %s > Find Handler:[%s]", g_HandlerInfo.Name, name);
	pthread_mutex_lock(&g_HandlerListMux);
	while(pLast != NULL)
	{
		//RuleEngineLog(Debug, " %s > Find Handler:[%s][%s]", g_HandlerInfo.Name, name, pLast->Name);
		if(strcmp(pLast->Name, name) == 0)
		{
			if(strcmp(pLast->strDevID, devID) == 0)
			{
				pTarget = pLast;
				break;
			}
		}

		pLast = pLast->next;
	}
	pthread_mutex_unlock(&g_HandlerListMux);
	return pTarget;
}

Handler_Capability* FindHandlerCapability(char* name, Handler_Capability* pList)
{
	return FindHandlerCapabilityEx(g_HandlerInfo.agentInfo->devId, name, pList);
}

bool IsSensor(MSG_CLASSIFY_T* plugin)
{
	MSG_CLASSIFY_T* data = NULL;
	if(plugin == NULL)
		return false;

	data = IoT_FindGroup(plugin, CAPAB_INFO);
	if(data)
	{
		MSG_ATTRIBUTE_T* attr = MSG_FindAttribute(data, CAPAB_NONSENSORDATA_FLAG, false);
		if(attr)
		{
			return !attr->bv;
		}
	}

	data = IoT_FindGroup(plugin, CAPAB_DATA);
	if(data)
		return true;
	else
	{
		data = IoT_FindGroup(plugin, CAPAB_INFORMATION);
		if(data == NULL)
		{
			return true;
		}
		else
		{
			MSG_ATTRIBUTE_T* attr = MSG_FindAttribute(data, CAPAB_NONSENSORDATA_FLAG, false);
			if(attr)
			{
				return !attr->bv;
			}
		}
	}

	return false;
}

bool IsAction(MSG_CLASSIFY_T* plugin)
{
	MSG_CLASSIFY_T* action = NULL;
	if(plugin == NULL)
		return false;

	action = IoT_FindGroup(plugin, CAPAB_ACTION);
	if(action)
		return true;
	
	if(strcmp(plugin->classname, "power_onoff") == 0)
	{
		return true;
	}
	else if(strcmp(plugin->classname, "protection") == 0)
	{
		return true;
	}
	else if(strcmp(plugin->classname, "recovery") == 0)
	{
		return true;
	}
	else if(strcmp(plugin->classname, "ProcessMonitor") == 0)
	{
		return true;
	}
	else if(strcmp(plugin->classname, "screenshot") == 0)
	{
		return true;
	}
	if(IoT_HasWritableSensorNode(plugin))
		return true;
	return false;
}

bool CheckPluginType(MSG_CLASSIFY_T* pCapability, char* devID, char* name, bool* bSensor, bool* bAction)
{
	bool bRet = false;

	MSG_CLASSIFY_T* dev = NULL;
	MSG_CLASSIFY_T* plugin = NULL;

	*bSensor = false;
	*bAction = false;

	if(pCapability == NULL || name == NULL)
		return bRet;

	dev = MSG_FindClassify(pCapability, devID);

	if(dev == NULL)
		return bRet;

	plugin = MSG_FindClassify(dev, name);

	if(plugin == NULL)
		return bRet;

	*bSensor = IsSensor(plugin);
	*bAction = IsAction(plugin);

	return bRet;
}

bool AddHandlerCapabilityEx(char* devID, char* name, char* data, Handler_Capability** pList)
{
	bool bRet = false;
	bool bNewOne = false;
	Handler_Capability *pHandlerCap = NULL;
	Handler_Capability *pLast = *pList, *pTarget = NULL;
	pHandlerCap = FindHandlerCapabilityEx(devID, name, *pList);
	pthread_mutex_lock(&g_HandlerListMux);
	if(pHandlerCap == NULL)
	{
		MSG_CLASSIFY_T *newRoot = NULL;
		pHandlerCap = (Handler_Capability *)calloc(1, sizeof(Handler_Capability));
		strncpy(pHandlerCap->Name, name, sizeof(pHandlerCap->Name));
		strncpy(pHandlerCap->strDevID, devID, sizeof(pHandlerCap->strDevID));
		pHandlerCap->pCapability = IoT_CreateRoot(devID);
		newRoot = IoT_AddGroup(pHandlerCap->pCapability, name);
		bRet = transfer_parse_ipso( data, newRoot);
		if(bRet)
		{
#ifdef MULIT_THREAD
			pHandlerCap->pHandlerKernel = HandlerKernelEx_Initialize((Handler_info *)&g_thrHandlerInfo);
#endif
			CheckPluginType(pHandlerCap->pCapability, devID, name, &pHandlerCap->bSensing, &pHandlerCap->bAction);
			if(pHandlerCap->bAction)
			{
				if(pHandlerCap->pAction != NULL)
					IoT_ReleaseAll(pHandlerCap->pAction);
				Action_Set_Action_Capability(pHandlerCap->strDevID, pHandlerCap->Name, pHandlerCap->pCapability, &pHandlerCap->pAction);
			}
#ifdef MULIT_THREAD
			if(pHandlerCap->pHandlerKernel)
			{
				HandlerKernelEx_SetCapability(pHandlerCap->pHandlerKernel, pHandlerCap->pCapability, false);
				HandlerKernelEx_Start(pHandlerCap->pHandlerKernel);
			}
#else
			IoT_ReleaseAll(pHandlerCap->pCapability);
			pHandlerCap->pCapability = NULL;
#endif
		}

		while(pLast != NULL)
		{
			pTarget = pLast;
			pLast = pLast->next;
		}
		if(pTarget==NULL)
			*pList = pHandlerCap;
		else
			pTarget->next = pHandlerCap;
		RuleEngineLog(Debug, " %s > Add Handler:[%s]", g_HandlerInfo.Name, name);
		bNewOne = true;
	}
	pthread_mutex_unlock(&g_HandlerListMux);
	if(g_Capability && bNewOne) /*Update Capability after Handler_Get_Capability triggered*/
		SendCapability();
	return bRet;
}

bool AddHandlerCapability(char* name, char* data, Handler_Capability** pList)
{
	return AddHandlerCapabilityEx(g_HandlerInfo.agentInfo->devId, name, data, pList);
}

void ClearHandlerCapability(Handler_Capability** pList)
{
	Handler_Capability *pLast = *pList, *pTarget = NULL;
	if( *pList == NULL)
		return;
	pthread_mutex_lock(&g_HandlerListMux);
	while(pLast != NULL)
	{
		pTarget = pLast;
		pLast = pLast->next;
#ifdef MULIT_THREAD
		if(pTarget->pHandlerKernel)
		{
			HandlerKernelEx_Stop(pTarget->pHandlerKernel);
			HandlerKernelEx_Uninitialize(pTarget->pHandlerKernel);
			pTarget->pHandlerKernel = NULL;
		}
#else
		if(pTarget->pThresholdList)
		{
			ThresholdCheck_DeleteAllThrNode(pTarget->pThresholdList);
			free(pTarget->pThresholdList); 
			pTarget->pThresholdList = NULL;
		}

#endif
		if(pTarget->pCapability)
			IoT_ReleaseAll(pTarget->pCapability);
		if(pTarget->pAction)
			IoT_ReleaseAll(pTarget->pAction);
		RuleEngineLog(Debug, " %s > Remove Handler:[%s]", g_HandlerInfo.Name, pTarget->Name);
		free(pTarget);
	}
	*pList = NULL;
	pthread_mutex_unlock(&g_HandlerListMux);
}

/*callback function to handle threshold rule check event*/
void on_threshold_triggered(bool isNormal, char* sensorname, double value, MSG_ATTRIBUTE_T* attr, void *pRev)
{
	thr_item_info_t* item = (thr_item_info_t*)pRev;
	thr_action_t* pAction = NULL;
	RuleEngineLog(Debug, " %s> threshold triggered:[%s, %s, %f]", g_HandlerInfo.Name, isNormal?"Normal":"Warning", sensorname, value);
	pAction = item->pActionList;
	while(pAction)
	{
		char devID[DEF_DEVID_LENGTH] = {0};
		char plugin[MAX_TOPIC_LEN] = {0};
		char sensor[DEF_MAX_STRING_LENGTH] = {0};
		char command[1024] = {0};
		char buffer[1024] = {0};

		if(isNormal)
			pAction->lock = false;

		if(pAction->normal != isNormal)
		{
			pAction = pAction->next;
			continue;
		}

		if(pAction->lock)
		{
			pAction = pAction->next;
			continue;
		}
		else if(pAction->once && !isNormal)
			pAction->lock = true;

		RuleParser_ActionCommand(pAction->pathname, devID, plugin, sensor);

		if(strcmp(devID, "+") == 0)
		{
			strcpy(devID, g_HandlerInfo.agentInfo->devId);
		}
		
		if(Action_Trigger_Command(plugin, sensor, pAction, command))
		{
			char topic[DEF_MAX_STRING_LENGTH] = {0};

			if(buffer)
			{
				HANDLER_NOTIFY_SEVERITY severity = Severity_Informational;

				snprintf(buffer, sizeof(buffer), "{\"subtype\":\"ACTION_TRIGGER_INFO\",\"msg\":\"Action \\\"%s\\\" triggered by \\\"%s\\\":%f\"}", pAction->pathname, sensorname, value);
				
				if(g_HandlerInfo.sendeventcbf)
					g_HandlerInfo.sendeventcbf(/*Handler Info*/&g_HandlerInfo, severity, /*message data*/buffer, /*message length*/strlen(buffer), /*preserved*/NULL, /*preserved*/NULL);
			}
#ifdef RMM3X
			sprintf(topic, DEF_CALLBACKREQ_TOPIC, devID);
#else
			sprintf(topic, DEF_CALLBACKREQ_TOPIC, g_HandlerInfo.agentInfo->productId, devID);
#endif
			
			if(g_HandlerInfo.sendcustcbf)
				g_HandlerInfo.sendcustcbf(NULL, 0,  topic, command, strlen(command), NULL, NULL);
		}
		
#if 0
		if(strcmp(devID, g_HandlerInfo.agentInfo->devId) == 0)
		{
			/*TODO: Check ServiceSDK does plugin exist. Send Command with ServiceSDK or not*/

		}
		else if(strcmp(plugin, "SenHub") == 0)
		{
			/*TODO: Send Command with WAPI*/
		}
		else
		{
			/*TODO: Send Command with SendCustomCB*/
		}
#endif

		pAction = pAction->next;
	}
}

/*Create Capability Message Structure to describe sensor data*/
MSG_CLASSIFY_T * CreateCapability()
{
	MSG_ATTRIBUTE_T* mySensor = NULL;
	char funcList[64] = {0};
	char monitorList[1024] = {0};
	char actionList[1024] = {0};
	char version[DEF_VERSION_LENGTH] = {0};
	int funcCode =NoneCode;
	Handler_Capability* pCurrentHandler = g_pCapablityList;

	MSG_CLASSIFY_T* myCapability = IoT_CreateRoot((char*)strHandlerName);
	MSG_CLASSIFY_T* myInfo = IoT_AddGroup(myCapability, CAPAB_INFO);
	MSG_CLASSIFY_T* myAct = IoT_AddGroup(myCapability, CAPAB_ACTION);
	
	mySensor = IoT_AddSensorNode(myInfo, CAPAB_NAME);
	IoT_SetStringValue(mySensor, g_HandlerInfo.Name, IoT_READONLY);

	mySensor = IoT_AddSensorNode(myInfo, CAPAB_DESCRIPTION);
	IoT_SetStringValue(mySensor, "This service is RuleEngine Service", IoT_READONLY);

	mySensor = IoT_AddSensorNode(myInfo, CAPAB_VERSION);
	snprintf(version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);
	IoT_SetStringValue(mySensor, version, IoT_READONLY);

	mySensor = IoT_AddGroupAttribute(myInfo, CAPAB_NONSENSORDATA_FLAG);
	IoT_SetBoolValue(mySensor, true, IoT_READONLY);

	sprintf(funcList, "%s,%s", ThresholdStr, EventStr);
	funcCode += ThresholdCode;
	funcCode += EventCode;
	if(g_bEIService)
	{
		strcat(funcList, ",");
		strcat(funcList, EIServiceStr);

		funcCode += EIServiceCode;
	}

	if(g_bWAPI)
	{
		strcat(funcList, ",");
		strcat(funcList, WAPIStr);

		funcCode += WAPICode;
	}

	mySensor = IoT_AddSensorNode(myInfo, CAPAB_FUNCTION_LIST);
	IoT_SetStringValue(mySensor, funcList, IoT_READONLY);

	
	mySensor = IoT_AddSensorNode(myInfo, CAPAB_FUNCTION_CODE);
	IoT_SetDoubleValue(mySensor, funcCode, IoT_READONLY, NULL);

	while(pCurrentHandler)
	{
		if(pCurrentHandler->bSensing)
		{
			if(strstr(monitorList, pCurrentHandler->Name) == 0)
			{
				if(strlen(monitorList)>0)
					strcat(monitorList, ",");
				strcat(monitorList, pCurrentHandler->Name);
			}
		}

		if(pCurrentHandler->bAction)
		{
			if(strstr(actionList, pCurrentHandler->Name) == 0)
			{
				if(strlen(actionList)>0)
					strcat(actionList, ",");
				strcat(actionList, pCurrentHandler->Name);
				
				IoT_AppendGroup(myAct, pCurrentHandler->pAction);			
			}
		}
		//RuleEngineLog(Debug, " %s> Plugin: %s", g_HandlerInfo.Name, pCurrentHandler->Name);
		//RuleEngineLog(Debug, " %s> Monitor: %s", g_HandlerInfo.Name, monitorList);
		//RuleEngineLog(Debug, " %s> Action: %s", g_HandlerInfo.Name, actionList);
		pCurrentHandler = pCurrentHandler->next;
	}
	
	mySensor = IoT_AddSensorNode(myInfo, CAPAB_MONITOR_LIST);
	IoT_SetStringValue(mySensor, monitorList, IoT_READONLY);

	mySensor = IoT_AddSensorNode(myInfo, CAPAB_ACTION_LIST);
	IoT_SetStringValue(mySensor, actionList, IoT_READONLY);
	
	return myCapability;
}

void* ThreadSendCapability(void* args)
{
	if(g_Capability)
	{
		int countdown=0;
		char* result = NULL;

		pthread_mutex_lock(&g_SendCapablityMux);
		countdown = g_SendCapabilityDelay;
		pthread_mutex_unlock(&g_SendCapablityMux);

		while(countdown>0)
		{
			pthread_mutex_lock(&g_SendCapablityMux);
			g_SendCapabilityDelay--;
			countdown = g_SendCapabilityDelay;
			pthread_mutex_unlock(&g_SendCapablityMux);

			usleep(100000);
		}
		pthread_mutex_lock(&g_SendCapablityMux);
		result = IoT_PrintCapability(g_Capability);
		pthread_mutex_unlock(&g_SendCapablityMux);
		if(result)
		{
			if(g_HandlerInfo.sendcapabilitycbf)
				g_HandlerInfo.sendcapabilitycbf(&g_HandlerInfo, result, strlen(result), NULL, NULL) == 0?true:false;
			free(result);
		}
	}

	pthread_mutex_lock(&g_SendCapablityMux);
	g_SendCapabilityDelay = 0;
	pthread_mutex_unlock(&g_SendCapablityMux);

	pthread_exit(0);
	return 0;
}

bool SendCapability()
{
	bool bRet = false;
	bool bCounting = false;
	pthread_t pCapabilityThread = NULL;

	pthread_mutex_lock(&g_SendCapablityMux);
	if(g_Capability)
		IoT_ReleaseAll(g_Capability);

	g_Capability = CreateCapability();

	if(g_SendCapabilityDelay > 0)
		bCounting = true;
	g_SendCapabilityDelay = 50;
	pthread_mutex_unlock(&g_SendCapablityMux);

	if(!bCounting)
	{
		if (pthread_create(&pCapabilityThread, NULL, ThreadSendCapability, NULL) != 0 )
		{
			pthread_mutex_lock(&g_SendCapablityMux);
			g_SendCapabilityDelay = 0;
			pthread_mutex_unlock(&g_SendCapablityMux);

			RuleEngineLog(Error, " %s> start handler thread failed! ", g_HandlerInfo.Name );	
			return bRet;
		}
		else
		{
			pthread_detach(pCapabilityThread);
		}
	}

	return bRet;
}

bool SendSetThresholdResult(int replyID, char *repMsg)
{
	char* repJsonStr = NULL;
	if(repMsg == NULL)
		return false;

	if(RuleParser_PackSetThrRep(repMsg, &repJsonStr))
	{
		if(g_HandlerInfo.sendcbf)
			g_HandlerInfo.sendcbf(&g_HandlerInfo, replyID, repJsonStr, strlen(repJsonStr), NULL, NULL);
	}
	if(repJsonStr)free(repJsonStr);	
	return true;
}

void DeleteWholeHandlersThresholds(int replyID)
{
	Handler_Capability *pLast = g_pCapablityList, *pTarget = NULL;
	
	while(pLast != NULL)
	{
#ifdef MULIT_THREAD
		if(pLast->pHandlerKernel)
		{
			/*Stop threshold check thread*/
			HandlerKernelEx_StopThresholdCheck(pLast->pHandlerKernel);
			/*clear threshold check callback function*/
			HandlerKernelEx_SetThresholdTrigger(pLast->pHandlerKernel, NULL);
			/*Delete all threshold rules*/
			HandlerKernelEx_DeleteAllThreshold(pLast->pHandlerKernel, replyID);
		}
#else
		if(pLast->pThresholdList)
		{
			char* buffer = (char*)calloc(1, 1024);
			ThresholdCheck_WhenDelThrCheckNormal(pLast->pThresholdList,&buffer, 1024);
			ThresholdCheck_DeleteAllThrNode(pLast->pThresholdList);
			free(pLast->pThresholdList); 
			pLast->pThresholdList = NULL;

			if(strlen(buffer) > 0)
			{
				int len = strlen(buffer) + 70;
				char* message = NULL;
				Handler_info_ex tmpHandlerInfo;
				message = (char*)calloc(1, len);
				sprintf(message,"{\"subtype\":\"THRESHOLD_CHECK_INFO\",\"thrCheckStatus\":\"True\",\"msg\":\"%s\"}", buffer); /*for custom handler*/

				memcpy(&tmpHandlerInfo, &g_HandlerInfo, sizeof(Handler_info_ex));
				strcpy(tmpHandlerInfo.Name, pLast->Name);

				if(tmpHandlerInfo.sendeventcbf && strlen(message) > 0)
					tmpHandlerInfo.sendeventcbf(/*Handler Info*/&tmpHandlerInfo, Severity_Informational, /*message data*/message, /*message length*/strlen(message), /*preserved*/NULL, /*preserved*/NULL);
				free(message);
			}
			free(buffer);
		}
#endif
		pLast = pLast->next;
	}
}

void DeleteDeviceThresholds(char* devid, int replyID)
{
	Handler_Capability *pLast = g_pCapablityList, *pTarget = NULL;
	
	while(pLast != NULL)
	{
		if(strcmp(pLast->strDevID, devid) == 0)
		{
#ifdef MULIT_THREAD
			if(pLast->pHandlerKernel)
			{
				/*Stop threshold check thread*/
				HandlerKernelEx_StopThresholdCheck(pLast->pHandlerKernel);
				/*clear threshold check callback function*/
				HandlerKernelEx_SetThresholdTrigger(pLast->pHandlerKernel, NULL);
				/*Delete all threshold rules*/
				HandlerKernelEx_DeleteAllThreshold(pLast->pHandlerKernel, replyID);
			}
#else
			if(pLast->pThresholdList)
			{
				char* buffer = (char*)calloc(1, 1024);
				ThresholdCheck_WhenDelThrCheckNormal(pLast->pThresholdList,&buffer, 1024);

				ThresholdCheck_DeleteAllThrNode(pLast->pThresholdList);
				free(pLast->pThresholdList); 
				pLast->pThresholdList = NULL;

				if(strlen(buffer) > 0)
				{
					int len = strlen(buffer) + 70;
					char* message = NULL;
					Handler_info_ex tmpHandlerInfo;
					message = (char*)calloc(1, len);
					sprintf(message,"{\"subtype\":\"THRESHOLD_CHECK_INFO\",\"thrCheckStatus\":\"True\",\"msg\":\"%s\"}", buffer); /*for custom handler*/
					
					memcpy(&tmpHandlerInfo, &g_HandlerInfo, sizeof(Handler_info_ex));
					strcpy(tmpHandlerInfo.Name, pLast->Name);

					if(tmpHandlerInfo.sendeventcbf && strlen(message) > 0)
						tmpHandlerInfo.sendeventcbf(/*Handler Info*/&tmpHandlerInfo, Severity_Informational, /*message data*/message, /*message length*/strlen(message), /*preserved*/NULL, /*preserved*/NULL);
					free(message);
				}
				free(buffer);
			}
#endif
		}
		pLast = pLast->next;
	}
}

bool SetThresholdRule(char* data)
{
	char* result = NULL;
	if(RuleParser_ParseThrInfo(data, g_HandlerInfo.agentInfo->devId, &result))
	{
		Handler_Capability* pCapab = NULL;
		cJSON* root = NULL;
		int size = 0;
		int i=0;
		
		root = cJSON_Parse(result);
		if(root == NULL)
		{
			RuleEngineLog(Error, " %s> cJSON Parse error: %s", strHandlerName, result);
			free(result);
			return false;
		}
#ifdef MULIT_THREAD
#else
		ThresholdCheck_Uninitialize();

		if(g_bThresholdCheck)
		{
			g_bThresholdCheck = false;
			pthread_join(g_thrThresholdCheck, NULL);
			g_thrThresholdCheck = 0;
		}
#endif
				
		size = cJSON_GetArraySize(root);
		if(size ==0)
		{   
			/*
			RuleEngineLog(Error, " %s> cJSON Array is empty: %s", strHandlerName, result);
			free(result);
			cJSON_Delete(root);
			*/
			/*If arrary is empty then trigger "Delete All"*/
			DeleteWholeHandlersThresholds(hk_set_thr_rep);
			free(result);
			cJSON_Delete(root);
			return true;
		}

		for(i=0; i<size; i++)		
		{
			cJSON* handlerNode = NULL;
			cJSON* idNode = NULL;
			cJSON* body = NULL;
			char* thresholdStr = NULL;
			cJSON* node = cJSON_GetArrayItem(root, i);
			if(node == NULL)
				continue;

			body = cJSON_GetObjectItem(node, AGENTINFO_BODY_STRUCT);
			if(body == NULL)
				body = cJSON_GetObjectItem(node, AGENTINFO_CONTENT_STRUCT);
			if(body == NULL)
				continue;
					
			handlerNode = cJSON_GetObjectItem(body, AGENTINFO_HANDLERNAME);
			if(handlerNode == NULL)
				continue;

			idNode = cJSON_GetObjectItem(body, AGENTINFO_DEVICEID);
			if(idNode == NULL)
				pCapab = FindHandlerCapability(handlerNode->valuestring, g_pCapablityList);
			else
				pCapab = FindHandlerCapabilityEx(idNode->valuestring, handlerNode->valuestring, g_pCapablityList);
			if(pCapab == NULL)
				continue;
#ifdef MULIT_THREAD
			if(pCapab->pHandlerKernel)
			{
				/*Stop threshold check thread*/
				HandlerKernelEx_StopThresholdCheck(pCapab->pHandlerKernel);
				/*setup threshold rule*/
				thresholdStr = cJSON_PrintUnformatted(node);
				HandlerKernelEx_SetThreshold(pCapab->pHandlerKernel, hk_set_thr_rep, thresholdStr);
				free(thresholdStr);
				/*register the threshold check callback function to handle trigger event*/
				HandlerKernelEx_SetThresholdTrigger(pCapab->pHandlerKernel, on_threshold_triggered);
				/*Restart threshold check thread*/
				HandlerKernelEx_StartThresholdCheck(pCapab->pHandlerKernel);
			}	
#else
			{
				char* response = NULL;
				char* warnmsg = NULL;
				bool bRet = false;
				Handler_info_ex tmpHandlerInfo;

				/*setup threshold rule*/
				if(pCapab->pThresholdList == NULL)
					pCapab->pThresholdList =  HandlerThreshold_CreateThrList();

				thresholdStr = cJSON_PrintUnformatted(node);
				bRet = ThresholdCheck_SetThreshold(pCapab->pThresholdList, thresholdStr, &response, &warnmsg);
				free(thresholdStr);

				memcpy(&tmpHandlerInfo, &g_HandlerInfo, sizeof(Handler_info_ex));
				strcpy(tmpHandlerInfo.Name, pCapab->Name);

				if(response != NULL)
				{
					if(g_HandlerInfo.sendcbf && strlen(response) > 0)
						g_HandlerInfo.sendcbf(&tmpHandlerInfo, hk_set_thr_rep, response, strlen(response), NULL, NULL);
					free(response);
				}

				if(warnmsg)
				{
					if(g_HandlerInfo.sendeventcbf && strlen(warnmsg) > 0)
						g_HandlerInfo.sendeventcbf(/*Handler Info*/&tmpHandlerInfo, Severity_Informational, /*message data*/warnmsg, /*message length*/strlen(warnmsg), /*preserved*/NULL, /*preserved*/NULL);
					free(warnmsg);
				}
			}
#endif
		}
		free(result);
		cJSON_Delete(root);
#ifdef MULIT_THREAD
#else
		ThresholdCheck_Initialize(on_threshold_triggered);

		g_bThresholdCheck = true;
		if (pthread_create(&g_thrThresholdCheck, NULL, RuleEngine_ThresholdCheckThread, NULL) != 0)
		{
			g_thrThresholdCheck = 0;
			g_bThresholdCheck = false;
		}
#endif
		return true;
	}
	else
	{
		RuleEngineLog(Error, " %s> Parse Set Threshold Command error: %s", strHandlerName, (char*) data);
		return false;
	}
}

bool DeleteThresholdRule(char* data)
{
	char* result = NULL;
	if(RuleParser_ParseThrInfo(data, g_HandlerInfo.agentInfo->devId, &result))
	{
		Handler_Capability* pCapab = NULL;
		cJSON* root = NULL;
		int size = 0;
		int i=0;
		
		root = cJSON_Parse(result);
		if(root == NULL)
		{
			RuleEngineLog(Error, " %s> cJSON Parse error: %s", strHandlerName, result);
			free(result);
			return false;
		}
		
#ifdef MULIT_THREAD
#else
		ThresholdCheck_Uninitialize();

		if(g_bThresholdCheck)
		{
			g_bThresholdCheck = false;
			pthread_join(g_thrThresholdCheck, NULL);
			g_thrThresholdCheck = 0;
		}
#endif

		size = cJSON_GetArraySize(root);
		if(size ==0)
		{
			/*
			RuleEngineLog(Error, " %s> cJSON Array is empty: %s", strHandlerName, result);
			free(result);
			cJSON_Delete(root);
			*/
			/*If arrary is empty then trigger "Delete All"*/
			DeleteWholeHandlersThresholds(hk_del_thr_rep);
			free(result);
			cJSON_Delete(root);
			return true;
		}

		for(i=0; i<size; i++)		
		{
			cJSON* handlerNode = NULL;
			cJSON* idNode = NULL;
			cJSON* body = NULL;
			char* thresholdStr = NULL;
			cJSON* node = cJSON_GetArrayItem(root, i);
			if(node == NULL)
				continue;

			body = cJSON_GetObjectItem(node, AGENTINFO_BODY_STRUCT);
			if(body == NULL)
				body = cJSON_GetObjectItem(node, AGENTINFO_CONTENT_STRUCT);
			if(body == NULL)
				continue;

			handlerNode = cJSON_GetObjectItem(body, AGENTINFO_HANDLERNAME);
			if(handlerNode == NULL)
				continue;
			
			if(strcmp(handlerNode->valuestring, "All") == 0)
			{
				idNode = cJSON_GetObjectItem(body, AGENTINFO_DEVICEID);
				if(idNode == NULL)
					continue;
				else
					DeleteDeviceThresholds(idNode->valuestring, hk_del_thr_rep);
			}
			else
			{
				idNode = cJSON_GetObjectItem(body, AGENTINFO_DEVICEID);
				if(idNode == NULL)
					pCapab = FindHandlerCapability(handlerNode->valuestring, g_pCapablityList);
				else
					pCapab = FindHandlerCapabilityEx(idNode->valuestring, handlerNode->valuestring, g_pCapablityList);
				if(pCapab == NULL)
					continue;
#ifdef MULIT_THREAD
				if(pCapab->pHandlerKernel)
				{
					/*Stop threshold check thread*/
					HandlerKernelEx_StopThresholdCheck(pCapab->pHandlerKernel);
					/*clear threshold check callback function*/
					HandlerKernelEx_SetThresholdTrigger(pCapab->pHandlerKernel, NULL);
					/*Delete all threshold rules*/
					HandlerKernelEx_DeleteAllThreshold(pCapab->pHandlerKernel, hk_del_thr_rep);
				}
#else
				if(pCapab->pThresholdList)
				{
					char* buffer = (char*)calloc(1, 1024);
					ThresholdCheck_WhenDelThrCheckNormal(pCapab->pThresholdList, &buffer,1024);

					ThresholdCheck_DeleteAllThrNode(pCapab->pThresholdList);
					free(pCapab->pThresholdList); 
					pCapab->pThresholdList = NULL;

					if(strlen(buffer) > 0)
					{
						int len = strlen(buffer) + 70;
						char* message = NULL;
						Handler_info_ex tmpHandlerInfo;
						message = (char*)calloc(1, len);
						sprintf(message,"{\"subtype\":\"THRESHOLD_CHECK_INFO\",\"thrCheckStatus\":\"True\",\"msg\":\"%s\"}", buffer); /*for custom handler*/
						
						memcpy(&tmpHandlerInfo, &g_HandlerInfo, sizeof(Handler_info_ex));
						strcpy(tmpHandlerInfo.Name, pCapab->Name);

						if(tmpHandlerInfo.sendeventcbf && strlen(message) > 0)
							tmpHandlerInfo.sendeventcbf(/*Handler Info*/&tmpHandlerInfo, Severity_Informational, /*message data*/message, /*message length*/strlen(message), /*preserved*/NULL, /*preserved*/NULL);
						free(message);
					}
					free(buffer);
				}
#endif
			}	
		}
		free(result);
		cJSON_Delete(root);

#ifdef MULIT_THREAD
#else
		ThresholdCheck_Initialize(on_threshold_triggered);

		g_bThresholdCheck = true;
		if (pthread_create(&g_thrThresholdCheck, NULL, RuleEngine_ThresholdCheckThread, NULL) != 0)
		{
			g_thrThresholdCheck = 0;
			g_bThresholdCheck = false;
		}
#endif
		return true;
	}
	else
	{
		RuleEngineLog(Error, " %s> Parse Set Threshold Command error: %s", strHandlerName, (char*) data);
		return false;
	}
}

AGENT_SEND_STATUS SendMessage( HANDLE const handle, int enum_act, 
									   void const * const requestData, unsigned int const requestLen, 
									   void *pRev1, void* pRev2 )
{
	Handler_info* hInfo = (Handler_info*)handle;
	RuleEngineLog(Debug, " %s> SendMessage:[%s, %d, %s]", strHandlerName, hInfo->Name, enum_act, requestData);
	return cagent_success;
}

AGENT_SEND_STATUS SendEventNotify( HANDLE const handle, HANDLER_NOTIFY_SEVERITY severity,
										 void const * const requestData, unsigned int const requestLen, 
										 void *pRev1, void* pRev2 )
{
	//Handler_info* hInfo = (Handler_info*)handle;
	//RuleEngineLog(Debug, " %s> SendMessage:[%s, %d, %s]", strHandlerName, hInfo->Name, severity, requestData);
	if(g_HandlerInfo.sendeventcbf)
		g_HandlerInfo.sendeventcbf(&g_HandlerInfo, severity, requestData, requestLen, pRev1, pRev2);
	return cagent_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  handler_success
 *           handler_fail
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *handler )
{
	Handler_info_ex* tmpinfo = NULL;
	if( handler == NULL )
		return handler_fail;

	tmpinfo = (Handler_info_ex*)handler;

	g_ruleenginehandlerlog = tmpinfo->loghandle;
	// 1. Topic of this handler
	snprintf( tmpinfo->Name, sizeof(tmpinfo->Name), "%s", strHandlerName );
	
	// 2. Copy agent info 
	memcpy(&g_HandlerInfo, tmpinfo, sizeof(Handler_info_ex));
#ifdef MULIT_THREAD
	memset(&g_thrHandlerInfo, 0, sizeof(Handler_info_ex));
	memcpy(&g_thrHandlerInfo, handler, sizeof(HANDLER_INFO));

	/*todo: redirect send callback*/
	g_thrHandlerInfo.sendcbf = SendMessage;
	g_thrHandlerInfo.sendeventcbf = SendEventNotify;
	g_thrHandlerInfo.sendcapabilitycbf = NULL;
	g_thrHandlerInfo.sendreportcbf = NULL;
	g_thrHandlerInfo.sendcustcbf = NULL;
	g_thrHandlerInfo.subscribecustcbf = NULL;
#else
#endif

	// 3. Callback function -> Send JSON Data by this callback function
	g_CurHandler = HandlerKernelEx_Initialize(handler);

	g_status = handler_status_no_init;

	pthread_mutex_init(&g_HandlerListMux, NULL);

	pthread_mutex_init(&g_SendCapablityMux, NULL);

	g_pCapablityList = NULL;

	return g_CurHandler != NULL?handler_success:handler_fail;
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
	g_ruleenginehandlerlog = NULL;
	pthread_mutex_lock(&g_SendCapablityMux);
	if(g_Capability)
		IoT_ReleaseAll(g_Capability);
	g_Capability = NULL;
	pthread_mutex_unlock(&g_SendCapablityMux);

	if(g_bEIService)
	{
		g_bEIService = false;
		UninitServiceSDKHandler();
	}

	if(g_bWAPI)
	{
		g_bWAPI = false;
		UninitSenHubHandle();
	}

	if(g_pCapablityList)
	{
		ClearHandlerCapability(&g_pCapablityList);
	}
#ifdef MULIT_THREAD
#else
	ThresholdCheck_Uninitialize();
#endif
	if(g_CurHandler)
	{
		pthread_mutex_destroy(&g_SendCapablityMux);
		pthread_mutex_destroy(&g_HandlerListMux);
		HandlerKernelEx_Uninitialize(g_CurHandler);
		g_CurHandler = NULL;
	}
	g_pCapablityList = NULL;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
 *  Input :  None
 *  Output: HANDLER_THREAD_STATUS pOutStatus       // cagent handler status
 *  Return:  handler_success
 *			 handler_fail
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	int iRet = handler_fail; 
	//RuleEngineLog(Debug, " %s> Get Status", strHandlerName);
	if(!pOutStatus) return iRet;
	/*user need to implement their thread status check function*/
	*pOutStatus = g_status;
	
	iRet = handler_success;
	return iRet;
}

/* **************************************************************************************
 *  Function Name: Handler_OnStatusChange
 *  Description: CAgent status change notify.
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  None
 * ***************************************************************************************/
void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *handler )
{
	
}

/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start handler thread
 *  Input :  None
 *  Output: None
 *  Return:  handler_success
 *           handler_fail
 * ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	g_status = handler_status_start;
	
	g_bEIService = InitServiceSDKHandler(Handler_RecvCapability, Handler_RecvData, g_ruleenginehandlerlog) == 1?true:false;

	g_bWAPI = InitSenHubHandle(Handler_RecvCapability, Handler_RecvData, g_ruleenginehandlerlog) == 1?true:false;

#ifdef MULIT_THREAD

#else
	
	ThresholdCheck_Initialize(on_threshold_triggered);

	g_bThresholdCheck = true;
	if (pthread_create(&g_thrThresholdCheck, NULL, RuleEngine_ThresholdCheckThread, NULL) != 0)
	{
		g_thrThresholdCheck = 0;
		g_bThresholdCheck = false;
	}
#endif

	if(g_CurHandler)
		HandlerKernelEx_Start(g_CurHandler);
	return cagent_success;
}


/* **************************************************************************************
 *  Function Name: Handler_Stop
 *  Description: Stop handler thread
 *  Input :  None
 *  Output: None
 *  Return:  handler_success
 *           handler_fail
 * ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	g_status = handler_status_stop;

	if(g_bEIService)
	{
		g_bEIService = false;
		UninitServiceSDKHandler();
	}

	if(g_bWAPI)
	{
		g_bWAPI = false;
		UninitSenHubHandle();
	}

#ifdef MULIT_THREAD

#else
	
	ThresholdCheck_Uninitialize();

	if(g_bThresholdCheck)
	{
		g_bThresholdCheck = false;
		pthread_join(g_thrThresholdCheck, NULL);
		g_thrThresholdCheck = 0;
	}
#endif

	

	if (g_CurHandler)
		HandlerKernelEx_Stop(g_CurHandler);

	return cagent_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Received Packet from Server
 *  Input : char * const topic, 
 *			void* const data, 
 *			const size_t datalen
 *  Output: void *pRev1, 
 *			void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int cmdID = 0;
	char sessionID[MAX_SESSION_LEN] = {0};
	printf(" >Recv Topic [%s] Data %s\n", topic, (char*) data );
	
	/*Parse Received Command*/
	if(HandlerKernelEx_ParseRecvCMDWithSessionID((char*)data, &cmdID, sessionID) != handler_success)
		return;
	switch(cmdID)
	{
	case hk_set_thr_req:
		{
			char repMsg[256] = {0};
			if(SetThresholdRule((char*) data))
			{
				sprintf(repMsg, "%s", "Threshold rule apply OK!");
				SendSetThresholdResult(hk_set_thr_rep, repMsg);
			}
			else
			{
				sprintf(repMsg, "%s", "Threshold apply failed!");
				SendSetThresholdResult(hk_set_thr_rep, repMsg);
			}
		}
		break;
	case hk_del_thr_req:
		{
			char repMsg[256] = {0};
			if(DeleteThresholdRule((char*) data))
			{
				sprintf(repMsg, "%s", "Delete threshold successfully!");
				SendSetThresholdResult(hk_del_thr_rep, repMsg);
			}
			else
			{
				sprintf(repMsg, "%s", "Delete threshold failed!");
				SendSetThresholdResult(hk_del_thr_rep, repMsg);
			}
		}
		break;
	case hk_auto_upload_req:
	case hk_get_sensors_data_req:
	case hk_set_sensors_data_req:
	default:
		{
			/* Send command not support reply message*/
			char repMsg[32] = {0};
			int len = 0;
			sprintf( repMsg, "{\"errorRep\":\"Unknown cmd!\"}" );
			len= strlen( "{\"errorRep\":\"Unknown cmd!\"}" ) ;
			if ( g_HandlerInfo.sendcbf )  g_HandlerInfo.sendcbf( &g_HandlerInfo, hk_error_rep, repMsg, len, NULL, NULL );
		}
		break;
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char ** : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	/*Hold capability until stable*/
	//char* result = NULL;
	int len = 0;

	printf("> %s Get Capability\n", strHandlerName);

	if(!pOutReply) return len;

	//if(g_Capability == NULL)
	//	g_Capability = CreateCapability();

	//result = IoT_PrintCapability(g_Capability);

	//len = strlen(result);
	//*pOutReply = (char *)calloc(1, len + 1);
	//strcpy(*pOutReply, result);

	SendCapability();

	//free(result);
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
	//HandlerKernelEx_AutoReportStart(g_CurHandler, pInQuery);
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : char *pInQuery, if *pInQuery = NULL, then stop all upload message.
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	//HandlerKernelEx_AutoReportStop(g_CurHandler, pInQuery);
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
	RuleEngineLog(Debug, "> %s Free Allocated Memory", strHandlerName);

	if(pInData)
	{
		free(pInData);
		pInData = NULL;
	}
}

void HANDLER_API Handler_RecvCapability(HANDLE const handler, void const * const requestData, unsigned int const requestLen)
{
	char name[MAX_TOPIC_LEN] = {0};
	char agentID[DEF_DEVID_LENGTH] = {0};

	HANDLER_INFO* phandler = (HANDLER_INFO*)handler;

	if(requestData == NULL || requestLen == 0)
		return;

	if(phandler)
	{
		if(phandler->agentInfo)
			if(strlen(phandler->agentInfo->devId)>0)
				strcpy(agentID, phandler->agentInfo->devId);
	}

	if(!transfer_get_ipso_handlername( (char*)requestData, name))
	{
		return;
	}
	if(strcmp(strHandlerName, name)==0)
		return;
	if(strlen(agentID) > 0)
		AddHandlerCapabilityEx(agentID, name, (char*)requestData, &g_pCapablityList);
	else
		AddHandlerCapability(name, (char*)requestData, &g_pCapablityList);
}

void HANDLER_API Handler_RecvData(HANDLE const handler, void const * const requestData, unsigned int const requestLen)
{
	Handler_Capability *pHandlerCap = NULL;
	char strHandlerName[MAX_TOPIC_LEN] = {0};
	char agentID[DEF_DEVID_LENGTH] = {0};
	MSG_CLASSIFY_T* newRoot = NULL;
	HANDLER_INFO* phandler = (HANDLER_INFO*)handler;

	if(requestData == NULL || requestLen == 0)
		return;

	if(phandler)
	{
		if(phandler->agentInfo)
			if(strlen(phandler->agentInfo->devId)>0)
				strcpy(agentID, phandler->agentInfo->devId);
	}

	if(!transfer_get_ipso_handlername( (char*)requestData, strHandlerName))
	{
		return;
	}
	if(strlen(agentID) > 0)
		pHandlerCap = FindHandlerCapabilityEx(agentID, strHandlerName, g_pCapablityList);
	else
		pHandlerCap = FindHandlerCapability(strHandlerName, g_pCapablityList);

	if(pHandlerCap == NULL)
		return;

	


#ifdef MULIT_THREAD
	if(pHandlerCap->pHandlerKernel)
		HandlerKernelEx_LockCapability(pHandlerCap->pHandlerKernel);
	newRoot = IoT_FindGroup(pHandlerCap->pCapability, strHandlerName);
	if(newRoot)
		transfer_parse_ipso( (char*)requestData, newRoot);
	if(pHandlerCap->pHandlerKernel)
		HandlerKernelEx_UnlockCapability(pHandlerCap->pHandlerKernel);
#else
	if(pHandlerCap->pThresholdList)
	{
		ThresholdCheck_UpdateValue(pHandlerCap->pThresholdList, (char*)requestData, agentID);
	}
#endif
}
