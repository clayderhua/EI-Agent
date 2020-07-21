#ifndef _UTIL_STORAGE_H
#define _UTIL_STORAGE_H 

#include "export.h"
#ifdef WIN32
#else
#endif

typedef struct DISKINFOITEM{
	char name[260];
	int diskid;
	unsigned long long readpersec;
	unsigned long long writepersec;
	unsigned long long freespace;
	unsigned long long totalspace;
}DiskInfoItem;

typedef struct DISKITEMLIST{
	DiskInfoItem* disk;
	struct DISKITEMLIST* next;
}DiskItemList;

#define MAX_ITEM_COUNT 26

#ifdef __cplusplus
extern "C" {
#endif

WISEPLATFORM_API DiskItemList* util_storage_getdisklist();

WISEPLATFORM_API void util_storage_freedisklist(DiskItemList* list);

WISEPLATFORM_API unsigned long long util_storage_gettotaldiskspace(char* drive);

WISEPLATFORM_API unsigned long long util_storage_getfreediskspace(char* drive);

WISEPLATFORM_API int util_storage_getdiskspeed(DiskItemList* list);

#ifdef __cplusplus
}
#endif

#endif
