#ifndef _LIST_HELPER_H_
#define _LIST_HELPER_H_

typedef struct LNode{
	void * pUserData;
	struct LNode * next;
}LNode, *PLNode;

typedef void(*LHFreeUserDataCB)(void * pUserData);

typedef int(*LHMatchCheckCB)(void * pUserData, void * key);

typedef struct LHList{
	PLNode head;
	PLNode trail;
	LHFreeUserDataCB freeUserDataCB;
}LHList, *PLHList;

#ifdef __cplusplus
extern "C" {
#endif

	PLHList LHInitList(LHFreeUserDataCB freeUserDataCB);

	void LHDestroyList(PLHList lhList);

	int LHAddNode(PLHList lhList, void * pUserData);

   PLNode LHFindNode(PLHList lhList, LHMatchCheckCB matchCheckCB, void * key);

	int LHDelNode(PLHList lhList, LHMatchCheckCB matchCheckCB, void * key);

	void LHDelAllNode(PLHList lhList);

	void LHIterate(PLHList lhList,int (*fnc)(void *args, void * pUserData), void *args);

	int LHListIsEmpty(PLHList lhList);

#ifdef __cplusplus
};
#endif

#endif
