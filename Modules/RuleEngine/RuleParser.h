#ifndef _RUEL_PARSER_H_
#define _RUEL_PARSER_H_
#pragma once

#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_CONTENT_STRUCT		"content"
#define AGENTINFO_HANDLERNAME			"handlerName"
#define AGENTINFO_THRESHOLD				"Threshold"
#define AGENTINFO_NAME					"n"
#define AGENTINFO_DEVICEID				"agentID"

#define REPLY_SET_THR					"setThrRep"

#ifdef __cplusplus
extern "C" {
#endif

bool RuleParser_ParseThrInfo(char * thrJsonStr, char* devID, char** thrHandlerStr);

bool RuleParser_PackSetThrRep(char * repStr, char ** outputStr);

bool RuleParser_ActionCommand(char* itemname, char * devid, char * plugin, char* snesor);

#ifdef __cplusplus
}
#endif

#endif