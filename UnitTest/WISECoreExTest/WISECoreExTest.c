/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/01/23 by Scott Chang								    */
/* Modified Date: 2017/01/23 by Scott Chang									*/
/* Abstract     : WISE Core Ex Test Application								*/
/* Reference    : None														*/
/****************************************************************************/
#include "network.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "Log.h"
#include "ConfigReader.h"
#include "WISECoreEx.h"
#include "WISEPlatform.h"
#include "IoTMessageGenerate.h"

#define WISECOREEXTEST_LOG_ENABLE
//#define DEF_WISECOREEXTEST_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_WISECOREEXTEST_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_WISECOREEXTEST_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
static LOGHANDLE WISECoreExTestLogHandle;
#ifdef WISECOREEXTEST_LOG_ENABLE
#define WISECoreExTestLog(level, fmt, ...)  do { if (WISECoreExTestLogHandle != NULL)   \
	WriteLog(WISECoreExTestLogHandle, DEF_SACLIENT_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define WISECoreExTestLog(level, fmt, ...)
#endif

#define TIME_STRING_FMT "\t[%Y/%m/%d %H:%M:%S]"
#define DEF_OSINFO_JSON "{\"content\":{\"cagentVersion\":\"%s\",\"cagentType\":\"%s\",\"osVersion\":\"%s\",\"biosVersion\":\"%s\",\"platformName\":\"%s\",\"processorName\":\"%s\",\"osArch\":\"%s\",\"totalPhysMemKB\":%d,\"macs\":\"%s\",\"IP\":\"%s\"},\"commCmd\":116,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":{\"$date\":%lld}}"

typedef struct TEST_CTX{
	WiCore_t core;
	int iIndex;
	char strClientID[37];
	char strHostName[32];
	char strCapTopic[256];
	char strReqTopic[256];
	char strDataTopic[256];

	struct CAPABILITY* pCapabilities;

	int iInterval;
	long long tsReport;
	int iHeartbeatRate;
	long long tsSend;
	bool bRunning;
	bool bConnected;
	bool bReconnect;
	long reconnectdelay;
	pthread_t connThread;
	long reportcountdown;

	struct TEST_CTX* next;
}test_contex_t;

static pthread_mutex_t g_writelock;

static pthread_mutex_t g_caplock;
static pthread_mutex_t g_countlock;
static long g_iConnected = 0;
static long g_iIndex = 0;
static long g_iTotal = 0;
static bool g_bStop = false;

struct CONFIG g_config;
//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------

void* threadreconnect(void* args);

long long _GetTimeTick()
{
	long long tick = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tick = (long long)tv.tv_sec*1000 + (long long)tv.tv_usec/1000;
	return tick;
}

void GetCurrentTimeString(char *string, int length, const char *fmt) {
	time_t oldt = 0;
	char timeStr[1024] = {0};
	time_t t = time(NULL);
	struct tm timetm;
	if(oldt != t) {
		localtime_r(&t, &timetm);
		strftime(timeStr, sizeof(timeStr), fmt, &timetm);
		oldt = t;
	}
	strncpy(string, timeStr, length);
}

void WriteConnectLog(int index, char* connstate)
{
	char timestr[256] = {0};
	char filename[256] = {0};
	FILE* stream;
	pthread_mutex_lock(&g_writelock);
	sprintf(filename, "connlog_%d.csv", g_iIndex);
	stream = fopen(filename,"a+");

	GetCurrentTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);

	fprintf(stream, "%s,%d,%s\n", timestr, index, connstate);
	fclose(stream);
	pthread_mutex_unlock(&g_writelock);
}

void* threadconnect(void* args)
{
	long long tsCurrent = 0;
	test_contex_t* test_ctx = NULL;

	if(args != NULL)
	{
		test_ctx = (test_contex_t*)args;
		core_ex_subscribe(test_ctx->core, test_ctx->strReqTopic, 1);
		while(!core_ex_device_register(test_ctx->core))
		{
			if(!test_ctx->bRunning)
				break;
			usleep(500000);
		}
		//WISECoreExTestLog(Normal, "Send AgentInfo: %s \n", test_ctx->strClientID);
	}
	else
	{
		pthread_exit(0);
		return NULL;
	}

	if(test_ctx->iHeartbeatRate)
		core_ex_heartbeat_send(test_ctx->core);

	while(test_ctx->bRunning)
	{
		tsCurrent = _GetTimeTick();
		if(test_ctx->tsSend == 0)
			test_ctx->tsSend = tsCurrent;
		
		if((tsCurrent - test_ctx->tsSend) >= test_ctx->iHeartbeatRate)
		{
			core_ex_heartbeat_send(test_ctx->core);
			test_ctx->tsSend = tsCurrent;
		}

		if(!test_ctx->bRunning)
			break;
		//pthread_mutex_lock(&g_caplock);
		if(test_ctx->pCapabilities && test_ctx->iInterval>0)
		{
			if(test_ctx->tsReport == 0)
				test_ctx->tsReport = tsCurrent;

			if((tsCurrent - test_ctx->tsReport) >= test_ctx->iInterval)
			{
				char* buf = NULL;
				
				if(test_ctx->reportcountdown != 0)
				{
					struct CAPABILITY* pCapability = test_ctx->pCapabilities;

					if(test_ctx->reportcountdown>0)
						test_ctx->reportcountdown--;

					while(pCapability)
					{
#ifdef IPSO_FORMAT
						buf = IoT_PrintFullDataEx(pCapability->pCapability, test_ctx->strClientID, true);
#else
						buf = IoT_PrintFullFlatData(pCapability->pCapability, test_ctx->strClientID, true);
#endif
						if(buf)
						{
							//printf("Send Report: %s \n", buf);
							core_ex_publish(test_ctx->core, test_ctx->strDataTopic, buf, strlen(buf), 0, 0);
							free(buf);
						}
						else
							WISECoreExTestLog(Error, "Report buffer is empty!");
						pCapability = pCapability->next;
					}
				}
				test_ctx->tsReport = tsCurrent;
			}
		}
		//pthread_mutex_unlock(&g_caplock);

		usleep(10000);
	}

	pthread_exit(0);
	return NULL;
}

void on_connect_cb(void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	WriteConnectLog(test_ctx->iIndex, "connected");
	if(test_ctx->bRunning)
	{
		test_ctx->bRunning = false;
		//pthread_cancel(test_ctx->connThread);
		pthread_join(test_ctx->connThread, NULL);
		test_ctx->connThread = 0;
	}

	test_ctx->bRunning = true;
	if(pthread_create(&test_ctx->connThread, NULL, threadconnect, test_ctx)!=0)
	{
		test_ctx->bRunning = false;
		test_ctx->connThread = 0;
	}
	{
		int connCount = 0;
		pthread_mutex_lock(&g_countlock);
		test_ctx->bConnected = true;
		g_iConnected++;
		connCount = g_iConnected;
		pthread_mutex_unlock(&g_countlock);
		if(connCount % 1 == 0 || g_iTotal == connCount)
			WISECoreExTestLog(Normal, "[%d] Connected Clients #%d\n", g_iIndex, connCount);
	}	
}

void on_lostconnect_cb(void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	WriteConnectLog(test_ctx->iIndex, "lostconnect");
	WISECoreExTestLog(Warning, "[%d] %s lostconnect: %s\n", g_iIndex, test_ctx->strClientID, core_ex_error_string_get(test_ctx->core));
	pthread_mutex_lock(&g_countlock);
	if(test_ctx->bConnected)
		g_iConnected--;
	test_ctx->bConnected = false;
	pthread_mutex_unlock(&g_countlock);


	//if(test_ctx->reconnectdelay >= 0)
	{
		pthread_t reconnthread;
		if(test_ctx->bReconnect)
		{
			WISECoreExTestLog(Error, "[%d-%d] Still reconnect: %s\n", g_iIndex, test_ctx->iIndex, test_ctx->strClientID);
			return;
		}
		test_ctx->bReconnect = true;
		WISECoreExTestLog(Normal, "[%d-%d] Reconnect: %s\n", g_iIndex, test_ctx->iIndex, test_ctx->strClientID);
		if(pthread_create(&reconnthread, NULL, threadreconnect, test_ctx)==0)
		{
			pthread_detach(reconnthread);
		}
	}
}

void on_disconnect_cb(void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	WriteConnectLog(test_ctx->iIndex, "disconnect");
	if(test_ctx->bRunning)
	{
		test_ctx->bRunning = false;
		//pthread_cancel(test_ctx->connThread);
		pthread_join(test_ctx->connThread, NULL);
		test_ctx->connThread = 0;
	}
	//WISECoreExTestLog(Warning, "[%d] %s disconnect \n", g_iIndex, test_ctx->strClientID);
	pthread_mutex_lock(&g_countlock);
	if(test_ctx->bConnected)
		g_iConnected--;
	test_ctx->bConnected = false;
	pthread_mutex_unlock(&g_countlock);
}

void on_msgrecv(const char* topic, const void *pkt, const long pktlength, void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	//WISECoreExTestLog(Normal, "%s packet received:\n [%s],\n %s\n", test_ctx->strClientID, topic, pkt);
	/* All messages received from subscribed topics will trigger this callback function.
	 * topic: received topic.
	 * pkt: received message in json string.
	 * pktlength: received message length.
	 */
}

void on_rename(const char* name, const int cmdid, const char* sessionid, const char* devid, void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	//WISECoreExTestLog(Normal, "%s rename to: %s\n", test_ctx->strClientID, name);
	strncpy(test_ctx->strHostName, name, sizeof(test_ctx->strHostName));
	core_ex_action_response(test_ctx->core, cmdid, sessionid, true, devid);
	return;
}

void on_update(const char* loginID, const char* loginPW, const int port, const char* path, const char* md5, const int cmdid, const char* sessionid, const char* devid, void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	//WISECoreExTestLog(Normal, "%s update: %s, %s, %d, %s, %s\n", test_ctx->strClientID, loginID, loginPW, port, path, md5);
	core_ex_action_response(test_ctx->core, cmdid, sessionid, true, devid);
	return;
}

void* threadreconnect(void* args)
{
	test_contex_t* test_ctx = (test_contex_t*)args;
	if(test_ctx != NULL)
	{
		if(test_ctx->bRunning)
		{
			test_ctx->bRunning = false;
			//pthread_cancel(test_ctx->connThread);
			pthread_join(test_ctx->connThread, NULL);
			test_ctx->connThread = 0;
		}
		while(test_ctx->bReconnect)
		{
			core_ex_disconnect(test_ctx->core, false);
			if(test_ctx->reconnectdelay>0)
				usleep(test_ctx->reconnectdelay*1000000);
			else
				usleep(1000000);
			if(!core_ex_connect(test_ctx->core, g_config.strServerIP, g_config.iPort, g_config.strConnID, g_config.strConnPW))
			{
				WISECoreExTestLog(Error, "[%d-%d] OnReconnect Unable to connect to broker. %s\n", g_iIndex, test_ctx->iIndex, core_ex_error_string_get(test_ctx->core));
			}
			else
			{
				test_ctx->bReconnect = false;
				break;
			}
		}
	}
	pthread_exit(0);
	return NULL;
}

void on_server_reconnect( const char* devid, void* userdata)
{
	test_contex_t* test_ctx = NULL;
	pthread_t reconnthread;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	if(test_ctx->reconnectdelay >= 0)
	{
		if(test_ctx->bReconnect)
		{
			WISECoreExTestLog(Error, "[%d-%d] Still reconnect: %s\n", g_iIndex, test_ctx->iIndex, test_ctx->strClientID);
			return;
		}
		test_ctx->bReconnect = true;
		WISECoreExTestLog(Normal, "[%d-%d] Reconnect: %s\n", g_iIndex, test_ctx->iIndex, test_ctx->strClientID);
		if(pthread_create(&reconnthread, NULL, threadreconnect, test_ctx)==0)
		{
			pthread_detach(reconnthread);
		}
	}
	else
	{
		if(!strcmp(test_ctx->strClientID, devid))
		{
			WISECoreExTestLog(Normal, "[%d-%d] Resync: %s\n", g_iIndex, test_ctx->iIndex, test_ctx->strClientID);
			core_ex_device_register(test_ctx->core);
		}
	}	
}

long long get_timetick(void* userdata)
{
	return (long long) time((time_t *) NULL);
}

bool SendOSInfo(test_contex_t* ctx)
{
	long long tick = 0;
	char strPayloadBuff[2048] = {0};
	char localip[16] = {0};

	if(ctx == NULL)
		return false;

	tick = get_timetick(NULL);

	network_ip_get(localip, sizeof(localip));

	snprintf(strPayloadBuff, sizeof(strPayloadBuff), DEF_OSINFO_JSON, 
												 "4.0.6",
												 "IPC",
												 "Windows 7",
												 "V1.13",
												 "SOM-5890",
												 "Intel(R) Atom(TM) CPU D525   @ 1.80GHz",
												 "X86",
												 2048,
												 "305A3A77B1DA",
												 localip,
												 ctx->strClientID,
												 tick);


	return core_ex_publish(ctx->core, ctx->strCapTopic, strPayloadBuff, strlen(strPayloadBuff), 0, 0);
}

void* threadgetcapab(void* args)
{
	test_contex_t* test_ctx = NULL;
	if(args != NULL)
	{
		test_ctx = (test_contex_t*)args;
		SendOSInfo(test_ctx);
		//WISECoreExTestLog(Normal, "Send OSInfo: %s\n", test_ctx->strClientID);
	}

	//pthread_mutex_lock(&g_caplock);
	if(test_ctx->pCapabilities)
	{
		struct CAPABILITY* pCapability = test_ctx->pCapabilities;
		while(pCapability)
		{
			char* buf = IoT_PrintFullCapability(pCapability->pCapability, test_ctx->strClientID);
			//WISECoreExTestLog(Normal, "Send Test Capability: %s\n", buf);
			core_ex_publish(test_ctx->core, test_ctx->strCapTopic, buf, strlen(buf), 0, 0);
			free(buf);
			pCapability = pCapability->next;
		}
	}
	//pthread_mutex_unlock(&g_caplock);
	pthread_exit(0);
	return NULL;
}

void on_get_capability(const void *pkt, const long pktlength, const char* devid, void* userdata)
{
	pthread_t getcapab = 0;
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;

	if(pthread_create(&getcapab, NULL, threadgetcapab, test_ctx)==0)
		pthread_detach(getcapab);
	/*TODO: send whole capability*/
	//WISECoreExTestLog(Normal, "Get Capability: %s\n", devid);
}

void on_start_report(const void *pkt, const long pktlength, const char* devid, void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	/*TODO: start report sensor data*/
	//WISECoreExTestLog(Normal, "Start Report: %s\n", devid);
}

void on_stop_report(const void *pkt, const long pktlength, const char* devid, void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	/*TODO: stop report sensor data*/
	//WISECoreExTestLog(Normal, "Stop Report: %s\n", devid);
}

void on_heartbeatrate_query(const char* sessionid, const char* devid, void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;
	core_ex_heartbeatratequery_response(test_ctx->core, test_ctx->iHeartbeatRate, sessionid, devid);
	//WISECoreExTestLog(Normal, "Heartbeat Rate Query: %s, %s\n", sessionid, devid);
}

void on_heartbeatrate_update(const int heartbeatrate, const char* sessionid, const char* devid, void* userdata)
{
	test_contex_t* test_ctx = NULL;
	if(userdata == NULL)
		return;
	test_ctx = (test_contex_t*)userdata;

	//WISECoreExTestLog(Normal, "Heartbeat Rate Update: %d, %s, %s\n", heartbeatrate, sessionid, devid);
	test_ctx->iHeartbeatRate = heartbeatrate*1000;
	core_ex_action_response(test_ctx->core, 130/*wise_heartbeatrate_update_rep*/, sessionid, true, devid);
	return;
}

void* threadinput(void* args)
{
	fgetc(stdin);
	g_bStop = true;
	pthread_exit(0);
	return NULL;
}

int main(int argc, char *argv[])
{
	char inifilepath[256] = "WISECoreExTest.ini";
	char containpath[256] = "capability.txt";
	char strMAC[13];
	struct CAPABILITY* pCapabilities = NULL;
	test_contex_t *root = NULL, *prev = NULL;
	int i=0, start = 0;
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	_CrtMemCheckpoint( &memStateStart);
#endif

	if(!ReadConfig(inifilepath, &g_config))
	{
		printf("Failed to read config [%s]\n", inifilepath);
		goto EXIT;
	}
	
	pCapabilities = LoadCapability(g_config.containpath);

	if(argc > 1)
		strcpy(g_config.strseed, argv[1]);

	if(argc > 2)
		start = atoi(argv[2]);

	if(argc > 3)
		g_iTotal = g_config.iTotalCount = atoi(argv[3]);

	if(argc > 4)
		g_iIndex = atoi(argv[4]);

	if(g_config.launchinterval <100)
		g_config.launchinterval = 100;

	WISECoreExTestLogHandle = InitLog(NULL);
	pthread_mutex_init(&g_writelock, NULL);
	pthread_mutex_init(&g_caplock, NULL);
	pthread_mutex_init(&g_countlock, NULL);
	printf("WISECoreExTest[%d] Start\n", g_iIndex);
	for(i=0; i<g_config.iTotalCount; i++)
	{
		test_contex_t* current = calloc(1, sizeof(test_contex_t));
		if(prev == NULL)
			root = current;
		else
			prev->next = current;
		prev = current;

		sprintf(strMAC, g_config.strseed, start+i);
		sprintf(current->strClientID, "00000001-0000-0000-0000-%s", strMAC);
		sprintf(current->strHostName, "%s%s",g_config.strPrefix, strMAC);
		current->iHeartbeatRate = 60000; //ms
		current->iInterval = g_config.frequency;
#ifdef _WISEPAAS_02_DEF_H_
		sprintf(current->strCapTopic, DEF_AGENTACT_TOPIC, g_config.strProductTag, current->strClientID);
		sprintf(current->strReqTopic, DEF_CALLBACKREQ_TOPIC, g_config.strProductTag, current->strClientID);
		sprintf(current->strDataTopic, DEF_AGENTREPORT_TOPIC, current->strClientID);
#else
		sprintf(current->strCapTopic, DEF_AGENTACT_TOPIC, current->strClientID);
		sprintf(current->strDataTopic, DEF_AGENTREPORT_TOPIC, current->strClientID);
#endif
		current->pCapabilities = pCapabilities;
		current->iIndex = i;
		current->reconnectdelay = g_config.reconnectdelay;
		current->reportcountdown = g_config.reportcountdown;
		current->core = core_ex_initialize(current->strClientID, current->strHostName, strMAC, current);

		if(!current->core)
		{
			WISECoreExTestLog(Error, "[%d] Unable to initialize AgentCore %s.\n", g_iIndex, current->strClientID);
			goto EXIT;
		}
		//WISECoreExTestLog(Normal, "Agent %5d Initialized\n",i);

		core_ex_connection_callback_set(current->core, on_connect_cb, on_lostconnect_cb, on_disconnect_cb, on_msgrecv);

		core_ex_action_callback_set(current->core, on_rename, on_update);

		core_ex_server_reconnect_callback_set(current->core, on_server_reconnect);

		core_ex_iot_callback_set(current->core, on_get_capability, on_start_report, on_stop_report);

		core_ex_time_tick_callback_set(current->core, get_timetick);

		core_ex_heartbeat_callback_set(current->core, on_heartbeatrate_query, on_heartbeatrate_update);

		core_ex_tag_set(current->core, g_config.strProductTag);

		core_ex_product_info_set(current->core, strMAC, NULL, "4.0.6", "IPC", "test", "test");

		if(strlen(g_config.strAccount) > 0 && strlen(g_config.strPasswd) > 0)
			core_ex_account_bind(current->core, g_config.strAccount, g_config.strPasswd);

		if(g_config.SSLMode == 1)
			core_ex_tls_set(current->core, "server.crt", NULL, "ca.crt", "ca.key", "05155853");
		else if(g_config.SSLMode == 2)
			core_ex_tls_psk_set(current->core, "05155853", current->strClientID, NULL);
		//if(!core_ex_connect(current->core, strServerIP, iPort, "admin", "05155853")){
		if(!core_ex_connect(current->core, g_config.strServerIP, g_config.iPort, g_config.strConnID, g_config.strConnPW)){
			WISECoreExTestLog(Error, "[%d-%d] Unable to connect to broker. %s\n", g_iIndex, i, core_ex_error_string_get(current->core));

			//if(test_ctx->reconnectdelay >= 0)
			{
				pthread_t reconnthread;
				if(current->bReconnect)
				{
					WISECoreExTestLog(Error, "[%d-%d] Still reconnect: %s\n", g_iIndex, current->iIndex, current->strClientID);
					return;
				}
				current->bReconnect = true;
				WISECoreExTestLog(Normal, "[%d-%d] Reconnect: %s\n", g_iIndex, current->iIndex, current->strClientID);
				if(pthread_create(&reconnthread, NULL, threadreconnect, current)==0)
				{
					pthread_detach(reconnthread);
				}
			}
		} else {
			//WISECoreExTestLog(Normal, "Connect to broker: %s\n", strServerIP);
		}
		usleep(g_config.launchinterval * 1000);
	}

EXIT:
	printf("Click enter to exit [%d]\n", g_iIndex);
	{
		pthread_t inputthread;
		if(pthread_create(&inputthread, NULL, threadinput, NULL)==0)
		{
			pthread_detach(inputthread);

			while(!g_bStop)
			{
				long countdown = 0;
				prev = root;
				while(prev)
				{
					countdown += prev->reportcountdown;
					prev = prev->next;
				}
				if(countdown == 0)
				{
					printf("[%d] Report Data Countdown to 0\n", g_iIndex);
					g_bStop = true;
				}
				usleep(1000000);
			}
		}
	}
	
	prev = root;
	for(i=0; i<g_config.iTotalCount; i++)
	{
		test_contex_t* current = prev;
		if(prev == NULL)
			break;
		prev = prev->next;
	
		if(current->bRunning)
		{
			current->bRunning = false;
			//pthread_cancel(current->connThread);
			pthread_join(current->connThread, NULL);
			current->connThread = 0;
		}

		core_ex_disconnect(current->core, false);
		WISECoreExTestLog(Normal, "[%d] Client %d disconnect\n", g_iIndex, i);
		usleep(100000);
		core_ex_uninitialize(current->core);
		current->pCapabilities = NULL;
		current->core = NULL;
		free(current);
	}

	pthread_mutex_lock(&g_caplock);
	FreeCapability(pCapabilities);
	pthread_mutex_unlock(&g_caplock);
	pthread_mutex_destroy(&g_caplock);
	pthread_mutex_unlock(&g_countlock);
	pthread_mutex_destroy(&g_countlock);
	pthread_mutex_unlock(&g_writelock);
	UninitLog(WISECoreExTestLogHandle);
#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif

	return 0;
}

