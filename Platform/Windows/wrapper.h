/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/08/18 by												*/
/* Modified Date: 2015/08/18 by												*/
/* Abstract     :  															*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __wrapper_h__
#define __wrapper_h__
#ifdef __cplusplus
extern "C" {
#endif
#include <process.h>
#include <direct.h>
#include <windows.h>
#include <stdio.h>

#define __func__ __FUNCTION__

#define PATH_MAX 260

#define popen _popen
#define pclose _pclose

#define getpid _getpid
#define mkdir(path,flag) _mkdir(path);
#define unlink _unlink
#define strdup _strdup
#define snprintf(dst,size,format,...) _snprintf(dst,size,format,##__VA_ARGS__)
#define strtok_r strtok_s
#define strcasecmp _stricmp
#define warn(format, ...) fprintf(stderr,format,##__VA_ARGS__)
#define strtoll _strtoi64
#define strtoull _strtoui64

#ifdef __cplusplus
}
#endif
#endif //__wrapper_h__
