#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "Sequential.h"
#include "Concurrent.h"
#include "MemoryCtrl.h"
#include "AdvCC.h"

void ADVCC_CALL Seq_Init(SeqHandler *handler) {
    if(*handler != NULL) {
        perror("handler is NOT null");
        return;
    }
    *handler = (SeqHandler)AdvMalloc(sizeof(SeqHandle));
    if(*handler == NULL) return;

    (*handler)->mAccessMutex = NULL;
    Mutex_Init(&(*handler)->mAccessMutex);

    (*handler)->mSeqAmount = 0;
    (*handler)->mSeqHead = NULL;
    (*handler)->mSeqTail = NULL;
    (*handler)->mBlockingSem = NULL;
}

void ADVCC_CALL Seq_InitBlocking(SeqHandler *handler, int maxItems) {
    if(*handler != NULL) {
        perror("handler is NOT null");
        return;
    }
    *handler = (SeqHandler)AdvMalloc(sizeof(SeqHandle));
    if(*handler == NULL) return;

    (*handler)->mAccessMutex = NULL;
    Mutex_Init(&(*handler)->mAccessMutex);

    (*handler)->mSeqAmount = 0;
    (*handler)->mSeqHead = NULL;
    (*handler)->mSeqTail = NULL;

    (*handler)->mBlockingSem = NULL;
    Semaphore_Init(&(*handler)->mBlockingSem, 2);

    (*handler)->mMaxItems = 0;

    if(maxItems > 0) {
        (*handler)->mMaxItems = maxItems;
        Semaphore_SetMax((*handler)->mBlockingSem, 1, maxItems);
    }
}

void ADVCC_CALL Seq_Destroy(SeqHandler *handler){
    if(*handler == NULL) {
        //perror("handler IS NULL");
        return;
    }
    Seq_Flush(*handler);
    Semaphore_Destory(&(*handler)->mBlockingSem);
    Mutex_Destory(&(*handler)->mAccessMutex);
    AdvFree(handler);
}

/**********************SEM********************************/

void __Seq_Sem_WaitForNewSpace(SeqHandler handler) {
    if(handler == NULL) return;
    if(handler->mMaxItems > 0) {
        /**upperbound**/
        Semaphore_Del(handler->mBlockingSem, 1);
    }
}

void __Seq_Sem_AddResource(SeqHandler handler) {
    if(handler == NULL) return;
    /**lowerbound**/
    Semaphore_Add(handler->mBlockingSem, 0);
}

void __Seq_Sem_WaitForNewResource(SeqHandler handler) {
    if(handler == NULL) return;
    /**lowerbound**/
    Semaphore_Del(handler->mBlockingSem, 0);
}

void __Seq_Sem_AddSpace(SeqHandler handler) {
    if(handler == NULL) return;
    if(handler->mMaxItems > 0) {
        /**upperbound**/
        Semaphore_Add(handler->mBlockingSem, 1);
    }
}

void __Seq_Sem_WaitForZero(SeqHandler handler) {
    if(handler == NULL) return;
    Semaphore_WaitForZero(handler->mBlockingSem, 0);
}
void __Seq_Sem_Reset(SeqHandler handler) {
    if(handler == NULL) return;
    Semaphore_SetMax(handler->mBlockingSem, 1, 0);
    if(handler->mMaxItems > 0) {
        Semaphore_SetMax(handler->mBlockingSem, 1, handler->mMaxItems);
    }
}

void __Seq_Sem_LockAccess(SeqHandler handler) {
    if(handler == NULL) return;
    Mutex_Lock(handler->mAccessMutex);
}

void __Seq_Sem_UnlockAccess(SeqHandler handler) {
    if(handler == NULL) return;
    Mutex_Unlock(handler->mAccessMutex);
}
/***************Find*************************************/
/**ThreadSafe**/
SeqElement * ADVCC_CALL Seq_FindEqual(SeqHandler handler, void *data) {
	if (handler == NULL) return NULL;
	if (data == NULL) return NULL;
	if (handler->mSeqAmount == 0) return NULL;

	/**Lock**/
	__Seq_Sem_LockAccess(handler);

	SeqElement *elm = handler->mSeqTail;
	while (elm != NULL) {
		if (memcmp(((char *)elm->mData) + sizeof(SeqInfo), data, ((SeqInfo *)elm->mData)->mDataSize) == 0) {
			__Seq_Sem_UnlockAccess(handler);
			/**Unlock**/
			return elm;
		}
		elm = elm->mPrev;
	}

	__Seq_Sem_UnlockAccess(handler);
	/**Unlock**/

	return NULL;
}

/**ThreadSafe**/
SeqElement * ADVCC_CALL Seq_SearchString(SeqHandler handler, char *string) {
	if (handler == NULL) return NULL;
	if (string == NULL || strlen(string) == 0) return NULL;
	if (handler->mSeqAmount == 0) return NULL;


	/**Lock**/
	__Seq_Sem_LockAccess(handler);


	SeqElement *elm = handler->mSeqTail;

	int Len = strlen(string);
	int DataLen = ((SeqInfo *)elm->mData)->mDataSize;
	int Pos = 0;
	int range = 0;
	int index = 0;
	char *pointer = NULL;
	while (elm != NULL) {
		pointer = ((char *)elm->mData) + sizeof(SeqInfo);
		Pos = 0;
		while (1) {
			index = pointer - (char *)elm->mData;
			range = DataLen - index;
			if (range <= 0) break;
			pointer = (char *)memchr(pointer, string[Pos], range);
			if (pointer != 0) {
				Pos++;
				if (Pos == Len) {
					if (*(pointer + 1) == 0) {
						__Seq_Sem_UnlockAccess(handler);
						/**Unlock**/
						return elm;
					}
					else break;
				}
			}
			else break;
		}
		elm = elm->mPrev;
	}

	__Seq_Sem_UnlockAccess(handler);
	/**Unlock**/

	return NULL;
}

/**ThreadSafe**/
SeqElement *__Seq_FindKey(SeqHandler handler, unsigned long key) {
	if (handler == NULL) return NULL;
	if (handler->mSeqAmount == 0) return NULL;

	/**Lock**/
	//__Seq_Sem_LockAccess(handler);

	SeqElement *elm = handler->mSeqHead;
	while (elm != NULL) {
		if (((SeqInfo *)elm->mData)->mKey == key) {
			__Seq_Sem_UnlockAccess(handler);
			/**Unlock**/
			return elm;
		}
		elm = elm->mNext;
	}

	//__Seq_Sem_UnlockAccess(handler);
	/**Unlock**/

	return NULL;
}
/*****************DEL**********************************/
void __Seq_DelLink(SeqElement *elm) {

	if (elm->mPrev != NULL) {
		elm->mPrev->mNext = elm->mNext;
		if (elm->mNext == NULL) {
			elm->handler->mSeqTail = elm->mPrev;
		}
	}

	if (elm->mNext != NULL) {
		elm->mNext->mPrev = elm->mPrev;
		if (elm->mPrev == NULL) {
			elm->handler->mSeqHead = elm->mNext;
		}
	}

	if (elm->mPrev == NULL && elm->mNext == NULL) {
		elm->handler->mSeqTail = NULL;
		elm->handler->mSeqHead = NULL;
	}
}

void __Seq_DelElement(SeqElement *elm) {
	__Seq_DelLink(elm);
	AdvFree(&(elm->mData));
	AdvFree(&elm);
}

void __Seq_DelTail(SeqHandler handler) {
	if (handler == NULL) return;
	SeqElement *elm = handler->mSeqTail;

	elm->handler->mSeqAmount--;
	__Seq_DelElement(elm);
	//printf("<%s,%d> handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);
}

void __Seq_DelHead(SeqHandler handler) {
	if (handler == NULL) return;
	SeqElement *elm = handler->mSeqHead;

	elm->handler->mSeqAmount--;
	__Seq_DelElement(elm);

	elm->handler->mSeqAmount--;
	//printf("<%s,%d> handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);
}
/*******************VIEW*******************************/

int ADVCC_CALL Seq_PeekWithKey(SeqHandler handler, void *data, unsigned long key) {
	if (handler == NULL) return CON_RESULT_NO_HANDLER;
	if (data == NULL) return CON_RESULT_ERROR_DESTINATION;
	//printf("<%s,%d> Seq_PopHead handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);

	/**Lock**/
	__Seq_Sem_LockAccess(handler);

	if (handler->mSeqAmount == 0) {
		__Seq_Sem_UnlockAccess(handler);
		/**Unlock**/
		return CON_RESULT_EMPTY;
	}

	SeqElement *elm = __Seq_FindKey(handler, key);
	memcpy(data, ((char *)elm->mData) + sizeof(SeqInfo), ((SeqInfo *)elm->mData)->mDataSize);

	__Seq_Sem_UnlockAccess(handler);
	/**Unlock**/
	return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PeekHead(SeqHandler handler, void *data) {
	if (handler == NULL) return CON_RESULT_NO_HANDLER;
	if (data == NULL) return CON_RESULT_ERROR_DESTINATION;
	//printf("<%s,%d> Seq_PopHead handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);

	/**Lock**/
	__Seq_Sem_LockAccess(handler);

	if (handler->mSeqAmount == 0) {
		__Seq_Sem_UnlockAccess(handler);
		/**Unlock**/
		return CON_RESULT_EMPTY;
	}

	SeqElement *elm = handler->mSeqHead;
	memcpy(data, ((char *)elm->mData) + sizeof(SeqInfo), ((SeqInfo *)elm->mData)->mDataSize);

	__Seq_Sem_UnlockAccess(handler);
	/**Unlock**/
	return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PeekTail(SeqHandler handler, void *data) {
	if (handler == NULL) return CON_RESULT_NO_HANDLER;
	if (data == NULL) return CON_RESULT_ERROR_DESTINATION;
	//printf("<%s,%d> Seq_PopHead handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);

	/**Lock**/
	__Seq_Sem_LockAccess(handler);

	if (handler->mSeqAmount == 0) {
		__Seq_Sem_UnlockAccess(handler);
		/**Unlock**/
		return CON_RESULT_EMPTY;
	}

	SeqElement *elm = handler->mSeqTail;
	memcpy(data, ((char *)elm->mData) + sizeof(SeqInfo), ((SeqInfo *)elm->mData)->mDataSize);

	__Seq_Sem_UnlockAccess(handler);
	/**Unlock**/
	return CON_RESULT_NORMAL;
}


/**ThreadSafe**/
void ADVCC_CALL Seq_ReQueueToHead(SeqHandler handler, SeqElement *seq){
    if(handler == NULL) return;
    if(handler->mSeqAmount == 0) return;

     /**Lock**/
    __Seq_Sem_LockAccess(handler);

    __Seq_DelLink(seq);

    //Add Head Link
    seq->mNext = handler->mSeqHead;
    seq->mPrev = NULL;
    if(handler->mSeqTail == NULL) {
        handler->mSeqTail = seq;
    } else {
        handler->mSeqHead->mPrev = seq;
    }


    handler->mSeqHead = seq;

    __Seq_Sem_UnlockAccess(handler);
    /**Unlock**/
    return;
}

/**ThreadSafe**/
void ADVCC_CALL Seq_ReQueueToTail(SeqHandler handler, SeqElement *seq){
    if(handler == NULL) return;
    if(handler->mSeqAmount == 0) return;

    /**Lock**/
    __Seq_Sem_LockAccess(handler);

    __Seq_DelLink(seq);

    //Add Tail Link
    seq->mNext = NULL;
    seq->mPrev = handler->mSeqTail;
    if(handler->mSeqHead == NULL) {
        //printf("<%s,%d> Seq_PushTail handler->mSeqHead == NULL\n",__FILE__,__LINE__);
        handler->mSeqHead = seq;
    } else {
        handler->mSeqTail->mNext = seq;
    }

    handler->mSeqTail = seq;


    __Seq_Sem_UnlockAccess(handler);
    /**Unlock**/
    return;
}

/**ThreadSafe**/
int ADVCC_CALL Seq_PeekSeqElement(SeqHandler handler, SeqElement *seq, void *data) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    if(seq == NULL) return CON_RESULT_ERROR_SOURCE;
    if(data == NULL) return CON_RESULT_ERROR_DESTINATION;
    if(handler->mSeqAmount == 0) return CON_RESULT_EMPTY;

    /**Lock**/
    __Seq_Sem_LockAccess(handler);

    memcpy(data,((char *)seq->mData)+sizeof(SeqInfo),((SeqInfo *)seq->mData)->mDataSize);

    __Seq_Sem_UnlockAccess(handler);
    /**Unlock**/

    return CON_RESULT_NORMAL;
}


/*******************GET*******************************/
int ADVCC_CALL Seq_GetAmount(SeqHandler handler) {
    if(handler == NULL) return 0;
    return handler->mSeqAmount;
}

/**ThreadSafe**/
int ADVCC_CALL Seq_GetAmountWithKey(SeqHandler handler, unsigned long key) {
    if(handler == NULL) return 0;
    if(handler->mSeqAmount == 0) return 0;

    /**Lock**/
    __Seq_Sem_LockAccess(handler);

    SeqElement *elm = handler->mSeqTail;
    int Count = handler->mSeqAmount;
    int Num = 0;
    while(elm != NULL) {
        if(((SeqInfo *)elm->mData)->mKey == key) Num++;
        elm = elm->mPrev;
        Count--;
    }

    __Seq_Sem_UnlockAccess(handler);
    /**Unlock**/

    return Num;
}

/*******************OPERATOR**************************/
/**ThreadSafe**/
int ADVCC_CALL Seq_PushTailWithKey(SeqHandler handler, void *data, int dataSize, unsigned long key) {
	if (handler == NULL) return CON_RESULT_NO_HANDLER;
	if (data == NULL || dataSize <= 0) return CON_RESULT_ERROR_SOURCE;

	/**Lock**/
	__Seq_Sem_LockAccess(handler);

	//printf("<%s,%d> Seq_PushTail handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);
	SeqElement *elm = (SeqElement *)AdvMalloc(sizeof(SeqElement));
	SeqInfo *SeqData = (SeqInfo *)AdvMalloc(sizeof(SeqInfo) + dataSize);

	memcpy(((char *)SeqData) + sizeof(SeqInfo), data, dataSize);
	elm->mData = SeqData;
	((SeqInfo *)elm->mData)->mKey = key;
	((SeqInfo *)elm->mData)->mDataSize = dataSize;

	elm->mNext = NULL;
	elm->mPrev = handler->mSeqTail;
	if (elm->mPrev != NULL) {
		elm->mPrev->mNext = elm;
	}

	if (handler->mSeqHead == NULL && handler->mSeqTail == NULL) {
		handler->mSeqHead = elm;
		handler->mSeqTail = elm;
	}


	handler->mSeqAmount++;
	handler->mSeqTail = elm;
	elm->handler = handler;

	__Seq_Sem_UnlockAccess(handler);
	/**Unlock**/

	return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PushTail(SeqHandler handler, void *data, int dataSize) {
	return Seq_PushTailWithKey(handler, data, dataSize, 0);
}

int ADVCC_CALL Seq_PushTailWithKeyBlocking(SeqHandler handler, void *data, int dataSize, unsigned long key) {
	if (handler == NULL) return CON_RESULT_NO_HANDLER;
	__Seq_Sem_WaitForNewSpace(handler);
	Seq_PushTailWithKey(handler, data, dataSize, key);
	__Seq_Sem_AddResource(handler);
	return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PushTailBlocking(SeqHandler handler, void *data, int dataSize) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    __Seq_Sem_WaitForNewSpace(handler);
    Seq_PushTail(handler, data, dataSize);
    __Seq_Sem_AddResource(handler);
    return CON_RESULT_NORMAL;
}



/**ThreadSafe**/
int ADVCC_CALL Seq_PushHeadWithKey(SeqHandler handler, void *data, int dataSize, unsigned long key) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    if(data == NULL || dataSize <= 0) return CON_RESULT_ERROR_SOURCE;

    /**Lock**/
    __Seq_Sem_LockAccess(handler);

    SeqElement *elm = (SeqElement *)AdvMalloc(sizeof(SeqElement));
	SeqInfo *SeqData = (SeqInfo *)AdvMalloc(sizeof(SeqInfo)+dataSize);

	memcpy(((char *)SeqData) + sizeof(SeqInfo), data, dataSize);
    elm->mData = SeqData;
	((SeqInfo *)elm->mData)->mKey = key;
    ((SeqInfo *)elm->mData)->mDataSize = dataSize;

    elm->mNext = handler->mSeqHead;
    elm->mPrev = NULL;
    if(elm->mNext != NULL) {
        elm->mNext->mPrev = elm;
    }

    if(handler->mSeqHead == NULL && handler->mSeqTail == NULL) {
        handler->mSeqHead = elm;
        handler->mSeqTail = elm;
    }

    handler->mSeqAmount++;
    handler->mSeqHead = elm;
    elm->handler = handler;
	
    __Seq_Sem_UnlockAccess(handler);
    /**Unlock**/
    return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PushHead(SeqHandler handler, void *data, int dataSize) {
	return Seq_PushHeadWithKey(handler, data, dataSize, 0);
}

int ADVCC_CALL Seq_PushHeadWithKeyBlocking(SeqHandler handler, void *data, int dataSize, unsigned long key) {
	if (handler == NULL) return CON_RESULT_NO_HANDLER;
	__Seq_Sem_WaitForNewSpace(handler);
	Seq_PushHeadWithKey(handler, data, dataSize, key);
	__Seq_Sem_AddResource(handler);
	return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PushHeadBlocking(SeqHandler handler, void *data, int dataSize) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    __Seq_Sem_WaitForNewSpace(handler);
    Seq_PushHead(handler, data, dataSize);
    __Seq_Sem_AddResource(handler);
    return CON_RESULT_NORMAL;
}



/**ThreadSafe**/
int ADVCC_CALL Seq_PopTail(SeqHandler handler, void *data) {
	if (handler == NULL) return CON_RESULT_NO_HANDLER;
	if (data == NULL) return CON_RESULT_ERROR_DESTINATION;
	if (handler->mSeqAmount == 0) return CON_RESULT_EMPTY;
	//printf("<%s,%d> Seq_PopTail handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);

	 /**Lock**/
	__Seq_Sem_LockAccess(handler);

	if (handler->mSeqAmount == 0) {
		__Seq_Sem_UnlockAccess(handler);
		/**Unlock**/
		return CON_RESULT_EMPTY;
	}

	SeqElement *elm = handler->mSeqTail;
	memcpy(data, ((char *)elm->mData) + sizeof(SeqInfo), ((SeqInfo *)elm->mData)->mDataSize);

	__Seq_DelTail(handler);

	__Seq_Sem_UnlockAccess(handler);
	/**Unlock**/
	return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PopTailBlocking(SeqHandler handler, void *data) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    __Seq_Sem_WaitForNewResource(handler);
    Seq_PopTail(handler, data);
    __Seq_Sem_AddSpace(handler);
    return CON_RESULT_NORMAL;
}

/**ThreadSafe**/
int ADVCC_CALL Seq_PopHead(SeqHandler handler, void *data) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    if(data == NULL) return CON_RESULT_ERROR_DESTINATION;
    //printf("<%s,%d> Seq_PopHead handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);

    /**Lock**/
    __Seq_Sem_LockAccess(handler);

    if(handler->mSeqAmount == 0) {
        __Seq_Sem_UnlockAccess(handler);
        /**Unlock**/
        return CON_RESULT_EMPTY;
    }

    SeqElement *elm = handler->mSeqHead;
    memcpy(data, ((char *)elm->mData) + sizeof(SeqInfo),((SeqInfo *)elm->mData)->mDataSize);
    __Seq_DelHead(handler);
    __Seq_Sem_UnlockAccess(handler);
    /**Unlock**/
    return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PopHeadBlocking(SeqHandler handler, void *data) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    __Seq_Sem_WaitForNewResource(handler);
    Seq_PopHead(handler, data);
    __Seq_Sem_AddSpace(handler);
    return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PopWithKey(SeqHandler handler, void *data, unsigned long key) {
	if (handler == NULL) return CON_RESULT_NO_HANDLER;
	if (data == NULL) return CON_RESULT_ERROR_DESTINATION;
	//printf("<%s,%d> Seq_PopHead handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);

	/**Lock**/
	__Seq_Sem_LockAccess(handler);

	if (handler->mSeqAmount == 0) {
		__Seq_Sem_UnlockAccess(handler);
		/**Unlock**/
		return CON_RESULT_EMPTY;
	}

	SeqElement *elm = __Seq_FindKey(handler, key);
	memcpy(data, ((char *)elm->mData) + sizeof(SeqInfo), ((SeqInfo *)elm->mData)->mDataSize);
	elm->handler->mSeqAmount--;
	__Seq_DelElement(elm);
	__Seq_Sem_UnlockAccess(handler);
	/**Unlock**/
	return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_PopWithKeyBlocking(SeqHandler handler, void *data, unsigned long key) {
	if (handler == NULL) return CON_RESULT_NO_HANDLER;
	__Seq_Sem_WaitForNewResource(handler);
	Seq_PopWithKey(handler, data, key);
	__Seq_Sem_AddSpace(handler);
	return CON_RESULT_NORMAL;
}


/**ThreadSafe**/
int ADVCC_CALL Seq_DelSeqElement(SeqHandler handler, SeqElement *seq) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    //printf("<%s,%d> Seq_PopHead handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);

    /**Lock**/
    __Seq_Sem_LockAccess(handler);

    if(handler->mSeqAmount == 0) {
        __Seq_Sem_UnlockAccess(handler);
        /**Unlock**/
        return CON_RESULT_EMPTY;
    }

	seq->handler->mSeqAmount--;
	__Seq_DelElement(seq);
    //printf("<%s,%d> handler->mSeqAmount = %d\n",__FILE__,__LINE__,handler->mSeqAmount);

    __Seq_Sem_UnlockAccess(handler);
    /**Unlock**/
    return CON_RESULT_NORMAL;
}

int ADVCC_CALL Seq_DelSeqElementBlocking(SeqHandler handler, SeqElement *seq) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    __Seq_Sem_WaitForNewResource(handler);
    Seq_DelSeqElement(handler, seq);
    __Seq_Sem_AddSpace(handler);
    return CON_RESULT_NORMAL;
}

/**ThreadSafe**/
void ADVCC_CALL Seq_Flush(SeqHandler handler) {
    if(handler == NULL) return;
    /**Lock**/
    __Seq_Sem_LockAccess(handler);
    while(Seq_GetAmount(handler) != 0){
        __Seq_DelTail(handler);
    }
    __Seq_Sem_Reset(handler);
    __Seq_Sem_UnlockAccess(handler);
    /**Unlock**/
}


/*****************WAIT**********************************/
int ADVCC_CALL Seq_WaitForEmptyBlocking(SeqHandler handler) {
    if(handler == NULL) return CON_RESULT_NO_HANDLER;
    __Seq_Sem_WaitForZero(handler);
    return CON_RESULT_NORMAL;
}

/*****************PRINT*********************************/

/**ThreadSafe**/
void ADVCC_CALL Seq_PrintAll(SeqHandler handler) {
    if(handler == NULL) return;
    if(handler->mSeqAmount == 0) return;

    /**Lock**/
    __Seq_Sem_LockAccess(handler);

    SeqElement *elm = handler->mSeqHead;
    int num = 0;

    printf("***<Head %p>\n", handler->mSeqHead);

    while(elm != NULL) {
        printf("Node [%d] %p\n", num, elm);
            printf("\tPrev [%p]\n", elm->mPrev);
            printf("\tNext [%p]\n", elm->mNext);
        printf("\n");
        elm = elm->mNext;
        num++;
    }
    printf("***<Tail %p>\n\n", handler->mSeqTail);
    __Seq_Sem_UnlockAccess(handler);
    /**Unlock**/

    return;
}







