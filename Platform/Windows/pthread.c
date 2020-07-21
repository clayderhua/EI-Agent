#include "pthread.h"

int pthread_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year     = wtm.wYear - 1900;
	tm.tm_mon     = wtm.wMonth - 1;
	tm.tm_mday     = wtm.wDay;
	tm.tm_hour     = wtm.wHour;
	tm.tm_min     = wtm.wMinute;
	tm.tm_sec     = wtm.wSecond;
	tm. tm_isdst    = -1;
	clock = mktime(&tm);
	tv->tv_sec = (long)clock;
	tv->tv_usec = wtm.wMilliseconds * 1000;

	return (0);
}


WISEPLATFORM_API int pthread_create(pthread_t *thread, void * attr, void *(*start_routine)(void*), void *arg)
{
	int iRet = 0;
	*thread = (HANDLE)CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, NULL);
	if(!(*thread)) iRet = -1;
	return iRet;
}

WISEPLATFORM_API int pthread_join(pthread_t thread, void **retval)
{
	int iRet = 0;
	unsigned long dwRet = 0;
	
	if(!thread) return -1;
	if(WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0) iRet = -1;
	GetExitCodeThread(thread, &dwRet);
	CloseHandle(thread);
	if(retval)
		*retval = (void *)dwRet;
	return iRet;
}

WISEPLATFORM_API pthread_t pthread_self(void)
{
	int threadID = GetCurrentThreadId();
	pthread_t thread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, threadID);
	return thread;
}

WISEPLATFORM_API int pthread_equal(pthread_t threadIdA, pthread_t threadIdB)
{
	int iRet = 0;
	if(threadIdA != threadIdB) iRet = -1;
	return iRet;
}

WISEPLATFORM_API int pthread_cancel(pthread_t thread)
{
	int iRet = 0;
	if(!TerminateThread(thread , 0)) iRet = -1;
	return iRet;
}

WISEPLATFORM_API void pthread_exit(void* retval)
{
	unsigned long code = (unsigned long)retval;
	ExitThread(code);
	return;
}

WISEPLATFORM_API int pthread_detach(pthread_t thread)
{
	int iRet = 0;
	if(!CloseHandle(thread))
		iRet = GetLastError();
	return iRet;
}

WISEPLATFORM_API int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	int iRet = 0;
	*mutex = CreateMutex(NULL, FALSE, NULL);
	if(!(*mutex)) iRet =  -1;
	return iRet;
}

WISEPLATFORM_API int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	int iRet = 0;
	if (* mutex == NULL) 
	{
		iRet = -1;
	}
	else
	{
		if(WaitForSingleObject(*mutex, INFINITE) == WAIT_FAILED) iRet = -1;
	}
	return iRet;
}

WISEPLATFORM_API int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	int iRet = 0;
	if(!ReleaseMutex(*mutex)) iRet = -1;
	return iRet;
}

WISEPLATFORM_API int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	int iRet = 0;
	if(*mutex)
	{
		CloseHandle(*mutex);
		*mutex = NULL;
	}
	return iRet;
}

WISEPLATFORM_API int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	int iRet = 0;
	cond->waiters_count = 0;
	cond->signal_event = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	InitializeCriticalSection(&cond->waiters_count_lock);
	cond->generation = 0;
	cond->num_wake = 0;
	return iRet;
}

WISEPLATFORM_API int pthread_cond_signal(pthread_cond_t *cond)
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

WISEPLATFORM_API int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
 	int rv;
	unsigned int wake = 0;
	unsigned long generation;
	unsigned long timeout_ms = INFINITE;
	struct timeval tv;

	pthread_gettimeofday(&tv, NULL);

	if(abstime)
		timeout_ms = (unsigned long)(((abstime->tv_sec - tv.tv_sec)*1000 + (abstime->tv_nsec - tv.tv_usec/1000)/1000000)>0?((abstime->tv_sec - tv.tv_sec)*1000 + (abstime->tv_nsec - tv.tv_usec/1000)/1000000):0);

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

WISEPLATFORM_API int pthread_cond_destroy(pthread_cond_t *cond)
{
   int iRet = 0;
	if(cond->signal_event)
	{
		CloseHandle(cond->signal_event);
		cond->signal_event = NULL;
	}
	DeleteCriticalSection(&cond->waiters_count_lock);
	return iRet;
}