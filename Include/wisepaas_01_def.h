/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/02/09 by Scott Chang								    */
/* Modified Date: 2017/07/17 by Scott Chang									*/
/* Abstract     : WISE-PaaS 1.0 definition for RMM 3.x						*/
/* Reference    : None														*/
/****************************************************************************/

#ifndef _WISEPAAS_01_DEF_H_
#define _WISEPAAS_01_DEF_H_

#include "wise/wisepaas_base_def.h"

#define DEF_GENERAL_HANDLER					"\"handlerName\":\"general\""
#define DEF_SERVERREDUNDANCY_HANDLER		"\"handlerName\":\"ServerRedundancy\""
#define DEF_GENERAL_REQID					"\"requestID\":109"
#define DEF_WISE_COMMAND					"\"commCmd\":%d"
#define DEF_SERVERCTL_STATUS				"\"statuscode\":%d"
#define DEF_WISE_TIMESTAMP					"\"sendTS\":%d"

#define DEF_WILLMSG_TOPIC					"/cagent/admin/%s/willmessage"	/*publish*/
#define DEF_INFOACK_TOPIC					"/cagent/admin/%s/agentinfoack"	/*publish*/
#define DEF_AGENTINFO_JSON					"{\"susiCommData\":{\"devID\":\"%s\",\"parentID\":\"%s\",\"hostname\":\"%s\",\"sn\":\"%s\",\"mac\":\"%s\",\"version\":\"%s\",\"type\":\"%s\",\"product\":\"%s\",\"manufacture\":\"%s\",\"account\":\"%s\",\"passwd\":\"%s\",\"status\":%d,\"commCmd\":1,\"requestID\":21,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":{\"$date\":%lld}}}"
#define DEF_AGENTACT_TOPIC					"/cagent/admin/%s/agentactionreq"	/*publish*/
#define DEF_AGENTREPORT_TOPIC				"/cagent/admin/%s/deviceinfo"	/*publish*/
#define DEF_EVENTNOTIFY_TOPIC				"/cagent/admin/%s/eventnotify"	/*publish*/
#define DEF_CALLBACKREQ_TOPIC				"/cagent/admin/%s/agentcallbackreq"	/*Subscribe*/
#define DEF_AGENTCONTROL_TOPIC				"/server/admin/+/agentctrl"	/*Subscribe*/
#define DEF_HEARTBEAT_TOPIC					"/cagent/admin/%s/notify"	/*publish*/
#define DEF_ACTION_RESULT_SESSION_JSON		"{\"susiCommData\":{\"agentID\":\"%s\",\"commCmd\":%d,\"catalogID\":4,\"handlerName\":\"general\",\"result\":\"%s\",\"sessionID\":\"%s\",\"sendTS\":{\"$date\":%lld}}}"
#define DEF_ACTION_RESULT_JSON				"{\"susiCommData\":{\"agentID\":\"%s\",\"commCmd\":%d,\"catalogID\":4,\"handlerName\":\"general\",\"result\":\"%s\",\"sendTS\":{\"$date\":%lld}}}"
#define DEF_AUTOREPORT_JSON					"{\"susiCommData\":{\"agentID\":\"%s\",\"commCmd\":2055,\"catalogID\":4,\"handlerName\":\"general\",\"data\":%s,\"sendTS\":{\"$date\":%lld}}}"
#define DEF_ACTION_RESPONSE_JSON			"{\"susiCommData\":{\"agentID\":\"%s\",\"commCmd\":%d,\"catalogID\":4,\"handlerName\":\"%s\",%s,\"sendTS\":{\"$date\":%lld}}}"
#define DEF_HEARTBEAT_MESSAGE_JSON			"{\"hb\":{\"devID\":\"%s\"}}"
#define DEF_HEARTBEATRATE_RESPONSE_SESSION_JSON	"{\"susiCommData\":{\"agentID\":\"%s\",\"commCmd\":%d,\"catalogID\":4,\"handlerName\":\"general\",\"heartbeatrate\":%d,\"sessionID\":\"%s\",\"sendTS\":{\"$date\":%lld}}}"

typedef enum{
	wise_unknown_cmd = 0,
	wise_agentinfo_cmd = 21,
	//--------------------------Global command define(101--130)--------------------------------
	wise_update_cagent_req = 111,
	wise_update_cagent_rep,
	wise_cagent_rename_req = 113,
	wise_cagent_rename_rep,
	wise_cagent_osinfo_rep = 116,
	wise_server_control_req = 125,
	wise_server_control_rep,

	wise_heartbeatrate_query_req = 127,
	wise_heartbeatrate_query_rep = 128,
	wise_heartbeatrate_update_req = 129,
	wise_heartbeatrate_update_rep = 130,

	wise_get_sensor_data_req = 523,
	wise_get_sensor_data_rep = 524,
	wise_set_sensor_data_req = 525,
	wise_set_sensor_data_rep = 526,

	wise_error_rep = 600,

	wise_get_capability_req = 2051,
	wise_get_capability_rep = 2052,
	wise_start_report_req = 2053,
	wise_start_report_rep = 2054,
	wise_report_data_rep = 2055,
	wise_stop_report_req = 2056,
	wise_stop_report_rep = 2057,
}wise_comm_cmd_t;

#endif