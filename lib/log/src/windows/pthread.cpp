#include <stdio.h>
#include "sys/time.h"
#include "pthread.h"

typedef void (*pthread_once_fun)(void);

static pthread_once_fun g_pthreadOnceFun;

BOOL CALLBACK InitHandleFunction (
    PINIT_ONCE InitOnce,
    PVOID Parameter,
    PVOID *lpContext)
{
	g_pthreadOnceFun();
	return true;
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	g_pthreadOnceFun = init_routine;
	BOOL bStatus = InitOnceExecuteOnce(
								once_control,          // One-time initialization structure
                                InitHandleFunction,   // Pointer to initialization callback function
                                NULL,                 // Optional parameter to callback function (not used)
                                NULL				// Receives pointer to event object stored in g_InitOnce
								);
	return (bStatus)? 0: -1;
}

int pthread_create(pthread_t *thread, void *attr,void *(*start_routine) (void *), void *arg) {
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)start_routine,arg,0,thread);
	return 0;
}

int pthread_join(pthread_t thread, void **retval) {
	DWORD exitCode;
	HANDLE h = OpenThread(THREAD_ALL_ACCESS, FALSE, thread);
	WaitForSingleObject(h, INFINITE);
	GetExitCodeThread(h, &exitCode);
	CloseHandle(h);
	if(retval)
		*retval = (void *)exitCode;
	return 0;
}

pthread_t pthread_self(void) {
	return GetCurrentThreadId();
}

int pthread_detach(pthread_t thread) {
	HANDLE h = OpenThread(THREAD_QUERY_INFORMATION, FALSE, thread);
	CloseHandle(h);
	return 0;
}

int pthread_cancel(pthread_t thread)
{
	int iRet = 0;
	HANDLE h = OpenThread(THREAD_QUERY_INFORMATION, FALSE, thread);
	if (!TerminateThread(h, 0)) iRet = -1;
	return iRet;
}

void pthread_exit(void* retval)
{
	unsigned long code = (unsigned long)retval;
	ExitThread(code);
	return;
}

int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t * attr) {
	*mutex=CreateMutex(NULL,FALSE,NULL);
	return 0;
}


int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	int iRet = 0;
	cond->waiters_count = 0;
	cond->signal_event = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	InitializeCriticalSection(&cond->waiters_count_lock);
	cond->generation = 0;
	cond->num_wake = 0;
	return iRet;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
	int iRet = 0;
	//Add code
	unsigned int wake = 0;
	EnterCriticalSection(&cond->waiters_count_lock);
	if (cond->waiters_count > cond->num_wake)
	{
		wake = 1;
		cond->num_wake++;
		cond->generation++;
	}
	LeaveCriticalSection(&cond->waiters_count_lock);

	if (wake)
	{
		ReleaseSemaphore(cond->signal_event, 1, NULL);
	}
	return iRet;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
	int rv;
	unsigned int wake = 0;
	unsigned long generation;
	unsigned long timeout_ms = INFINITE;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	if (abstime)
		timeout_ms = (unsigned long)(((abstime->tv_sec - tv.tv_sec) * 1000 + (abstime->tv_nsec - tv.tv_usec / 1000) / 1000000) > 0 ? ((abstime->tv_sec - tv.tv_sec) * 1000 + (abstime->tv_nsec - tv.tv_usec / 1000) / 1000000) : 0);

	EnterCriticalSection(&cond->waiters_count_lock);
	cond->waiters_count++;
	generation = cond->generation;
	LeaveCriticalSection(&cond->waiters_count_lock);

	pthread_mutex_unlock(mutex);

	do
	{
		unsigned long res = WaitForSingleObject(cond->signal_event, timeout_ms);

		EnterCriticalSection(&cond->waiters_count_lock);

		if (cond->num_wake)
		{
			if (cond->generation != generation)
			{
				cond->num_wake--;
				cond->waiters_count--;
				rv = 0;
				break;
			}
			else
			{
				wake = 1;
			}
		}
		else if (res != WAIT_OBJECT_0)
		{
			cond->waiters_count--;
			rv = 1;
			break;
		}

		LeaveCriticalSection(&cond->waiters_count_lock);

		if (wake)
		{
			wake = 0;
			ReleaseSemaphore(cond->signal_event, 1, NULL);
		}
	} while (1);

	LeaveCriticalSection(&cond->waiters_count_lock);
	pthread_mutex_lock(mutex);
	return rv;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
	int iRet = 0;
	if (cond->signal_event)
	{
		CloseHandle(cond->signal_event);
		cond->signal_event = NULL;
	}
	DeleteCriticalSection(&cond->waiters_count_lock);
	return iRet;
}
