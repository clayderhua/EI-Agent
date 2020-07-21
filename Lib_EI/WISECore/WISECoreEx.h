/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/01/23 by Scott Chang								    */
/* Modified Date: 2017/01/23 by Scott Chang									*/
/* Abstract     : WISE Core Extend API definition							*/
/* Reference    : None														*/
/****************************************************************************/

#ifndef _WISE_CORE_EX_H_
#define _WISE_CORE_EX_H_

#include <stdbool.h>

#ifdef RMM3X
	#include "wise/wisepaas_01_def.h"
#else
	#include "wise/wisepaas_02_def.h"
#endif

typedef enum {
   core_offline = 0,
   core_online = 1, 
} lite_conn_status;

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <windows.h>
#ifndef WISECORE_API
	#define WISECORE_API __declspec(dllexport)
#endif
#else
	#define WISECORE_API
#endif

typedef void (*CORE_CONNECTED_CALLBACK)(void* userdata);
typedef void (*CORE_LOSTCONNECTED_CALLBACK)(void* userdata);
typedef void (*CORE_DISCONNECT_CALLBACK)(void* userdata);
typedef void (*CORE_MESSAGE_RECV_CALLBACK)(const char* topic, const void *pkt, const long pktlength, void* userdata);
typedef void (*CORE_RENAME_CALLBACK)(const char* name, const int cmdid, const char* sessionid, const char* clientid, void* userdata);
typedef void (*CORE_UPDATE_CALLBACK)(const char* loginID, const char* loginPW, const int port, const char* path, const char* md5, const int cmdid, const char* sessionid, const char* clientid, void* userdata);
typedef void (*CORE_SERVER_RECONNECT_CALLBACK)(const char* clientid, void* userdata);
typedef void (*CORE_GET_CAPABILITY_CALLBACK)(const void *pkt, const long pktlength, const char* clientid, void* userdata);
typedef void (*CORE_START_REPORT_CALLBACK)(const void *pkt, const long pktlength, const char* clientid, void* userdata);
typedef void (*CORE_STOP_REPORT_CALLBACK)(const void *pkt, const long pktlength, const char* clientid, void* userdata);
typedef void (*CORE_QUERY_HEARTBEATRATE_CALLBACK)(const char* sessionid, const char* clientid, void* userdata);
typedef void (*CORE_UPDATE_HEARTBEATRATE_CALLBACK)(const int heartbeatrate, const char* sessionid, const char* clientid, void* userdata);
typedef long long (*CORE_GET_TIME_TICK_CALLBACK)(void* userdata);
typedef void* WiCore_t;

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Function: core_ex_initialize
 *
 * General initialization of the WISEAgentLite API. Prior to calling any WISEAgentLite API functions, 
 * the library needs to be initialized by calling this function. The status code for all 
 * WISEAgentLite API function will be lite_no_init unless this function is called.
 *
 * Parameters:
 * 	strClientID -	the unique id for connection.
 * 	strHostName -	local device name.
 * 	strMAC -		local device MAC address
 * 	userdata -		A user pointer that will be passed as an argument to any
 *                  callbacks that are specified.
 *  soln -			An eigenvalue to decide the solution of server.
 *
 * Returns:
 * 		pointer of WISE Core Object.
 */
WISECORE_API WiCore_t core_ex_initialize(char* strClientID, char* strHostName, char* strMAC, void* userdata);
WISECORE_API WiCore_t core_ex_initialize_soln(char *soln, char* strClientID, char* strHostName, char* strMAC, void* userdata);
/* 
 * Function: core_ex_uninitialize
 *
 * General function to uninitialized the WISEAgentLite API library that should be called before program exit.
 *
 * Parameters:
 * core -		pointer of WISE Core Object.
 *
 * Returns:
 * 	None
 */
WISECORE_API void core_ex_uninitialize(WiCore_t core);

/* 
 * Function: core_ex_tag_set
 *
 * assign the production tag let server to identify the target device supported functions.
 *
 * Parameters:
 *  core -				pointer of WISE Core Object.
 * 	strTag -			supported tag list format: "AAA,BBB".
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_tag_set(WiCore_t core, char* strTag);


/* 
 * Function: core_ex_product_info_set
 *
 * assign the production information let server to identify the target device.
 *
 * Parameters:
 *  core -				pointer of WISE Core Object.
 * 	strSerialNum -		device serial number.
 *  strParentID -		parent ID.
 * 	strVersion -		agent version.
 * 	strType -			device type.
 * 	strProduct -		product name.
 * 	strManufacture -	manufacture name.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_product_info_set(WiCore_t core, char* strSerialNum, char* strParentID, char* strVersion, char* strType, char* strProduct, char* strManufacture);

/* 
 * Function: core_ex_account_bind
 *
 * user account, on remote server, to management the device
 *
 * Parameters:
 *  core -					pointer of WISE Core Object.
 * 	strLoginID -		login ID.
 * 	strLoginPW -		login password.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_account_bind(WiCore_t core, char* strLoginID, char* strLoginPW);

/* 
 * Function: core_ex_connection_callback_set
 *
 * Register the callback function to handle the connection event.
 *
 * Parameters:
 *  core -				pointer of WISE Core Object.
 * 	on_connect -		Function Pointer to handle connect success event.
 *  on_lost_connect -	Function Pointer to handle lost connect event,
 *						The SAClient will reconnect automatically, if left as NULL.
 *  on_disconnect -		Function Pointer to handle disconnect event.
 *  on_msg_recv -		Function Pointer to handle message receive event.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_connection_callback_set(WiCore_t core, CORE_CONNECTED_CALLBACK on_connect, CORE_LOSTCONNECTED_CALLBACK on_lostconnect, CORE_DISCONNECT_CALLBACK on_disconnect, CORE_MESSAGE_RECV_CALLBACK on_msg_recv);

/* 
 * Function: core_ex_action_callback_set
 *
 * Register the callback function to handle the action event.
 *
 * Parameters:
 *  core -			pointer of WISE Core Object.
 * 	on_rename -		Function Pointer to handle rename event.
 *  on_update -		Function Pointer to handle update event.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_action_callback_set(WiCore_t core, CORE_RENAME_CALLBACK on_rename, CORE_UPDATE_CALLBACK on_update);

/*
 * Function: core_ex_action_response
 *
 * Send rename, update or heartbeat rate update action response back to server.
 *
 * Parameters:
 *  core -			pointer of WISE Core Object.
 *  cmdid -			command ID of request action
 * 	sessionid -		session ID of request action.
 *  success -		result of request action.
 *  clientid -		client ID of request device.
 *
 * Returns:
 *  boolean value for success or not.	
 */
WISECORE_API bool core_ex_action_response(WiCore_t core, const int cmdid, const char * sessoinid, bool success, const char* clientid);

/* 
 * Function: core_ex_server_reconnect_callback_set
 *
 * Register the callback function to handle the server reconnect event.
 *
 * Parameters:
 *  core -						pointer of WISE Core Object.
 * 	on_server_reconnect -		Function Pointer to handle server reconnect event.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_server_reconnect_callback_set(WiCore_t core, CORE_SERVER_RECONNECT_CALLBACK on_server_reconnect);

/* 
 * Function: core_ex_iot_callback_set
 *
 * Register the callback function to handle the get IoT capability or start/stop report sensor data.
 *
 * Parameters:
 *  core -					pointer of WISE Core Object.
 * 	on_get_capability	-	Function Pointer to handle get capability event.
 *  on_start_report		-	Function Pointer to handle start report event.
 *  on_stop_report		-	Function Pointer to handle stop report event.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_iot_callback_set(WiCore_t core, CORE_GET_CAPABILITY_CALLBACK on_get_capability, CORE_START_REPORT_CALLBACK on_start_report, CORE_STOP_REPORT_CALLBACK on_stop_report);

/* 
 * Function: core_ex_time_tick_callback_set
 *
 * Register the callback function to assign time tick.
 *
 * Parameters:
 *  core -					pointer of WISE Core Object.
 * 	get_time_tick	-		Function Pointer to handle get time tick event.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_time_tick_callback_set(WiCore_t core, CORE_GET_TIME_TICK_CALLBACK get_time_tick);

/* 
 * Function: core_ex_heartbeat_callback_set
 *
 * Register the callback function to handle the heartbeat rate query and update command.
 *
 * Parameters:
 *  core -						pointer of WISE Core Object.
 * 	on_query_heartbeatrate	-	Function Pointer to handle heartbeat rate query event.
 *  on_update_heartbeatrate	-	Function Pointer to handle heartbeat rate update event.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_heartbeat_callback_set(WiCore_t core, CORE_QUERY_HEARTBEATRATE_CALLBACK on_query_heartbeatrate, CORE_UPDATE_HEARTBEATRATE_CALLBACK on_update_heartbeatrate);

/*
 * Function: core_ex_heartbeatratequery_response
 *
 * Send heartbeat rate update response back to server.
 *
 * Parameters:
 *  core -			pointer of WISE Core Object.
 *  heartbeatrate - current heartbeat rate.
 * 	sessionid -		session ID of request action.
 *  clientid -		client ID of request device.
 *
 * Returns:
 *  boolean value for success or not.	
 */
WISECORE_API bool core_ex_heartbeatratequery_response(WiCore_t core, const int heartbeatrate, const char * sessoinid, const char* clientid);

/*
 * Function: core_ex_tls_set
 *
 * Configure the client for certificate based SSL/TLS support. Must be called
 * before <wc_connect>.
 *
 * Cannot be used in conjunction with <core_tls_psk_set>.
 *
 * Define the Certificate Authority certificates to be trusted (ie. the server
 * certificate must be signed with one of these certificates) using cafile.
 *
 * If the server you are connecting to requires clients to provide a
 * certificate, define certfile and keyfile with your client certificate and
 * private key. If your private key is encrypted, provide a password callback
 * function.
 *
 * Parameters:
 *  core -			pointer of WISE Core Object.
 *  cafile -		path to a file containing the PEM encoded trusted CA
 *					certificate files. Either cafile or capath must not be NULL.
 *  capath -		path to a directory containing the PEM encoded trusted CA
 *					certificate files. See mosquitto.conf for more details on
 *					configuring this directory. Either cafile or capath must not
 *					be NULL.
 *  certfile -		path to a file containing the PEM encoded certificate file
 *					for this client. If NULL, keyfile must also be NULL and no
 *					client certificate will be used.
 *  keyfile -		path to a file containing the PEM encoded private key for
 *					this client. If NULL, certfile must also be NULL and no
 *					client certificate will be used.
 *  password -		if keyfile is encrypted, set the password to allow your client
 *					to pass the correct password for decryption.
 *
 * Returns:
 * 	boolean value for success or not.	
 *
 */
WISECORE_API bool core_ex_tls_set(WiCore_t core, const char *cafile, const char *capath, const char *certfile, const char *keyfile, const char *password);

/*
 * Function: core_ex_tls_psk_set
 *
 * Configure the client for pre-shared-key based TLS support. Must be called
 * before <wc_connect>.
 *
 * Cannot be used in conjunction with <core_tls_set>.
 *
 * Parameters:
 *  core -		pointer of WISE Core Object.
 *  psk -		the pre-shared-key in hex format with no leading "0x".
 *  identity -	the identity of this client. May be used as the username
 *				depending on the server settings.
 *	ciphers -	a string describing the PSK ciphers available for use. See the
 *				"openssl ciphers" tool for more information. If NULL, the
 *				default ciphers will be used.
 *
 * Returns:
 * 	boolean value for success or not.	
 *
 */
WISECORE_API bool core_ex_tls_psk_set(WiCore_t core, const char *psk, const char *identity, const char *ciphers);

/* 
 * Function: core_ex_connect
 *
 * Connect to server that defined in Configuration data of lite_initialize parameters.
 *
 * Parameters:
 *  core -			pointer of WISE Core Object.
 * 	strServerIP -	remote server URL or IP address.
 * 	iServerPort -	connection protocol listen port.
 * 	strConnID -		connection protocol access id
 * 	strConnPW -		connection protocol access password
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_connect(WiCore_t core, char* strServerIP, int iServerPort, char* strConnID, char* strConnPW);

/* 
 * Function: core_ex_disconnect
 *
 * Disconnect from server.
 *
 * Parameters:
 *  core -		pointer of WISE Core Object.
 * 	bForce -	Is force to disconnect.
 *
 * Returns:
 * 	None
 */
WISECORE_API void core_ex_disconnect(WiCore_t core, bool bForce);

/* 
 * Function: core_ex_device_register
 *
 * Send device information, wrapped in JSON format, to register device.
 *
 * Parameters:
 *  core -		pointer of WISE Core Object.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_device_register(WiCore_t core);

/* 
 * Function: core_ex_heartbeat_send
 *
 * Send heartbeat message, wrapped in JSON format, to server.
 *
 * Parameters:
 *  core -		pointer of WISE Core Object.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_heartbeat_send(WiCore_t core);

/* 
 * Function: core_ex_publish
 *
 * Send message, wrapped in JSON format, to server on specific topic.
 *
 * Parameters:
 *  core -		pointer of WISE Core Object.
 * 	topic -		the MQTT topic to publish.
 * 	pkt -		the message to publish, in JSON string struct.
 *  pktlength -	the message length.
 * 	retain	-	enable flag to retain this message in broker.
 *  qos		-	QoS 1, 2, 3
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_publish(WiCore_t core, char const * topic, void * pkt, long pktlength, int retain, int qos);

/* 
 * Function: core_ex_subscribe
 *
 * subscribe and receive the message from server on specific topic.
 *
 * Parameters:
 *  core -		pointer of WISE Core Object.
 * 	topic	-	the topic to subscribe.
 *  qos		-	QoS 1, 2, 3
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_subscribe(WiCore_t core, char const * topic, int qos);

/* 
 * Function: core_ex_unsubscribe
 *
 * stop to receive message from server on specific topic.
 *
 * Parameters:
 *  core -		pointer of WISE Core Object.
 * 	topic -		the topic to unsubscripted.
 *
 * Returns:
 * 	boolean value for success or not.	
 */
WISECORE_API bool core_ex_unsubscribe(WiCore_t core, char const * topic);

/* 
 * Function: core_ex_error_string_get
 *
 * Call to obtain a const string description the error number.
 *
 * Parameters:
 *  core -		pointer of WISE Core Object.
 *
 * Returns:
 *	A constant string describing the error.
 */
WISECORE_API const char* core_ex_error_string_get(WiCore_t core);

#ifdef __cplusplus
}
#endif

#endif //_WISE_CORE_EX_H_
