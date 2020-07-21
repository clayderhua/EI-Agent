/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/01/20 by Scott Chang								    */
/* Modified Date: 2017/01/20 by Scott Chang									*/
/* Abstract     : WISE Carrier Extend API definition of MQTT				*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef _WISE_CARRIER_EX_MQTT_H_
#define _WISE_CARRIER_EX_MQTT_H_

#ifndef bool
#define bool unsigned char
#define false (unsigned char)0
#define true (unsigned char)1
#endif

typedef enum{
	mc_err_success = 0,
	mc_conn_accept = 400,
	mc_err_invalid_param = 500,
	mc_err_no_init,
	mc_err_init_fail,
	mc_err_already_init,
	mc_err_no_connect,
	mc_err_malloc_fail,
	mc_err_not_support,
}mc_err_code;

typedef void (*WICAR_CONNECT_CB)(void *userdata);
typedef void (*WICAR_DISCONNECT_CB)(void *userdata);
typedef void (*WICAR_LOSTCONNECT_CB)(void *userdata);
typedef void (*WICAR_MESSAGE_CB)(const char* topic, const void* payload, const int payloadlen, void *userdata);
typedef void* WiCar_t;

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <windows.h>
#ifndef WISE_CARRIER_API
	#define WISE_CARRIER_API __declspec(dllexport)
#endif
#else
	#define WISE_CARRIER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif


// Function: WiCarEx_MQTT_LibraryTag
/*
 * Can be used to obtain module tag for the connection library.
 * This allows the application to compare the library version.
 *
 * Parameters:
 *  tag   -    an char pointer. If not NULL, the tag of the module will be returned in this pointer(tag like "mosquito_3.1").
 *
 * Returns:
 *	none
 */
WISE_CARRIER_API const char* WiCarEx_MQTT_LibraryTag();

/*
 * Function: WiCarEx_MQTT_Init
 *
 * Initialize the mqtt client instance , set basic callback and assign a user data pointer.
 *
 * Parameters:
 * 	on_connect    -	Will be trigger by connect event.
 * 	on_disconnect -	Will be trigger by manual disconnect event.
 *  soln -			An eigenvalue to decide the solution of server.
 *
 * Returns:
 * 	pointer of WISE Carrier Object.
 *
 */
WISE_CARRIER_API WiCar_t WiCarEx_MQTT_Init(WICAR_CONNECT_CB on_connect, WICAR_DISCONNECT_CB on_disconnect, void *userdata);
WISE_CARRIER_API WiCar_t WiCarEx_MQTT_Init_soln(char *soln, WICAR_CONNECT_CB on_connect, WICAR_DISCONNECT_CB on_disconnect, void *userdata);
/*
 * Function: WiCarEx_MQTT_Uninit
 *
 * Release the mqtt client instance.
 *
 * Parameters:
 * pmosq	-	pointer of WISE Carrier Object.
 *
 * Returns:
 * 	boolean value for success or not.
 *
 */
WISE_CARRIER_API void WiCarEx_MQTT_Uninit(WiCar_t pmosq);

//Parameter
/*
 * Function: WiCarEx_MQTT_SetWillMsg
 *
 * Set will message and //MUST// before connect.
 *
 * Parameters:
 * 	pmosq	-	pointer of WISE Carrier Object.
 * 	topic	-	topic name
 * 	msg		-	message
 * 	msglen	-	message size
 *
 * Returns:
 * 	boolean value for success or not.
 *
 */
WISE_CARRIER_API bool WiCarEx_MQTT_SetWillMsg(WiCar_t pmosq, const char* topic, const void *msg, int msglen);

/*
 * Function: WiCarEx_MQTT_SetAuth
 *
 * Set authorization and //MUST// before connect.
 *
 * Parameters:
 * 	pmosq		-	pointer of WISE Carrier Object.
 * 	username	-	user name or NULL to disable authentication.
 * 	password	-	password
 *
 * Returns:
 * 	boolean value for success or not.
 *
 */
WISE_CARRIER_API bool WiCarEx_MQTT_SetAuth(WiCar_t pmosq, char const * username, char const * password);

/*
 * Function: WiCarEx_MQTT_SetKeepLive
 *
 * Set keep alive timeout and //MUST// before connect.
 *
 * Parameters:
 * 	pmosq		-	pointer of WISE Carrier Object.
 * 	keeplive	-	the number of seconds to kept connection.
 *
 * Returns:
 * 	boolean value for success or not.
 *
 */
WISE_CARRIER_API bool WiCarEx_MQTT_SetKeepLive(WiCar_t pmosq, int keepalive);

//TLS
/*
 * Function: WiCarEx_MQTT_SetTls
 *
 * Configure the client for certificate based SSL/TLS support. //Must// before connect.
 *
 * Cannot be used in conjunction with <WiCar_MQTT_SetTlsPsk>.
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
 * 	pmosq -			pointer of WISE Carrier Object.
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
 *	boolean value for success or not.
 *
 */
WISE_CARRIER_API bool WiCarEx_MQTT_SetTls(WiCar_t pmosq, const char *cafile, const char *capath, const char *certfile, const char *keyfile, const char* password);
/*
 * Function: WiCarEx_MQTT_SetTlsPsk
 *
 * Configure the client for pre-shared-key based TLS support. //Must// before connect.
 *
 * Cannot be used in conjunction with <WiCar_MQTT_SetTls>.
 *
 * Parameters:
 * 	pmosq -		pointer of WISE Carrier Object.
 *  psk -		the pre-shared-key in hex format with no leading "0x".
 *  identity -	the identity of this client. May be used as the username
 *				depending on the server settings.
 *	ciphers -	a string describing the PSK ciphers available for use. See the
 *				"openssl ciphers" tool for more information. If NULL, the
 *				default ciphers will be used.
 *
 * Returns:
 *	boolean value for success or not.
 *
 */
WISE_CARRIER_API bool WiCarEx_MQTT_SetTlsPsk(WiCar_t pmosq, const char *psk, const char *identity, const char *ciphers);

/*
 * Function: WiCarEx_MQTT_Connect
 *
 * Connect to MQTT server.
 *
 * Parameters:
 * 	pmosq -				pointer of WISE Carrier Object.
 * 	address -			the url or ip of the server.
 * 	port -				the network port. Usually 1883.
 * 	clientId -			a unique id for client. Usually "0000"+MAC.
 * 	on_lostconnect -	Will be called by connection lost.
 *
 * Returns:
 *  boolean value for success or not.
 */
WISE_CARRIER_API bool WiCarEx_MQTT_Connect(WiCar_t pmosq, const char* address, int port, const char* clientId, WICAR_LOSTCONNECT_CB on_lostconnect);

/*
 * Function: WiCarEx_MQTT_Reconnect
 *
 * if connecting, re-connect to MQTT server.
 *
 * Parameters:
 * 	pmosq -				pointer of WISE Carrier Object.
 *
 * Returns:
 *  boolean value for success or not.
 */
WISE_CARRIER_API bool WiCarEx_MQTT_Reconnect(WiCar_t pmosq);

/*
 * Function: WiCarEx_MQTT_Disconnect
 *
 * Disconnect from the server.
 *
 * Parameters:
 * 	pmosq -		pointer of WISE Carrier Object.
 * 	bForce -	enable flag to dsiconnect without waiting disconnect packet send.
 *
 * Returns:
 *  boolean value for success or not.
 */
WISE_CARRIER_API bool WiCarEx_MQTT_Disconnect(WiCar_t pmosq, int force);

/*
 * Function: WiCarEx_MQTT_Publish
 *
 * Publish a messge on a given topic name.
 *
 * Parameters:
 * 	pmosq	-	pointer of WISE Carrier Object.
 * 	topic 	-	the topic to publish message.
 * 	msg 	-	pointer to the data to send.
 * 	msglen 	-	the size of the payload (bytes).
 * 	retain	-	enable flag to retain this message in broker.
 *  qos		-	QoS 1, 2, 3
 *
 * Returns:
 *  the bytes of sending.
 */
WISE_CARRIER_API bool WiCarEx_MQTT_Publish(WiCar_t pmosq, const char* topic, const void *msg, int msglen, int retain, int qos);

/*
 * Function: WiCarEx_MQTT_Subscribe
 *
 * Subscribe a topic and recieve messages from assigned callback function.
 *
 * Parameters:
 * 	pmosq		-	pointer of WISE Carrier Object.
 * 	topic 		-	the topic of subscribe.
 *  qos			-	QoS 1, 2, 3
 * 	on_recieve 	-	callback function to recieve message from broker.
 *
 * Returns:
 *  boolean value for success or not.
 */
WISE_CARRIER_API bool WiCarEx_MQTT_Subscribe(WiCar_t pmosq, const char* topic, int qos, WICAR_MESSAGE_CB on_recieve);

/*
 * Function: WiCarEx_MQTT_UnSubscribe
 *
 * Unsubscribe a topic.
 *
 * Parameters:
 * 	pmosq		-	pointer of WISE Carrier Object.
 * 	topic 		-	the topic of unsubscribe.
 *
 * Returns:
 *  boolean value for success or not.
 */
WISE_CARRIER_API bool WiCarEx_MQTT_UnSubscribe(WiCar_t pmosq, const char* topic);

//error
/*
 * Function: WiCarEx_MQTT_GetCurrentErrorString
 *
 * if any error occurs or false returns, you can get error comment from this function.
 *
 * Parameters:
 * 	pmosq		-	pointer of WISE Carrier Object.
 *
 * Returns:
 *  error comment string.
 */
WISE_CARRIER_API const char *WiCarEx_MQTT_GetCurrentErrorString(WiCar_t pmosq);

#ifdef __cplusplus
}
#endif

#endif //_WISE_CARRIER_EX_MQTT_H_
