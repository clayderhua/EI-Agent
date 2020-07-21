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
#include <time.h>
#include <windows.h>

#define CLOCK_REALTIME	0

struct timezone {
   int tz_minuteswest;     /* minutes west of Greenwich */
   int tz_dsttime;         /* type of DST correction */
};
#if _MSC_VER < 1900
struct timespec {
	time_t   tv_sec;        /* seconds */
	long     tv_nsec;       /* nanoseconds */
};
#endif
WISEPLATFORM_API int gettimeofday(struct timeval *tv, struct timezone *tz);
WISEPLATFORM_API int clock_gettime(int X, struct timespec *ts);
WISEPLATFORM_API struct tm* localtime_r(const time_t *time, struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif //__sys_time_h__