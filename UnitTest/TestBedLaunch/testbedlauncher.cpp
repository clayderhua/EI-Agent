/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/01/23 by Scott Chang								    */
/* Modified Date: 2017/01/23 by Scott Chang									*/
/* Abstract     : WISE Core Ex Test Application								*/
/* Reference    : None														*/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "util_path.h"
#include "util_process.h"
#include "ReadINI.h"
#include <pthread.h>

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------
#ifdef WIN32
#define WISECoreExTest "WISECoreExTest.exe"
#define socket_limit 500
#else
#define WISECoreExTest "./WISECoreExTest"
#define socket_limit 300
#endif
static bool g_bStop = false;

void* threadinput(void* args)
{
	fgetc(stdin);
	g_bStop = true;
	pthread_exit(0);
	return NULL;
}

int main(int argc, char *argv[])
{
	char strseed[13] = "305A3A7%05d";
	char inifilepath[256] = "WISECoreExTest.ini";
	int i=0, count = 10, start = 0;
	int cntproc = 1, balance = 0, extra = 0;
	int offset = 0;
	int limit = socket_limit;
	HANDLE *procList = NULL;
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	_CrtMemCheckpoint( &memStateStart);
#endif

	if(argc > 1)
		limit = atoi(argv[1]);

	if(util_is_file_exist(inifilepath)) {
		count = GetIniKeyInt("Platform","total", inifilepath);
		strcpy(strseed, GetIniKeyString("Platform","seedID", inifilepath));
	}
	cntproc = count / limit;
	if(count % limit != 0) cntproc++;
	balance = count/cntproc;
	extra = count%cntproc;

	procList = (HANDLE *)calloc(cntproc, sizeof(HANDLE));

	for(i=0; i<cntproc; i++)
	{
		char cmd[256] = {0};
		int total = balance;
		if(extra > 0)
		{
			 total++;
			 extra--;
		}
		sprintf(cmd, "%s %s %d %d %d", WISECoreExTest, strseed, offset, total, i);
		procList[i] = util_process_cmd_launch_no_wait(cmd);
		offset += total;
	}

EXIT:
	printf("Click enter to exit\n");
	{
		pthread_t inputthread;
		if(pthread_create(&inputthread, NULL, threadinput, NULL)==0)
		{
			pthread_detach(inputthread);

			while(!g_bStop)
			{
				bool bRunning = false;
				for(i=0; i<cntproc; i++)
				{
					bRunning |= util_is_process_running(procList[i]);
				}
				if(!bRunning)
				{
					printf("All WISECore Stopped!\n");
					g_bStop = true;
				}
				usleep(1000000);
			}
		}
	}

	for(i=0; i<cntproc; i++)
	{
		util_process_kill_handle(procList[i]);
	}
#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif

	return 0;
}

