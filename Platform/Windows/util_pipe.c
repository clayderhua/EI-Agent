#include "util_pipe.h"

WISEPLATFORM_API bool util_pipe_create(HANDLE * hReadPipe, HANDLE * hWritePipe)
{
	bool bRet = false;
	if(hReadPipe == NULL || hWritePipe == NULL) return bRet;
	{
		SECURITY_ATTRIBUTES sa;
		sa.nLength= sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;
		bRet = CreatePipe(hReadPipe, hWritePipe, &sa,0)?true:false;
	}
	return bRet;
}

WISEPLATFORM_API bool util_pipe_close(HANDLE handle)
{
	return CloseHandle(handle)?true:false;
}

WISEPLATFORM_API bool util_pipe_read(HANDLE handle, char * buf, unsigned int nSizeToRead, unsigned int * nSizeRead)
{
	return ReadFile(handle, buf, nSizeToRead, (LPDWORD)nSizeRead, NULL)?true:false;
}

WISEPLATFORM_API bool util_pipe_write(HANDLE handle, char * buf, unsigned int nSizeToWrite, unsigned int * nSizeWrite)
{
	return WriteFile(handle, buf, nSizeToWrite, (LPDWORD)nSizeWrite, NULL)?true:false;
}

WISEPLATFORM_API bool util_pipe_duplicate(HANDLE srcHandle, HANDLE * trgHandle)
{
	bool bRet = false;
	if(trgHandle == NULL) return bRet;
	{
		HANDLE handle = GetCurrentProcess();
		bRet = DuplicateHandle(handle, srcHandle, handle, trgHandle, 0, TRUE,DUPLICATE_SAME_ACCESS); 
	}
	return bRet; 
}

WISEPLATFORM_API HANDLE util_pipe_PrepAndLaunchRedirectedChild(HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	// Set up the start up info struct.
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = hChildStdOut;
	si.hStdInput  = hChildStdIn;
	si.hStdError  = hChildStdErr;
	// Use this if you want to hide the child:
	//     si.wShowWindow = SW_HIDE;
	// Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
	// use the wShowWindow flags.


	// Launch the process that you want to redirect (in this case,
	// Child.exe). Make sure Child.exe is in the same directory as
	// redirect.c launch redirect from a command line to prevent location
	// confusion.
	if (!CreateProcess(NULL,"Cmd.exe /a",NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi))
		return 0;

	// Close any unnecessary handles.
	if (!CloseHandle(pi.hThread)) return 0;
	// Set global child process handle to cause threads to exit.
	return pi.hProcess;
}