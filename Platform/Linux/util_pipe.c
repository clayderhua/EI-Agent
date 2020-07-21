#include "util_pipe.h"
#include <unistd.h>
#include <fcntl.h>

bool util_pipe_create(HANDLE * hReadPipe, HANDLE * hWritePipe)
{
	bool bRet = false;
	if(hReadPipe == NULL || hWritePipe == NULL) return bRet;
	{
		int fds[2];
		if(pipe(fds) == 0)
		{
			int flags = 0;
			flags = fcntl(fds[0], F_GETFL);
			fcntl(fds[0], F_SETFL,flags | O_NONBLOCK);
			fcntl(fds[1], F_SETFL,flags | O_NONBLOCK);
			*hReadPipe = (HANDLE)fds[0];
			*hWritePipe = (HANDLE)fds[1];
			bRet = true;
		}
	}
	return bRet;
}

bool util_pipe_close(HANDLE handle)
{
	bool bRet = false;
	if(handle == NULL) return bRet;
	if(close(handle) == 0)
	{
		bRet = true;
	}
	return bRet;
}

bool util_pipe_read(HANDLE handle, char * buf, unsigned int nSizeToRead, unsigned int * nSizeRead)
{
	bool bRet = false;
	if(handle == NULL || buf == NULL || nSizeRead == NULL) return bRet;
	{
		ssize_t tmpReadRet = 0;
		tmpReadRet = read(handle, buf, nSizeToRead);
		if(tmpReadRet != -1)
		{
			if(tmpReadRet>0)
			{
				*nSizeRead = tmpReadRet;
			}
			bRet = true;
		}
	}
	return bRet;
}

bool util_pipe_write(HANDLE handle, char * buf, unsigned int nSizeToWrite, unsigned int * nSizeWrite)
{
	bool bRet = false;
	if(handle == NULL || buf == NULL || nSizeWrite == NULL) return bRet;
	{
		ssize_t tmpWriteRet = 0;
		tmpWriteRet = write(handle, buf, nSizeToWrite);
		if(tmpWriteRet != -1)
		{
			if(tmpWriteRet>0)
			{
				*nSizeWrite = tmpWriteRet;
			}
			bRet = true;
		}
	}
	return bRet;
}

bool util_pipe_duplicate(HANDLE srcHandle, HANDLE * trgHandle)
{
	bool bRet = false;
	if(trgHandle == NULL) return bRet;
	{
		HANDLE newHandle;
		newHandle = dup(srcHandle);
		if(newHandle != -1)
		{
			int flags = 0;
			flags = fcntl(newHandle, F_GETFL);
			fcntl(newHandle, F_SETFL,flags | O_NONBLOCK);
			*trgHandle = newHandle;
			bRet = true;
		}
	}
	return bRet;
}

 HANDLE util_pipe_PrepAndLaunchRedirectedChild(HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr)
 {
	HANDLE hRet = NULL;
	pid_t pid;
	pid = fork();
	if(pid == 0)
	{
		dup2(hChildStdIn, STDIN_FILENO);
		dup2(hChildStdOut, STDOUT_FILENO);
		dup2(hChildStdErr, STDERR_FILENO);
#ifdef ANDROID
		execlp("/system/bin/sh", "sh", NULL);
#else
		execlp("/bin/sh", "sh", NULL);
#endif
	}
	else if(pid > 0)
	{
		hRet = pid;
	}
	return hRet;
 }
