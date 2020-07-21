/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2016/03/01 by Scott Chang								    */
/* Modified Date: 2017/02/23 by Scott Chang									*/
/* Abstract     : WISE Core Test Application								*/
/* Reference    : None														*/
/****************************************************************************/
#include "network.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "sys/time.h"
#include "WISECore.h"
#include "HandlerKernel.h"
#include "util_path.h"
#include "IPSOParser.h"
#include "IoTMessageGenerate.h"
#include "WISEPlatform.h"

#define DEF_OSINFO_JSON "{\"content\":{\"cagentVersion\":\"%s\",\"cagentType\":\"%s\",\"osVersion\":\"%s\",\"biosVersion\":\"%s\",\"platformName\":\"%s\",\"processorName\":\"%s\",\"osArch\":\"%s\",\"totalPhysMemKB\":%d,\"macs\":\"%s\",\"IP\":\"%s\"},\"commCmd\":116,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":{\"$date\":%lld}}"
char g_strClientID[37] = "00000001-0000-0000-0000-305A3A77B1CC";
char g_strHostName[11] = "TestClient";
char g_strProductTag[37] = "RMM";

MSG_CLASSIFY_T* g_pCapability = NULL;
void* g_pHandler = NULL;
//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------
void SubscribeRMMTopic()
{
	char topic[256] = {0};
	sprintf(topic, DEF_CALLBACKREQ_TOPIC, g_strProductTag, g_strClientID);

	core_subscribe(topic, 0);
}

void* threadconnect(void* args)
{
	char strRecvTopic[256] = {0};

	while(!core_device_register())
	{
		usleep(500000);
	}

	printf("CB_Connected \n");

	SubscribeRMMTopic();

	while(true)
	{
		core_heartbeat_send();
		usleep(60000*1000);
	}

	pthread_exit(0);
	return NULL;
}

void on_connect_cb(void* userdata)
{
	pthread_t conn = 0;
	if(pthread_create(&conn, NULL, threadconnect, NULL)==0)
		pthread_detach(conn);
}

void on_lostconnect_cb(void* userdata)
{
	//printf("CB_Lostconnect %s\n", core_error_string_get());
}

void on_disconnect_cb(void* userdata)
{
	printf("CB_Disconnect \n");
}

/*callback function to handle threshold rule check event*/
void on_threshold_triggered(threshold_event_type type, char* sensorname, double value, MSG_ATTRIBUTE_T* attr, void *pRev)
{
	printf(" SampleHandler> threshold triggered:[%d, %s, %f]", type, sensorname, value);
}

/*callback function to handle get sensor data event*/
bool on_get_sensor(get_data_t* objlist, void *pRev)
{
	get_data_t *current = objlist;
	if(objlist == NULL) return false;

	while(current)
	{
		current->errcode = STATUSCODE_SUCCESS;
		strcpy(current->errstring, STATUS_SUCCESS);

		switch(current->attr->type)
		{
		case attr_type_numeric:
			printf(" SampleHandler> get: %s value:%d", current->sensorname, current->attr->v);
		 break;
		case attr_type_boolean:
			printf(" SampleHandler> get: %s value:%s", current->sensorname, current->attr->bv?"true":"false");
		 break;
		case attr_type_string:
			printf(" SampleHandler> get: %s value:%s", current->sensorname, current->attr->sv);
		 break;
		case attr_type_date:
			printf(" SampleHandler> get: %s value:Date:%s", current->sensorname, current->attr->sv);
		 break;
		case attr_type_timestamp:
		 printf(" SampleHandler> get: %s value:Timestamp:%d",current->sensorname, current->attr->v);
		 break;
		}

		current = current->next;
	}
	return true;
}

/*callback function to handle set sensor data event*/
bool on_set_sensor(set_data_t* objlist, void *pRev)
{
	set_data_t *current = objlist;
	if(objlist == NULL) return false;
	while(current)
	{
		current->errcode = STATUSCODE_SUCCESS;
		strcpy(current->errstring, STATUS_SUCCESS);

		switch(current->newtype)
		{
		case attr_type_numeric:
			printf(" SampleHandler> set: %s value:%d", current->sensorname, current->v);
		 break;
		case attr_type_boolean:
			printf(" SampleHandler> set: %s value:%s", current->sensorname, current->bv?"true":"false");
		 break;
		case attr_type_string:
			printf(" SampleHandler> set: %s value:%s", current->sensorname, current->sv);
		 break;
		}

		current = current->next;
	}

	return true;
}

void on_msgrecv(const char* topic, const void *pkt, const long pktlength, void* userdata)
{
	int cmdID = 0;
	char sessionID[MAX_SESSION_LEN] = {0};

	printf("Packet received:\n [%s],\n %s\n", topic, pkt);
	/* All messages received from subscribed topics will trigger this callback function.
	 * topic: received topic.
	 * pkt: received message in json string.
	 * pktlength: received message length.
	 */

	if(g_pHandler)
	{		
		/*Parse Received Command*/
		if(HandlerKernelEx_ParseRecvCMDWithSessionID((char*)pkt, &cmdID, sessionID) != handler_success)
			return;
		switch(cmdID)
		{
		case hk_get_capability_req:
			if(g_pCapability)
			{
				HandlerKernelEx_LockCapability(g_pHandler);
				HandlerKernelEx_SetCapability(g_pHandler, g_pCapability, true);
				HandlerKernelEx_UnlockCapability(g_pHandler);
			}
			break;
		case hk_auto_upload_req:
			/*start live report*/
			HandlerKernelEx_LiveReportStart(g_pHandler, hk_auto_upload_rep, (char*)pkt);
			break;
		case hk_set_thr_req:
			/*Stop threshold check thread*/
			HandlerKernelEx_StopThresholdCheck(g_pHandler);
			/*setup threshold rule*/
			HandlerKernelEx_SetThreshold(g_pHandler, hk_set_thr_rep,(char*) pkt);
			/*register the threshold check callback function to handle trigger event*/
			HandlerKernelEx_SetThresholdTrigger(g_pHandler, on_threshold_triggered);
			/*Restart threshold check thread*/
			HandlerKernelEx_StartThresholdCheck(g_pHandler);
			break;
		case hk_del_thr_req:
			/*Stop threshold check thread*/
			HandlerKernelEx_StopThresholdCheck(g_pHandler);
			/*clear threshold check callback function*/
			HandlerKernelEx_SetThresholdTrigger(g_pHandler, NULL);
			/*Delete all threshold rules*/
			HandlerKernelEx_DeleteAllThreshold(g_pHandler, hk_del_thr_rep);
			break;
		case hk_get_sensors_data_req:
			/*Get Sensor Data with callback function*/
			HandlerKernelEx_GetSensorData(g_pHandler, hk_get_sensors_data_rep, sessionID, (char*)pkt, on_get_sensor);
			break;
		case hk_set_sensors_data_req:
			/*Set Sensor Data with callback function*/
			HandlerKernelEx_SetSensorData(g_pHandler, hk_set_sensors_data_rep, sessionID, (char*)pkt, on_set_sensor);
			break;
		default:
			{
				/* Send command not support reply message*/
				char topic[128] = {0};
				char repMsg[32] = {0};
				int len = 0;
				sprintf( repMsg, "{\"errorRep\":\"Unknown cmd!\"}" );
				len= strlen( "{\"errorRep\":\"Unknown cmd!\"}" ) ;
				sprintf(topic, DEF_AGENTACT_TOPIC, g_strProductTag, g_strClientID);
				core_publish(topic, repMsg, len, 0, 0);
			}
			break;
		}
	}
}

void on_rename(const char* name, const int cmdid, const char* sessionid, const char* devid, void* userdata)
{
	printf("rename to: %s\n", name);

	core_action_response(cmdid, sessionid, true, devid);
	return;
}

void on_update(const char* loginID, const char* loginPW, const int port, const char* path, const char* md5, const int cmdid, const char* sessionid, const char* devid, void* userdata)
{
	printf("Update: %s, %s, %d, %s, %s\n", loginID, loginPW, port, path, md5);

	core_action_response(cmdid, sessionid, true, devid);
	return;
}

void on_server_reconnect(const char* devid, void* userdata)
{
	if(!strcmp(g_strClientID, devid))
		core_device_register();
	/*TODO: resend whole capability*/
}

long long get_timetick(void* userdata);

bool SendOSInfo()
{
	long long tick = 0;
	char strPayloadBuff[2048] = {0};
	char strTopicBuff[256] = {0};
	char localip[16] = {0};

	tick = get_timetick(NULL);

	network_ip_get(localip, sizeof(localip));

	snprintf(strPayloadBuff, sizeof(strPayloadBuff), DEF_OSINFO_JSON, 
												 "4.0.0",
												 "IPC",
												 "Windows 7",
												 "V1.13",
												 "SOM-5890",
												 "Intel(R) Atom(TM) CPU D525   @ 1.80GHz",
												 "X86",
												 2048,
												 "305A3A77B1DA",
												 localip,
												 g_strClientID,
												 tick);


#ifdef _WISEPAAS_02_DEF_H_
	sprintf(strTopicBuff, DEF_AGENTACT_TOPIC, g_strProductTag, g_strClientID);
#else
	sprintf(strTopicBuff, DEF_AGENTACT_TOPIC, g_strClientID);
#endif
	
	return core_publish(strTopicBuff, strPayloadBuff, strlen(strPayloadBuff), 0, 0);
}

void* threadgetcapab(void* args)
{
	SendOSInfo();


	if(g_pHandler)
	{
		HandlerKernelEx_LockCapability(g_pHandler);
		HandlerKernelEx_SetCapability(g_pHandler, g_pCapability, true);
		HandlerKernelEx_UnlockCapability(g_pHandler);
	}

	printf("on_get_capability \n");
	pthread_exit(0);
	return NULL;
}

void on_get_capability(const void *pkt, const long pktlength, const char* devid, void* userdata)
{
	/*TODO: send whole capability*/
	pthread_t getcapab = 0;
	if(pthread_create(&getcapab, NULL, threadgetcapab, NULL)==0)
		pthread_detach(getcapab);
}

void on_start_report(const void *pkt, const long pktlength, const char* devid, void* userdata)
{
	/*TODO: start report sensor data*/
	if(g_pHandler)
	{
		HandlerKernelEx_AutoReportStart(g_pHandler, pkt);
	}
}

void on_stop_report(const void *pkt, const long pktlength, const char* devid, void* userdata)
{
	/*TODO: stop report sensor data*/
	if(g_pHandler)
	{
		HandlerKernelEx_AutoReportStart(g_pHandler, pkt);
	}
}

void on_heartbeatrate_query(const char* sessionid, const char* devid, void* userdata)
{
	core_heartbeatratequery_response(60,sessionid, devid);
}

long long get_timetick(void* userdata)
{
	long long tick = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tick = (long long)tv.tv_sec*1000 + (long long)tv.tv_usec/1000;
	
	return tick;
}

void on_heartbeatrate_update(const int heartbeatrate, const char* sessionid, const char* devid, void* userdata)
{
	printf("Heartbeat Rate Update: %d, %s, %s\n", heartbeatrate, sessionid, devid);

	core_action_response(130/*wise_heartbeatrate_update_rep*/, sessionid, true, devid);
	return;
}

AGENT_SEND_STATUS send_cbf(HANDLE const handler, int enum_act, void const * const requestData, unsigned int const requestLen, void *pRev1, void* pRev2 )
{
	char* buff = NULL;
	char topic[128] = {0};
	HANDLER_INFO_EX* pHandler = NULL;
	bool bResult = false;
	int length = 0;
	if(handler == NULL || requestData == NULL)
		return cagent_send_data_error;
	pHandler = (HANDLER_INFO_EX*)handler;
	
	length =  strlen(DEF_ACTION_RESPONSE_JSON) + requestLen + 12 + strlen(pHandler->Name);
	buff = calloc(1,length);
	snprintf(buff, length, DEF_ACTION_RESPONSE_JSON, enum_act, pHandler->Name, (char*)requestData, get_timetick(NULL));

	sprintf(topic, DEF_AGENTACT_TOPIC, g_strProductTag, pHandler->agentInfo->devId);

	bResult = core_publish(topic, buff, strlen(buff), 0, 0);

	free(buff);
	if(bResult)
		return cagent_success;
	else
		return cagent_send_data_error;
}


AGENT_SEND_STATUS send_capability_cbf( HANDLE const handler, void const * const requestData, unsigned int const requestLen, void *pRev1, void* pRev2 )
{

	MSG_CLASSIFY_T* root = NULL;
	HANDLER_INFO_EX* pHandler = NULL;
	if(handler == NULL || requestData == NULL)
		return cagent_send_data_error;
	pHandler = (HANDLER_INFO_EX*)handler;
	root = IoT_CreateRoot(pHandler->Name);
	if(transfer_parse_ipso((char*)requestData,  root))
	{
		char topic[128] = {0};
		char* buff = IoT_PrintFullCapability(root, pHandler->agentInfo->devId);
		sprintf(topic, DEF_AGENTACT_TOPIC, g_strProductTag, pHandler->agentInfo->devId);
		if(core_publish(topic, buff, strlen(buff), 0, 0))
			return cagent_success;
		else
		{

			return cagent_send_data_error;
		}
	}
	return cagent_send_data_error;
}

AGENT_SEND_STATUS send_autoreport_cbf( HANDLE const handler, void const * const requestData, unsigned int const requestLen, void *pRev1, void* pRev2 )
{
	char* buff = NULL;
	char topic[128] = {0};
	HANDLER_INFO_EX* pHandler = NULL;
	bool bResult = false;
	int length = 0;
	if(handler == NULL || requestData == NULL)
		return cagent_send_data_error;
	pHandler = (HANDLER_INFO_EX*)handler;
	length =  strlen(DEF_ACTION_RESPONSE_JSON) + requestLen + 12 + strlen(pHandler->Name);
	buff = calloc(1,length);
	snprintf(buff, length, DEF_ACTION_RESPONSE_JSON, 2055, "general", (char*)requestData, get_timetick(NULL));
	sprintf(topic, DEF_AGENTREPORT_TOPIC, pHandler->agentInfo->devId);

	bResult = core_publish(topic, buff, strlen(buff), 0, 0);

	free(buff);
	if(bResult)
		return cagent_success;
	else
		return cagent_send_data_error;
}

AGENT_SEND_STATUS  send_event_cbf( HANDLE const handler, HANDLER_NOTIFY_SEVERITY severity, void const * const requestData, unsigned int const requestLen, void *pRev1, void* pRev2 )
{
	char* buff = NULL;
	char topic[128] = {0};
	HANDLER_INFO_EX* pHandler = NULL;
	bool bResult = false;
	int length = 0;
	if(handler == NULL || requestData == NULL)
		return cagent_send_data_error;
	pHandler = (HANDLER_INFO_EX*)handler;
	length =  strlen(DEF_ACTION_RESPONSE_JSON) + requestLen + 12 + strlen(pHandler->Name);
	buff = calloc(1,length);
	snprintf(buff, length, DEF_ACTION_RESPONSE_JSON, 2059, pHandler->Name, (char*)requestData, get_timetick(NULL));
	sprintf(topic, DEF_EVENTNOTIFY_TOPIC, g_strProductTag, pHandler->agentInfo->devId);

	bResult = core_publish(topic, buff, strlen(buff), 0, 0);

	free(buff);
	if(bResult)
		return cagent_success;
	else
		return cagent_send_data_error;
}

MSG_CLASSIFY_T* LoadCapability(char* path)
{
	int size = 0;
	char* buff = NULL; 
	char handler[256] = {0};
	MSG_CLASSIFY_T* root = NULL;
	size = util_file_size_get(path);
	buff = calloc(1, size+1);
	if(buff == NULL)
		return root;
	if(util_file_read(path, buff, size)==0)
		return root;
	if(!transfer_get_ipso_handlername(buff, handler))
		return root;
	root = IoT_CreateRoot(handler);
	transfer_parse_ipso(buff, root);
	free(buff);
	return root;
}

void* CreateSampleHandler(cagent_agent_info_body_t* agentinfo)
{
	void* pHandler = NULL;
	HANDLER_INFO_EX myhandler;
	memset(&myhandler, 0, sizeof(HANDLER_INFO_EX));
	strcpy(myhandler.Name, "SUSIControl");
	myhandler.sendcbf = send_cbf;
	myhandler.sendcapabilitycbf = send_capability_cbf;
	myhandler.sendreportcbf = send_autoreport_cbf;
	myhandler.sendeventcbf = send_event_cbf;
	myhandler.agentInfo = calloc(1, sizeof(cagent_agent_info_body_t));
	memcpy(myhandler.agentInfo, agentinfo, sizeof(cagent_agent_info_body_t));

	pHandler = HandlerKernelEx_Initialize((HANDLER_INFO*)&myhandler);
	g_pCapability = LoadCapability("capability.txt");
	if(g_pCapability != NULL)
		HandlerKernelEx_SetCapability(pHandler, g_pCapability,false);
	printf("Create Sample Handler\n");
	return pHandler;
}

void ReleaseSampleHandler(void* phandler)
{
	HandlerKernelEx_Uninitialize(phandler);
}

void CreateAgentInfo(cagent_agent_info_body_t* agentinfo)
{
	if(agentinfo!= NULL)
	{
		strcpy(agentinfo->hostname, g_strHostName);
		strcpy(agentinfo->devId, g_strClientID);
		strcpy(agentinfo->sn, "305A3A700000");
		strcpy(agentinfo->mac, "305A3A700000");
		strcpy(agentinfo->version, "4.0.0");
		strcpy(agentinfo->type, "IPC");
		strcpy(agentinfo->product, "test");
		strcpy(agentinfo->manufacture, "test");
		agentinfo->status = 0;
	}
}

void UpdateData(char* path, MSG_CLASSIFY_T* root)
{
	int size = 0;
	char* buff = NULL; 
	char handler[256] = {0};
	if(root == NULL)
		return;

	size = util_file_size_get(path);
	buff = calloc(1, size+1);
	if(buff == NULL)
		return;
	if(util_file_read(path, buff, size)==0)
		return;
	if(!transfer_get_ipso_handlername(buff, handler))
		return;
	transfer_parse_ipso(buff, root);
	free(buff);
}

void* threadaccessdata(void* args)
{
	
	while(true)
	{
		HandlerKernelEx_LockCapability(g_pHandler);
		UpdateData("capability.txt", g_pCapability);
		HandlerKernelEx_UnlockCapability(g_pHandler);
		usleep(1000*1000);
	}
	pthread_exit(0);
	return NULL;
}

pthread_t StartAccessData()
{
	pthread_t thread = 0;
	if(pthread_create(&thread, NULL, threadaccessdata, NULL)!=0)
		thread = 0;
	return thread;
}

void StopAccessData(pthread_t thread)
{
	if(thread != 0)
	{
		pthread_cancel(thread);
		pthread_join(thread, NULL);
	}
}

int main(int argc, char *argv[])
{
	//char strServerIP[64] = "dev-wisepaas.eastasia.cloudapp.azure.com";
	char strServerIP[64] = "172.22.12.29";
	int iPort = 1883;
	pthread_t threaddataaccess = 0;
	int SSLMode = 0;  //0:disable, 1:CA Mode, 2: PSK Mode.
	cagent_agent_info_body_t agentinfo;
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	_CrtMemCheckpoint( &memStateStart);
#endif
	CreateAgentInfo(&agentinfo);

	g_pHandler = CreateSampleHandler(&agentinfo);

	
	threaddataaccess = StartAccessData();

	if(!core_initialize(g_strClientID, g_strHostName, agentinfo.mac, NULL))
	{
		printf("Unable to initialize AgentCore.\n");
		goto EXIT;
	}
	printf("Agent Initialized\n");

	core_connection_callback_set(on_connect_cb, on_lostconnect_cb, on_disconnect_cb, on_msgrecv);

	core_action_callback_set(on_rename, on_update);

	core_server_reconnect_callback_set(on_server_reconnect);

	core_iot_callback_set(on_get_capability, on_start_report, on_stop_report);

	core_time_tick_callback_set(get_timetick);

	core_heartbeat_callback_set(on_heartbeatrate_query, on_heartbeatrate_update);

	core_tag_set(g_strProductTag);

	core_product_info_set(agentinfo.sn, NULL, agentinfo.version, agentinfo.type, agentinfo.product, agentinfo.manufacture);

	if(SSLMode == 1)
		core_tls_set( "server.crt", NULL, "ca.crt", "ca.key", "05155853");
	else if(SSLMode == 2)
		core_tls_psk_set("05155853", g_strClientID, NULL);

	if(!core_connect(strServerIP, iPort, "admin", "05155853")){
		printf("Unable to connect to broker. %s\n", core_error_string_get());
		goto EXIT;
	} else {
		printf("Connect to broker: %s\n", strServerIP);
	}
	
EXIT:
	printf("Click enter to exit\n");
	fgetc(stdin);

	StopAccessData(threaddataaccess);

	ReleaseSampleHandler(g_pHandler);

	core_disconnect(true);
	printf("Send Client Info: disconnect\n");
	core_uninitialize();
#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif

	return 0;
}

