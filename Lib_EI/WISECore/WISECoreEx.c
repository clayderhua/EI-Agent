/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/01/23 by Scott Chang								    */
/* Modified Date: 2017/01/23 by Scott Chang									*/
/* Abstract     : WISE Core Extend API definition							*/
/* Reference    : None														*/
/****************************************************************************/
#include "WISECoreEx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
#include "liteparse.h"
#include "version.h"
#include "WISEConnectorEx.h"

#ifdef WIN32
	#define snprintf(dst,size,format,...) _snprintf(dst,size,format,##__VA_ARGS__)
#endif

typedef struct{
	/*clinet info, user must define*/
	char strClientID[DEF_DEVID_LENGTH];
	char strHostName[DEF_HOSTNAME_LENGTH];
	char strMAC[DEF_MAC_LENGTH];

	/*product info*/
	char strSerialNum[DEF_SN_LENGTH];
	char strVersion[DEF_MAX_STRING_LENGTH];
	char strType[DEF_MAX_STRING_LENGTH];
	char strProduct[DEF_MAX_STRING_LENGTH];
	char strManufacture[DEF_MAX_STRING_LENGTH];
	char strTag[DEF_MAX_STRING_LENGTH];
	char strParentID[DEF_DEVID_LENGTH];
	
	/*account bind*/
	char strLoginID[DEF_USER_PASS_LENGTH];
	char strLoginPW[DEF_USER_PASS_LENGTH];

	/*client status*/
	WiConn_t conn;
	lite_conn_status iStatus;
	int pSocketfd;

	CORE_CONNECTED_CALLBACK on_connect_cb;
	CORE_LOSTCONNECTED_CALLBACK on_lostconnect_cb;
	CORE_DISCONNECT_CALLBACK on_disconnect_cb;
	CORE_MESSAGE_RECV_CALLBACK on_msg_recv_cb;
	CORE_RENAME_CALLBACK on_rename_cb;
	CORE_UPDATE_CALLBACK on_update_cb;
	CORE_SERVER_RECONNECT_CALLBACK on_server_reconnect;
	CORE_GET_CAPABILITY_CALLBACK on_get_capability;
	CORE_START_REPORT_CALLBACK on_start_report;
	CORE_STOP_REPORT_CALLBACK on_stop_report;
	CORE_QUERY_HEARTBEATRATE_CALLBACK on_query_heartbeatrate;
	CORE_UPDATE_HEARTBEATRATE_CALLBACK on_update_heartbeatrate;
	CORE_GET_TIME_TICK_CALLBACK on_get_timetick;

	bool bInited;
	char strPayloadBuff[4096];
	char strTopicBuff[128];
	int iErrorCode;
	long long tick;
	void* userdata;
}core_contex_t;

typedef enum {
	core_success = 0,               // No error.
	core_param_error,
	core_no_init, 
	core_no_connnect,
	core_buff_not_enough,
	core_internal_error,
	core_not_support = 5000
} core_result;

bool _ex_get_agentinfo_string(core_contex_t* pHandle, lite_conn_status iStatus, char* strInfo, int iLength)
{
	int iRet = 0;
	
	long long tick = 0;

	if(!pHandle)
		return false;

	if(!strInfo)
	{
		pHandle->iErrorCode = core_param_error;
		return false;
	}

	if(pHandle->on_get_timetick)
		tick = pHandle->on_get_timetick(pHandle->userdata);
	else
	{
		//tick = (long long) time((time_t *) NULL);
		tick = pHandle->tick;
		pHandle->tick++;
	}
#ifdef _WISEPAAS_02_DEF_H_
	iRet = snprintf(strInfo, iLength, DEF_AGENTINFO_JSON, pHandle->strParentID?pHandle->strParentID:"",
												 pHandle->strHostName?pHandle->strHostName:"",
												 pHandle->strSerialNum?pHandle->strSerialNum:(pHandle->strMAC?pHandle->strMAC:""),
												 pHandle->strMAC?pHandle->strMAC:"",
												 pHandle->strVersion?pHandle->strVersion:"",
												 pHandle->strType?pHandle->strType:"IPC",
												 pHandle->strProduct?pHandle->strProduct:"",
												 pHandle->strManufacture?pHandle->strManufacture:"",
												 pHandle->strLoginID?pHandle->strLoginID:"anonymous",
												 pHandle->strLoginPW?pHandle->strLoginPW:"",
												 iStatus,
												 pHandle->strTag?pHandle->strTag:"",
												 pHandle->strClientID,
												 tick);
#else
	iRet = snprintf(strInfo, iLength, DEF_AGENTINFO_JSON, pHandle->strClientID?pHandle->strClientID:"",
												 pHandle->strParentID?pHandle->strParentID:"",
												 pHandle->strHostName?pHandle->strHostName:"",
												 pHandle->strSerialNum?pHandle->strSerialNum:(pHandle->strMAC?pHandle->strMAC:""),
												 pHandle->strMAC?pHandle->strMAC:"",
												 pHandle->strVersion?pHandle->strVersion:"",
												 pHandle->strType?pHandle->strType:"IPC",
												 pHandle->strProduct?pHandle->strProduct:"",
												 pHandle->strManufacture?pHandle->strManufacture:"",
												 pHandle->strLoginID?pHandle->strLoginID:"anonymous",
												 pHandle->strLoginPW?pHandle->strLoginPW:"",
												 iStatus,
												 pHandle->strClientID,
												 tick);
#endif
	if(iRet>=0)
	{
		pHandle->iErrorCode = core_success;
		return true;
	}
	else
	{
		pHandle->iErrorCode = core_buff_not_enough;
		return false;
	}
}

bool _ex_send_agent_connect(core_contex_t* pHandle)
{
	if(!pHandle)
		return false;

	if(pHandle->iStatus != core_online)
	{
		pHandle->iErrorCode = core_no_connnect;
		return false;
	}

	if(!_ex_get_agentinfo_string(pHandle, core_online, pHandle->strPayloadBuff, sizeof(pHandle->strPayloadBuff)))
		return false;

	sprintf(pHandle->strTopicBuff, DEF_INFOACK_TOPIC, pHandle->strClientID);
	
	if(wc_ex_publish(pHandle->conn, (char *)pHandle->strTopicBuff, pHandle->strPayloadBuff, strlen(pHandle->strPayloadBuff), false, 0))
	{
		pHandle->iErrorCode = core_success;
		return true;
	}
	else
	{
		pHandle->iErrorCode = core_internal_error;
		return false;
	}
}

bool _ex_send_agent_disconnect(core_contex_t* pHandle)
{
	if(!pHandle)
		return false;

	if(pHandle->iStatus != core_online)
	{
		pHandle->iErrorCode = core_no_connnect;
		return false;
	}

	if(!_ex_get_agentinfo_string(pHandle, core_offline, pHandle->strPayloadBuff, sizeof(pHandle->strPayloadBuff)))
		return false;
	sprintf(pHandle->strTopicBuff, DEF_INFOACK_TOPIC, pHandle->strClientID);
	if(wc_ex_publish(pHandle->conn, (char *)pHandle->strTopicBuff, pHandle->strPayloadBuff, strlen(pHandle->strPayloadBuff), false, 0))
	{
		pHandle->iErrorCode = core_success;
		return true;
	}
	else
	{
		pHandle->iErrorCode = core_internal_error;
		return false;
	}
}

void _ex_on_connect_cb(void *pUserData)
{
	core_contex_t* pHandle = NULL;
	
	if(!pUserData)
		return;
	pHandle = (core_contex_t*)pUserData;
	if(pHandle)
	{
		pHandle->pSocketfd = 0;
		pHandle->iStatus = core_online;
	}

	if(pHandle->on_connect_cb)
		pHandle->on_connect_cb(pHandle->userdata);
}

void _ex_on_lostconnect_cb(void *pUserData)
{
	core_contex_t* pHandle = NULL;
	
	if(!pUserData)
		return;
	pHandle = (core_contex_t*)pUserData;

	if(pHandle)
	{
		pHandle->pSocketfd = -1;
		pHandle->iStatus = core_offline;
		pHandle->iErrorCode = core_internal_error;
	}

	if(pHandle->on_lostconnect_cb)
		pHandle->on_lostconnect_cb(pHandle->userdata);
}

void _ex_on_disconnect_cb(void *pUserData)
{
	core_contex_t* pHandle = NULL;
	
	if(!pUserData)
		return;
	pHandle = (core_contex_t*)pUserData;

	if(pHandle)
	{
		pHandle->iStatus = core_offline;
		pHandle->pSocketfd = -1;
	}

	if(pHandle->on_disconnect_cb)
		pHandle->on_disconnect_cb(pHandle->userdata);
}

void _ex_on_rename(core_contex_t* pHandle, char* cmd, const char* strClientID)
{
	// {"commCmd":113,"catalogID":4,"handlerName":"general","sessionID":"0BD843BFB2A34E60A56C3B686BB41C90", "devName":"TestClient_123"}
	char strName[DEF_HOSTNAME_LENGTH] = {0};
	char strSessionID[33] = {0};

	if(!pHandle)
		return;

	lp_value_get(cmd, "sessionID", strSessionID, sizeof(strSessionID));
	lp_value_get(cmd, "devName", strName, sizeof(strName));

	strncpy(pHandle->strHostName, strName, sizeof(pHandle->strHostName));

	if(pHandle->on_rename_cb)
		pHandle->on_rename_cb(strName, wise_cagent_rename_rep, strSessionID, strClientID, pHandle->userdata);
}

void _ex_on_update(core_contex_t* pHandle, char* cmd, const char* strClientID)
{
	/*{"commCmd":111,"catalogID":4,"requestID":16,"params":{"userName":"sa30Read","pwd":"sa30Read","port":2121,"path":"/upgrade/SA30Agent_V3.0.15.exe","md5":"758C9D0A8654A93D09F375D33E262507"}}*/
	char strUserName[DEF_USER_PASS_LENGTH] = {0};
	char strPwd[DEF_USER_PASS_LENGTH] = {0};
	char strPath[DEF_MAX_PATH] = {0};
	char strMD5[33] = {0};
	char strPort[6] ={0};
	char strSessionID[33] = {0};
	int iPort = 2121;
	
	if(!pHandle)
		return;

	lp_value_get(cmd, "sessionID", strSessionID, sizeof(strSessionID));
	lp_value_get(cmd, "userName", strUserName, sizeof(strUserName));
	lp_value_get(cmd, "pwd", strPwd, sizeof(strPwd));
	lp_value_get(cmd, "path", strPath, sizeof(strPath));
	lp_value_get(cmd, "md5", strMD5, sizeof(strMD5));
	if(lp_value_get(cmd, "port", strPort, sizeof(strPort)))
	{
		iPort = atoi(strPort);
	}

	if(pHandle->on_update_cb)
		pHandle->on_update_cb(strUserName, strPwd, iPort, strPath, strMD5, wise_update_cagent_rep, strSessionID, strClientID, pHandle->userdata);
}

void _ex_on_heartbeatrate_query(core_contex_t* pHandle, char* cmd, const char* strClientID)
{
	// {"commCmd":127,"catalogID":4,"handlerName":"general","sessionID":"0BD843BFB2A34E60A56C3B686BB41C90"}
	char strSessionID[33] = {0};
	if(!pHandle)
		return;

	lp_value_get(cmd, "sessionID", strSessionID, sizeof(strSessionID));

	if(pHandle->on_query_heartbeatrate)
		pHandle->on_query_heartbeatrate(strSessionID, strClientID, pHandle->userdata);
}

void _ex_on_heartbeatrate_update(core_contex_t* pHandle, char* cmd, const char* strClientID)
{
	// {"commCmd":129,"catalogID":4,"handlerName":"general","sessionID":"0BD843BFB2A34E60A56C3B686BB41C90", "heartbeatrate":60}
	char strRate[33] = {0};
	char strSessionID[33] = {0};
	int iRate;
	if(!pHandle)
		return;
	lp_value_get(cmd, "sessionID", strSessionID, sizeof(strSessionID));
	lp_value_get(cmd, "heartbeatrate", strRate, sizeof(strRate));
	iRate = atoi(strRate);
	if(pHandle->on_update_heartbeatrate)
		pHandle->on_update_heartbeatrate(iRate, strSessionID, strClientID, pHandle->userdata);
}

void _ex_on_server_reconnect(core_contex_t* pHandle, const char* strClientID)
{
	if(!pHandle)
		return;
	if(pHandle->on_server_reconnect)
		pHandle->on_server_reconnect(strClientID, pHandle->userdata);
}

void _ex_get_devid(const char* topic, char* devid)
{
	char *start = NULL, *end = NULL;
#ifdef _WISEPAAS_02_DEF_H_
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
#else
	if(topic == NULL) return;
	if(devid == NULL) return;
	start = strstr(topic, "/cagent/admin/");
	if(start)
	{
		start += strlen("/cagent/admin/");
		end = strstr(start, "/");
		if(end)
			strncpy(devid, start, end-start);
	}
#endif
}

void _ex_get_custom(const char* topic, char* custom)
{
#ifdef _WISEPAAS_02_DEF_H_
	int pos = 2;
	char *start = NULL, *end = NULL;
	if(topic == NULL) return;
	if(custom == NULL) return;
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
			strncpy(custom, start, end-start);
	}
#else
	strcpy(custom, "");
#endif
}

bool _ex_check_cmd(char *payload, char *fmt, wise_comm_cmd_t comm) {
	char cmd[16] = {0};
	sprintf(cmd, fmt, comm);
	return strstr((char*)payload, cmd)!=0?true:false;
}

void _ex_on_message_recv(const char* topic, const void* payload, const int payloadlen, void *pUserData)
{
	char devID[DEF_DEVID_LENGTH] = {0};
	char customID[DEF_DEVID_LENGTH] = {0};
	core_contex_t* pHandle = NULL;
	
	if(!pUserData)
		return;
	pHandle = (core_contex_t*)pUserData;

	_ex_get_devid(topic, devID);
	_ex_get_custom(topic, customID);

	if(strstr((char*)payload, DEF_GENERAL_HANDLER))
	{
		if(pHandle->on_rename_cb && _ex_check_cmd(payload, DEF_WISE_COMMAND, wise_cagent_rename_req))
		{
			_ex_on_rename(pHandle, (char*)payload, devID);
			return;
	    }
		if(pHandle->on_update_cb && _ex_check_cmd(payload, DEF_WISE_COMMAND, wise_update_cagent_req))
		{
			_ex_on_update(pHandle, (char*)payload, devID);
			return;
	    }
		if(_ex_check_cmd(payload, DEF_WISE_COMMAND, wise_server_control_req))
		{
			if(_ex_check_cmd(payload, DEF_SERVERCTL_STATUS, 4)) //server reconnect.
			{
				_ex_on_server_reconnect(pHandle, devID);
				return;
			}
		}
		if(_ex_check_cmd(payload, DEF_WISE_COMMAND, wise_heartbeatrate_query_req))
		{
			_ex_on_heartbeatrate_query(pHandle, (char*)payload, devID);
			return;
		}
		if(_ex_check_cmd(payload, DEF_WISE_COMMAND, wise_heartbeatrate_update_req))
		{
			_ex_on_heartbeatrate_update(pHandle, (char*)payload, devID);
			return;
		}
		if(pHandle->on_get_capability && _ex_check_cmd(payload, DEF_WISE_COMMAND, wise_get_capability_req))
		{
			pHandle->on_get_capability(payload, payloadlen, devID, pHandle->userdata);
			return;
		}
		if(pHandle->on_start_report && _ex_check_cmd(payload, DEF_WISE_COMMAND, wise_start_report_req))
		{
			pHandle->on_start_report(payload, payloadlen, devID, pHandle->userdata);
			return;
		}
		if(pHandle->on_stop_report && _ex_check_cmd(payload, DEF_WISE_COMMAND, wise_stop_report_req))
		{
			pHandle->on_stop_report(payload, payloadlen, devID, pHandle->userdata);
			return;
		}
	}
	else if(strstr((char*)payload, DEF_SERVERREDUNDANCY_HANDLER))
	{
		if(_ex_check_cmd(payload, DEF_WISE_COMMAND, wise_server_control_req))
		{
			if(_ex_check_cmd(payload, DEF_SERVERCTL_STATUS, 4)) //server reconnect.
			{
				_ex_on_server_reconnect(pHandle, devID);
				return;
	        }
        }
	}

	if(pHandle->on_msg_recv_cb)
		pHandle->on_msg_recv_cb(topic, payload, payloadlen, pHandle->userdata);
}


WISECORE_API WiCore_t core_ex_initialize(char* strClientID, char* strHostName, char* strMAC, void* userdata)
{
	core_contex_t* tHandleCtx = NULL;
	WiConn_t conn = NULL;

	if(!strClientID)
		return tHandleCtx;

	if(!strHostName)
		return tHandleCtx;

	if(!strMAC)
		return tHandleCtx;

	tHandleCtx = calloc(1, sizeof(core_contex_t));

	tHandleCtx->tick = 0;
	strncpy(tHandleCtx->strClientID, strClientID, sizeof(tHandleCtx->strClientID));
	strncpy(tHandleCtx->strHostName, strHostName, sizeof(tHandleCtx->strHostName));
	strncpy(tHandleCtx->strMAC, strMAC, sizeof(tHandleCtx->strMAC));

	conn = wc_ex_initialize(tHandleCtx->strClientID, tHandleCtx);
	if(!conn)
	{
		free(tHandleCtx);
		return NULL;
	}
	tHandleCtx->conn = conn;
	tHandleCtx->userdata = userdata;
	wc_ex_callback_set(conn, _ex_on_connect_cb, _ex_on_lostconnect_cb, _ex_on_disconnect_cb, _ex_on_message_recv);
	tHandleCtx->bInited = true;
	tHandleCtx->iErrorCode = core_success;
	return tHandleCtx;
}

WISECORE_API WiCore_t core_ex_initialize_soln(char *soln, char* strClientID, char* strHostName, char* strMAC, void* userdata)
{
	core_contex_t* tHandleCtx = NULL;
	WiConn_t conn = NULL;

	if (!strClientID)
		return tHandleCtx;

	if (!strHostName)
		return tHandleCtx;

	if (!strMAC)
		return tHandleCtx;

	tHandleCtx = calloc(1, sizeof(core_contex_t));

	tHandleCtx->tick = 0;
	strncpy(tHandleCtx->strClientID, strClientID, sizeof(tHandleCtx->strClientID));
	strncpy(tHandleCtx->strHostName, strHostName, sizeof(tHandleCtx->strHostName));
	strncpy(tHandleCtx->strMAC, strMAC, sizeof(tHandleCtx->strMAC));

	conn = wc_ex_initialize_soln(soln, tHandleCtx->strClientID, tHandleCtx);
	if (!conn)
	{
		free(tHandleCtx);
		return NULL;
	}
	tHandleCtx->conn = conn;
	tHandleCtx->userdata = userdata;
	wc_ex_callback_set(conn, _ex_on_connect_cb, _ex_on_lostconnect_cb, _ex_on_disconnect_cb, _ex_on_message_recv);
	tHandleCtx->bInited = true;
	tHandleCtx->iErrorCode = core_success;
	return tHandleCtx;
}

WISECORE_API void core_ex_uninitialize(WiCore_t core)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return;
	tHandleCtx = (core_contex_t*) core;
	tHandleCtx->iErrorCode = core_success;
	if(tHandleCtx->bInited)
	{
		wc_ex_callback_set(tHandleCtx->conn, NULL, NULL, NULL, NULL);
		wc_ex_uninitialize(tHandleCtx->conn);
		tHandleCtx->conn = NULL;
	}
	tHandleCtx->tick = 0;
	free(tHandleCtx);
}

WISECORE_API bool core_ex_tag_set(WiCore_t core, char* strTag)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(strTag)
	{
		strncpy(tHandleCtx->strTag, strTag, sizeof(tHandleCtx->strTag));
	}
	tHandleCtx->iErrorCode = core_success;
	return true;
}


WISECORE_API bool core_ex_product_info_set(WiCore_t core, char* strSerialNum, char* strParentID, char* strVersion, char* strType, char* strProduct, char* strManufacture)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(strSerialNum)
		strncpy(tHandleCtx->strSerialNum, strSerialNum, sizeof(tHandleCtx->strSerialNum));

	if(strVersion)
		strncpy(tHandleCtx->strVersion, strVersion, sizeof(tHandleCtx->strVersion));
	else
	{
		sprintf(tHandleCtx->strVersion, "%d.%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);
	}

	if(strType)
		strncpy(tHandleCtx->strType, strType, sizeof(tHandleCtx->strType));
	
	if(strProduct)
		strncpy(tHandleCtx->strProduct, strProduct, sizeof(tHandleCtx->strProduct));

	if(strManufacture)
		strncpy(tHandleCtx->strManufacture, strManufacture, sizeof(tHandleCtx->strManufacture));

	if(strParentID)
		strncpy(tHandleCtx->strParentID, strParentID, sizeof(tHandleCtx->strParentID));

	tHandleCtx->iErrorCode = core_success;
	return true;
}

WISECORE_API bool core_ex_account_bind(WiCore_t core, char* strLoginID, char* strLoginPW)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(strLoginID)
		strncpy(tHandleCtx->strLoginID, strLoginID, sizeof(tHandleCtx->strLoginID));
	
	if(strLoginPW)
		strncpy(tHandleCtx->strLoginPW, strLoginPW, sizeof(tHandleCtx->strLoginPW));

	tHandleCtx->iErrorCode = core_success;
	return true;
}

WISECORE_API bool core_ex_tls_set(WiCore_t core, const char *cafile, const char *capath, const char *certfile, const char *keyfile, const char *password)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	/*if(!cafile && !capath)
	{
		tHandleCtx->iErrorCode = core_param_error;
		return false;
	}

	if(!certfile)
	{
		tHandleCtx->iErrorCode = core_param_error;
		return false;
	}

	if(!keyfile)
	{
		tHandleCtx->iErrorCode = core_param_error;
		return false;
	}*/

	if(wc_ex_tls_set(tHandleCtx->conn, cafile, capath, certfile, keyfile, password))
	{
		tHandleCtx->iErrorCode = core_success;
		return true;
	}
	else
	{
		tHandleCtx->iErrorCode = core_internal_error;
		return false;
	}
}

WISECORE_API bool core_ex_tls_psk_set(WiCore_t core, const char *psk, const char *identity, const char *ciphers)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(wc_ex_tls_psk_set(tHandleCtx->conn, psk, identity?identity:tHandleCtx->strClientID, ciphers))
	{
		tHandleCtx->iErrorCode = core_success;
		return true;
	}
	else
	{
		tHandleCtx->iErrorCode = core_internal_error;
		return false;
	}
}

WISECORE_API bool core_ex_connect(WiCore_t core, char* strServerIP, int iServerPort, char* strConnID, char* strConnPW)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(!strServerIP)
	{
		tHandleCtx->iErrorCode = core_param_error;
		return false;
	}

	if(!_ex_get_agentinfo_string(tHandleCtx, core_offline, tHandleCtx->strPayloadBuff, sizeof(tHandleCtx->strPayloadBuff)))
		return false;
	sprintf(tHandleCtx->strTopicBuff, DEF_WILLMSG_TOPIC, tHandleCtx->strClientID);
	if(wc_ex_connect(tHandleCtx->conn, strServerIP, iServerPort, strConnID, strConnPW, 120, tHandleCtx->strTopicBuff, tHandleCtx->strPayloadBuff, strlen(tHandleCtx->strPayloadBuff)))
	{
		tHandleCtx->iErrorCode = core_success;
		return true;
	}
	else
	{
		tHandleCtx->iErrorCode = core_internal_error;
		return false;
	}
}

WISECORE_API void core_ex_disconnect(WiCore_t core, bool bForce)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return;
	tHandleCtx = (core_contex_t*) core;

	tHandleCtx->iErrorCode = core_success;
	if(!tHandleCtx->bInited)
		return;

	if(tHandleCtx->iStatus == core_online)
	{
		_ex_send_agent_disconnect(tHandleCtx);
	}

	wc_ex_disconnect(tHandleCtx->conn, bForce);
	tHandleCtx->iErrorCode = core_success;
	return;
}

WISECORE_API bool core_ex_connection_callback_set(WiCore_t core, CORE_CONNECTED_CALLBACK on_connect, CORE_LOSTCONNECTED_CALLBACK on_lostconnect, CORE_DISCONNECT_CALLBACK on_disconnect, CORE_MESSAGE_RECV_CALLBACK on_msg_recv)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	tHandleCtx->on_connect_cb = on_connect;
	tHandleCtx->on_lostconnect_cb = on_lostconnect;
	tHandleCtx->on_disconnect_cb = on_disconnect;
	tHandleCtx->on_msg_recv_cb = on_msg_recv;

	tHandleCtx->iErrorCode = core_success;
	return true;
}

WISECORE_API bool core_ex_action_callback_set(WiCore_t core, CORE_RENAME_CALLBACK on_rename, CORE_UPDATE_CALLBACK on_update)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	tHandleCtx->on_rename_cb = on_rename;
	tHandleCtx->on_update_cb = on_update;

	tHandleCtx->iErrorCode = core_success;
	return true;
}

WISECORE_API bool core_ex_action_response(WiCore_t core, const int cmdid, const char * sessoinid, bool success, const char* clientid)
{
	core_contex_t* tHandleCtx = NULL;
	long long tick = 0;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(tHandleCtx->iStatus != core_online)
	{
		tHandleCtx->iErrorCode = core_no_connnect;
		return false;
	}

	if(tHandleCtx->on_get_timetick)
		tick = tHandleCtx->on_get_timetick(tHandleCtx->userdata);
	else
	{
		//tick = (long long) time((time_t *) NULL);
		tick = tHandleCtx->tick;
		tHandleCtx->tick++;
	}

	if(sessoinid)
		snprintf(tHandleCtx->strPayloadBuff, sizeof(tHandleCtx->strPayloadBuff), DEF_ACTION_RESULT_SESSION_JSON, clientid?clientid:tHandleCtx->strClientID, cmdid, success?"SUCCESS":"FALSE", sessoinid, tick);
	else
		snprintf(tHandleCtx->strPayloadBuff, sizeof(tHandleCtx->strPayloadBuff), DEF_ACTION_RESULT_JSON, clientid?clientid:tHandleCtx->strClientID, cmdid, success?"SUCCESS":"FALSE", tick);
#ifdef _WISEPAAS_02_DEF_H_
	sprintf(tHandleCtx->strTopicBuff, DEF_AGENTACT_TOPIC, DEF_PRESERVE_PRODUCT_NAME, clientid?clientid:tHandleCtx->strClientID);
#else
	sprintf(tHandleCtx->strTopicBuff, DEF_AGENTACT_TOPIC, clientid?clientid:tHandleCtx->strClientID);
#endif
	if(wc_ex_publish(tHandleCtx->conn, tHandleCtx->strTopicBuff, tHandleCtx->strPayloadBuff, strlen(tHandleCtx->strPayloadBuff), false, 0))
	{
		tHandleCtx->iErrorCode = core_success;
		return true;
	}
	else
	{
		tHandleCtx->iErrorCode = core_internal_error;
		return false;
	}
}

WISECORE_API bool core_ex_server_reconnect_callback_set(WiCore_t core, CORE_SERVER_RECONNECT_CALLBACK on_server_reconnect)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	tHandleCtx->on_server_reconnect = on_server_reconnect;

	tHandleCtx->iErrorCode = core_success;
	return true;
}

WISECORE_API bool core_ex_iot_callback_set(WiCore_t core, CORE_GET_CAPABILITY_CALLBACK on_get_capability, CORE_START_REPORT_CALLBACK on_start_report, CORE_STOP_REPORT_CALLBACK on_stop_report)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	tHandleCtx->on_get_capability = on_get_capability;
	tHandleCtx->on_start_report = on_start_report;
	tHandleCtx->on_stop_report = on_stop_report;

	tHandleCtx->iErrorCode = core_success;
	return true;
}

WISECORE_API bool core_ex_time_tick_callback_set(WiCore_t core, CORE_GET_TIME_TICK_CALLBACK get_time_tick)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}
	tHandleCtx->on_get_timetick = get_time_tick;

	tHandleCtx->iErrorCode = core_success;
	return true;
}

WISECORE_API bool core_ex_heartbeat_callback_set(WiCore_t core, CORE_QUERY_HEARTBEATRATE_CALLBACK on_query_heartbeatrate, CORE_UPDATE_HEARTBEATRATE_CALLBACK on_update_heartbeatrate)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	tHandleCtx->on_query_heartbeatrate = on_query_heartbeatrate;
	tHandleCtx->on_update_heartbeatrate = on_update_heartbeatrate;

	tHandleCtx->iErrorCode = core_success;
	return true;
}

WISECORE_API bool core_ex_heartbeatratequery_response(WiCore_t core, const int heartbeatrate, const char * sessoinid, const char* clientid)
{
	core_contex_t* tHandleCtx = NULL;
	long long tick = 0;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(tHandleCtx->iStatus != core_online)
	{
		tHandleCtx->iErrorCode = core_no_connnect;
		return false;
	}

	if(tHandleCtx->on_get_timetick)
		tick = tHandleCtx->on_get_timetick(tHandleCtx->userdata);
	else
	{
		//tick = (long long) time((time_t *) NULL);
		tick = tHandleCtx->tick;
		tHandleCtx->tick++;
	}

	sprintf(tHandleCtx->strPayloadBuff, DEF_HEARTBEATRATE_RESPONSE_SESSION_JSON, clientid, wise_heartbeatrate_query_rep, heartbeatrate, sessoinid, tick);
#ifdef _WISEPAAS_02_DEF_H_
	sprintf(tHandleCtx->strTopicBuff, DEF_AGENTACT_TOPIC, DEF_PRESERVE_PRODUCT_NAME, clientid);
#else
	sprintf(tHandleCtx->strTopicBuff, DEF_AGENTACT_TOPIC, clientid);
#endif
	if(wc_ex_publish(tHandleCtx->conn, (char *)tHandleCtx->strTopicBuff, tHandleCtx->strPayloadBuff, strlen(tHandleCtx->strPayloadBuff), false, 0))
	{
		tHandleCtx->iErrorCode = core_success;
		return true;
	}
	else
	{
		tHandleCtx->iErrorCode = core_internal_error;
		return false;
	}
}

WISECORE_API bool core_ex_publish(WiCore_t core, char const * topic, void * pkt, long pktlength, int retain, int qos)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(!topic)
	{
		tHandleCtx->iErrorCode = core_param_error;
		return false;
	}

	if(!pkt)
	{
		tHandleCtx->iErrorCode = core_param_error;
		return false;
	}

	if(tHandleCtx->iStatus != core_online)
	{
		tHandleCtx->iErrorCode = core_no_connnect;
		return false;
	}

	if(wc_ex_publish(tHandleCtx->conn, (char *)topic, pkt, pktlength, retain==1?true:false, qos))
	{
		tHandleCtx->iErrorCode = core_success;
		return true;
	}
	else
	{
		tHandleCtx->iErrorCode = core_internal_error;
		return false;
	}
}

WISECORE_API bool core_ex_device_register(WiCore_t core)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(tHandleCtx->iStatus != core_online)
	{
		tHandleCtx->iErrorCode = core_no_connnect;
		return false;
	}
#ifdef _WISEPAAS_02_DEF_H_
	sprintf(tHandleCtx->strTopicBuff, DEF_CALLBACKREQ_TOPIC, DEF_PRESERVE_PRODUCT_NAME, tHandleCtx->strClientID);
#else
	sprintf(tHandleCtx->strTopicBuff, DEF_CALLBACKREQ_TOPIC, tHandleCtx->strClientID);
#endif
	if(!wc_ex_subscribe(tHandleCtx->conn, tHandleCtx->strTopicBuff, 0))
		return false;
	//sprintf(tHandleCtx->strTopicBuff, DEF_ACTIONACK_TOPIC, tHandleCtx->strClientID);
	//wc_ex_subscribe(tHandleCtx->conn, tHandleCtx->strTopicBuff, 0);

	if(!wc_ex_subscribe(tHandleCtx->conn, DEF_AGENTCONTROL_TOPIC, 0))
		return false;
	{
#define AZURE_IOT_HUB_SUB_PREFIX "devices/%s/messages/devicebound/#"
	char *devid = tHandleCtx->strClientID;
	char replacedtopic[256] = { 0 };
	sprintf(replacedtopic, AZURE_IOT_HUB_SUB_PREFIX, devid);
	if(!wc_ex_subscribe(tHandleCtx->conn, replacedtopic, 0))
		return false;
	}
	return _ex_send_agent_connect(tHandleCtx);
}

WISECORE_API bool core_ex_heartbeat_send(WiCore_t core)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(tHandleCtx->iStatus != core_online)
	{
		tHandleCtx->iErrorCode = core_no_connnect;
		return false;
	}
	sprintf(tHandleCtx->strPayloadBuff, DEF_HEARTBEAT_MESSAGE_JSON, tHandleCtx->strClientID);
	sprintf(tHandleCtx->strTopicBuff, DEF_HEARTBEAT_TOPIC, tHandleCtx->strClientID);
	if(wc_ex_publish(tHandleCtx->conn, (char *)tHandleCtx->strTopicBuff, tHandleCtx->strPayloadBuff, strlen(tHandleCtx->strPayloadBuff), false, 0))
	{
		tHandleCtx->iErrorCode = core_success;
		return true;
	}
	else
	{
		tHandleCtx->iErrorCode = core_internal_error;
		return false;
	}
}

WISECORE_API bool core_ex_subscribe(WiCore_t core, char const * topic, int qos)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(!topic)
	{
		tHandleCtx->iErrorCode = core_param_error;
		return false;
	}

	if(wc_ex_subscribe(tHandleCtx->conn, (char *)topic, qos))
	{
		tHandleCtx->iErrorCode = core_success;
		return true;
	}
	else
	{
		tHandleCtx->iErrorCode = core_internal_error;
		return false;
	}
}

WISECORE_API bool core_ex_unsubscribe(WiCore_t core, char const * topic)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return false;
	tHandleCtx = (core_contex_t*) core;

	if(!tHandleCtx->bInited)
	{
		tHandleCtx->iErrorCode = core_no_init;
		return false;
	}

	if(!topic)
	{
		tHandleCtx->iErrorCode = core_param_error;
		return false;
	}


	if(wc_ex_unsubscribe(tHandleCtx->conn, (char *)topic))
	{
		tHandleCtx->iErrorCode = core_success;
		return true;
	}
	else
	{
		tHandleCtx->iErrorCode = core_internal_error;
		return false;
	}
}

WISECORE_API const char* core_ex_error_string_get(WiCore_t core)
{
	core_contex_t* tHandleCtx = NULL;
	if(core == NULL)
		return "No initialized.";
	tHandleCtx = (core_contex_t*) core;

	switch(tHandleCtx->iErrorCode){
		case core_success:
			return "No error.";
		case core_param_error:
			return "Invalided parameters.";
		case core_no_init:
			return "No initialized.";
		case core_no_connnect:
			return "No connected.";
		case core_buff_not_enough:
			return "Created buffer size not enough.";
		default:
		case core_internal_error:
			return wc_ex_current_error_string_get(tHandleCtx->conn);
		case core_not_support:
			return "Not support!";
	}
}
