#include "srp/susiaccess_handler_api.h"
#include "OTAHandler.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#include "cJSON.h"
#include "SUEClient.h"
#include "ErrorDef.h"
#include "util_path.h"
#include "iniparser.h"
#include "cp_fun.h"
#include "cjson_ipso_util.h"

//-----------------------------------------------------------------------------
// Logger defines:
//-----------------------------------------------------------------------------
#define LOG_TAG	"OTA"
#include "Log.h"

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define VA_ARGS_TO_STRING(...) #__VA_ARGS__
#define DEFINE_TO_STRING(x) VA_ARGS_TO_STRING(x)

#define OTA_None					0x00
#define OTA_SUE						0x01

#define NoneStr					"none"
#define OTASUEStr					"ota_sue"

typedef struct ota_capability_info_t{
	char libVersion[256];
	char funcsStr[256];
	char os[256];
	char arch[256];
	char tags[1024];
	unsigned int funcsCode;
} ota_capability_info_t;

#define AGENTINFO_BODY_STRUCT						"susiCommData"
#define AGENTINFO_CMDTYPE							"commCmd"
#define OTA_INFOMATION								"Information"
#define OTA_E_FLAG									"e"
#define OTA_BN_FLAG									"bn"
#define OTA_VERSION_FLAG							"version"
#define OTA_NONSENSORDATA_FLAG						"nonSensorData"
#define OTA_FUNCTION_LIST							"functionList"
#define OTA_FUNCTION_CODE							"functionCode"
#define OTA_ERROR_REP								"errorRep"
#define OTA_OS										"os"
#define OTA_ARCHITECTURE							"architecture"
#define OTA_TAGS									"tags"
#define DEF_ENV_CONFIG_NAME							"env_config.ini"

#define cagent_ota_request	2103
#define cagent_ota_action	31003
typedef enum
{
	unknown_cmd = 0,
	ota_error_rep = 100,
	ota_get_capability_req = 521,
	ota_get_capability_rep = 522
}susi_comm_cmd_t;

//#define DEF_PUBLISH_TOPIC	"/sueclient/admin/%s/pub"
//#define DEF_SUBSCRIBE_TOPIC	"/sueserver/admin/%s/pub"
#define DEF_PUBLISH_TOPIC	"/wisepaas/general/ota/%s/otaack"
#define DEF_SUBSCRIBE_TOPIC	"/wisepaas/general/ota/%s/otareq"
#define DEF_PKG_TYPE		"OTAPkg"
//#define DEF_OTA_CFG_FORMAT "[SUECCore]\nPkgRootPath=%s"
const char strHandlerName[MAX_TOPIC_LEN] = {"OTAHandler"};
const int iRequestID = cagent_ota_request;
const int iActionID = cagent_ota_action;
//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
bool g_bSUEClientInit = false;
bool g_bSUEClientStart = false;
char g_ConfigFile[MAX_PATH] = {0};
//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static Handler_info  g_HandlerInfo;
static HANDLER_THREAD_STATUS g_status = handler_status_no_init;
static ota_capability_info_t g_cpbInfo;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;
static HandlerSendEventCbf g_sendeventcbf = NULL;
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

static void tolowerStr(char* str)
{
	int len = strlen(str);
	int i;

	for (i = 0; i < len; i++) {
		str[i] = tolower(str[i]);
	}
}

// append tags and check ',' in last char
#define APPEND_TAGS(dst, tags) do {\
	if (dst[0] != '\0' && dst[strlen(dst)-1] != ',') \
		strncat(dst, ",", sizeof(dst)-strlen(dst)-1); \
	strncat(dst, tags, sizeof(dst)-strlen(dst)-1);\
} while(0);

static int readConfig(char* file)
{
	dictionary* dic;
	char* value;
	FILE* iniFile;
	char runtimeTags[64] = {0};

	if(SUEGetSysOS(g_cpbInfo.os, sizeof(g_cpbInfo.os)) != SUEC_SUCCESS) {
		memset(g_cpbInfo.os, 0, sizeof(g_cpbInfo.os));
	}

	if(SUEGetSysArch(g_cpbInfo.arch, sizeof(g_cpbInfo.arch)) != SUEC_SUCCESS) {
		memset(g_cpbInfo.arch, 0, sizeof(g_cpbInfo.arch));
	}

	dic = iniparser_load(file);
	if (!dic) { // if no ini file, create it
		LOGI("Create config [%s]", file);
		iniFile = fopen(file, "w");
		if (iniFile) {
			fwrite("[OTA]\r\n", strlen("[OTA]\r\n"), 1, iniFile);
		}
		fclose(iniFile);
		iniFile = NULL;
		
		// load ini again
		dic = iniparser_load(file);
		if (!dic) {
			LOGE("Read config [%s] failed", file);
			return -1;
		}
	}

	// get device tags
	// if has forceTags, tags = forceTags
	// if no forceTags, tags = buildinTags + runtimeTags + appendTags
	value = iniparser_getstring(dic, "OTA:forceTags", "");
	if (value[0] != '\0') { // has force tags
		strncpy(g_cpbInfo.tags, value, sizeof(g_cpbInfo.tags));
	} else { // no force tags
		// 1) check build time tags
		// you can define OTA_TAGS_CONFIG to customize ota tags, ex: make CFLAGS=-DOTA_TAGS_CONFIG="tag1,tag2"
#ifdef OTA_TAGS_CONFIG // config
		strncpy(g_cpbInfo.tags, DEFINE_TO_STRING(OTA_TAGS_CONFIG), sizeof(g_cpbInfo.tags));
#endif

		// 2) check run time tags
		// prepare runtimeTags
		if (strcmp(g_cpbInfo.arch, ARCH_X86) == 0) {
			strcpy(runtimeTags, "x86");
		} else if (strcmp(g_cpbInfo.arch, ARCH_X64) == 0) {
#ifdef WIN32
			strcpy(runtimeTags, "x64,x86");
#else
			strcpy(runtimeTags, "x64");
#endif
		} else {
			strcpy(runtimeTags, g_cpbInfo.arch);
		}

		// append runtimeTags
		if (runtimeTags[0] != '\0') {
			APPEND_TAGS(g_cpbInfo.tags, runtimeTags);
		}

		// 3) check append tags
		value = iniparser_getstring(dic, "OTA:appendTags", "");
		if (value[0] != '\0') {
			APPEND_TAGS(g_cpbInfo.tags, value);
		}
	}
	g_cpbInfo.tags[sizeof(g_cpbInfo.tags)-1] = '\0';
	tolowerStr(g_cpbInfo.tags);
	LOGI("OTA tags: [%s]", g_cpbInfo.tags);

	// save build-in tags in ini file
#ifdef OTA_TAGS_CONFIG
	iniparser_set(dic, "OTA:buildinTags", DEFINE_TO_STRING(OTA_TAGS_CONFIG));
#endif

	iniparser_set(dic, "OTA:runtimeTags", runtimeTags);

	iniFile = fopen(file, "w");
	if (iniFile) {
		iniparser_dump_ini(dic, iniFile);
		fclose(iniFile);
	}

	// free ini dic
	iniparser_freedict(dic);

	return 0;
}

int Parser_PackCpbInfo(ota_capability_info_t * cpbInfo, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *root = NULL, *pSUSICommDataItem = NULL, *handlerName = NULL, *infoItem = NULL;
	cJSON *subItem = NULL, *eItem = NULL;

	if(cpbInfo == NULL || outputStr == NULL)
		return outLen;

	root = cJSON_CreateObject();
	cJSON_AddObjectToObject(root, AGENTINFO_BODY_STRUCT, pSUSICommDataItem);
	cJSON_AddObjectToObject(pSUSICommDataItem, strHandlerName, handlerName);
	cJSON_AddObjectToObject(handlerName, OTA_INFOMATION, infoItem);

	eItem = cJSON_CreateArray();
	if (eItem) {
		cJSON_AddItemToObject(infoItem, OTA_E_FLAG, eItem);

		if (cpbInfo->libVersion[0] != '\0') {
			cJSON_IPSO_NewROStringItem(subItem, OTA_VERSION_FLAG, cpbInfo->libVersion);
			cJSON_AddItemToArray(eItem, subItem);
		}
		if (cpbInfo->funcsStr[0] != '\0') {
			cJSON_IPSO_NewROStringItem(subItem, OTA_FUNCTION_LIST, cpbInfo->funcsStr);
			cJSON_AddItemToArray(eItem, subItem);
		}
		if (cpbInfo->os[0] != '\0') {
			cJSON_IPSO_NewROStringItem(subItem, OTA_OS, cpbInfo->os);
			cJSON_AddItemToArray(eItem, subItem);
		}
		if (cpbInfo->arch[0] != '\0') {
			cJSON_IPSO_NewROStringItem(subItem, OTA_ARCHITECTURE, cpbInfo->arch);
			cJSON_AddItemToArray(eItem, subItem);
		}
		if (cpbInfo->tags[0] != '\0') {
			cJSON_IPSO_NewROStringItem(subItem, OTA_TAGS, cpbInfo->tags);
			cJSON_AddItemToArray(eItem, subItem);
		}

		cJSON_IPSO_NewRONumberItem(subItem, OTA_FUNCTION_CODE, cpbInfo->funcsCode);
		cJSON_AddItemToArray(eItem, subItem);
	}

	cJSON_AddStringToObject(infoItem, OTA_BN_FLAG, OTA_INFOMATION);
	cJSON_AddBoolToObject(infoItem, OTA_NONSENSORDATA_FLAG, 1);

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *) malloc(outLen);
	strcpy(*outputStr, out);
	free(out);

	LOGD("Parser_PackCpbInfo: outputStr=[%s]\n", *outputStr);

error:
	if (root)
		cJSON_Delete(root);
	LOGD("Parser_PackCpbInfo: outLen=[%d]\n", outLen);

	return outLen;
}

int Parser_PackErrorRep(char * errorStr, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(errorStr == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, OTA_ERROR_REP, errorStr);

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);
	printf("%s\n",out);
	free(out);
	return outLen;
}

bool ParseReceivedData(void* data, int datalen, int * cmdID)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return false;
	if(datalen<=0) return false;
	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	target = cJSON_GetObjectItem(body, AGENTINFO_CMDTYPE);
	if(target)
	{
		*cmdID = target->valueint;
	}
	cJSON_Delete(root);
	return true;
}


void GetCapability()
{
	char * cpbStr = NULL;
	int jsonStrlen = 0;
	/*power_capability_info_t cpbInfo;
	memset((char*)&cpbInfo, 0, sizeof(power_capability_info_t));
	CollectCpbInfo(&cpbInfo);*/

	jsonStrlen = Parser_PackCpbInfo(&g_cpbInfo, &cpbStr);
	if(jsonStrlen > 0 && cpbStr != NULL)
	{
		g_sendcbf(&g_HandlerInfo, ota_get_capability_rep, cpbStr, strlen(cpbStr)+1, NULL, NULL);
		if(cpbStr)free(cpbStr);
	}
	else
	{
		char * errorRepJsonStr = NULL;
		char errorStr[128];
		int jsonStrlen = 0;
		sprintf(errorStr, "Command(%d), Get capability error!", ota_get_capability_req);
		jsonStrlen = Parser_PackErrorRep(errorStr, &errorRepJsonStr);
		if(jsonStrlen > 0 && errorRepJsonStr != NULL)
		{
			g_sendcbf(&g_HandlerInfo, ota_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
		}
		if(errorRepJsonStr)free(errorRepJsonStr);
	}
}

int OTAOutputMsgCB(char *msg, int msgLen, void * userData)
{
	//printf("\n######DemoOutputMsgCB######\n");
	//printf("OutputMsg:%s\n",msg);
	//printf("############################\n\n");
	if(g_sendcustcbf)
	{
		char topic[128] = {0};
		sprintf(topic, DEF_PUBLISH_TOPIC, g_HandlerInfo.agentInfo->devId);
		g_sendcustcbf(NULL, 0, topic, msg, strlen(msg), NULL, NULL);
	}
	return 0;
}

int OTATaskStatusCB(PTaskStatusInfo pTaskStatusInfo, void * userData)
{
	//printf("\n######SUECTaskStatusCB######\n");
	if(pTaskStatusInfo)
	{
		if(pTaskStatusInfo->pkgName) printf("PkgName:%s\n", pTaskStatusInfo->pkgName);
		if(pTaskStatusInfo->taskType == TASK_DL) printf("TaskType:Download\n");
		if(pTaskStatusInfo->taskType == TASK_DP) printf("TaskType:Deploy\n");
		if(pTaskStatusInfo->taskType == TASK_UNKNOW) printf("TaskType:Unknow\n");
		switch(pTaskStatusInfo->statusCode)
		{
		case SC_QUEUE:
			{
				printf("Status: Queue\n");
				break;
			}
		case SC_START:
			{
				printf("Status: Start\n");
				break;
			}
		case SC_DOING:
			{
				if(pTaskStatusInfo->taskType == TASK_DL)
				{
					printf("Status: Doing");
					printf(" ,download %d%%\n", pTaskStatusInfo->u.dlPercent);
				}
				else if(pTaskStatusInfo->taskType == TASK_DP)
				{
					printf("Status: Doing");
					if(pTaskStatusInfo->u.msg)printf(" ,msg: %s\n", pTaskStatusInfo->u.msg);
					else printf("\n");
				}
				break;
			}
		case SC_FINISHED:
			{
				printf("Status: Finished\n");
				break;
			}
		default:
			{
				printf("Status:Error");
				printf(" ,msg: %d, %s\n", pTaskStatusInfo->errCode, SUECGetErrMsg(pTaskStatusInfo->errCode));
			}
		}
	}
	//printf("############################\n\n");
	return 0;
}

int OTAPkgDeployCheckCB(const void * const checkHandle, SUECNotifyDPCheckMsgCB notifyDpMsgCB, int * isQuit, void * userData)
{
	int iRet = -1;
	if(NULL == checkHandle || notifyDpMsgCB == NULL || NULL == isQuit) return iRet;
	{
		char dpEvent[256] = {0};
		int curPercent = 0;
		while((*isQuit) && curPercent<=100)
		{
			memset(dpEvent, 0, sizeof(dpEvent));
			sprintf(dpEvent, "Deploy percent %d%%.",curPercent);
			notifyDpMsgCB(checkHandle, dpEvent, strlen(dpEvent)+1);
			curPercent+=10;
			usleep(1000*1000);
		}
	}
	return 0;
}

int StartupSUEClient(char * devID, char * cfgFile)
{
	int iRet = 0;
	//if(!util_is_file_exist(cfgFile))
	//	GenerateOTACfg(cfgFile);
	iRet = SUECInit(devID, cfgFile, g_cpbInfo.tags);
	if(iRet == 0)
	{
		//Set suspend
		SUECSetOutputAct(OPAT_SUSPEND);
		//SUECSetOutputMsgCB(OTAOutputMsgCB, NULL);
		//SUECSetTaskStatusCB(OTATaskStatusCB, NULL);
		//SUECSetDpCheckCB(DEF_PKG_TYPE, OTAPkgDeployCheckCB, NULL);
		//iRet = SUECStart(NULL);
		g_bSUEClientInit = true;
	}
	return iRet;
}

int StopSUEClient()
{
	if(g_bSUEClientInit)
	{
		if(g_bSUEClientStart){
			SUECStop();
			g_bSUEClientStart = false;
		}
		SUECUninit();
		g_bSUEClientInit = false;
	}

	return 0;
}

void OnSUECRecv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2)
{
	LOGD("SUEClient >Recv Topic [%s] Data %s", topic, (char*) data );
	//if(g_bSUEClientInit)
	if(g_bSUEClientStart)
		SUECInputMsg(data, datalen);
}

void OnSUECStatus(bool bConnect, char const* devID)
{
	if(!g_bSUEClientInit)
		return;
	if(bConnect)
	{
		SUECSetOutputAct(OPAT_CONTINUE);
		//start first time
		if(g_bSUEClientStart == false)
		{
			//SUECSetOutputAct(OPAT_SUSPEND);
			SUECSetOutputMsgCB(OTAOutputMsgCB, NULL);
			SUECSetTaskStatusCB(OTATaskStatusCB, NULL);
			SUECSetDpCheckCB(DEF_PKG_TYPE, OTAPkgDeployCheckCB, NULL);
			SUECStart(NULL);
			g_bSUEClientStart = true;
			//iRet = SUECStart(NULL);
		}
		if(g_subscribecustcbf)
		{
			char topic[128] = {0};
			sprintf(topic, DEF_SUBSCRIBE_TOPIC, devID);
			g_subscribecustcbf( topic, OnSUECRecv);
		}
	}
	else
		SUECSetOutputAct(OPAT_SUSPEND);
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
	if( pluginfo == NULL )
		return handler_fail;

	// 1. Topic of this handler
	strncpy(pluginfo->Name, strHandlerName, sizeof(pluginfo->Name));
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	LOGD(" %s> Initialize", strHandlerName);
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

	util_path_combine(g_ConfigFile, pluginfo->WorkDir, DEF_ENV_CONFIG_NAME);
	memset(&g_cpbInfo, 0, sizeof(ota_capability_info_t));
	g_cpbInfo.funcsCode = OTA_SUE;
	strcpy(g_cpbInfo.funcsStr, OTASUEStr);

	readConfig(DEF_ENV_CONFIG_NAME);

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
	StopSUEClient();
	g_sendcbf = NULL;
	g_sendcustcbf = NULL;
	g_sendreportcbf = NULL;
	g_sendcapabilitycbf = NULL;
	g_subscribecustcbf = NULL;
	g_sendeventcbf = NULL;
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
	LOGD(" %s> Get Status", strHandlerName);
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
	LOGD(" %s> Update Status", strHandlerName);
	if(pluginfo)
		memcpy(&g_HandlerInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_HandlerInfo, 0, sizeof(HANDLER_INFO));
		strncpy( g_HandlerInfo.Name, strHandlerName, sizeof( g_HandlerInfo.Name));
		g_HandlerInfo.RequestID = iRequestID;
		g_HandlerInfo.ActionID = iActionID;
	}
	OnSUECStatus(pluginfo->agentInfo->status == AGENT_STATUS_ONLINE, pluginfo->agentInfo->devId);
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
	LOGD("> %s Start", strHandlerName);

	StartupSUEClient(g_HandlerInfo.agentInfo->devId, g_ConfigFile);

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
int HANDLER_API Handler_Stop( void )
{
	LOGD("> %s Stop", strHandlerName);
	StopSUEClient();
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
	int commCmd = unknown_cmd;

	LOGD(" >Recv Topic [%s] Data %s", topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &commCmd))
		return;

	switch(commCmd)
	{
	case ota_get_capability_req:
		{
			GetCapability();
			break;
		}
	default:
		{
			g_sendcbf(&g_HandlerInfo, ota_error_rep, "Unknown cmd!", strlen("Unknown cmd!")+1, NULL, NULL);
			break;
		}
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
	LOGD("> %s Start Report", strHandlerName);
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
	LOGD("> %s Stop Report", strHandlerName);
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
	int len = 0;
	char * cpbStr = NULL;
	int jsonStrlen = 0;
	//LOGD("> %s Get Capability", strHandlerName);
	if(!pOutReply) return len;

	jsonStrlen = Parser_PackCpbInfo(&g_cpbInfo, &cpbStr);
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
 *  Function Name: Handler_MemoryFree
 *  Description: free the mamory allocated for Handler_Get_Capability
 *  Input : char *pInData.
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_MemoryFree(char *pInData)
{
	LOGD("> %s Free Allocated Memory", strHandlerName);

	if(pInData)
	{
		free(pInData);
		pInData = NULL;
	}
	return;
}
