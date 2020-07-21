/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2016 by Zach Chih															     */
/* Modified Date: 2016/11/29 by Zach Chih															 */
/* Abstract     : DataSync                                   													*/
/* Reference    : None																									 */
/****************************************************************************/

#include "DataSync.h"
#include "unistd.h"
#include "sqlite3.h"
#include "util_path.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>
#include "ReadINI.h"
#include "HandlerKernel.h"
#include "IPSOParser.h"

#ifdef WIN32
	#include "wrapper.h"
#else
	#include "wrapper.h" //Add for OpenWRT
	#include <sys/vfs.h>
	#include <limits.h>
#endif

#define INVALID_CAPID	UINT32_MAX  /* 2^32-1 */
#define INVALID_DATAID	UINT64_MAX /* 2^64-1 */

#define cagent_action_general			2001
#define general_info_spec_rep			2052
#define general_info_upload_rep			2055

// redefine max topic len
#undef MAX_TOPIC_LEN
#define MAX_TOPIC_LEN DEF_MAX_PATH

#define DATASYNC_CAP_STR "{\"DataSync\":{\"RecoverProgress\":{\"bn\":\"RecoverProgress\",\"e\":["\
						 "{\"n\":\"total\",\"v\":%d,\"asm\":\"r\"},"\
						 "{\"n\":\"current\",\"v\":%d,\"asm\":\"r\"},"\
						 "{\"n\":\"start\",\"v\":%"PRId64",\"asm\":\"r\"},"\
						 "{\"n\":\"end\",\"v\":%"PRId64",\"asm\":\"r\"},"\
						 "{\"n\":\"batchSize\",\"v\":%d,\"asm\":\"r\"},"\
						 "{\"n\":\"sendInterval\",\"v\":%d,\"asm\":\"r\"},"\
						 "{\"n\":\"keepTime\",\"v\":%"PRId64",\"asm\":\"r\"}]}}}"

typedef struct {
	// for progress report
	int total;
	int current;
	int reportRunning;
	int stopInsert;
} DataSyncCacheType;

typedef struct {
	double filesize; // MB
	long limit_size;

	int duration; // HR
	int64_t limit_duration;
	int64_t limit_time;

	int64_t keeptimeSec;  // Sec.
	int64_t keeptimeMS;  // MSec.
	int batchSize; // Item
	int wrapSendInterval; // Sec.
	int64_t limit_preserve; // MB
} DataSyncConfigType;

typedef struct {
	uint32_t capID;
	int64_t ts;
	char handler[MAX_TOPIC_LEN];
	char topic[MAX_TOPIC_LEN];
	char* capStr;
} Cap_Entry;

typedef struct {
	uint64_t dataID;
	uint32_t DCapID;
	int type;
	int64_t ts;
	char topic[MAX_TOPIC_LEN];
	char* dataStr;
} Data_Entry;

typedef struct {
	int cap_number;
	int cap_index;
	int* cap_ids;
	char deviceID[MAX_TOPIC_LEN]; // save the deviceID from cap topic
	Cap_Entry cap;
	Data_Entry* data;
	int data_count;
} Batch_Entry;

typedef struct{
   pthread_t threadHandler;
   bool isThreadRunning;
} thread_context_t;

//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
int do_recover();
int updateMonitor();


//-----------------------------------------------------------------------------
// Global Variable
//-----------------------------------------------------------------------------
// config from ini
static DataSyncConfigType g_config;
// status cache
static DataSyncCacheType g_cacheInfo;
static Handler_info_ex g_PluginInfo;
static void* g_handlerKernel = NULL;
static bool blost_on = false;
static bool cond = true;
static bool bComplete = true;
static bool bUninit = false;
static int64_t lost_t=0,next_lost_t=0,recon_t=0;
static int printCount = 0;

#ifdef ANDROID
typedef pthread_t sp_pthread_t;
static int pthread_cancel(sp_pthread_t thread) {
        return (kill(thread, SIGTERM));
}
#endif

// thread related
static pthread_mutex_t g_sendBatchMutex;
static pthread_cond_t g_sendBatchCond;
// for send batch thread
static thread_context_t g_SendBatchContex;

// save publish callback
static PUBLISHCB publish_CB = NULL;

// for database
static sqlite3 *db = NULL;
static char *db_path = NULL;

// report buffer
static char g_repBuffer[512];
static char *g_workdir = NULL;

// id counter
static uint64_t g_dataIDCount = 0;
// protect id counter
static pthread_mutex_t g_dbMux;
static MSG_CLASSIFY_T* g_capRoot = NULL;

static susiaccess_packet_body_t g_publishPacket;
static unsigned int g_packetContentLen = 0;

#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		fprintf(stderr, "[DataSync] DllInitializer\n");
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL) // Dynamic load
		{
			// Initialize your stuff or whatever
			// Return FALSE if you don't want your module to be dynamically loaded
		}
		else // Static load
		{
			// Return FALSE if you don't want your module to be statically loaded
		}
	}

	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
	{
		printf("[DataSync] DllFinalizer\n");
		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			DataSync_Uninitialize();
		}
		else // Process is terminating
		{
			// Cleanup
			DataSync_Uninitialize();
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
	fprintf(stderr, "[DataSync] DllInitializer\n");
}

__attribute__((destructor))
/**
 * It is called when shared lib is being unloaded.
 *
 */
static void Finalizer()
{
	fprintf(stderr, "[DataSync] DllFinalizer\n");
	DataSync_Uninitialize();
}
#endif

//-----------------------------------------------------------------------------
// Callbacks
//-----------------------------------------------------------------------------

/*
	do SQL command with error handle
	return SQLITE_OK for success
*/
int do_sqlite(sqlite3 *db,
			  const char *sql,
			  int (*callback)(void*, int, char**, char**),
			  void *param,
			  char* ok_msg,
			  int ignoreError)
{
	int rc;
	char *zErrMsg = NULL;

#ifdef DEBUG_TEST
	LOGD("do_sqlite(%s)", sql);
#endif

	rc = sqlite3_exec(db, sql, callback, param, &zErrMsg);
	if(rc != SQLITE_OK){
		if (!ignoreError) {
			LOGE("SQL error: command=[%s]", sql);
			LOGE("zErrMsg=[%s]", zErrMsg);
		}
		sqlite3_free(zErrMsg);
	} else if (ok_msg) {
		LOGD("SQL success: msg=[%.164s]", ok_msg);
	}

	return rc;
}

/*
	delete CapID in TB_Cap that is not in TB_Data
*/
void reset_CapID()
{
	char sqlite_str[256] = {0};

	LOGD("%s call", __FUNCTION__);

	// delete all CapID which has no Rep to reference
	snprintf(sqlite_str, sizeof(sqlite_str), "delete from TB_Cap where CapID not in (select DCapID from TB_Data);");
	do_sqlite(db, sqlite_str, NULL, NULL, NULL, 1);
}


/*
	save CapID that select from database

 * Arguments:
 *
 *     param - The argument pass by sqlite3_exec()
 *      argc - The number of columns in the result set
 *      argv - The row's data
 * azColName - The column names
 */
int reload_CapID_cb(void *param, int argc, char **argv, char **azColName)
{
	uint32_t* capIDCount = (uint32_t*) param;

	if (argc <= 0) { // not found
		*capIDCount = 0; // you are the first here
	} else {
		*capIDCount = strtoul(argv[0], NULL, 10);
	}

	return SQLITE_OK;
}

bool read_INI(char *workdir)
{
	FILE *fPtr;

	char *temp_INI_name=NULL;
	char *iniPath=NULL;
	char buffer[128];
	const char *strValue = NULL;
	char *endptr = NULL;

	iniPath=(char *)calloc(1,strlen(workdir)+strlen("DataSync.ini")+1);
	temp_INI_name=(char *)calloc(strlen("DataSync")+1+4,sizeof(char));	//+4 for ".ini"
	strcpy(temp_INI_name,"DataSync");
	strcat(temp_INI_name,".ini");
	strcpy(iniPath,workdir);
	strcat(iniPath,temp_INI_name);

	 fPtr = fopen(iniPath, "r");
	 if(fPtr)
	 {
			printf("INI Opened Successfully\n");
			g_config.filesize = GetIniKeyDoubleDef((char*) "DataSync", (char*) "Storage", iniPath, 0.0);
			g_config.duration = GetIniKeyIntDef((char*) "DataSync", (char*) "Duration", iniPath, 0);
			g_config.keeptimeSec = (int64_t) GetIniKeyIntDef((char*) "DataSync", (char*) "RetainDWTP", iniPath, 60); // default 60 SEC.
			g_config.keeptimeMS = g_config.keeptimeSec * 1000;
			g_config.batchSize = GetIniKeyIntDef((char*) "DataSync", (char*) "BatchSize", iniPath, 5); // default 5 items.
			g_config.wrapSendInterval = GetIniKeyIntDef((char*) "DataSync", (char*) "WrapSendInterval", iniPath, 5); // default 5 SEC.

			strValue = GetIniKeyStringDef("DataSync", "PreserveStorage", iniPath, buffer, sizeof(buffer), "200"); // MB
			g_config.limit_preserve = strtoll(strValue, &endptr, 10);
			if (g_config.limit_preserve < 0 || strValue == endptr) { // invalid value
				g_config.limit_preserve = 200;
				LOGI("overflow, set preserve as default value %"PRId64" MB", g_config.limit_preserve);
			}
			LOGI("Storage=%f, Duration=%d, RetainDWTP=%"PRId64", BatchSize=%d, Interval=%d, preserve=%"PRId64"\n",
					g_config.filesize, g_config.duration, g_config.keeptimeMS, g_config.batchSize, g_config.wrapSendInterval, g_config.limit_preserve);

			if(iniPath)
				free(iniPath);
			if(temp_INI_name)
				free(temp_INI_name);
			fclose(fPtr);
			return true;
	 }
	 else
	 {
			printf("INI Opened Unsuccessfully...\n");
			if(iniPath)
				free(iniPath);
			if(temp_INI_name)
				free(temp_INI_name);
			return false;
	 }
}

//-----------------------------------------------------------------------------
// Threads
//-----------------------------------------------------------------------------
/*
	cond=true, bComplete=true
		recover complete
		pthread_cond_timedwait()

	cond=true, bComplete=false
		recover in process
		do_recover()

	cond=false, bComplete=true
		wait connection to recover
		pthread_cond_timedwait()

	cond=false, bComplete=false
		recover fail, wait connection to recover again
		pthread_cond_timedwait()

*/
void* SendBatchThreadStart(void *args)
{
	struct timespec timeToWait;

	while(g_SendBatchContex.isThreadRunning)
	{
		if(cond && blost_on && (!bComplete) && (!bUninit)) {
			do_recover();
		}

		timeToWait.tv_sec = time(NULL) + 3600; // wait 1 hour
		timeToWait.tv_nsec = 0;

		pthread_mutex_lock(&g_sendBatchMutex);
		if (!cond || bComplete) { // if no batch send worker
			pthread_cond_timedwait(&g_sendBatchCond, &g_sendBatchMutex, &timeToWait);
		}
		pthread_mutex_unlock(&g_sendBatchMutex);
	}
	return 0;
}

#ifdef DEBUG_TEST
/*
	A thread to simulation a disconnect/connect event
*/
void* testThread(void *args) {
	//do_recover();
	int i;

	DataSync_Set_LostTime(time(NULL));
	for (i = 0; i < 40; i++) {
		fprintf(stderr, "------------------------- testThread start %d\n", i);
		sleep(1);
	}

	DataSync_Set_ReConTime(time(NULL));
	fprintf(stderr, "------------------------- testThread end %d\n", i);
	/*
	for (i = 0; i < 40; i++) {
		fprintf(stderr, "------------------------- testThread wait %d\n", i);
		sleep(1);
	}

	DataSync_Set_LostTime(time(NULL));
	for (i = 0; i < 20; i++) {
		fprintf(stderr, "------------------------- testThread start2 %d\n", i);
		sleep(1);
	}

	DataSync_Set_ReConTime(time(NULL));
	fprintf(stderr, "------------------------- testThread end2 %d\n", i);
	*/
	return NULL;
}
#endif // DEBUG_TEST

/*
	A thread to do auto report
{"DataSync":{"RecoverProgress":{"bn":"RecoverProgress","e":[
{"n":"total","v":%d,"asm":"r"},
{"n":"current","v":%d,"asm":"r"},
{"n":"start","v":%d,"asm":"r"},
{"n":"end","v":%d,"asm":"r"},
{"n":"batchSize","v":%d,"asm":"r"},
{"n":"sendInterval","v":%d,"asm":"r"},
{"n":"keepTime","v":%llu,"asm":"r"}]}}}
*/
void* autoReportThread(void* args)
{
	char *ptr;
	LOGD("[Recover] autoReportThread start\n");

	while (g_cacheInfo.reportRunning) {
		// update capability to kernel handler
		Handler_Get_Capability(&ptr);

		// time to stop
		if (g_cacheInfo.total <= g_cacheInfo.current) {
			break;
		}

		// sleep
		sleep(g_config.wrapSendInterval);
	}

	// update capability before stop
	Handler_Get_Capability(&ptr);

	LOGD("[Recover] autoReportThread stop\n");

	return NULL;
}

static void start_report_progress()
{
	pthread_t pHandler;
	g_cacheInfo.reportRunning = 1;
	pthread_create(&pHandler, NULL, autoReportThread, NULL);
}

static void stop_report_progress()
{
	g_cacheInfo.reportRunning = 0;
}

//-----------------------------------------------------------------------------
// Plugin APIs
//-----------------------------------------------------------------------------
int HANDLER_API Handler_Initialize( HANDLER_INFO *handler )
{
	LOGD("%s call", __FUNCTION__);
	g_PluginInfo = *((Handler_info_ex*) handler);
	g_handlerKernel = HandlerKernelEx_Initialize((HANDLER_INFO*) &g_PluginInfo);
	return handler_success;
}

int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	//fprintf(stderr, "%s call\n", __FUNCTION__);
	return handler_success;
}

void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *handler )
{
	char *ptr;
	LOGD("%s call, status=%d\n", __FUNCTION__, handler->agentInfo->status);
	if (handler->agentInfo->status == AGENT_STATUS_ONLINE) {
		Handler_Get_Capability(&ptr);
	}
}

int HANDLER_API Handler_Start( void )
{
	char capHandlerName[256] = {0};
	int64_t zero64 = 0;

	LOGD("%s call", __FUNCTION__);

	// init handler kernel thread
	if(g_handlerKernel)
		HandlerKernelEx_Start(g_handlerKernel);

	// create capability root
	memset(g_repBuffer, 0, sizeof(g_repBuffer));
	sprintf(g_repBuffer, DATASYNC_CAP_STR,
			g_cacheInfo.total,
			g_cacheInfo.current,
			zero64,
			zero64,
			g_config.batchSize,
			g_config.wrapSendInterval,
			g_config.keeptimeSec);
	if(transfer_get_ipso_handlername(g_repBuffer, capHandlerName)) {
		g_capRoot = IoT_CreateRoot(capHandlerName);
	}

	return handler_success;
}

int HANDLER_API Handler_Stop( void )
{
	LOGD("%s call", __FUNCTION__);

	if(g_SendBatchContex.isThreadRunning == true)
	{
		g_SendBatchContex.isThreadRunning = false;
		if (g_SendBatchContex.threadHandler) {
			//pthread_cancel((pthread_t)g_SendBatchContex.threadHandler);
			// wakeup thread
			pthread_mutex_lock(&g_sendBatchMutex);
			pthread_cond_signal(&g_sendBatchCond);
			pthread_mutex_unlock(&g_sendBatchMutex);
			pthread_join((pthread_t)g_SendBatchContex.threadHandler, NULL);
			g_SendBatchContex.threadHandler = 0;
		}
	}

	if (g_handlerKernel) {
		HandlerKernelEx_Stop(g_handlerKernel);
	}

	// lock critical before free because handler kernel will refer to it
	HandlerKernelEx_LockCapability(g_handlerKernel);
	if (g_capRoot) {
		IoT_ReleaseAll(g_capRoot);
	}
	HandlerKernelEx_UnlockCapability(g_handlerKernel);

	return handler_success;
}

void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	char *ptr;
	char *filter = NULL;
	cJSON* root = NULL;
	cJSON* susiCommData;
	cJSON* requestItems;

	// add request all to query string, because datasync is hidden by WISE-Cloud
	do {
		root = cJSON_Parse((const char*) pInQuery);
		if (!root) {
			break;;
		}
		susiCommData = cJSON_GetObjectItem(root, "susiCommData");
		if (!susiCommData) {
			break;
		}
		requestItems = cJSON_GetObjectItem(susiCommData, "requestItems");
		if (!requestItems) {
			break;
		}
		cJSON_AddItemToObject(requestItems, "All", cJSON_CreateObject());
		filter = cJSON_PrintUnformatted(root);
		if (!filter) {
			break;
		}

		LOGD("%s call", __FUNCTION__);
		HandlerKernelEx_SetAutoReportFilter(g_handlerKernel, filter);
		Handler_Get_Capability(&ptr);
	} while (0);

	if (filter) {
		free(filter);
	}
	if (root) {
		cJSON_Delete(root);
	}
}

void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	//LOGD("%s call", __FUNCTION__);
}

/*
{"DataSync":{"RecoverProgress":{"bn":"RecoverProgress","e":[
{"n":"total","v":%d,"asm":"r"},
{"n":"current","v":%d,"asm":"r"},
{"n":"start","v":%d,"asm":"r"},
{"n":"end","v":%d,"asm":"r"},
{"n":"batchSize","v":%d,"asm":"r"},
{"n":"sendInterval","v":%d,"asm":"r"},
{"n":"keepTime","v":%llu,"asm":"r"}]}}}
*/
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	memset(g_repBuffer, 0, sizeof(g_repBuffer));
	sprintf(g_repBuffer, DATASYNC_CAP_STR,
			g_cacheInfo.total,
			g_cacheInfo.current,
			(lost_t > g_config.keeptimeMS)? (lost_t - g_config.keeptimeMS)/1000: lost_t/1000,
			recon_t/1000,
			g_config.batchSize,
			g_config.wrapSendInterval,
			g_config.keeptimeSec);

	*pOutReply = g_repBuffer;
	LOGD("%s call, buf=[%s]", __FUNCTION__, *pOutReply);

	// set capability to kernel handler
	transfer_parse_ipso(g_repBuffer, g_capRoot);

	HandlerKernelEx_SetCapability(g_handlerKernel, g_capRoot, false);
	HandlerKernelEx_SendAutoReportOnce(g_handlerKernel);

	return strlen(*pOutReply) + 1;
}

void HANDLER_API Handler_MemoryFree(char *pInData)
{
	//fprintf(stderr, "%s call\n", __FUNCTION__);
}

void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int cmdID = 0;
	char sessionID[MAX_SESSION_LEN] = {0};

	//Parse Received Command
	if(HandlerKernelEx_ParseRecvCMDWithSessionID((char*)data, &cmdID, sessionID) != handler_success)
			return;

	LOGD("Handler_Recv, cmdID=%d\n", cmdID);
	switch(cmdID)
	{
		case hk_get_sensors_data_req:
			HandlerKernelEx_GetSensorData(g_handlerKernel, hk_get_sensors_data_rep, sessionID, (char*)data, NULL);
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
// Recover related function
//-----------------------------------------------------------------------------
/*
	ex: "/wisepaas/device/00000001-0000-0000-0000-000C2982D9F6/devinfoack"
*/
int parse_topic(char* topic, char* topic_parsed, char** prefix, char** prodcutID, char** deviceID, char** action)
{
	char *saveptr;
	char* ptr;

	strcpy(topic_parsed, topic);

	ptr = strtok_r(topic_parsed, "/", &saveptr); // wisepaas
	if (prefix)
		*prefix = ptr;

	ptr = strtok_r(NULL, "/", &saveptr); // device
	if (prodcutID)
		*prodcutID = ptr;

	ptr = strtok_r(NULL, "/", &saveptr); // 00000001-0000-0000-0000-000C2982D9F6
	if (deviceID)
		*deviceID = ptr;

	ptr = strtok_r(NULL, "/", &saveptr); // devinfoack
	if (action)
		*action = ptr;

	if ( (!prodcutID || (prodcutID && *prodcutID)) &&
		 (!deviceID || (deviceID && *deviceID)) &&
		 (!action || (action && *action)) &&
		 (!prefix || (prefix && *prefix)) )
	{
		return 0;
	}

	return -1;
}

/*
	Send MQTT with saclient_publish(), it will wrap data with "content {...}" json format
	To avoid memory fragment, this is not thread-safe function.
*/
int publish_packet(char* topic,
				   const char* handler,
				   int cmd,
				   const char* data)
{
	char topic_parsed[MAX_TOPIC_LEN];
	char* deviceID;

	if (parse_topic(topic, topic_parsed, NULL, NULL, &deviceID, NULL)) {
		LOGD("publish_packet: parse_topic fail, topic=[%s]", topic);
		return -1;
	}

	// fill packet
	strncpy(g_publishPacket.devId, deviceID, sizeof(g_publishPacket.devId));
	strncpy(g_publishPacket.handlerName, handler, sizeof(g_publishPacket.handlerName));

	g_publishPacket.cmd = cmd;
#ifdef RMM3X
	g_publishPacket.type = pkt_type_susiaccess;
#else
	g_publishPacket.type = pkt_type_wisepaas;
#endif
	g_packetContentLen = strlen(data)+1;
	g_publishPacket.content = (char*) malloc(g_packetContentLen);
	if (g_publishPacket.content == NULL) {
		LOGE("publish_packet: malloc fail");
		return -1;
	}
	strcpy(g_publishPacket.content, data);

	LOGD("[Recover] publish_CB(%s, %s)", topic, g_publishPacket.content);
	if (publish_CB)
		publish_CB(topic, 0, 0, &g_publishPacket);

	free(g_publishPacket.content);

	return 0;
}

/*
	public a packet with format topic
	ex: /wisepaas/device/%s/devinfoack
*/
int publish_packet_fmt(char* topicFmt,
					   char* deviceID,
					   const char* handler,
					   int cmd,
					   const char* data)
{
	char topic[MAX_TOPIC_LEN];

	sprintf(topic, topicFmt, deviceID);
	return publish_packet(topic, handler, cmd, data);
}

/*
	send data in batch with "data_list" json format
*/
int send_data_list(Batch_Entry* batch)
{
	int i;
	cJSON *root, *ary, *item;
	char* dataList = NULL;

	if (batch->data_count <= 0) { // invalid batch
		return -1;
	}
	// create data_list to send batch
	root = cJSON_CreateObject();
	if (!root) {
		LOGE("cJSON_Create fail");
		return -1;
	}
	ary = cJSON_CreateArray();
	if (!ary) {
		LOGE("cJSON_Create fail");
		cJSON_Delete(root);
		return -1;
	}

	cJSON_AddItemToObject(root, "data_list", ary);
	for (i = 0; i < batch->data_count; i++) {
		item = cJSON_Parse(batch->data[i].dataStr);
		cJSON_AddItemToArray(ary, item);
	}
	dataList = cJSON_PrintUnformatted(root);
	if (dataList) {
		publish_packet_fmt(batch->data[0].topic, batch->deviceID, "general", batch->data[0].type, dataList);
		g_cacheInfo.current += batch->data_count;
		if (g_cacheInfo.current > g_cacheInfo.total) {
			LOGD("sqlite bug, reset %d to %d", g_cacheInfo.current, g_cacheInfo.total);
			g_cacheInfo.current = g_cacheInfo.total; // sqlite bug, the number count may miss match
		}
		free(dataList);
	}
	cJSON_Delete(root);

	return 0;
}

/*
	fill batch data in Batch_Entry
	Please NOTE: you must free dataStr by yourself before free batch
*/
int fill_batch_data_cb(void *param, int argc, char **argv, char **azColName)
{
	Batch_Entry* batch = (Batch_Entry*) param;
	Data_Entry* entry;
	void* newPtr;

	// if disconnect suddenly, abort it.
	if (!cond) {
		bComplete = false;
		LOGE("fill_batch_data_cb: abort by cond=false");
		return SQLITE_ABORT;
	}

	if (g_config.batchSize <= batch->data_count || batch->data_count < 0) {
		batch->data_count = 0;
	}
	entry = &batch->data[batch->data_count];

	// fill data entry
	entry->dataID = strtoull(argv[0], NULL, 10);
	entry->DCapID = strtoul(argv[1], NULL, 10);
	entry->type = strtol(argv[2], NULL, 10);
	entry->ts = strtoll(argv[3], NULL, 10);
	strncpy(entry->topic, argv[4], sizeof(entry->topic));

	newPtr = malloc(strlen(argv[5])+1);
	if (newPtr == NULL) {
		LOGE("fill_batch_data_cb: malloc fail");
		return SQLITE_NOMEM;
	}
	free(entry->dataStr);
	entry->dataStr = (char*) newPtr;
	strcpy(entry->dataStr, argv[5]);

	// increase batch count
	batch->data_count++;

	return SQLITE_OK;
}

/*
	fill the Cap_Entry
*/
int fill_cap_entry_cb(void *param, int argc, char **argv, char **azColName)
{
	Cap_Entry* entry = (Cap_Entry*) param;
	void* newPtr;

	entry->capID = strtoul(argv[0], NULL, 10);
	entry->ts = strtoll(argv[1], NULL, 10);
	strncpy(entry->handler, argv[2], sizeof(entry->handler));
	strncpy(entry->topic, argv[3], sizeof(entry->topic));

	newPtr = malloc(strlen(argv[4])+1);
	if (newPtr == NULL) {
		LOGE("fill_cap_entry_cb: malloc fail");
		return SQLITE_NOMEM;
	}
	free(entry->capStr);
	entry->capStr = (char*) newPtr;
	strcpy(entry->capStr, argv[4]);

	return SQLITE_OK;
}

/*
	send recover report data with specified capID
*/
int send_recover_report(Batch_Entry* batch, uint32_t capID, int64_t start, int64_t end)
{
	char sqlite_str[256] = {0};
	char topic_parsed[MAX_TOPIC_LEN];
	char* deviceID;
	uint64_t dataID = 0;

	// get cap content first
	batch->cap.capID = INVALID_CAPID;
	snprintf(sqlite_str, sizeof(sqlite_str), "select * from TB_Cap where CapID=%"PRIu32" limit 1;", capID);
	do_sqlite(db, sqlite_str, fill_cap_entry_cb, &batch->cap, NULL, 0);
	if (batch->cap.capID == INVALID_CAPID) { // not found
		LOGD("no cap to recover");
		return -1;
	}
	LOGD("[Recover] capID=%"PRIu32" START, handler=[%s], topic=[%s]", capID, batch->cap.handler, batch->cap.topic);

	// get deviceID
	parse_topic(batch->cap.topic, topic_parsed, NULL, NULL, &deviceID, NULL);
	strncpy(batch->deviceID, deviceID, sizeof(batch->deviceID));

	// send cap entry
	publish_packet(batch->cap.topic, "general", wise_get_capability_rep, batch->cap.capStr);
	g_cacheInfo.current++;
	if (g_cacheInfo.current > g_cacheInfo.total) {
		LOGD("sqlite bug, reset %d to %d", g_cacheInfo.current, g_cacheInfo.total);
		g_cacheInfo.current = g_cacheInfo.total; // sqlite bug, the number count may miss match
	}

	// loop to send report data with batch
	batch->data_count = 0; // reset counter
	while (1) {
		snprintf(sqlite_str, sizeof(sqlite_str), "select * from TB_Data where DCapID=%"PRIu32" and DataID>=%"PRIu64" and %"PRId64"<=TS and TS<=%"PRId64" order by DataID limit %d;",
				capID,
				dataID,
				start,
				end,
				g_config.batchSize);

		if (do_sqlite(db, sqlite_str, fill_batch_data_cb, batch, NULL, 0)) { // error when cond is disconnect
			break;
		}
		// no data to send
		if (batch->data_count <= 0) {
			break;
		}

		send_data_list(batch);
		LOGD("[Recover] capID=%"PRIu32", start=%"PRId64", end=%"PRId64", data_count=%d, total=%d, current=%d",
					capID, start, end, batch->data_count, g_cacheInfo.total, g_cacheInfo.current);

		dataID = batch->data[batch->data_count-1].dataID + 1; // increase dataID to skip this item
		// reset data_count
		batch->data_count = 0;
		sleep(g_config.wrapSendInterval);
	}
	LOGD("[Recover] capID=%"PRIu32" END", capID);

	return 0;
}

/*
	fill data entry, you must call free() your self
*/
int fill_data_entry_cb(void *param, int argc, char **argv, char **azColName)
{
	Data_Entry* entry = (Data_Entry*) param;

	// fill data entry
	entry->dataID = strtoull(argv[0], NULL, 10);
	entry->DCapID = strtoul(argv[1], NULL, 10);
	entry->type = strtol(argv[2], NULL, 10);
	entry->ts = strtoll(argv[3], NULL, 10);

	if (argc >= 5) {
		strncpy(entry->topic, argv[4], sizeof(entry->topic));
	}
	if (argc >= 6) {
		char* ptr = (char*) malloc(strlen(argv[5])+1);
		if (!ptr) {
			LOGE("fill_data_entry_cb: malloc fail");
			return SQLITE_NOMEM;
		}
		entry->dataStr = ptr;
		strcpy(entry->dataStr, argv[5]);
	}

	return SQLITE_OK;
}

/*
	send recover raw data which DCapID = INVALID_CAPID
*/
int send_recover_data(int64_t start, int64_t end)
{
	char sqlite_str[256] = {0};
	Data_Entry dataEntry;
	uint64_t dataID = 0;

	memset(&dataEntry, 0, sizeof(dataEntry));

	LOGD("[Recover] capID=INVALID_CAPID START");
	// loop to send raw data
	while (1) {
		snprintf(sqlite_str, sizeof(sqlite_str), "select * from TB_Data where DCapID=%"PRIu32" and DataID>=%"PRIu64" and %"PRId64"<=TS and TS<=%"PRId64" order by DataID limit 1;",
				INVALID_CAPID,
				dataID,
				start,
				end);
		dataEntry.dataID = INVALID_DATAID;
		if (do_sqlite(db, sqlite_str, fill_data_entry_cb, &dataEntry, NULL, 0) != SQLITE_OK || dataEntry.dataID == INVALID_DATAID) {
			break; // break if no data
		}
		// send data entry
		publish_packet(dataEntry.topic, "general", dataEntry.type, dataEntry.dataStr);
		if (dataEntry.dataStr) {
			free(dataEntry.dataStr);
			dataEntry.dataStr = NULL;
		}
		g_cacheInfo.current++;
		if (g_cacheInfo.current > g_cacheInfo.total) {
			LOGD("sqlite bug, reset %d to %d", g_cacheInfo.current, g_cacheInfo.total);
			g_cacheInfo.current = g_cacheInfo.total; // sqlite bug, the number count may miss match
		}

		LOGD("[Recover] start=%"PRId64", end=%"PRId64", total=%d, current=%d",
					start, end, g_cacheInfo.total, g_cacheInfo.current);

		// increase start time
		dataID = dataEntry.dataID + 1; // increase dataID, to skip current item

		// sleep after batch size send
		if (g_cacheInfo.current % g_config.batchSize == 0) {
			sleep(g_config.wrapSendInterval);
		}

		if (!cond) {
			bComplete = false;
			break;
		}
	}
	if (dataEntry.dataStr) {
		free(dataEntry.dataStr);
		dataEntry.dataStr = NULL;
	}
	LOGD("[Recover] capID=INVALID_CAPID END");

	return 0;
}

/*
	get distinct capID within recover time period
*/
int get_diff_capID_count_cb(void *param, int argc, char **argv, char **azColName)
{
	int* cap_number = (int*)param;

	*cap_number = strtol(argv[0], NULL, 10);
	LOGD("[Recover] cap_number=%d", *cap_number);

	return SQLITE_OK;
}

/*
	fill cap_ids with distinct capID
*/
int get_diff_capID_ids_cb(void *param, int argc, char **argv, char **azColName)
{
	Batch_Entry* batch = (Batch_Entry*) param;

	batch->cap_ids[batch->cap_index] = strtol(argv[0], NULL, 10);
	batch->cap_index++;

	return SQLITE_OK;
}

/*
	NOTE: you must free batch.cap_ids if cap_number > 0
	return the count of cap_number, -1 for fail
*/
int get_diff_capID_between(Batch_Entry* batch, int64_t start, int64_t end)
{
	char sqlite_str[256] = {0};

	snprintf(sqlite_str, sizeof(sqlite_str), "select count(distinct DCapID) from TB_Data where DCapID!=%"PRIu32" and %"PRId64"<=TS and TS<=%"PRId64";",
			INVALID_CAPID,
			start,
			end);
	if (do_sqlite(db, sqlite_str, get_diff_capID_count_cb, &batch->cap_number, NULL, 0) != SQLITE_OK) {
		return -1;
	}

	batch->cap_index = 0; // reset index
	batch->cap_ids = (int*) calloc(batch->cap_number, sizeof(int));
	snprintf(sqlite_str, sizeof(sqlite_str), "select distinct DCapID from TB_Data where DCapID!=%"PRIu32" and %"PRId64"<=TS and TS<=%"PRId64" order by TS;",
			INVALID_CAPID,
			start,
			end);
	if (do_sqlite(db, sqlite_str, get_diff_capID_ids_cb, batch, NULL, 0) != SQLITE_OK) {
		return -1;
	}

	return batch->cap_number;
}

/*
	save total items in TB_Data that need to recover
*/
int get_total_data_cb(void *param, int argc, char **argv, char **azColName)
{
	int* number = (int*) param;
	*number = strtol(argv[0], NULL, 10);
	return SQLITE_OK;
}

/*
	calculate total
*/
int get_progress_total(Batch_Entry* batch, int64_t start, int64_t end)
{
	int number = 0;
	char sqlite_str[256] = {0};
	int total = batch->cap_number;

	snprintf(sqlite_str, sizeof(sqlite_str), "select count(DataID) from TB_Data where %"PRId64"<=TS and TS<=%"PRId64";",
				start,
				end);
	if (do_sqlite(db, sqlite_str, get_total_data_cb, &number, NULL, 0) != SQLITE_OK) {
		return -1;
	}
	total += number;
	return total;
}

/*
	do recover after re-connect
*/
int do_recover()
{
	int64_t start = lost_t - g_config.keeptimeMS;
	int64_t end = recon_t;
	Batch_Entry batch;
	int i;

	LOGD("do_recover: lost_t=%"PRId64", keeptimeMS=%"PRId64", recon_t=%"PRId64"\n", lost_t, g_config.keeptimeMS, recon_t);

	// set complete true first to avoid do recover again
	bComplete = true;

	memset(&batch, 0, sizeof(batch));

	// allocate batch data list
	batch.data = (Data_Entry*) calloc(g_config.batchSize, sizeof(Data_Entry));
	if (!batch.data) {
		LOGE("do_recover: calloc fail");
		return -1;
	}

	// get capID list
	get_diff_capID_between(&batch, start, end);
	g_cacheInfo.total = get_progress_total(&batch, start, end);
	g_cacheInfo.current = 0; // reset proc count
	// start to report progress
	start_report_progress();

	// loop to send recover report data with specified capID
	for (i = 0; i < batch.cap_number; i++) {
		if (!cond) {
			bComplete = false;
			break;
		}
		send_recover_report(&batch, batch.cap_ids[i], start, end);

		// prepare next batch send
		if (batch.data_count > 0) {
			sleep(g_config.wrapSendInterval);
		}
	}
	// send raw data without capbility
	send_recover_data(start, end);

	// stop report
	stop_report_progress();

	// free resource
	if (batch.cap_ids) {
		free(batch.cap_ids);
	}
	if (batch.cap.capStr) {
		free(batch.cap.capStr);
	}
	for (i = 0; i < g_config.batchSize; i++) {
		if (batch.data[i].dataStr) {
			free(batch.data[i].dataStr);
		}
	}
	free(batch.data);

	return (bComplete == true)? 0: -1;
}

//-----------------------------------------------------------------------------
// Hook APIs
//-----------------------------------------------------------------------------
bool DATASYNC_API DataSync_Initialize(char* pWorkdir, Handler_List_t *pLoaderList, void* pLogHandle)
{
	int rc;
	int64_t tick=time(NULL);
	char sqlite_str[256] = {0};
	char ok_msg[128] = {0};

	// variable init
	memset(&g_publishPacket, 0, sizeof(susiaccess_packet_body_t));
	g_packetContentLen = 0;

	memset(&g_cacheInfo, 0, sizeof(g_cacheInfo));
	memset(&g_config, 0, sizeof(g_config));
	lost_t = tick * 1000;

	pthread_mutex_init(&g_dbMux, NULL);
	pthread_mutex_init(&g_sendBatchMutex, NULL);
	pthread_cond_init(&g_sendBatchCond, NULL);

	g_workdir = (char*) calloc(strlen(pWorkdir) + 1, sizeof(char));
	strcpy(g_workdir, pWorkdir);

	// read ini file
	read_INI(pWorkdir);
	g_config.limit_size = g_config.filesize * 1024 * 1024; // change to bytes
	g_config.limit_duration = (int64_t) g_config.duration*60*60*1000;
	g_config.limit_time = (int64_t)tick*1000 + g_config.limit_duration;

	if(g_config.limit_size == 0 && g_config.limit_duration == 0 && g_config.limit_preserve == 0) {
		g_config.limit_size=20*1000*1000; // default file size limit to 20MB
	}

	LOGI("DataSync_Initialize: limit_duration:%"PRId64" limit_size:%ld keeptimeMS:%"PRId64"\n", g_config.limit_duration,g_config.limit_size,g_config.keeptimeMS);

	// init database
	db_path = (char *) calloc(1, strlen(g_workdir) + strlen("/datasync.db")+1);
	sprintf(db_path, "%s%s", g_workdir, "/datasync.db");

	rc = sqlite3_open(db_path, &db);
	if( rc ){
	  LOGE("Can't open database: %s", sqlite3_errmsg(db));
      sqlite3_close(db);
	  db = NULL;
	  return false;
    }
	else
		LOGI("Open database successfully");

	// Enable WAL mode to improve disk I/O performance
	do_sqlite(db, "PRAGMA synchronous = NORMAL", NULL, NULL, NULL, 0);
	do_sqlite(db, "PRAGMA journal_mode = WAL", NULL, NULL, NULL, 0);

	// try to create Cap table
	snprintf(sqlite_str, sizeof(sqlite_str), "create table if not exists TB_Cap (CapID UNSIGNED INT, TS UNSIGNED BIG INT, Handler varchar(65535), Topic varchar(65535), CapStr varchar(65535));");
	snprintf(ok_msg, sizeof(ok_msg), "create TB_Cap table - Successful");
	do_sqlite(db, sqlite_str, NULL, NULL, ok_msg, 1);

	// try to create Rep table
	snprintf(sqlite_str, sizeof(sqlite_str), "create table if not exists TB_Data (DataID UNSIGNED BIG INT, DCapID UNSIGNED INT, Type int, TS UNSIGNED BIG INT, Topic varchar(65535), DataStr varchar(65535));");
	snprintf(ok_msg, sizeof(ok_msg), "create TB_Data table - Successful");
	do_sqlite(db, sqlite_str, NULL, NULL, ok_msg, 1);

	// Create index to improve search performance with big database file
	do_sqlite(db, "create index if not exists iTS ON TB_Data(TS)", NULL, NULL, NULL, 0);

	// reset CapID in TB_Cap
	reset_CapID();

	// trigger preserve space check in first insert
	updateMonitor();

	// init batch thread
	g_SendBatchContex.threadHandler = 0;
	g_SendBatchContex.isThreadRunning = true;
	if(pthread_create(&g_SendBatchContex.threadHandler, NULL, SendBatchThreadStart, &g_SendBatchContex) != 0)
	{
		g_SendBatchContex.isThreadRunning = false;
		printf("> start SendBatch thread failed!\r\n");
		LOGE("start SendBatch thread failed!");
		return false;
    }

	return true;
}

void DATASYNC_API DataSync_Uninitialize()
{
	fprintf(stderr, "DataSync_Uninitialize()\n");
	bUninit=true;

	if (g_workdir) {
		free(g_workdir);
		g_workdir = NULL;
	}

	if(db_path)
	{
		free(db_path);
		db_path=NULL;
	}

	if(db)
	{
		if(sqlite3_close(db)==SQLITE_OK )
			printf("SQLite : db closed successfully");
		db=NULL;
	}

	pthread_mutex_destroy(&g_sendBatchMutex);
	pthread_cond_destroy(&g_sendBatchCond);
	pthread_mutex_destroy(&g_dbMux);

	HandlerKernelEx_Uninitialize(g_handlerKernel);
}

void DATASYNC_API DataSync_SetFuncCB(PUBLISHCB g_publishCB)
{
	publish_CB = g_publishCB;
}


cJSON* FindeJSONNode(cJSON* node, char* name)
{
	cJSON* target = NULL;
	cJSON* current = node;
	cJSON* opTS = NULL;
	if(node == NULL)
		return target;
	while(current)
	{
		opTS = cJSON_GetObjectItem(node, name);
		if(opTS == NULL)
		{
			opTS = FindeJSONNode(current->child, name);
		}
		if(opTS!= NULL)
		{
			target = opTS;
			break;
		}
		current = current->next;
	}
	return target;
}

void DATASYNC_API DataSync_Insert_Cap(void* const handle, char *cap, char *captopic, int result)
{
	int64_t ts = time(NULL) * 1000;
	char* insert_str = NULL;
	int insert_str_len = 0;
	char* strCap = NULL;
	Handler_info* hdl = (Handler_info*) handle;
	uint32_t capID = INVALID_CAPID;

	if(strcmp(hdl->Name, g_PluginInfo.Name) == 0) { // report itself
		return;
	}

	strCap = cap;

	// delete specified CapID which has no Rep to reference
	insert_str_len = 256;
	insert_str = (char*) malloc(insert_str_len);
	if (!insert_str) {
		LOGE("DataSync_Insert_Cap: malloc fail 1");
	}

	snprintf(insert_str, insert_str_len, "delete from TB_Cap where Handler is '%s' and topic is '%s' and CapID not in (select DCapID from TB_Data);",
				hdl->Name,
				captopic);
	do_sqlite(db, insert_str, NULL, NULL, NULL, 1);

	// get max CapID if datasync.db already exist
	snprintf(insert_str, insert_str_len, "select CapID from TB_Cap order by CapID desc limit 1;");

	pthread_mutex_lock(&g_dbMux);
	do_sqlite(db, insert_str, reload_CapID_cb, &capID, NULL, 0);
	capID++;
	// check capID limit
	if(capID == INVALID_CAPID) {
		capID = 0;
	}
	free(insert_str);

	// do insert
	if(strCap)
	{
		insert_str_len = strlen(strCap) + 1 + 256;
		insert_str = (char*) malloc(insert_str_len);
		if (insert_str) {
			snprintf(insert_str, insert_str_len, "insert into TB_Cap values(%"PRIu32",%"PRId64",'%s','%s','%s');",
						capID,
						ts,
						hdl->Name,
						captopic,
						strCap);
			do_sqlite(db, insert_str, NULL, NULL, insert_str, 0);
			free(insert_str);
		} else {
			LOGE("DataSync_Insert_Cap: malloc fail 2");
		}
	}
	else {
		LOGD("Capability not valid: %s", cap);
    }
	pthread_mutex_unlock(&g_dbMux);
}

/*
	save latest capID
*/
int find_last_CapID_callback(void *param, int argc, char **argv, char **azColName)
{
	uint32_t* capID = (uint32_t*) param;

	if (argc <= 0) { // not found
		return SQLITE_ERROR;
	} else { // found
		*capID = strtoul(argv[0], NULL, 10);
	}

	return SQLITE_OK;
}

/*
	find latest CapID with specified topic and handler
	Return 0 for success, -1 for fail
*/
int find_last_CapID(char* topic, char* handler, uint32_t* capID_Ptr)
{
	char sqlite_str[256] = {0};
	int ret;
	uint32_t capID = INVALID_CAPID;

	if (!topic || !handler) {
		return -1;
	}

	// find Cap with topic and handler
	snprintf(sqlite_str, sizeof(sqlite_str), "select CapID from TB_Cap where Handler like '%s' and Topic like '%s' order by TS desc limit 1;",
				handler,
				topic);
	ret = do_sqlite(db, sqlite_str, find_last_CapID_callback, &capID, NULL, 0);
	if (ret != SQLITE_OK || capID == INVALID_CAPID) {
		return -1;
	}
	*capID_Ptr = capID;

	return 0;
}

static int count_escape(char *str)
{
	int count = 0;
	char *ptr = str;

	while (ptr && *ptr != '\0') {
		if (*ptr == '\'') {
			count++;
		}
		ptr++;
	}
	return count;
}

static int replace_escape(char *dst, char *src)
{
	char *psrc = src;
	char *pdst = dst;

	while (pdst && psrc && *psrc != '\0') {
		if (*psrc == '\'') {
			*pdst++ = *psrc;
		}
		*pdst = *psrc;
		psrc++;
		pdst++;
	}
	return 0;
}

/*
	insert a record
*/
int insert_to_db(char* handler,
				 uint64_t dataID,
				 uint32_t capID,
				 int type,
				 int64_t ts,
				 char* topic,
				 char* dataStr)
{
	char *insert_str = NULL;
	int insert_str_len = 0;
	int rc = SQLITE_ERROR;
	int count = 0;
	char *dataStrEsc = NULL;

	count = count_escape(dataStr);
	if (count > 0) {
		dataStrEsc = (char*) malloc(strlen(dataStr) + 1 + count);
		replace_escape(dataStrEsc, dataStr);
	} else {
		dataStrEsc = dataStr;
	}

	insert_str_len = strlen(dataStrEsc)+1+256;
	insert_str = (char*) malloc(insert_str_len);
	if(!insert_str) {
		return SQLITE_NOMEM;
	}

	snprintf(insert_str, insert_str_len, "insert into TB_Data values(%"PRIu64",%"PRIu32",%d,%"PRId64",'%s','%s');",
				dataID,
				capID,
				type,
				ts,
				topic,
				dataStrEsc);
	LOGD("insert %s Data - %"PRIu64" %"PRIu32" %"PRId64" %s %.64s", handler, dataID, capID, ts, topic, dataStrEsc);
	do_sqlite(db, insert_str, NULL, NULL, NULL, 0);

	free(insert_str);
	if (count > 0) {
		free(dataStrEsc);
	}

	#ifdef DEBUG_TEST
	static pthread_t threadHandler = 0;
	if (threadHandler == 0 && dataID >= 3) {
		if (threadHandler == 0) {
			pthread_create(&threadHandler, NULL, testThread, NULL);
		}
	}
	#endif
	return rc;
}


/*
	update oldest record in TB_Data
*/
int update_db_with_oldest(char* handler,
						  uint64_t dataID,
						  uint32_t capID,
						  int type,
						  int64_t ts,
						  char* topic,
						  char* dataStr)
{
	char *sqlite_str = NULL;
	int sqlite_str_len = 0;
	int rc = SQLITE_ERROR;
	Data_Entry dataEntry;
	int count = 0;
	char *dataStrEsc = NULL;

	count = count_escape(dataStr);
	if (count > 0) {
		dataStrEsc = (char*) malloc(strlen(dataStr) + 1 + count);
		replace_escape(dataStrEsc, dataStr);
	} else {
		dataStrEsc = dataStr;
	}
	sqlite_str_len = strlen(dataStrEsc)+1+1024;
	sqlite_str = (char*) malloc(sqlite_str_len);
	if(!sqlite_str) {
		return SQLITE_NOMEM;
	}
	sqlite_str[sqlite_str_len-1] = '\0';

	// find DataID and TS to identify oldest record
	snprintf(sqlite_str, sqlite_str_len,  "select DataID,DCapID,type,ts from TB_Data order by TS limit 1;");
	rc = do_sqlite(db, sqlite_str, fill_data_entry_cb, &dataEntry, NULL, 0);
	if (rc != SQLITE_OK) {
		free(sqlite_str);
		return rc;
	}

	// update oldest record
	snprintf(sqlite_str, sqlite_str_len, "update TB_Data set DataID=%"PRIu64",DCapID=%"PRIu32",Type=%d,TS=%"PRId64",Topic='%s',DataStr='%s' where DataID==%"PRIu64" and TS==%"PRId64";",
				dataID,
				capID,
				type,
				ts,
				topic,
				dataStrEsc,
				dataEntry.dataID,
				dataEntry.ts);
	LOGD("update %s Data - %"PRIu64" %"PRIu32" %"PRId64" %s %.64s", handler, dataID, capID, ts, handler, dataStrEsc);
	rc = do_sqlite(db, sqlite_str, NULL, NULL, NULL, 0);

	// doesn't need to free(dataEntry.dataStr) because we don't select dataStr from table
	free(sqlite_str);
	if (count > 0) {
		free(dataStrEsc);
	}
	return rc;
}

/*
	if has data before keeptimeMS
*/
int update_db_with_keeptime(char* handler,
							uint64_t dataID,
							uint32_t capID,
							int type,
							int64_t ts,
							char* topic,
							char* dataStr)
{
	int rc = SQLITE_ERROR;
	int sqlite_str_len = 0;
	char *sqlite_str = NULL;
	int64_t tmp_ls_t;
	Data_Entry dataEntry;
	int count = 0;
	char *dataStrEsc = NULL;

	if(next_lost_t != 0)
		tmp_ls_t = next_lost_t;
	else
		tmp_ls_t = lost_t;

	count = count_escape(dataStr);
	if (count > 0) {
		dataStrEsc = (char*) malloc(strlen(dataStr) + 1 + count);
		replace_escape(dataStrEsc, dataStr);
	} else {
		dataStrEsc = dataStr;
	}
	sqlite_str_len = strlen(dataStr)+1+1024;
	sqlite_str = (char*) malloc(sqlite_str_len);
	if(!sqlite_str) {
		return SQLITE_NOMEM;
	}
	sqlite_str[sqlite_str_len-1] = '\0';

	// find DataID and TS to identify oldest record
	// if we can't find data before keeptimeMS, just drop this item
	dataEntry.dataID = INVALID_DATAID;
	snprintf(sqlite_str, sqlite_str_len,  "select DataID,DCapID,type,ts from TB_Data where TS<%"PRId64" order by TS limit 1;",
				tmp_ls_t - g_config.keeptimeMS);
	rc = do_sqlite(db, sqlite_str, fill_data_entry_cb, &dataEntry, NULL, 0);
	if (rc != SQLITE_OK || dataEntry.dataID == INVALID_DATAID) { // no record found, means all records are within keeptime
		LOGD("Drop %s KData - %"PRIu64" %"PRIu32" %"PRId64" %s %.64s", handler, dataID, capID, ts, handler, dataStrEsc);
		free(sqlite_str);
		return rc;
	}

	// update oldest record
	snprintf(sqlite_str, sqlite_str_len, "update TB_Data set DataID=%"PRIu64",DCapID=%"PRIu32",Type=%d,TS=%"PRId64",Topic='%s',DataStr='%s' where DataID==%"PRIu64" and TS==%"PRId64";",
				dataID,
				capID,
				type,
				ts,
				topic,
				dataStrEsc,
				dataEntry.dataID,
				dataEntry.ts);
	LOGD("update %s KData - %"PRIu64" %"PRIu32" %"PRId64" %s %.64s", handler, dataID, capID, ts, handler, dataStrEsc);
	do_sqlite(db, sqlite_str, NULL, NULL, NULL, 0);

	free(sqlite_str);
	if (count > 0) {
		free(dataStrEsc);
	}
	return rc;
}

int updateMonitor()
{
	int stopInsert = 0;
	int64_t freeStorage;

	if (g_config.limit_preserve == 0) {
		return 0;
	}

#ifdef WIN32
	GetDiskFreeSpaceEx(g_workdir, (PULARGE_INTEGER)&freeStorage, NULL, NULL);
#else
	struct statfs diskInfo;
	if (statfs(g_workdir, &diskInfo)) {
		return 0; // fail
	}
	freeStorage = diskInfo.f_bavail * diskInfo.f_bsize;
#endif

	// change to MB
	freeStorage = freeStorage / (1024 * 1024);

	if (printCount % 10 == 0) {
		LOGD("freeStorage=%"PRId64", limit_preserve=%"PRId64", stopInsert=%d", freeStorage, g_config.limit_preserve, freeStorage <= g_config.limit_preserve);
	}

	if(freeStorage > g_config.limit_preserve) {
		stopInsert = 0;
	} else {
		stopInsert = 1;
	}

	if (stopInsert != g_cacheInfo.stopInsert) {
		g_cacheInfo.stopInsert = stopInsert;
		LOGD("stopInsert=%d\n", g_cacheInfo.stopInsert);
	}
	printCount++;

	return 0;
}

void DATASYNC_API DataSync_Insert_Data(void* const handle, char *content, char *topic, int type)
{
	int64_t ts = time(NULL) * 1000;
	struct stat st;
	char* dataStr = NULL;
	long file_size = 0;
	int64_t file_duration = 0;
	Handler_info* hdl = (Handler_info*) handle;
	char topic_tmp[MAX_TOPIC_LEN] = {0};
	char topic_parsed[MAX_TOPIC_LEN] = {0};
	char* topic_target = NULL;
	char *prefix;
	char *prodcutID = NULL;
	char *deviceID = NULL;
	char *action = NULL;
	uint32_t capID = INVALID_CAPID;
	uint64_t dataID = INVALID_DATAID;
	int ret;

	if(strcmp(hdl->Name, g_PluginInfo.Name) == 0) { // report itself
		return;
	}

	if (type == wise_report_data_rep) { // it's a report data type, so we can extract deviceID from topic to save storage space
		if (parse_topic(topic, topic_parsed, &prefix, &prodcutID, &deviceID, &action) || deviceID == NULL) {
			LOGE("parse_topic() fail!! topic=[%s]", topic);
			return;
		}
		// create capability topic
		snprintf(topic_tmp, sizeof(topic_tmp), "/wisepaas/%%/%s/agentactionack", deviceID); // create a cap topic
		ret = find_last_CapID(topic_tmp, hdl->Name, &capID);
		if (ret == -1) { // if it is a report data but no capID in TB_Cap, drop it.
			return;
		}
		// create report data topic
		sprintf(topic_tmp, "/%s/%s/%%s/%s", prefix, prodcutID, action); // save a topic without prodcutID, /wisepaas/RMM/%s/devinfoack
		topic_target = topic_tmp; // redirect target topic to report data topic
	} else {
		topic_target = topic;
	}

	// check config
	if (g_config.limit_size == 0 && g_config.limit_duration == 0 && g_config.limit_preserve == 0.0) {
		LOGE("Invalid Storage(%ld), Duration(%d) or PreserveStorage(%"PRId64") setting in ini file!\n",
			g_config.limit_size/1000000, g_config.duration, g_config.limit_preserve);
		return;
	}

	updateMonitor();
	dataStr = content;

	pthread_mutex_lock(&g_dbMux);
	g_dataIDCount++;
	if (g_dataIDCount == INVALID_DATAID) {
		g_dataIDCount = 0;
	}
	dataID = g_dataIDCount;
	pthread_mutex_unlock(&g_dbMux);

	if(dataStr != NULL)
	{
		if(g_config.limit_size != 0) {
			if(db_path)
				if(stat(db_path, &st) == 0)
					file_size = st.st_size;

			if (printCount % 10 == 0) {
				LOGD("limit_size=%ld, file_size=%ld", g_config.limit_size, file_size);
			}
		}
		else if(g_config.limit_duration != 0) {
			int64_t tick = time(NULL);
			file_duration = (int64_t) tick*1000;

			if (printCount % 10 == 0) {
				LOGD("limit_time=%"PRId64", file_duration=%"PRId64, g_config.limit_time, file_duration);
			}
		}

		// limit is reach
		if ( g_cacheInfo.stopInsert || // preserve storage reach limit
			 (g_config.limit_size != 0 && file_size >= g_config.limit_size) || // file size limit reach
			 (g_config.limit_duration != 0 && file_duration >= g_config.limit_time) ) // file duration limit reach
		{
			if (g_config.keeptimeMS && (!cond)) { // keeptimeMS is enabled
				update_db_with_keeptime(hdl->Name, dataID, capID, type, ts, topic_target, dataStr);
			} else { // no keeptime need, update last
				update_db_with_oldest(hdl->Name, dataID, capID, type, ts, topic_target, dataStr);
			}
		}
		else { // limit is not reach
			insert_to_db(hdl->Name, dataID, capID, type, ts, topic_target, dataStr);
		}
	}
}

void DATASYNC_API DataSync_Set_LostTime(int64_t losttime)
{
	if(lost_t!=0)
	{
		next_lost_t=losttime*1000;
		LOGD("Set_LostTime, next_lost_t=%"PRId64"",next_lost_t);
	}
	else
	{
		lost_t=losttime*1000;
		LOGD("Set_LostTime, lost_t=%"PRId64"", lost_t);
	}

	pthread_mutex_lock(&g_sendBatchMutex);
	cond=false;
	blost_on=true;
	pthread_mutex_unlock(&g_sendBatchMutex);
}

void DATASYNC_API DataSync_Set_ReConTime(int64_t recontime)
{
	recon_t=recontime*1000;
	if(next_lost_t!=0)
		if(recon_t>next_lost_t)
			lost_t=next_lost_t;

	LOGD("Set_ReConTime, lost_t=%"PRId64", recon_t=%"PRId64"",lost_t,recon_t);

	pthread_mutex_lock(&g_sendBatchMutex);
	bComplete = false;
	cond=true;
	pthread_cond_signal(&g_sendBatchCond);
	pthread_mutex_unlock(&g_sendBatchMutex);
}
