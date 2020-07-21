#include <stdlib.h>
#include "ListHelper.h"

PLHList LHInitList(LHFreeUserDataCB freeUserDataCB)
{
	PLHList lhList = (PLHList)malloc(sizeof(LHList));
	if(lhList)
	{
		lhList->head = NULL;
		lhList->trail = NULL;
		lhList->freeUserDataCB = freeUserDataCB;
	}
	return lhList;
}

void LHDestroyList(PLHList lhList)
{
	if(lhList)
	{
		LHDelAllNode(lhList);
		free(lhList);
	}
}

int LHAddNode(PLHList lhList, void * pUserData)
{
	int iRet = -1;
	if(lhList != NULL && pUserData != NULL)
	{
		PLNode pLNode = (PLNode)malloc(sizeof(LNode));
		if(pLNode)
		{
			pLNode->pUserData = pUserData;
			if(lhList->trail != NULL)
			{
				lhList->trail->next = pLNode;
			}

			lhList->trail = pLNode;
			lhList->trail->next = NULL;

			if(lhList->head == NULL)
			{
				lhList->head = lhList->trail;
			}
			iRet = 0;
		}
	}
	return iRet;
}

PLNode LHFindNode(PLHList lhList, LHMatchCheckCB matchCheckCB, void * key)
{
	PLNode pLNode = NULL;
	if(lhList != NULL && matchCheckCB != NULL && key != NULL)
	{
		PLNode curLNode = lhList->head;
		while(curLNode)
		{
			if(matchCheckCB(curLNode->pUserData, key))
			{
				pLNode = curLNode;
				break;
			}
			curLNode = curLNode->next;
		}
	}
	return pLNode;
}

int LHDelNode(PLHList lhList, LHMatchCheckCB matchCheckCB, void * key)
{
	int iRet = -1;
	if(lhList != NULL && matchCheckCB != NULL && key != NULL)
	{
		PLNode preLNode = NULL, delLNode = NULL;
		PLNode curLNode = lhList->head;
		iRet = 1;
		while(curLNode)
		{
			if(matchCheckCB(curLNode->pUserData, key))
			{
				delLNode = curLNode;
				if(delLNode == lhList->head)
				{
					lhList->head = curLNode->next;
					curLNode = lhList->head;
				}
				else
				{
					preLNode->next = curLNode->next;
					curLNode = preLNode->next;
				}
				if(delLNode == lhList->trail)
				{
					lhList->trail = preLNode;
				}
				if(lhList->freeUserDataCB) lhList->freeUserDataCB(delLNode->pUserData); 
				delLNode->pUserData = NULL;
				free(delLNode);
				delLNode = NULL;
				iRet = 0;
			}
			else
			{
				preLNode = curLNode;
				curLNode = preLNode->next;
			}
		}
	}
	return iRet;
}

void LHDelAllNode(PLHList lhList)
{
	if(lhList)
	{
		PLNode pLNode = NULL;
		while(lhList->head) 
		{
			pLNode = lhList->head;
			lhList->head = pLNode->next;
			if(pLNode == lhList->trail)
			{
				lhList->trail = NULL;
			}
			if(pLNode)
			{
				if(lhList->freeUserDataCB) lhList->freeUserDataCB(pLNode->pUserData); 
				pLNode->pUserData = NULL;
				free(pLNode);
				pLNode = NULL;
			}
		}
	}
}

int LHListIsEmpty(PLHList lhList)
{
	return lhList !=NULL && lhList->head != NULL?0:1;
}

void LHIterate(PLHList lhList,int (*fnc)(void *args, void * pUserData), void *args)
{
	if(lhList != NULL && lhList->head != NULL && fnc != NULL)
	{
		PLNode curNode = lhList->head;
		while(curNode)
		{
			fnc(args, curNode->pUserData);
			curNode = curNode->next;
		}
	}
}