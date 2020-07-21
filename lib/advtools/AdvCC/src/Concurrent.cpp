#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "MemoryCtrl.h"
#include "Concurrent.h"


//Mutex
void Mutex_Init(MutexHandler *handler) {
    if(*handler != NULL) {
        perror("handler is NOT null");
        return;
    }
    *handler = (MutexHandler)AdvMalloc(sizeof(MutexHandle));
    if(*handler == NULL) return;
    pthread_mutex_init(&((*handler)->mMutex),NULL);
}

void Mutex_Lock(MutexHandler handler) {
    //printf("%s:%d\n",__FILE__,__LINE__);
    pthread_mutex_lock(&handler->mMutex);
    //printf("%s:%d\n",__FILE__,__LINE__);
}

void Mutex_Unlock(MutexHandler handler) {
    //printf("%s:%d\n",__FILE__,__LINE__);
    pthread_mutex_unlock(&handler->mMutex);
    //printf("%s:%d\n",__FILE__,__LINE__);
}

void Mutex_Destory(MutexHandler *handler) {
    if(*handler == NULL) {
        //perror("handler IS NULL");
        return;
    }
    pthread_mutex_destroy(&((*handler)->mMutex));
    AdvFree(handler);
}

//Semaphore
void Semaphore_Init(SemaphoreHandler *handler, int num) {
    if(*handler != NULL) {
        perror("handler is NOT null");
        return;
    }
    *handler = (SemaphoreHandler)AdvMalloc(sizeof(SemaphoreHandle));
    if(*handler == NULL) return;
    (*handler)->mNum = num;
    (*handler)->mSem = semget(IPC_PRIVATE,num,IPC_CREAT|00666);
}

void Semaphore_Add(SemaphoreHandler handler, int index) {
    if(handler == NULL) return;
    if(index < -1) return;
    //printf("%s:%d\n",__FILE__,__LINE__);
    if(index == -1) {
        struct sembuf *op = (struct sembuf *)AdvMalloc(sizeof(struct sembuf)*handler->mNum);
        int i = 0;
        for(i = 0 ; i < handler->mNum ; i++) {
            op[i].sem_num = i;
            op[i].sem_op = 1;
            op[i].sem_flg = SEM_UNDO;
        }
        semop(handler->mSem,op,handler->mNum);
        AdvFree(&op);
    } else {
        //printf("%s:%d arg.val = %d\n",__FILE__,__LINE__,semctl(handler->mSem,0,GETVAL,0));
        struct sembuf op;
        op.sem_num = index;
        op.sem_op = 1;
        op.sem_flg = SEM_UNDO;
        semop(handler->mSem,&op,1);
    }
    //printf("%s:%d\n",__FILE__,__LINE__);
}

void Semaphore_Del(SemaphoreHandler handler, int index) {
    if(handler == NULL) return;
    if(index < -1) return;
    //printf("%s:%d\n",__FILE__,__LINE__);
    if(index == -1) {
        struct sembuf *op = (struct sembuf *)AdvMalloc(sizeof(struct sembuf)*handler->mNum);
        int i = 0;
        for(i = 0 ; i < handler->mNum ; i++) {
            op[i].sem_num = i;
            op[i].sem_op = -1;
            op[i].sem_flg = SEM_UNDO;
        }
        semop(handler->mSem,op,handler->mNum);
        AdvFree(&op);
    } else {
        //printf("%s:%d arg.val = %d\n",__FILE__,__LINE__,semctl(handler->mSem,0,GETVAL,0));
        struct sembuf op;
        op.sem_num = index;
        op.sem_op = -1;
        op.sem_flg = SEM_UNDO;
        semop(handler->mSem,&op,1);
    }
    //printf("%s:%d\n",__FILE__,__LINE__);
}

void Semaphore_Destory(SemaphoreHandler *handler) {
    if(*handler == NULL) {
        //perror("handler IS NULL");
        return;
    }
    if((*handler)->mSem != -1) {
        semctl((*handler)->mSem,0,IPC_RMID);
    }
    AdvFree(handler);
}

void Semaphore_SetMax(SemaphoreHandler handler, int index, int value) {
    if(handler == NULL) return;
    if(index < -1) return;
    if(index == -1) {
        int i = 0;
        for(i = 0 ; i < handler->mNum ; i++) {
            semctl(handler->mSem,i,SETVAL,value);
        }
    } else {
        semctl(handler->mSem,index,SETVAL,value);
    }
}

void Semaphore_WaitForZero(SemaphoreHandler handler, int index) {
    if(handler == NULL) return;
    if(index < -1) return;
    //printf("%s:%d\n",__FILE__,__LINE__);

    if(index == -1) {
        struct sembuf *op = (struct sembuf *)AdvMalloc(sizeof(struct sembuf)*handler->mNum);
        int i = 0;
        for(i = 0 ; i < handler->mNum ; i++) {
            op[i].sem_num = i;
            op[i].sem_op = 0;
            op[i].sem_flg = SEM_UNDO;
        }
        semop(handler->mSem,op,handler->mNum);
        AdvFree(&op);
    } else {
        //printf("%s:%d arg.val = %d\n",__FILE__,__LINE__,semctl(handler->mSem,0,GETVAL,0));
        struct sembuf op;
        op.sem_num = index;
        op.sem_op = 0;
        op.sem_flg = SEM_UNDO;
        semop(handler->mSem,&op,1);
    }
    //printf("%s:%d\n",__FILE__,__LINE__);
}
