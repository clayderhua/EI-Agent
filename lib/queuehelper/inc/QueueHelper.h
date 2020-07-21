#ifndef _QUEUE_HELPER_H_
#define _QUEUE_HELPER_H_

#define DEF_QHNODE_CNT_MAX   10240

typedef struct QNode{
	void * pUserData;
	struct QNode *  next;
}QNode, *PQNode;

typedef void(*QHFreeUserDataCB)(void * pUserData);

typedef struct QHQueue{
	PQNode head;
	PQNode trail;
	unsigned int nodeCnt;
	QHFreeUserDataCB freeUserDataCB;
	unsigned int cntMax;
}QHQueue, *PQHQueue;

#ifdef __cplusplus
extern "C" {
#endif

PQHQueue QHInitQueue(unsigned int nodeCntMax, QHFreeUserDataCB freeUserDataCB);

void QHDestroyQueue(PQHQueue pQHQueue);

int QHEnqueue(PQHQueue pQHQueue, PQNode pQNode);

PQNode QHDequeue(PQHQueue pQHQueue);

PQNode QHPeek(PQHQueue pQHQueue);

#ifdef __cplusplus
};
#endif

#endif
