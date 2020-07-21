//-----------------------------------------------------------------------------
// Log System
//-----------------------------------------------------------------------------
#ifndef _SQPLUGIN_LOG_H_
#define _SQPLUGIN_LOG_H_

#include <Log.h>
#include <stdlib.h>
#define DEF_LOG_NAME    "SQLog.txt"   //default log file name
#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
static void* g_SQLogHandle = NULL;
#ifdef LOG_ENABLE
#define SQLog(level, fmt, ...)  do { if (g_SQLogHandle != NULL)   \
	WriteLog(g_SQLogHandle, DEF_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define HDDLog(level, fmt, ...)
#endif

#endif // !_SQPLUGIN_LOG_H_