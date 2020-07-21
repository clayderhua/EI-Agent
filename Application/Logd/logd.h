#ifndef __LOGD_H__
#define __LOGD_H__

#include "Log.h"

#define LOGD_VERSION			"1.0.0"
#define DEF_MAX_LOG_NUM			10
#define DEF_MAX_LOG_SIZE		10*1024*1024	// byte
#define DEF_LOG_PATH			"logs" // related to module path
#define LOGD_LOCAL_PIPE_PORT	9279

#define LOGD_APPEND_STR(startPtr, curPtr, str) do {\
	if (curPtr == NULL) { curPtr = startPtr; } \
	curPtr += snprintf(curPtr, MAX_BUFFER_SIZE - (curPtr - startPtr), "%s", str) + 1;\
} while (0)

/*
	read next string after startPtr string
	curPtr must pointer to startPtr or NULL
*/
#define LOGD_READ_NEXT_STR(startPtr, curPtr, length) do {\
	if (curPtr == NULL) { curPtr = startPtr; } \
	else { \
		curPtr += strlen(curPtr) + 1;\
		if ((curPtr - startPtr) >= length || *curPtr == '\0') { curPtr = NULL; }\
	}\
} while (0)

#define LOGD_READ_NEXT_STR_BREAK(startPtr, curPtr, length) {\
	READ_NEXT_STR(startPtr, curPtr, length);\
	if (curPtr == NULL) break;\
} while(0)

#endif
