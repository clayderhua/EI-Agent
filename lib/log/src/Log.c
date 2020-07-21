#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#include "udp-socket.h"
#include "Log.h"

#include <unistd.h>
#include <pthread.h>

#ifdef WIN32
#include "wrapper.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
	static int uninit_log();
#ifdef __cplusplus
}
#endif

#define DEF_LOG_PATH			"logs"

static int g_logSocket = INVALID_SOCKET;
static struct addrinfo *g_serverAddr = NULL;
static int g_initTime = 0;
static int g_toStderr = DEF_TO_STDERR;
static int g_logLevel = LOG_NORMAL;
static char g_logdIP[16];
static int g_logdPort = DEF_LOGD_PORT;
static pthread_once_t g_logLockOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_logLock;

#ifdef FILE_MONITOR_THREAD
	#ifdef __cplusplus
	extern "C" {
	#endif
	void* file_monitor_thread(void *arg);
	#ifdef __cplusplus
	}
	#endif
	static pthread_t g_fileMonitorThread;
	static file_monitor_st g_fm;
#endif

// original log variable
static LOGHANDLE loghandle = (LOGHANDLE)0;

////////////////////////////////////////////////////////////////////////////////
// Log Config
////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32

#include <windows.h>
int util_module_path_get(char * moudlePath)
{
	int iRet = 0;
	char * lastSlash = NULL;
	char tempPath[512] = {0};
	if(NULL == moudlePath) return iRet;
	if(ERROR_SUCCESS != GetModuleFileName(NULL, tempPath, sizeof(tempPath)))
	{
		lastSlash = strrchr(tempPath, FILE_SEPARATOR);
		if(NULL != lastSlash)
		{
			strncpy(moudlePath, tempPath, lastSlash - tempPath + 1);
			iRet = lastSlash - tempPath + 1;
			moudlePath[iRet] = 0;
		}
	}
	return iRet;
}

#else // !windows
int util_module_path_get(char * moudlePath)
{
	strcpy(moudlePath,".");
	return 0;
}
#endif

static char *ltrim(char *str, const char *seps)
{
    size_t totrim;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    totrim = strspn(str, seps);
    if (totrim > 0) {
        size_t len = strlen(str);
        if (totrim == len) {
            str[0] = '\0';
        }
        else {
            memmove(str, str + totrim, len + 1 - totrim);
        }
    }
    return str;
}

static char *rtrim(char *str, const char *seps)
{
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

static char *trim(char *str, const char *seps)
{
    return ltrim(rtrim(str, seps), seps);
}

static char* get_config_value(char *line, const char *key)
{
	char *linePtr = line;
	const char *keyPtr = key;

	// skip space
	while (*linePtr != '\0') {
		if (*linePtr != ' ' && *linePtr != '\t') {
			break;
		}
		linePtr++;
	}
	// check key string
	while (*linePtr != '\0') {
		if (*keyPtr == '\0') {
			break;
		}
		if (*linePtr != *keyPtr) {
			return NULL;
		}
		linePtr++;
		keyPtr++;
	}
	// find '='
	while (*linePtr != '\0') {
		if (*linePtr == '=') {
			linePtr++;
			break;
		}
		linePtr++;
	}
	return trim(linePtr, NULL);
}

// send log to logd only if the socket if exist, or you will make a infinity loop...
#define CLOGD(fmt, ...) do { \
	if (g_logSocket != INVALID_SOCKET) { send2logd(LOG_TAG, LOG_DEBUG, fmt, ##__VA_ARGS__); } \
	else { fprintf(stderr, LOG_TAG fmt " > stderr\n", ##__VA_ARGS__); } \
} while (0);

#define LOGC_INI_GROUP	"[LogClient]"

static int append_empty_logini(char* filename)
{
	FILE *file = fopen(filename, "a");

	CLOGD("%s", "append_empty_logini");
	if (!file) {
		return -1;
	}
	fputs("\n" LOGC_INI_GROUP "\n", file);
	fputs("#log_level=4, LOG_FATAL(0), LOG_ALARM(1), LOG_ERROR(2), LOG_WARNING(2), LOG_NORMAL(4), LOG_DEBUG(5)\n", file);
	fputs("#to_stderr=1, 1: print to stderr, 0: doesn't print stderr\n", file);
	fputs("#logd_ip=127.0.0.1, ip of logd\n", file);
	fputs("#logd_port=9278\n", file);
	fclose(file);

	return 0;
}

static int read_log_config(char* filename)
{
	char line[128];
	FILE *file;
	char *ptr;
	int isValid = 0;

	CLOGD("%s", "read_log_config");
	file = fopen(filename, "r");
	if (!file) {
		if (errno == ENOENT) {
			return append_empty_logini(filename);
		} else {
			CLOGD("open %s fail, errno=%d", filename, errno);
			return -1;
		}
	}

	while ( fgets(line, sizeof(line)-1, file) != NULL ) {
		if (line[0] == '#') {
			continue;
		}
		if (strncmp(line, LOGC_INI_GROUP, strlen(LOGC_INI_GROUP)) == 0) {
			isValid = 1;
			continue;
		}

		line[sizeof(line)-1] = '\0';
		if ((ptr = get_config_value(line, "log_level")) != NULL) {
			g_logLevel = strtol(ptr, NULL, 10);
			LOGD("config: log_level=%d", g_logLevel);
			continue;
		}
		if ((ptr = get_config_value(line, "to_stderr")) != NULL) {
			g_toStderr = strtol(ptr, NULL, 10);
			LOGD("config: to_stderr=%d", g_toStderr);
			continue;
		}
		if ((ptr = get_config_value(line, "logd_ip")) != NULL) {
			strncpy(g_logdIP, ptr, sizeof(g_logdIP));
			g_logdIP[sizeof(g_logdIP)-1] = '\0';
			LOGD("config: g_logdIP=%s", g_logdIP);
			continue;
		}
		if ((ptr = get_config_value(line, "logd_port")) != NULL) {
			g_logdPort = strtol(ptr, NULL, 10);
			LOGD("config: logd_port=%d", g_logdPort);
			continue;
		}
	}
	fclose(file);

	if (!isValid) {
		CLOGD("%s format is invalid", filename);
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Logd Client Utility
////////////////////////////////////////////////////////////////////////////////
// format "02-27 10:19:00", 14 char
static void get_datetime_str(time_t timer, char *datetimeStr)
{
    struct tm *tmCur;

    tmCur = localtime(&timer);

	strftime(datetimeStr, 16, "%m-%d %H:%M:%S", tmCur);
}

static void init_log_once()
{
	pthread_mutex_init(&g_logLock, NULL);
}

static int init_log()
{
	int nonblocking = 1;
	char moudlePath[PATH_MAX];
	char filename[PATH_MAX];
	int ret = 0;

	// set default value
	strcpy(g_logdIP, "127.0.0.1");

	// load config
	util_module_path_get(moudlePath);
	sprintf(filename, "%s%c%s", moudlePath, FILE_SEPARATOR, "log.ini");
	read_log_config(filename);

	// thread will block here until first init_log_once() is complete
	pthread_once(&g_logLockOnce, init_log_once);

#ifdef FILE_MONITOR_THREAD
	pthread_mutex_lock(&g_logLock);
	if (!g_fm.running) {
		// init log config monitor thread
		g_fm.running = 1;
		strcpy(g_fm.file, filename);
		g_fm.fun = (file_monitor_fn) read_log_config;
		ret = pthread_create(&g_fileMonitorThread, NULL, &file_monitor_thread, &g_fm);
		if (ret) {
			fprintf(stderr, "pthread_create(file_monitor_thread) fail\n");
			g_fm.running = 0;
			ret = -1;
		}
	}
	pthread_mutex_unlock(&g_logLock);
#endif

	pthread_mutex_lock(&g_logLock);  // protect g_logSocket couldn't init twice
	if (g_logSocket == INVALID_SOCKET) {
		// init log client socket
		g_logSocket = init_client_udp_socket(g_logdIP, g_logdPort, nonblocking, &g_serverAddr);
		if (g_logSocket == INVALID_SOCKET) {
			ret = -1;
		}
	}
	pthread_mutex_unlock(&g_logLock);
	return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Logd Client Function
////////////////////////////////////////////////////////////////////////////////
#ifdef FILE_MONITOR_THREAD
static void copyfile(const char* source, const char* target)
{
	FILE* fpi = NULL, * fpo = NULL;
	int  nByteRead = 0;
	char szBuffer[256];

	do {
		if ((fpi = fopen(source, "rb")) == NULL)
			break;
		if ((fpo = fopen(target, "wb")) == NULL)
			break;
		while ((nByteRead = fread(szBuffer, sizeof(char), 256, fpi)) > 0)
			fwrite(szBuffer, sizeof(char), nByteRead, fpo);

		fclose(fpi);
		fclose(fpo);
	} while (0);
}
#endif

static int uninit_log()
{
	int ret = 0;

#ifdef FILE_MONITOR_THREAD
	FILE *fp;
	char bakfile[PATH_MAX];

	pthread_mutex_lock(&g_logLock); // protect g_fm.running=0 before pthread_join return
	// stop file monitor thread
	g_fm.running = 0;
	// trigger thread to end, we must change file, copy file to .bak and rename .bak to original file
	snprintf(bakfile, sizeof(bakfile), "%s.bak", g_fm.file);
	bakfile[PATH_MAX - 1] = '\0';
	// guarantee the file is exist
	fp = fopen(g_fm.file, "a+");
	if (fp) {
		fclose(fp);
	}
	copyfile(g_fm.file, bakfile);
	rename(bakfile, g_fm.file);

	ret = pthread_join(g_fileMonitorThread, NULL);
	if (ret) {
		fprintf(stderr, "pthread_join(file_monitor_thread) fail\n");
		ret = -1;
	}
	pthread_mutex_unlock(&g_logLock);
#endif

	pthread_mutex_lock(&g_logLock); // protect g_logSocket will not release while init...
	if (g_logSocket != INVALID_SOCKET) {
		close(g_logSocket);
		g_logSocket = INVALID_SOCKET;
	}
	pthread_mutex_unlock(&g_logLock);

	return ret;
}

void send2logd(const char *tag, int level, const char *fmt, ...)
{
	struct timeval tv;
	unsigned char buffer[LOGD_MAX_BUF_LEN];
	char *msgBuf;
	char *ptr;
	char datetimeStr[16];
	int32_t length;
	va_list ap;

	if (g_logLevel < level) {
		return;
	}

	gettimeofday(&tv, NULL);

	// check socket in each log step to recover socket is socket fail
	if (g_logSocket == INVALID_SOCKET &&
		tv.tv_sec - g_initTime > 10) // retry to init per 10 sec.
	{
		g_initTime = tv.tv_sec;
		init_log();
	}

	if (g_logSocket == INVALID_SOCKET && g_toStderr == 0) {
		return;
	}

	// check length
	get_datetime_str(tv.tv_sec, datetimeStr);

	// make message
	//length = strlen(message); // message length
	msgBuf = (char*) buffer + sizeof(int32_t);

	// make format string "03-02 09:22:26.392 [tag] "
	ptr = msgBuf;
	va_start(ap, fmt);
	ptr += sprintf(ptr, "%s.%03d [%.12s] ", datetimeStr, (int) tv.tv_usec/1000, tag);
	ptr += vsnprintf(ptr, sizeof(buffer)-sizeof(int32_t)-(ptr-msgBuf), fmt, ap);
	if (ptr > (char*)buffer+sizeof(buffer)-1) { // message is too large
		ptr = (char*) buffer + (sizeof(buffer) - 12); // "[truncated]" + '\0'
		strcpy(ptr, "[truncated]");
	} else {
		*ptr = '\0';
	}
	va_end(ap);

	// make length
	length = strlen(msgBuf)+1; // message + payload length
	memcpy(buffer, (unsigned char*) &length, sizeof(int32_t));

	if (g_toStderr) {
		fprintf(stderr, "%s%s", msgBuf, FILE_NEWLINE);
	}
	if (g_logSocket != INVALID_SOCKET) {
		send_udp_message(g_logSocket, buffer, sizeof(int32_t)+length, g_serverAddr);
	}
}

/*
	An copy function of send2logd() to make prebuilt plugin can find the symbol.
	The only different is that it use default LOG_TAG
*/
void PrintAndWriteLog(int level, const char *file, const char *func, const char *line, const char* levels, const char *fmt, ...)
{
	struct timeval tv;
	unsigned char buffer[LOGD_MAX_BUF_LEN];
	char *msgBuf;
	char *ptr;
	char datetimeStr[16];
	int32_t length;
	va_list ap;

	if (g_logLevel < level) {
		return;
	}

	gettimeofday(&tv, NULL);

	// check socket in each log step to recover socket is socket fail
	if (g_logSocket == INVALID_SOCKET &&
		tv.tv_sec - g_initTime > 10) // retry to init per 10 sec.
	{
		g_initTime = tv.tv_sec;
		init_log();
	}

	if (g_logSocket == INVALID_SOCKET && g_toStderr == 0) {
		return;
	}

	// check length
	get_datetime_str(tv.tv_sec, datetimeStr);

	// make message
	//length = strlen(message); // message length
	msgBuf = (char*) buffer + sizeof(int32_t);

	// make format string "03-02 09:22:26.392 [tag] "
	ptr = msgBuf;
	va_start(ap, fmt);
	ptr += sprintf(ptr, "%s.%03d [%.12s] ", datetimeStr, (int) tv.tv_usec/1000, LOG_TAG);
	ptr += vsnprintf(ptr, sizeof(buffer)-sizeof(int32_t)-(ptr-msgBuf), fmt, ap);
	if (ptr > (char*)buffer+sizeof(buffer)-1) { // message is too large
		ptr = (char*) buffer + (sizeof(buffer) - 12); // "[truncated]" + '\0'
		strcpy(ptr, "[truncated]");
	} else {
		*ptr = '\0';
	}
	va_end(ap);

	// make length
	length = strlen(msgBuf)+1; // message + payload length
	memcpy(buffer, (unsigned char*) &length, sizeof(int32_t));

	if (g_toStderr) {
		fprintf(stderr, "%s%s", msgBuf, FILE_NEWLINE);
	}
	if (g_logSocket != INVALID_SOCKET) {
		send_udp_message(g_logSocket, buffer, sizeof(int32_t)+length, g_serverAddr);
	}
}

LOGHANDLE InitLog(char * logFileName)
{
	init_log();

	// loghandle is inited
	loghandle = (LOGHANDLE)1;
	return loghandle;
}

void UninitLog(LOGHANDLE logHandle)
{
	uninit_log();
}

// Backward compatible to old libLog in windows
#ifdef WIN32
int __stdcall TransId(int level)
#else
int TransId(int level)
#endif
{
	return level;
}

void ElsWriteLog(const char *DataPath, const char *data, int len)
{
}
