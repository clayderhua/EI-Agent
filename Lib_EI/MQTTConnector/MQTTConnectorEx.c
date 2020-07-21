/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/01/20 by Scott Chang								    */
/* Modified Date: 2017/01/20 by Scott Chang									*/
/* Abstract     : WISE Connector Extend API definition						*/
/* Reference    : None														*/
/****************************************************************************/
#include "WISEConnectorEx.h"
#include "WiseCarrierEx_MQTT.h"
#include <stdio.h>

typedef struct{
	WiCar_t *mosq;

	WC_CONNECT_CALLBACK on_connect_cb;
	WC_LOSTCONNECT_CALLBACK on_lostconnect_cb;
	WC_DISCONNECT_CALLBACK on_disconnect_cb;
	WC_MESSAGE_CALLBACK on_message_cb;

	char *clientID;
	void *userdata;

}mosq_conn_t;

void wc_ex_connect_callback(void *userdata)
{	
	mosq_conn_t* pmosq = NULL;
	if(userdata == NULL)
		return;
	pmosq = (mosq_conn_t*)userdata;
	//printf("wc_connect_callback...\n");
	if(pmosq->on_connect_cb!=NULL)
		pmosq->on_connect_cb(pmosq->userdata);
}

void wc_ex_disconnect_callback(void *userdata)
{	
	mosq_conn_t* pmosq = NULL;
	if(userdata == NULL)
		return;
	pmosq = (mosq_conn_t*)userdata;
	//printf("wc_disconnect_callback...\n");
	if(pmosq->on_disconnect_cb!=NULL)
		pmosq->on_disconnect_cb(pmosq->userdata);
}

void wc_ex_lostconnect_callback(void *userdata)
{
	mosq_conn_t* pmosq = NULL;
	if(userdata == NULL)
		return;
	pmosq = (mosq_conn_t*)userdata;
	//printf("wc_lostconnect_callback...\n");
	if(pmosq->on_lostconnect_cb!=NULL)
		pmosq->on_lostconnect_cb(pmosq->userdata);
}

void wc_ex_message_callback(const char* topic, const void* payload, const int payloadlen, void *userdata)
{	
	mosq_conn_t* pmosq = NULL;
	if(userdata == NULL)
		return;
	pmosq = (mosq_conn_t*)userdata;
	//printf("wc_message_callback...\n");
	if(pmosq->on_message_cb!=NULL)
		pmosq->on_message_cb(topic,payload,payloadlen,pmosq->userdata);
}


WISE_CONNECTOR_API WiConn_t wc_ex_initialize(char const * devid, void* userdata)
{	
	mosq_conn_t* pmosq = calloc(1, sizeof(mosq_conn_t));

	WiCar_t *mosq = WiCarEx_MQTT_Init(wc_ex_connect_callback, wc_ex_disconnect_callback, pmosq);
	if(mosq)
	{	
		//printf("wc_initialize...\n");
		pmosq->userdata=userdata;
		//printf("g_userdata : %u\n",g_userdata);
		pmosq->clientID=(char *)devid;
		//printf("devid: %s\n",clientID);
		pmosq->mosq = mosq;
		return (WiConn_t)pmosq;
	}
	else
	{
		free(pmosq);
		return NULL;
	}
}

WISE_CONNECTOR_API WiConn_t wc_ex_initialize_soln(char *soln, char const * devid, void* userdata)
{
	mosq_conn_t* pmosq = calloc(1, sizeof(mosq_conn_t));

	WiCar_t *mosq = WiCarEx_MQTT_Init_soln(soln, wc_ex_connect_callback, wc_ex_disconnect_callback, pmosq);
	if (mosq)
	{
		//printf("wc_initialize...\n");
		pmosq->userdata = userdata;
		//printf("g_userdata : %u\n",g_userdata);
		pmosq->clientID = (char *)devid;
		//printf("devid: %s\n",clientID);
		pmosq->mosq = mosq;
		return (WiConn_t)pmosq;
	}
	else
	{
		free(pmosq);
		return NULL;
	}
}


WISE_CONNECTOR_API void wc_ex_uninitialize(WiConn_t conn)
{
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		WiCarEx_MQTT_Uninit(pmosq->mosq);
		pmosq->mosq = NULL;
		free(pmosq);
	}
}


WISE_CONNECTOR_API void wc_ex_callback_set(WiConn_t conn, WC_CONNECT_CALLBACK connect_cb, WC_LOSTCONNECT_CALLBACK lostconnect_cb, WC_DISCONNECT_CALLBACK disconnect_cb, WC_MESSAGE_CALLBACK message_cb)
{
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		pmosq->on_connect_cb=connect_cb;
		pmosq->on_lostconnect_cb=lostconnect_cb;
		pmosq->on_disconnect_cb=disconnect_cb;
		pmosq->on_message_cb=message_cb;
	}
}


WISE_CONNECTOR_API bool wc_ex_tls_set(WiConn_t conn, const char *cafile, const char *capath, const char *certfile, const char *keyfile, const char* password)
{
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		if(WiCarEx_MQTT_SetTls(pmosq->mosq, cafile, capath, certfile, keyfile, password))
			return true;
		else
			return false;
	}
	else
		return false;
}


WISE_CONNECTOR_API bool wc_ex_tls_psk_set(WiConn_t conn, const char *psk, const char *identity, const char *ciphers)
{
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		if(WiCarEx_MQTT_SetTlsPsk(pmosq->mosq, psk, identity, ciphers))
			return true;
		else
			return false;
	}
	else
		return false;
}


WISE_CONNECTOR_API bool wc_ex_connect(WiConn_t conn, char const * ip, int port, char const * username, char const * password, int keepalive, char* willtopic, const void *willmsg, int msglen )
{	
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		if(WiCarEx_MQTT_SetKeepLive(pmosq->mosq, keepalive))
			if(WiCarEx_MQTT_SetAuth(pmosq->mosq, username, password))
				if(WiCarEx_MQTT_SetWillMsg(pmosq->mosq, willtopic, willmsg, msglen))
					if(WiCarEx_MQTT_Connect(pmosq->mosq, ip, port, pmosq->clientID, wc_ex_lostconnect_callback))
						return true;
					else
						return false;
				else
					return false;
			else
				return false;
		else
			return false;
	}
	else
		return false;
}


WISE_CONNECTOR_API bool wc_ex_disconnect(WiConn_t conn, bool bForce)
{
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		if(WiCarEx_MQTT_Disconnect(pmosq->mosq, bForce))
			return true;
		else
			return false;
	}
	else
		return false;
}


WISE_CONNECTOR_API bool wc_ex_publish(WiConn_t conn, const char* topic, const void *msg, int msglen, int retain, int qos)
{
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		if(WiCarEx_MQTT_Publish(pmosq->mosq, topic,msg,msglen,retain,qos))
			return true;
		else
			return false;
	}
	else
		return false;
}

WISE_CONNECTOR_API bool wc_ex_subscribe(WiConn_t conn, const char* topic, int qos)
{
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		if(WiCarEx_MQTT_Subscribe(pmosq->mosq, topic, qos,wc_ex_message_callback))
			return true;
		else
			return false;
	}
	else
		return false;
}

WISE_CONNECTOR_API bool wc_ex_unsubscribe(WiConn_t conn, const char* topic)
{
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		if(WiCarEx_MQTT_UnSubscribe(pmosq->mosq, topic))
			return true;
		else
			return false;
	}
	else
		return false;
}

WISE_CONNECTOR_API const char* wc_ex_current_error_string_get(WiConn_t conn)
{
	if(conn)
	{
		mosq_conn_t* pmosq = (mosq_conn_t*)conn;
		return WiCarEx_MQTT_GetCurrentErrorString(pmosq->mosq);
	}
	else
		return NULL;
}
