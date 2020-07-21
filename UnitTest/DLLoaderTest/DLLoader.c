#include "agentlog.h"
#include <string.h>
#include <stdio.h>
#include "util_path.h"
#include "WISEPlatform.h"
#include "util_libloader.h"

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	int iRet = 0;
	char filepath[260] = {0};
	char* error = NULL;
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif

	if(argc<2)
	{
		printf("Usage: DLLoader <FileName>\n");
		return 1;
	}

	strcpy(filepath, argv[1]);

	util_dlexist(filepath);
	error = util_dlerror();
	if(error)
	{
		printf("%s\n", error);
		free(error);
	}
		

	printf("Click enter to exit");
	fgetc(stdin);

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif

	return 0;
}

