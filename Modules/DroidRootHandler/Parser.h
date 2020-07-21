#ifndef _DROIDROOT_PARSER_H_
#define _DROIDROOT_PARSER_H_
#include <stdbool.h>
#include <string.h>
#include "cJSON.h"
#include "DroidRootHandler.h"


#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"

//first layout
#define HANDLER_NAME                    "DroidRoot"
// second layout
#define DROID_ROOTCTRL					"rootctrl"
//
#define DROID_E_FLAG                                "e"
#define DROID_N_FLAG                                "n"
#define DROID_V_FLAG                                "v"
#define DROID_SV_FLAG                               "sv"
#define DROID_BV_FLAG                               "bv"
#define DROID_BN_FLAG				"bn"
#define DROID_BU_FLAG				"bu"
#define DROID_BU_VALUE_BU				"bu"
#define DROID_ASM_FLAG				"asm"
#define DROID_VER_FLAG				"ver"
#define DROID_VER_VALUE_1			1
#define DROID_SESSION_ID		"sessionID"
#define DROID_SENSOR_ID_LIST	"sensorIDList"	
#define DROID_SENSOR_INFO_LIST            "sensorInfoList"
#define DROID_STATUS_CODE_FLAG            "StatusCode"

// set status string
#define  STATUS_SUCCESS	"Success"
#define  STATUS_SETTING     "Setting";
#define  STATUS_REQUEST_ERROR     "Request Error";
#define  STATUS_NOT_FOUND     "Not Found";
#define  STATUS_WRITE     "Write Only";
#define  STATUS_READ     "Read Only";
#define  STATUS_REQUEST_TIMEOUT     "Request Timeout";
#define  STATUS_RESOURCE_LOSE     "Resource Lose";
#define  STATUS_FORMAT_ERROR     "Format Error";
#define  STATUS_OUTOF_RANGE     "Out of Range";
#define  STATUS_SYNTAX_ERROR     "Syntax Error";
#define  STATUS_LOCKED     "Resource Locked";
#define  STATUS_FAIL     "Fail";
#define  STATUS_NOT_IMPLEMENT     "Not Implement";
#define  STATUS_SYS_BUSY     "Sys Busy";

// set status code 
#define  STATUSCODE_SUCCESS     200;
#define  STATUSCODE_SETTING     202;
#define  STATUSCODE_REQUEST_ERROR     400;
#define  STATUSCODE_NOT_FOUND     404;
#define  STATUSCODE_WRITE     405;
#define  STATUSCODE_READ     405;
#define  STATUSCODE_REQUEST_TIMEOUT     408;
#define  STATUSCODE_RESOURCE_LOSE     410;
#define  STATUSCODE_FORMAT_ERROR     415;
#define  STATUSCODE_OUTOF_RANGE     416;
#define  STATUSCODE_SYNTAX_ERROR     422;
#define  STATUSCODE_LOCKED     426;
#define  STATUSCODE_FAIL     500;
#define  STATUSCODE_NOT_IMPLEMENT     401;
#define  STATUSCODE_SYS_BUSY     503;

#endif
