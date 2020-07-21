#ifndef _UTIL_PIPE_H
#define _UTIL_PIPE_H 
#include <stdbool.h>
#include "export.h"

#ifdef ANDROID
	#ifndef HANDLE
	typedef int HANDLE;
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

	WISEPLATFORM_API bool util_pipe_create(HANDLE * hReadPipe, HANDLE * hWritePipe);

	WISEPLATFORM_API bool util_pipe_close(HANDLE handle);

	WISEPLATFORM_API bool util_pipe_read(HANDLE handle, char * buf, unsigned int nSizeToRead, unsigned int * nSizeRead);

	WISEPLATFORM_API bool util_pipe_write(HANDLE handle, char * buf, unsigned int nSizeToWrite, unsigned int * nSizeWrite);

	WISEPLATFORM_API bool util_pipe_duplicate(HANDLE srcHandle, HANDLE * trgHandle);

	WISEPLATFORM_API HANDLE util_pipe_PrepAndLaunchRedirectedChild(HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr);

#ifdef __cplusplus
}
#endif

#endif
