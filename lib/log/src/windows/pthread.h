/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/08/18 by Fred Chang									*/
/* Modified Date: 2015/08/18 by Fred Chang									*/
/* Abstract     :  					*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __pthread_h__
#define __pthread_h__
#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#define pthread_once_t		INIT_ONCE
#define PTHREAD_ONCE_INIT	INIT_ONCE_STATIC_INIT
#define pthread_t DWORD
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
int pthread_create(pthread_t *thread, void *attr,void *(*start_routine) (void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
pthread_t pthread_self(void);
int pthread_detach(pthread_t thread);

#define pthread_mutex_t HANDLE
#define __SIZEOF_PTHREAD_MUTEXATTR_T 4
typedef union
{
  char __size[__SIZEOF_PTHREAD_MUTEXATTR_T];
  int __align;
} pthread_mutexattr_t;

int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t * attr);

#define pthread_mutex_destroy(x)	CloseHandle(*x)
#define pthread_mutex_lock(x)		WaitForSingleObject(*x,INFINITE)
#define pthread_mutex_unlock(x)		ReleaseMutex(*x)

typedef struct { 
	UINT waiters_count;
	CRITICAL_SECTION waiters_count_lock;
	HANDLE signal_event;
	unsigned long num_wake;
	unsigned long generation;
} pthread_cond_t;
#define pthread_condattr_t unsigned long


int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cancel(pthread_t thread);
void pthread_exit(void *retval);
#ifdef __cplusplus
}
#endif

#endif //__pthread_h__