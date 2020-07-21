#include "SAClientLog.h"
#include "network.h"
#include "SAClient.h"
#include "WISECore.h"
#include "topic.h"
#include "scparser.h"
#include "smloader.h"
#include "des.h"
#include "base64.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "WISEPlatform.h"  //for strtok_r, strdup and strcasecmp wrapping
#include "msgqueue.h"
#include <sys/time.h>

#define DEF_DES_KEY					"29B4B9C5"
#define DEF_DES_IV					"42b19631"

void* g_coreloghandle = NULL;

susiaccess_agent_conf_body_t * g_config = NULL; 
susiaccess_agent_profile_body_t * g_profile = NULL;

bool g_bConnected = false;
bool g_bEnableHeartbeat = false;
int g_iHeartbeatRate = 60;
pthread_t g_threadHeartbeat = 0;

SACLIENT_CONNECTED_CALLBACK g_conn_cb = NULL;
SACLIENT_LOSTCONNECT_CALLBACK g_lost_conn_cb = NULL;
SACLIENT_DISCONNECT_CALLBACK g_disconn_cb = NULL;

bool DES_BASE64Decode(char * srcBuf,char **destBuf)
{
	bool bRet = false;
	if(srcBuf == NULL || destBuf == NULL) return bRet;
	{
		char *base64Dec = NULL;
		int decLen = 0;
		int len = strlen(srcBuf);
		int iRet = Base64Decode(srcBuf, len, &base64Dec, &decLen);
		if(iRet == 0)
		{
			char plaintext[512] = {0};
			iRet = DESDecodeEx(DEF_DES_KEY, DEF_DES_IV,  base64Dec, decLen, plaintext);
			if(iRet == 0)
			{
				*destBuf = (char *)malloc(len + 1);
				memset(*destBuf, 0, len + 1);
				strcpy(*destBuf, plaintext);
				bRet = true;
			}
		}
		if(base64Dec) free(base64Dec);
	}

	return bRet;
}

void saclient_get_devid(const char* topic, char* devid)
{
	char *start = NULL, *end = NULL;
	int pos = 3;
	if(topic == NULL) return;
	if(devid == NULL) return;
	start = strstr(topic, "/wisepaas/"); //verify support topic start with "/wisepaas/"
	if(start)
	{
		while(pos >0)
		{
			start = strstr(start, "/")+1;
			if(start == 0)
				return;
			pos--;
		}
		end = strstr(start, "/");
		if(end)
			strncpy(devid, start, end-start);
	}
	else
	{
		start = strstr(topic, "/cagent/admin/");
		if(start)
		{
			start += strlen("/cagent/admin/");
			end = strstr(start, "/");
			if(end)
				strncpy(devid, start, end-start);
		}
	}
}

void* threadheartbeat(void* args)
{
	int interval=1000;
	int iHeartbeat = g_iHeartbeatRate;

	smloader_internal_subscribe(g_profile);

	while(!core_device_register())
	{
		SAClientLog(g_coreloghandle, Warning, "Send Agent Info Fail!");
		if(!g_bEnableHeartbeat)
			break;
		usleep(500000);
	}

	SAClientLog(g_coreloghandle, Debug, "Send Agent Info Success!");

	smloader_connect_status_update(AGENT_STATUS_ONLINE);

	if(g_conn_cb != NULL)
		g_conn_cb();

	SAClientLog(g_coreloghandle, Debug, "Start heart beat thread");
	if(iHeartbeat>0)
	{
		if( core_heartbeat_send())
		{
			SAClientLog(g_coreloghandle, Debug, "Send heartbeat");
		}
		else {
			SAClientLog(g_coreloghandle, Error, "Send heartbeat failed, error code: %s", core_error_string_get());
		}
	}

	while(g_bEnableHeartbeat)
	{
		int count = 0;
		if(g_iHeartbeatRate!=iHeartbeat)
		{
			iHeartbeat = g_iHeartbeatRate;
		}
		count = iHeartbeat*1000/interval;
		while(count > 0)
		{
			if(g_iHeartbeatRate!=iHeartbeat)
			{
				iHeartbeat = g_iHeartbeatRate;
				count = iHeartbeat*1000/interval;
			}

			if(!g_bEnableHeartbeat)
				break;

			count--;
			usleep(interval*1000);
		}
		if(!g_bEnableHeartbeat)
			break;

		if(iHeartbeat>0)
		{
			if( core_heartbeat_send())
			{
				SAClientLog(g_coreloghandle, Debug, "Send heartbeat");
			}
			else {
				SAClientLog(g_coreloghandle, Error, "Send heartbeat failed, error code: %s", core_error_string_get());
			}
		}
	}

	SAClientLog(g_coreloghandle, Debug, "Stop heart beat thread");
	pthread_exit(0);
	return 0;
}


void saclient_on_connect_cb(void* userdata)
{
	pthread_t onconn = 0;
	g_bConnected = true;

	if(g_threadHeartbeat  == 0)
	{
		g_bEnableHeartbeat = true;
		if(pthread_create(&g_threadHeartbeat, NULL, threadheartbeat, NULL)!=0)
			g_threadHeartbeat = 0;
	}
	SAClientLog(g_coreloghandle, Normal, "Broker connected!");
}

void saclient_on_lost_connect_cb(void* userdata)
{
	g_bConnected = false;

	SAClientLog(g_coreloghandle, Error, "Broker Lost connect! %s", core_error_string_get());

	if(g_threadHeartbeat)
	{
		g_bEnableHeartbeat = false;
		pthread_join(g_threadHeartbeat, NULL);
		g_threadHeartbeat = 0;
	}

	smloader_connect_status_update(AGENT_STATUS_OFFLINE);

	if(g_lost_conn_cb != NULL)
		g_lost_conn_cb();
}

void saclient_on_disconnect_cb(void* userdata)
{
	g_bConnected = false;

	if(g_threadHeartbeat)
	{
		g_bEnableHeartbeat = false;
		pthread_join(g_threadHeartbeat, NULL);
		g_threadHeartbeat = 0;
	}

	smloader_connect_status_update(AGENT_STATUS_OFFLINE);
	
	if(g_disconn_cb != NULL)
		g_disconn_cb();
	SAClientLog(g_coreloghandle, Normal, "Broker Disconnected!");
}

void saclient_on_server_reconnect(const char* devid, void* userdata)
{
	/* No need to check device id
	if(strlen(devid) > 0)
		if(strcmp(devid, g_profile->devId))
			return;
	*/
	SAClientLog(g_coreloghandle, Debug, "Received Server Reconnect Command");
	core_device_register();
	smloader_connect_status_update(AGENT_STATUS_ONLINE);
}

void saclient_on_heartbeatrate_query(const char* sessionid, const char* devid, void* userdata)
{
	core_heartbeatratequery_response(g_iHeartbeatRate,sessionid, devid);
}

void saclient_on_heartbeatrate_update(const int heartbeatrate, const char* sessionid, const char* devid, void* userdata)
{
	SAClientLog(g_coreloghandle, Debug, "Heartbeat Rate Update: %d, %s, %s", heartbeatrate, sessionid, devid);

	g_iHeartbeatRate = heartbeatrate;

	core_action_response(130/*wise_heartbeatrate_update_rep*/, sessionid, true, devid);
	return;
}

void  saclient_on_recv(const char* topic, const void *pkt, const long pktlength, void* userdata)
{
	struct saclient_topic_entry * target = saclient_topic_find(topic);
	SAClientLog(g_coreloghandle, Debug, "Received Topic: %s, Data: %s", topic, (char *)pkt);
	if(target != NULL)
	{
		susiaccess_packet_body_t packet;
		
		scparser_message_parse(pkt, pktlength, &packet);
		if(packet.content == NULL) {
			SAClientLog(g_coreloghandle, Warning, "packet.content is NULL: %s, Data: %s", topic, (char *)pkt);
			packet.content = calloc(1, 3);
			strncpy(packet.content, "{}", 3); // Assign an empty json object to prevent null pointer access violated.
		}

		if(strlen(packet.devId) == 0)
		{
			char devid[37]={0};
			saclient_get_devid(topic, devid);
			strcpy(packet.devId, devid);
		}

		if(target->callback_func)
			target->callback_func(topic, &packet, NULL, NULL);
		if(packet.content)
			free(packet.content);
	}
}

long long saclient_get_timetick(void* userdata)
{
	long long tick = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tick = (long long)tv.tv_sec*1000 + (long long)tv.tv_usec/1000;
	return tick;
}

int saclient_priv_connect()
{
	int iRet = saclient_false;
	bool bRet = false;
	int serverport = 0;
	char* loginID = NULL;
	char* loginPwd = NULL;
	char* desSrc = NULL;
	char* token = NULL; 
	if(!g_config)
		return iRet;

	if(DES_BASE64Decode(g_config->serverAuth, &desSrc))
	{
		if(strstr(desSrc, ";")>0)
		{
			loginID = strtok_r(desSrc, ";", &token);
			loginPwd =  strtok_r(NULL, ";", &token);
		}
		else
		{
			loginID = strtok_r(desSrc, ":", &token);
			loginPwd =  strtok_r(NULL, ":", &token);
		}
	}
	else
	{
		desSrc = strdup(g_config->serverAuth);
		if(strstr(desSrc, ";")>0)
		{
			loginID = strtok_r(desSrc, ";", &token);
			loginPwd =  strtok_r(NULL, ";", &token);
		}
		else
		{
			loginID = strtok_r(desSrc, ":", &token);
			loginPwd =  strtok_r(NULL, ":", &token);
		}
	}

	SAClientLog(g_coreloghandle, Normal, "Connecting to broker: %s", g_config->serverIP);
	// skip invalid ip address
	if (strcmp(g_config->serverIP, "0.0.0.0") == 0) {
		SAClientLog(g_coreloghandle, Normal, "Connecting to broker: %s", g_config->serverIP);
		return iRet;
	}

	serverport = atoi(g_config->serverPort);
	
	bRet = core_connect(g_config->serverIP, serverport, loginID, loginPwd);

	if(desSrc)
	{
		free(desSrc);
		desSrc = NULL;
		loginID = NULL;
		loginPwd = NULL;
	}

	if(!bRet)
	{
		smloader_connect_status_update(AGENT_STATUS_CONNECTION_FAILED);

		SAClientLog(g_coreloghandle, Error, "Unable to connect to broker: %s, error: %s.", g_config->serverIP, core_error_string_get());
		return iRet;
	}
	else
	{
			iRet = saclient_success;
	}
	return iRet;
}

void saclient_priv_disconnect()
{
	struct saclient_topic_entry *iter_topic = NULL;
	struct saclient_topic_entry *tmp_topic = NULL;
	
	g_bConnected = false;
	core_disconnect(false);

	iter_topic = saclient_topic_first();
	while(iter_topic != NULL)
	{
		tmp_topic = iter_topic->next;
		core_unsubscribe(iter_topic->name);
		saclient_topic_remove(iter_topic->name);
		iter_topic = tmp_topic;
	}
}

int SACLIENT_API saclient_initialize(susiaccess_agent_conf_body_t * config, susiaccess_agent_profile_body_t * profile, void * loghandle)
{
	int iRet = saclient_false;
	if(config == NULL)
	{
		iRet = saclient_config_error;
		return iRet;
	}

	if(profile == NULL)
	{
		iRet = saclient_profile_error;
		return iRet;
	}

	if(config->tlstype == tls_type_tls)
	{
		FILE *fptr = NULL;
		//if((strlen(config->cafile)==0 && strlen(config->capath)==0) || (strlen(config->certfile)>0 && strlen(config->keyfile)==0) || (strlen(config->certfile)==0 && strlen(config->keyfile)>0)) return saclient_config_error;
		if(strlen(config->certfile) > 0)
		{
			fptr = fopen(config->certfile, "r");
			if(fptr){
				fclose(fptr);
			}else{
				return saclient_config_error;
			}
		}

		if(strlen(config->cafile) > 0)
		{
			fptr = fopen(config->cafile, "r");
			if(fptr){
				fclose(fptr);
			}else{
				return saclient_config_error;
			}
		}

		if(strlen(config->keyfile) > 0)
		{
			fptr = fopen(config->keyfile, "r");
			if(fptr){
				fclose(fptr);
			}else{
				return saclient_config_error;
			}
		}
	}

	if(loghandle != NULL)
	{
		g_coreloghandle = loghandle;
		SAClientLog(g_coreloghandle, Debug, "Start logging: %s", __FUNCTION__);
	}
	else
	{
		g_coreloghandle = NULL;
	}

	if(core_initialize_soln(config->solution, profile->devId, profile->hostname, profile->mac, NULL))
	{
		if(config->tlstype == tls_type_tls)
		{
			if( core_tls_set(strlen(config->cafile)>0?config->cafile:NULL, strlen(config->capath)>0?config->capath:NULL, strlen(config->certfile)>0?config->certfile:NULL, strlen(config->keyfile)>0?config->keyfile:NULL, strlen(config->cerpasswd)>0?config->cerpasswd:NULL))
				iRet = saclient_success;
			else
			{
				SAClientLog(g_coreloghandle, Error, "Mosquitto SetTLS Failed, error: %s", core_error_string_get());
				iRet = saclient_config_error;
				return iRet;
			}
		}
		else if(config->tlstype == tls_type_psk)
		{
			if(core_tls_psk_set(config->psk, config->identity, strlen(config->ciphers)>0?config->ciphers:NULL))
				iRet = saclient_success;
			else
			{
				SAClientLog(g_coreloghandle, Error, "Mosquitto SetTLSPSK Failed, error: %s", core_error_string_get());
				iRet = saclient_config_error;
				return iRet;
			}
		}
		iRet = saclient_success;
		core_connection_callback_set(saclient_on_connect_cb, saclient_on_lost_connect_cb, saclient_on_disconnect_cb, msgqueue_on_recv);

		core_action_callback_set(NULL, NULL);

		core_server_reconnect_callback_set(saclient_on_server_reconnect);

		core_time_tick_callback_set(saclient_get_timetick);

		core_heartbeat_callback_set(saclient_on_heartbeatrate_query, saclient_on_heartbeatrate_update);

		core_tag_set(profile->productId);

		core_product_info_set(profile->sn, profile->parentID, profile->version, profile->type, profile->product, profile->manufacture);

		core_account_bind(profile->account, profile->passwd);

		//core_local_ip_set(NULL);

	}
	else
	{
		iRet = saclient_false;
		return iRet;
	}

	g_config = malloc(sizeof(susiaccess_agent_conf_body_t));
	memset(g_config, 0, sizeof(susiaccess_agent_conf_body_t));
	memcpy(g_config, config, sizeof(susiaccess_agent_conf_body_t));
	g_profile = malloc(sizeof(susiaccess_agent_profile_body_t));
	memset(g_profile, 0, sizeof(susiaccess_agent_profile_body_t));
	memcpy(g_profile, profile, sizeof(susiaccess_agent_profile_body_t));

	/*Init Message Queue*/
	msgqueue_init(1000, saclient_on_recv);

	/*Load SAManager*/
	smloader_init(g_config, g_profile, g_coreloghandle);
	smloader_callback_set(saclient_publish, saclient_subscribe, saclient_server_connect_ssl, saclient_disconnect, saclient_unsubscribe);
	
	return iRet;
}

void SACLIENT_API  saclient_uninitialize()
{
	smloader_callback_set(NULL, NULL, NULL, NULL, NULL);

	saclient_disconnect();

	msgqueue_uninit();

	/*Release SAManager*/	
	smloader_uninit();
	
	g_conn_cb = NULL;
	g_lost_conn_cb = NULL;
	g_disconn_cb = NULL;

	if(g_config != NULL)
	{
		free(g_config);
		g_config = NULL;
	}
	if(g_profile != NULL)
	{
		free(g_profile);
		g_profile = NULL;
	}
	
	core_connection_callback_set(NULL, NULL, NULL, NULL);

	core_action_callback_set(NULL, NULL);

	core_time_tick_callback_set(NULL);

	core_heartbeat_callback_set(NULL, NULL);

	if(g_threadHeartbeat)
	{
		g_bEnableHeartbeat = false;
		pthread_join(g_threadHeartbeat, NULL);
		g_threadHeartbeat = 0;
	}

	core_uninitialize();
	g_coreloghandle = NULL;
}

int SACLIENT_API saclient_reinitialize(susiaccess_agent_conf_body_t * config, susiaccess_agent_profile_body_t * profile)
{
	int iRet = saclient_false;

	/*Uninitialize*/
	core_connection_callback_set(NULL, NULL, NULL, NULL);

	core_action_callback_set(NULL, NULL);

	core_heartbeat_callback_set(NULL, NULL);

	saclient_priv_disconnect();

	core_time_tick_callback_set(NULL);

	/*Reset Message Queue*/
	msgqueue_clear();

	core_uninitialize();

	if(g_config == NULL)
	{
		iRet = saclient_config_error;
		return iRet;
	}

	if(g_profile == NULL)
	{
		iRet = saclient_profile_error;
		return iRet;
	}

	if(config)
	{
		memcpy(g_config, config, sizeof(susiaccess_agent_conf_body_t));
	}

	if(profile)
	{
		memcpy(g_profile, profile, sizeof(susiaccess_agent_profile_body_t));
	}

	if(core_initialize_soln(g_config->solution, g_profile->devId, g_profile->hostname, g_profile->mac, NULL))
	{
		if(config->tlstype == tls_type_tls)
		{
			if(core_tls_set(strlen(g_config->cafile)>0?g_config->cafile:NULL, strlen(g_config->capath)>0?g_config->capath:NULL, strlen(g_config->certfile)>0?g_config->certfile:NULL, strlen(g_config->keyfile)>0?g_config->keyfile:NULL, strlen(g_config->cerpasswd)>0?g_config->cerpasswd:NULL))
				iRet = saclient_success;
			else
			{
				SAClientLog(g_coreloghandle, Error, "Mosquitto SetTLS Failed, error code: %s", core_error_string_get());
				iRet = saclient_config_error;
				return iRet;
			}
		}
		else if(g_config->tlstype == tls_type_psk)
		{
			if(core_tls_psk_set(g_config->psk, g_config->identity, strlen(g_config->ciphers)>0?g_config->ciphers:NULL))
				iRet = saclient_success;
			else
			{
				SAClientLog(g_coreloghandle, Error, "Mosquitto SetTLSPSK Failed, error: %s", core_error_string_get());
				iRet = saclient_config_error;
				return iRet;
			}
		}
		iRet = saclient_success;
		core_connection_callback_set(saclient_on_connect_cb, saclient_on_lost_connect_cb, saclient_on_disconnect_cb, msgqueue_on_recv);

		core_action_callback_set(NULL, NULL);

		core_server_reconnect_callback_set(saclient_on_server_reconnect);

		core_heartbeat_callback_set(saclient_on_heartbeatrate_query, saclient_on_heartbeatrate_update);

		core_time_tick_callback_set(saclient_get_timetick);

		core_tag_set(g_profile->productId);

		core_product_info_set(g_profile->sn, g_profile->parentID, g_profile->version, g_profile->type, g_profile->product, g_profile->manufacture);

		core_account_bind(g_profile->account, g_profile->passwd);
	}
	else
		return iRet;

	return iRet;
}

int SACLIENT_API saclient_connect()
{
	return saclient_priv_connect();
}

void SACLIENT_API saclient_reconnect()
{
	saclient_reinitialize(NULL, NULL);
	saclient_priv_connect();
}

int SACLIENT_API  saclient_server_connect(char const * ip, int port, char const * mqttauth)
{
	return saclient_server_connect_ssl(ip, port, mqttauth, tls_type_none, NULL);
}

int SACLIENT_API  saclient_server_connect_ssl(char const * ip, int port, char const * mqttauth, tls_type tlstype, char const * psk)
{
	susiaccess_agent_conf_body_t config;

	if(!g_config)
		return saclient_config_error;

	memset(&config, 0, sizeof(susiaccess_agent_conf_body_t));
	memcpy(&config, g_config, sizeof(susiaccess_agent_conf_body_t));

	if(ip)
	{
		strncpy(config.serverIP, ip, strlen(ip)+1);
	}

	if(port>0)
	{
		sprintf(config.serverPort, "%d", port);
	}

	if(mqttauth)
	{
		strncpy(config.serverAuth, mqttauth, strlen(mqttauth)+1);
	}

	config.tlstype = tlstype;

	if(psk)
	{
		strncpy(config.psk, psk, strlen(psk)+1);
	}

	return  saclient_server_connect_config(&config, NULL);
}

int SACLIENT_API  saclient_server_connect_config(susiaccess_agent_conf_body_t * config, susiaccess_agent_profile_body_t * profile)
{
	saclient_reinitialize(config, profile);

	return  saclient_connect();
}

void SACLIENT_API saclient_disconnect()
{
	saclient_priv_disconnect();
}


void SACLIENT_API  saclient_connection_callback_set(SACLIENT_CONNECTED_CALLBACK on_connect, SACLIENT_LOSTCONNECT_CALLBACK on_lost_connect, SACLIENT_DISCONNECT_CALLBACK on_disconnect)
{
	g_conn_cb = on_connect;
	g_lost_conn_cb = on_lost_connect;
	g_disconn_cb = on_disconnect;
}

int SACLIENT_API  saclient_publish(char const * topic, int qos, int retain, susiaccess_packet_body_t const * pkt)
{
	int iRet = saclient_false;
	char* pPayload = NULL;
	
	if(!pkt)
		return saclient_send_data_error;

	if(!g_bConnected)
		return saclient_no_connect;

	pPayload = scparser_packet_print(pkt);

	if(!pPayload)
	{
		SAClientLog(g_coreloghandle, Error, "Incorrect Packet Format: <name: %s, command: %d, content: %s>", pkt->handlerName, pkt->cmd, pkt->content);
		return saclient_send_data_error;
	}
	if( core_publish((char *)topic, pPayload, strlen(pPayload), retain, qos))
	{
		iRet = saclient_success;
		SAClientLog(g_coreloghandle, Debug, "Send Topic: %s, Data: %s", topic, pPayload);
	}
	else {
		SAClientLog(g_coreloghandle, Error, "Send Failed, error code: %s, Packet: <name: %s, command: %d, content: %s>", core_error_string_get(), pkt->handlerName, pkt->cmd, pkt->content);
	}
	free(pPayload);
	return iRet;
}

int SACLIENT_API  saclient_subscribe(char const * topic, int qos, SACLIENT_MESSAGE_RECV_CALLBACK msg_recv_callback)
{
	int iRet = saclient_false;

	if(!msg_recv_callback)
		return saclient_callback_null;
	
	if(core_subscribe((char *)topic, qos))
	{
		struct saclient_topic_entry * entry = saclient_topic_find(topic);
		if(entry == NULL)
		{
			saclient_topic_add(topic, (TOPIC_MESSAGE_CB)msg_recv_callback);
		}
		else
		{
			entry->callback_func = (TOPIC_MESSAGE_CB)msg_recv_callback;
		}
		iRet = saclient_success;
		SAClientLog(g_coreloghandle, Debug, "Subscribe Topic: %s", topic);
	}
	
	return iRet;
}

int SACLIENT_API  saclient_unsubscribe(char const * topic)
{
	int iRet = saclient_false;

	if(!topic)
		return iRet;
	
	if(core_unsubscribe((char *)topic))
	{
		struct saclient_topic_entry * entry = saclient_topic_find(topic);
		if(entry)
		{
			saclient_topic_remove(topic);
		}
		iRet = saclient_success;
		SAClientLog(g_coreloghandle, Debug, "Unsubscribe Topic: %s", topic);
	}
	
	return iRet;
}

int SACLIENT_API  saclient_getsocketaddress(char* clientip, int size)
{
	/*Not supported by WISECore*/
	return saclient_false;
}
