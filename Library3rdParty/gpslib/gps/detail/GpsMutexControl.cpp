
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
    // TODO: �b�o�̨S���ˬd ReleaseMutex() �O�_���\�Q����
	ReleaseMutex(handle);

	return 1;
}
