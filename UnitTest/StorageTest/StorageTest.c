#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util_storage.h"

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	int iRet = 0;
	DiskItemList* disklist = NULL;
	DiskItemList* item = NULL;
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif

	disklist = util_storage_getdisklist();	
	item = disklist;
	while(item)
	{
		if(item->disk)
		{
			DiskInfoItem* disk = item->disk;
			disk->totalspace = util_storage_gettotaldiskspace(disk->name);
			disk->freespace = util_storage_getfreediskspace(disk->name);
			/*if(disk->totalspace>0)
				disk->usage = (1-(double)disk->freespace/(double)disk->totalspace)*100;*/
			printf("Disk(%s) %I64u/%I64u MB\n", disk->name, disk->freespace, disk->totalspace);
		}
		item = item->next;
	}

	printf("Click enter to exit");
	fgetc(stdin);

	util_storage_freedisklist(disklist);
	disklist = NULL;

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif
	return iRet;
}
