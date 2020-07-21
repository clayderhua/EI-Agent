#ifndef _RULE_ENGINE_LOG_H_
#define _RULE_ENGINE_LOG_H_

#include <Log.h>

#define DEF_LOG_NAME    "RuleEngineLog.txt"   //default log file name
#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

static LOGHANDLE g_ruleenginehandlerlog = NULL;

#ifdef LOG_ENABLE
#define RuleEngineLog(level, fmt, ...)  do { if (g_ruleenginehandlerlog != NULL)   \
	WriteLog(g_ruleenginehandlerlog, DEF_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define RuleEngineLog(level, fmt, ...)
#endif

#endif