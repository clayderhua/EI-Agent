#ifndef _PROCESS_MONITOR_LOG_H_
#define _PROCESS_MONITOR_LOG_H_

#include "Log.h"

#define DEF_LOG_NAME    "ProcessMonitorLog.txt"   //default log file name
#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

static void* g_processloghandle = NULL;

#ifdef LOG_ENABLE
#define ProcessMonitorLog(level, fmt, ...)  do { if (g_processloghandle != NULL)   \
	WriteLog(g_processloghandle, DEF_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define ProcessMonitorLog(level, fmt, ...)
#endif

#endif