/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2016/03/21 by Scott68 Chang							    */
/* Modified Date: 2016/03/21 by Scott68 Chang								*/
/* Abstract     : Mosquitto Carrier API definition						    */
/* Reference    : None														*/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "WiseCarrier_MQTT.h"

mc_err_code g_iErrorCode = mc_err_success;
WiCar_t g_mosq = NULL;

WISE_CARRIER_API const char * WiCar_MQTT_LibraryTag()
{
	return WiCarEx_MQTT_LibraryTag();
}

WISE_CARRIER_API bool WiCar_MQTT_Init(WICAR_CONNECT_CB on_connect, WICAR_DISCONNECT_CB on_disconnect, void *userdata)
{
	if(g_mosq)
	{
		g_iErrorCode = mc_err_already_init;
		return false;
	}
	g_mosq = WiCarEx_MQTT_Init(on_connect, on_disconnect, userdata);
	if(g_mosq == NULL){
		g_iErrorCode = mc_err_malloc_fail;
		return false;
	}
	return true;
}

WISE_CARRIER_API bool WiCar_MQTT_Init_soln(char *soln, WICAR_CONNECT_CB on_connect, WICAR_DISCONNECT_CB on_disconnect, void *userdata)
{
	if (g_mosq)
	{
		g_iErrorCode = mc_err_already_init;
		return false;
	}
	g_mosq = WiCarEx_MQTT_Init_soln(soln, on_connect, on_disconnect, userdata);
	if (g_mosq == NULL) {
		g_iErrorCode = mc_err_malloc_fail;
		return false;
	}
	return true;
}

WISE_CARRIER_API void WiCar_MQTT_Uninit()
{
	WiCarEx_MQTT_Uninit(g_mosq);
	g_iErrorCode = mc_err_success;
	g_mosq = NULL;

}

WISE_CARRIER_API bool WiCar_MQTT_SetWillMsg(const char* topic, const void *msg, int msglen)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}

	return WiCarEx_MQTT_SetWillMsg(g_mosq, topic, msg, msglen);
}

WISE_CARRIER_API bool WiCar_MQTT_SetAuth(char const * username, char const * password)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}

	return WiCarEx_MQTT_SetAuth(g_mosq, username, password);
}

WISE_CARRIER_API bool WiCar_MQTT_SetKeepLive(int keepalive)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}

	return WiCarEx_MQTT_SetKeepLive(g_mosq, keepalive);
}

WISE_CARRIER_API bool WiCar_MQTT_SetTls(const char *cafile, const char *capath, const char *certfile, const char *keyfile, const char* password)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}
	return WiCarEx_MQTT_SetTls(g_mosq, cafile, capath, certfile, keyfile, password);
}

WISE_CARRIER_API bool WiCar_MQTT_SetTlsPsk(const char *psk, const char *identity, const char *ciphers)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}
	return WiCarEx_MQTT_SetTlsPsk(g_mosq, psk, identity, ciphers);
}

WISE_CARRIER_API bool WiCar_MQTT_Connect(const char* address, int port, const char* clientId, WICAR_LOSTCONNECT_CB on_lostconnect)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}
	return WiCarEx_MQTT_Connect(g_mosq, address, port, clientId, on_lostconnect);
}

WISE_CARRIER_API bool WiCar_MQTT_Reconnect()
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}
	
	return WiCarEx_MQTT_Reconnect(g_mosq);
}

WISE_CARRIER_API bool WiCar_MQTT_Disconnect(int force)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}

	return WiCarEx_MQTT_Disconnect(g_mosq, force);
}

WISE_CARRIER_API bool WiCar_MQTT_Publish(const char* topic, const void *msg, int msglen, int retain, int qos)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}
	return WiCarEx_MQTT_Publish(g_mosq, topic, msg, msglen, retain, qos);
}

WISE_CARRIER_API bool WiCar_MQTT_Subscribe(const char* topic, int qos, WICAR_MESSAGE_CB on_recieve)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}

	return WiCarEx_MQTT_Subscribe(g_mosq, topic, qos, on_recieve);
}

WISE_CARRIER_API bool WiCar_MQTT_UnSubscribe(const char* topic)
{
	if(!g_mosq){
		g_iErrorCode = mc_err_no_init;
		return false;
	}

	return WiCarEx_MQTT_UnSubscribe(g_mosq, topic);
}

WISE_CARRIER_API const char *WiCar_MQTT_GetCurrentErrorString()
{
	if(g_mosq == NULL)
		return "No initialized.";
	else
		return WiCarEx_MQTT_GetCurrentErrorString(g_mosq);
}
