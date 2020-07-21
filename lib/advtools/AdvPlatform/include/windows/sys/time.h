/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/08/18 by Fred Chang									*/
/* Modified Date: 2015/08/18 by Fred Chang									*/
/* Abstract     :  					*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __sys_time_h__
#define __sys_time_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "export.h"
#if _MSC_VER < 1900
#include <time.h>
struct timespec {
	time_t   tv_sec;        /* seconds */
	long     tv_nsec;       /* nanoseconds */
};
#else
#include <time.h>
#endif
#include <windows.h>

struct timezone {
   int tz_minuteswest;     /* minutes west of Greenwich */
   int tz_dsttime;         /* type of DST correction */
};

ADVPLAT_EXPORT int ADVPLAT_CALL gettimeofday(struct timeval *tv, struct timezone *tz);

#ifdef __cplusplus
}
#endif

#endif //__sys_time_h__