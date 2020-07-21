#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "export.h"

#define pthread_t HANDLE

#define pthread_mutex_t HANDLE

#define pthread_mutexattr_t unsigned long

typedef struct { 
	UINT waiters_count;
	CRITICAL_SECTION waiters_count_lock;
	HANDLE signal_event;
	unsigned long num_wake;
	unsigned long generation;
} pthread_cond_t;

#if _MSC_VER < 1900
#include <time.h>
#include <sys/time.h>
#else
#include <time.h>
#endif


#define pthread_condattr_t unsigned long

WISEPLATFORM_API int pthread_create(pthread_t *thread, void *attr, void *(*start_routine)(void*), void *arg);

WISEPLATFORM_API int pthread_join(pthread_t thread, void **retval);

WISEPLATFORM_API pthread_t pthread_self(void);

WISEPLATFORM_API int pthread_equal(pthread_t threadIdA, pthread_t threadIdB);

WISEPLATFORM_API int pthread_cancel(pthread_t thread);

WISEPLATFORM_API void pthread_exit(void *retval);

WISEPLATFORM_API int pthread_detach(pthread_t thread);

WISEPLATFORM_API int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);

WISEPLATFORM_API int pthread_mutex_lock(pthread_mutex_t *mutex);

WISEPLATFORM_API int pthread_mutex_unlock(pthread_mutex_t *mutex);

WISEPLATFORM_API int pthread_mutex_destroy(pthread_mutex_t *mutex);

WISEPLATFORM_API int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);

WISEPLATFORM_API int pthread_cond_signal(pthread_cond_t *cond);

WISEPLATFORM_API int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);

WISEPLATFORM_API int pthread_cond_destroy(pthread_cond_t *cond);

#ifdef __cplusplus
}
#endif

#endif