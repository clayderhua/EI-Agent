#ifndef _MINI_UNZIP_LIB_H_
#define _MINI_UNZIP_LIB_H_

#ifndef LOG_TAG
#define LOG_TAG "OTA"
#endif
#include "Log.h"

#ifdef __cplusplus
extern "C"{
#endif

	int MiniUnzip(char * srcZipPath, char * dstUnzipPath, char * password);

	int CheckZipComment(char * srcZipPath, char * zipcomment, int size);

#ifdef __cplusplus
};
#endif

#endif
