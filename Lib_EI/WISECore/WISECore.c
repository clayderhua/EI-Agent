/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2016/03/01 by Scott Chang								    */
/* Modified Date: 2016/03/01 by Scott Chang									*/
/* Abstract     : WISE Core API definition									*/
/* Reference    : None														*/
/****************************************************************************/
#include "WISECore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WISEConnector.h"

WiCore_t g_tHandleCtx = NULL;

WISECORE_API bool core_initialize(char* strClientID, char* strHostName, char* strMAC, void* userdata)
{
	g_tHandleCtx = core_ex_initialize(strClientID, strHostName, strMAC, userdata);

	if(g_tHandleCtx == NULL)
		return false;
	else
		return true;
}

WISECORE_API bool core_initialize_soln(char *soln, char* strClientID, char* strHostName, char* strMAC, void* userdata)
{
	g_tHandleCtx = core_ex_initialize_soln(soln, strClientID, strHostName, strMAC, userdata);

	if (g_tHandleCtx == NULL)
		return false;
	else
		return true;
}

WISECORE_API void core_uninitialize()
{
	core_ex_uninitialize(g_tHandleCtx);
	g_tHandleCtx = NULL;
}

WISECORE_API bool core_tag_set(char* strTag)
{
	return core_ex_tag_set(g_tHandleCtx, strTag);
}

WISECORE_API bool core_product_info_set(char* strSerialNum, char* strParentID, char* strVersion, char* strType, char* strProduct, char* strManufacture)
{
	return core_ex_product_info_set(g_tHandleCtx, strSerialNum, strParentID, strVersion, strType, strProduct, strManufacture);
}

WISECORE_API bool core_account_bind(char* strLoginID, char* strLoginPW)
{
	return core_ex_account_bind(g_tHandleCtx, strLoginID, strLoginPW);
}

WISECORE_API bool core_tls_set(const char *cafile, const char *capath, const char *certfile, const char *keyfile, const char *password)
{
	return core_ex_tls_set(g_tHandleCtx, cafile, capath, certfile, keyfile, password);
}

WISECORE_API bool core_tls_psk_set(const char *psk, const char *identity, const char *ciphers)
{
	return core_ex_tls_psk_set(g_tHandleCtx, psk, identity, ciphers);
}

WISECORE_API bool core_connect(char* strServerIP, int iServerPort, char* strConnID, char* strConnPW)
{
	return core_ex_connect(g_tHandleCtx, strServerIP, iServerPort, strConnID, strConnPW);
}

WISECORE_API void core_disconnect(bool bForce)
{
	core_ex_disconnect(g_tHandleCtx, bForce);
}

WISECORE_API bool core_connection_callback_set(CORE_CONNECTED_CALLBACK on_connect, CORE_LOSTCONNECTED_CALLBACK on_lostconnect, CORE_DISCONNECT_CALLBACK on_disconnect, CORE_MESSAGE_RECV_CALLBACK on_msg_recv)
{
	return core_ex_connection_callback_set(g_tHandleCtx, on_connect, on_lostconnect, on_disconnect, on_msg_recv);
}

WISECORE_API bool core_action_callback_set(CORE_RENAME_CALLBACK on_rename, CORE_UPDATE_CALLBACK on_update)
{
	return core_ex_action_callback_set(g_tHandleCtx, on_rename, on_update);
}

WISECORE_API bool core_action_response(const int cmdid, const char * sessoinid, bool success, const char* clientid)
{
	return core_ex_action_response(g_tHandleCtx, cmdid, sessoinid, success, clientid);
}

WISECORE_API bool core_server_reconnect_callback_set(CORE_SERVER_RECONNECT_CALLBACK on_server_reconnect)
{
	return core_ex_server_reconnect_callback_set(g_tHandleCtx, on_server_reconnect);
}

WISECORE_API bool core_iot_callback_set(CORE_GET_CAPABILITY_CALLBACK on_get_capability, CORE_START_REPORT_CALLBACK on_start_report, CORE_STOP_REPORT_CALLBACK on_stop_report)
{
	return core_ex_iot_callback_set(g_tHandleCtx, on_get_capability, on_start_report, on_stop_report);
}

WISECORE_API bool core_time_tick_callback_set(CORE_GET_TIME_TICK_CALLBACK get_time_tick)
{
	return core_ex_time_tick_callback_set(g_tHandleCtx, get_time_tick);
}

WISECORE_API bool core_heartbeat_callback_set(CORE_QUERY_HEARTBEATRATE_CALLBACK on_query_heartbeatrate, CORE_UPDATE_HEARTBEATRATE_CALLBACK on_update_heartbeatrate)
{
	return core_ex_heartbeat_callback_set(g_tHandleCtx, on_query_heartbeatrate, on_update_heartbeatrate);
}

WISECORE_API bool core_heartbeatratequery_response(const int heartbeatrate, const char * sessoinid, const char* clientid)
{
	return core_ex_heartbeatratequery_response(g_tHandleCtx, heartbeatrate, sessoinid, clientid);
}

WISECORE_API bool core_publish(char const * topic, void * pkt, long pktlength, int retain, int qos)
{
	return core_ex_publish(g_tHandleCtx, topic, pkt, pktlength, retain, qos);
}

WISECORE_API bool core_device_register()
{
	return core_ex_device_register(g_tHandleCtx);
}

WISECORE_API bool core_heartbeat_send()
{
	return core_ex_heartbeat_send(g_tHandleCtx);
}

WISECORE_API bool core_subscribe(char const * topic, int qos)
{
	return core_ex_subscribe(g_tHandleCtx, topic, qos);
}

WISECORE_API bool core_unsubscribe(char const * topic)
{
	return core_ex_unsubscribe(g_tHandleCtx, topic);
}

WISECORE_API const char* core_error_string_get()
{
	return core_ex_error_string_get(g_tHandleCtx);
}
