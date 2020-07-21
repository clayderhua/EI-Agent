#include "SAWatchdog.h"
#include "srp/susiaccess_def.h"
#include "version.h"
#include "service.h"
#include "WatchdogL.h"
#include "Log.h"
#include <stdio.h>
#include <string.h>
#include "util_path.h"
#include "WISEPlatform.h"

LOGHANDLE g_WDLogHandle = NULL;
#define DEF_SRVC_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
#define wdLog(objLoghandle, level, fmt, ...)  do { if (objLoghandle != NULL)   \
   WriteIndividualLog(objLoghandle, "watchdog", DEF_SRVC_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)

int SAWatchdogSrvcStart()
{
   return StartSAWatchdog(g_WDLogHandle)?0:-1;
}

int SAWatchdogSrvcStop()
{
   return StopSAWatchdog()?0:-1;
}

int main(int argc, char *argv[])
{
	bool isSrvcInit;
	char version[DEF_VERSION_LENGTH] = {0};
	char moudlePath[MAX_PATH] = {0};
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif  

	sprintf(version, "%d.%d.%d.%d",VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);

	memset(moudlePath, 0 , sizeof(moudlePath));
	
	util_module_path_get(moudlePath);
	
	/*Enable Log*/
	g_WDLogHandle = InitLog(moudlePath);

	/*Load service name fomr config and init service*/
	isSrvcInit = ServiceInit("SAWatchdog", version, SAWatchdogSrvcStart, SAWatchdogSrvcStop, g_WDLogHandle)==0?true:false;
		
	if(!isSrvcInit) return -1;

	if(argv[1] != NULL)
	{
		char cmdChr[MAX_CMD_LEN] = {'\0'};
		char *token = NULL;
		memcpy(cmdChr, argv[1], strlen(argv[1]));
		do 
		{
			if(ExecuteCmd(strtok_r(cmdChr, "\n", &token))!=0) break;
			memset(cmdChr, 0, sizeof(cmdChr));
			fgets(cmdChr, sizeof(cmdChr), stdin);			
		} while (true);
	}
	else
	{
		if(isSrvcInit)
		{
			LaunchService();
		}
	}
	if(isSrvcInit) ServiceUninit();

	if(g_WDLogHandle!=NULL)
	{
		UninitLog(g_WDLogHandle);
		g_WDLogHandle = NULL;
	}

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif
	return 0;
}
