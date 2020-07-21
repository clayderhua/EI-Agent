/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2016/01/22 by Scott Chang								    */
/* Modified Date: 2016/01/22 by Scott Chang									*/
/* Abstract     : Cross platform Network API definition	for Windows			*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef _UTIL_NETWORK_H_
#define _UTIL_NETWORK_H_

#ifdef WIN32
#include <winsock2.h>
#include <IPHlpApi.h>
#define socket_handle   SOCKET
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <fcntl.h>
#define socket_handle  int
#ifndef INVALID_SOCKET
 #define INVALID_SOCKET  ~0
#endif
#endif
#include <stdbool.h>
#include "export.h"

//************************Define private fuction return code************************
typedef enum {
   network_error = -1,
   network_success = 0,               // No error. 
   network_no_init, 
   network_callback_null,
   network_callback_error,
   network_no_connnect,
   network_connect_error,
   network_init_error,   
   network_waitsock_timeout = 0x10,
   network_waitsock_error,
   network_act_unrecognized,
   network_terminate_error,
   network_send_data_error,
   network_reg_action_comm_error,
   network_reg_callback_comm_error,
   network_report_agentinfo_error,  
   network_send_action_data_error,
   network_send_callback_response_data_error,
   network_reg_willmsg_error,
   network_send_willmsg_error,
   // S--Added by wang.nan--S
   network_mindray_main_error,
   // E--Added by wang.nan--E
} network_status_t;

typedef enum{
   network_waitsock_read     = 0x01,
   network_waitsock_write    = 0x02,
   network_waitsock_rw       = 0x03,
}network_waitsock_mode_t;

#ifdef __cplusplus
extern "C" {
#endif

WISEPLATFORM_API int network_host_name_get(char * phostname, int size);
WISEPLATFORM_API int network_ip_get(char * ipaddr, int size);
WISEPLATFORM_API int network_ip_list_get(char ipsStr[][16], int n);
WISEPLATFORM_API int network_mac_get(char * macstr);
WISEPLATFORM_API int network_mac_get_ex(char * macstr);
WISEPLATFORM_API int network_mac_list_get(char macsStr[][20], int n);
WISEPLATFORM_API int network_local_ip_get(int socket, char* clientip, int size);
WISEPLATFORM_API bool network_magic_packet_send(char * mac, int size);

#ifdef __cplusplus
}
#endif

#endif

