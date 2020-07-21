#include <stdio.h>
#include <malloc.h>
#include "QueueHelper.h"

PQHQueue QHInitQueue(unsigned int nodeCntMax, QHFreeUserDataCB freeUserDataCB)
{
	PQHQueue pQHQueue = (PQHQueue)malloc(sizeof(QHQueue));
	if(pQHQueue)
	{
		if(nodeCntMax > 0) pQHQueue->cntMax = nodeCntMax;
		else pQHQueue->cntMax = DEF_QHNODE_CNT_MAX;
		pQHQueue->head = NULL;
		pQHQueue->trail = NULL;
		pQHQueue->nodeCnt = 0;
		pQHQueue->freeUserDataCB = freeUserDataCB;
	}
	return pQHQueue;
}

void QHDestroyQueue(PQHQueue pQHQueue)
{
	if(pQHQueue)
	{
		PQNode pQNode = NULL;
		while( (pQNode = QHDequeue(pQHQueue)) != NULL )
		{
			if(pQHQueue->freeUserDataCB) pQHQueue->freeUserDataCB(pQNode->pUserData);
			pQNode->pUserData = NULL;
			free(pQNode);
			pQNode = NULL;
		}
		pQHQueue->head = NULL;
		pQHQueue->trail = NULL;
		pQHQueue->nodeCnt = 0;
		free(pQHQueue);
	}
}

int QHEnqueue(PQHQueue pQHQueue, PQNode pQNode)
{
	int iRet = 0;
	if(pQHQueue != NULL && pQNode != NULL)
	{
		if(pQHQueue->trail != NULL)
		{
			pQHQueue->trail->next = pQNode;
		}

		pQHQueue->trail = pQNode;
		pQHQueue->nodeCnt++;
		pQHQueue->trail->next = NULL;

		if(pQHQueue->head == NULL)
		{
			pQHQueue->head = pQHQueue->trail;
		}

		if(pQHQueue->nodeCnt > pQHQueue->cntMax)
		{
			PQNode pDelNode = QHDequeue(pQHQueue);
			if(pQHQueue->freeUserDataCB) pQHQueue->freeUserDataCB(pDelNode); 
			free(pDelNode);
			pDelNode = NULL;
		}
	}
	else iRet = -1;
	return iRet;
}

PQNode QHDequeue(PQHQueue pQHQueue)
{
	PQNode pQNode = NULL;
	if(NULL == pQHQueue || NULL == pQHQueue->head) return NULL;
	pQNode = pQHQueue->head;
	pQHQueue->head = pQNode->next;
	pQHQueue->nodeCnt--;
	if(pQNode == pQHQueue->trail)
	{
		pQHQueue->trail = NULL;
	}
	return pQNode;
}

PQNode QHPeek(PQHQueue pQHQueue)
{
	PQNode pQNode = NULL;
	if(NULL == pQHQueue || NULL == pQHQueue->head) return NULL;
	pQNode = pQHQueue->head;
	return pQNode;
}