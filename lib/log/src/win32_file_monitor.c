#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "Log.h"

#ifdef __cplusplus
extern "C" {
#endif
void* file_monitor_thread(void *arg);
#ifdef __cplusplus
}
#endif

void* file_monitor_thread(void *arg)
{
	file_monitor_st* fm = (file_monitor_st*) arg;
	DWORD dwWaitStatus;
	HANDLE dwChangeHandles[1];
	char monitorFile[PATH_MAX];
	char *ptr;

	strcpy(monitorFile, fm->file);
	ptr = strrchr(monitorFile, FILE_SEPARATOR);
	if (ptr) {
		*ptr = '\0';
	}
	dwChangeHandles[0] = FindFirstChangeNotification( 
      //"C:\\Users\\terry.lu\\Desktop\\Wise-Agent\\logd", // directory to watch 
	  monitorFile,
      FALSE,                         // do not watch subtree 
      FILE_NOTIFY_CHANGE_LAST_WRITE); // watch file name changes 

	if (dwChangeHandles[0] == INVALID_HANDLE_VALUE || dwChangeHandles[0] == NULL) {
		fprintf(stderr, "ERROR: FindFirstChangeNotification function failed., GetLastError=%d\n", GetLastError());
		return NULL;
	}

	while (fm->running) 
	{
		// Wait for notification.
		//fprintf(stderr, "Waiting for notification...\n");

		dwWaitStatus = WaitForMultipleObjects(1, dwChangeHandles, FALSE, INFINITE); 

		switch (dwWaitStatus) 
		{ 
		case WAIT_OBJECT_0: // file changed
			fm->fun((void*) arg);
			if ( FindNextChangeNotification(dwChangeHandles[0]) == FALSE ) {
				fprintf(stderr, "ERROR: FindNextChangeNotification function failed. GetLastError=%d\n", GetLastError());
				return NULL;
			}
			break;
		case WAIT_OBJECT_0 + 1: // folder changed
		case WAIT_TIMEOUT:
			// ignore
			break;

		default: 
			fprintf(stderr, "ERROR: Unhandled dwWaitStatus. GetLastError=%d\n", GetLastError());
			return NULL;
		}
	}
	
	fprintf(stderr, "end file_monitor_thread\n");
	return NULL;
}
