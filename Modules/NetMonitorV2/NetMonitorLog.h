#ifndef _NETWORK_LOG_H_
#define _NETWORK_LOG_H_

#define LOG_TAG "NetMonitor"
#include "Log.h"

#define DEF_LOG_NAME    "NETWORKLog.txt"   //default log file name
#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

#ifdef LOG_ENABLE
#define NETWORKLog(level, fmt, ...)  do { \
	WriteLog(g_netloghandle, DEF_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define NETWORKLog(level, fmt, ...)
#endif

#endif