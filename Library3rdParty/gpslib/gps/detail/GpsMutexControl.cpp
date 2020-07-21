
#include <stdafx.h>

#include <windows.h> 
#include <detail\GpsMutexControl.hpp>
int lockMutex(HANDLE handle)  
{ 
    if (WaitForSingleObject(handle, INFINITE) == WAIT_FAILED) 
	{
	    return false;
//      std::cerr << "no mutex" << std::endl;
//	    ExitThread(0); // should not execute this line
	}
	return 1;
}

int releaseMutex(HANDLE handle) 
{
    // TODO: 在這裡沒有檢查 ReleaseMutex() 是否成功被執行
	ReleaseMutex(handle);

	return 1;
}
