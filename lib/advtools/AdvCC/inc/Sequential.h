#ifndef __SEQUENTIAL_H__
#define __SEQUENTIAL_H__

#include "Concurrent.h"

struct SeqInfo {
	unsigned long mKey;
	unsigned int mDataSize;
	void *Content;
};

typedef struct SeqHandle SeqHandle;
typedef SeqHandle* SeqHandler;

struct SeqElement {
	void *mData;
	SeqElement *mNext;
	SeqElement *mPrev;
	SeqHandler handler;
};

struct SeqHandle {
	SeqElement *mSeqHead;
	SeqElement *mSeqTail;
	int mSeqAmount;
	MutexHandler mAccessMutex;
	SemaphoreHandler mBlockingSem;
	int mMaxItems;
};

void __Seq_Sem_WaitForNewSpace(SeqHandler handler);
void __Seq_Sem_AddResource(SeqHandler handler);
void __Seq_Sem_WaitForNewResource(SeqHandler handler);
void __Seq_Sem_AddSpace(SeqHandler handler);
void __Seq_Sem_WaitForZero(SeqHandler handler);
void __Seq_Sem_Reset(SeqHandler handler);
void __Seq_Sem_LockAccess(SeqHandler handler);
void __Seq_Sem_UnlockAccess(SeqHandler handler);

void __Seq_DelLink(SeqElement *elm);
void __Seq_DelTail(SeqHandler handler);
void __Seq_DelHead(SeqHandler handler);

#endif //__SEQUENTIAL_H__

