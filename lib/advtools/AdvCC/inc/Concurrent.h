#ifndef __CONCURRENT_H__
#define __CONCURRENT_H__

#include <pthread.h>
//Mutex
typedef struct MutexHandle {
    pthread_mutex_t mMutex;
} MutexHandle;
typedef MutexHandle* MutexHandler;

void Mutex_Init(MutexHandler *handler);
void Mutex_Lock(MutexHandler handler);
void Mutex_Unlock(MutexHandler handler);
void Mutex_Destory(MutexHandler *handler);


//Semaphore
typedef struct SemaphoreHandle {
    int mSem;
    int mNum;
} SemaphoreHandle;
typedef SemaphoreHandle* SemaphoreHandler;

void Semaphore_Init(SemaphoreHandler *handler, int num);
void Semaphore_Add(SemaphoreHandler handler, int index); //index = -1: All
void Semaphore_Del(SemaphoreHandler handler, int index); //index = -1: All
void Semaphore_Destory(SemaphoreHandler *handler);

void Semaphore_SetMax(SemaphoreHandler handler, int index, int value); //index = -1: All
void Semaphore_WaitForZero(SemaphoreHandler handler, int index); //index = -1: All

#endif //__CONCURRENT_H__