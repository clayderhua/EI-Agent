#ifndef _LOG_H_
#define _LOG_H_

// customize parameters
#define DEF_LOGD_PORT		9278
#define LOGD_MAX_BUF_LEN	2048
#define DEF_TO_STDERR		1 // 1: default(no config) print to stderr, 0: default no print
// enable file monitor feature
#define FILE_MONITOR_THREAD

#ifndef LOG_TAG
	#define LOG_TAG	"WiseAgent"
#endif

#ifdef WIN32
	#ifndef PATH_MAX
	#define PATH_MAX 260
	#endif
	#define FILE_SEPARATOR		'\\'
	#define FILE_NEWLINE		"\r\n"
#else
	#include <linux/limits.h>
	#define FILE_SEPARATOR		'/'
	#define FILE_NEWLINE		"\n"
#endif

// log util
#define STR_HELPER(x) #x
#define TO_STR(x) STR_HELPER(x)

#ifndef S__LINE__
	#define S__LINE__ TO_STR(__LINE__)
#endif
#ifndef __func__
	#define __func__ __FUNCTION__
#endif

typedef void*  LOGHANDLE;
typedef int LOGMODE;

#define LOG_MODE_NULL_OUT         0x00
#define LOG_MODE_CONSOLE_OUT      0x01
#define LOG_MODE_FILE_OUT         0x02
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

// new level define
enum {
	LOG_FATAL,
	LOG_ALARM,
	LOG_ERROR,
	LOG_WARNING,
	LOG_NORMAL,
	LOG_DEBUG
};

// old level define
typedef enum LogLevel { 
   Debug = 0,
   Normal, 
   Warning, 
   Error,
   Alarm,
   Fatal
}LogLevel;

// for file monitor
typedef void* (*file_monitor_fn)(void* arg);
typedef struct {
	char file[PATH_MAX];
	file_monitor_fn fun;
	int running;
} file_monitor_st;

#ifdef __cplusplus
extern "C" {
#endif

LOGHANDLE InitLog(char * logFileName);

void UninitLog(LOGHANDLE logHandle);

// Backward compatible to old libLog in windows
#ifdef WIN32
__declspec(dllexport) int __stdcall TransId(int level);
#else
int TransId(int level);
#endif

void ElsWriteLog(const char *DataPath, const char *data, int len);

void send2logd(const char *tag, int level, const char *fmt, ...);

void PrintAndWriteLog(int level, const char *file, const char *func, const char *line, const char* levels, const char *fmt, ...);

// Android like log macro
#define LOGE(fmt, ...) do { send2logd(LOG_TAG, LOG_ERROR, fmt, ##__VA_ARGS__); } while (0)
#define LOGI(fmt, ...) do { send2logd(LOG_TAG, LOG_NORMAL, fmt, ##__VA_ARGS__); } while (0)
#define LOGD(fmt, ...) do { send2logd(LOG_TAG, LOG_DEBUG, fmt, ##__VA_ARGS__); } while (0)

// original agent log function
#define WriteLog(logHandle, logMode, level, format, ...) do { send2logd(LOG_TAG, level, format, ##__VA_ARGS__);; } while (0)

#define WriteElsLog(DataPath, data, len)
//ElsWriteLog(DataPath, data, len)

#define WriteIndividualLog(logHandle, group, logMode, level, format, ...) do { send2logd(LOG_TAG, level, format, ##__VA_ARGS__); } while (0)


#ifdef __cplusplus
}
#endif
#endif
