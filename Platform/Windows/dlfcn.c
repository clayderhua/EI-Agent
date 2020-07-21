#include "dlfcn.h"
#include <stdio.h>

WISEPLATFORM_API void * dlopen(const char *filename, int flags)
{
	void* lib = NULL;
	SetErrorMode(SEM_FAILCRITICALERRORS);
	lib = LoadLibrary(filename);
	SetErrorMode((UINT)NULL);
	return lib;
}

WISEPLATFORM_API int dlclose(void *handle)
{
	BOOL bRet = FreeLibrary((HMODULE)handle);
	if(bRet == FALSE)
		return -1;
	else
		return 0;
}

WISEPLATFORM_API void * dlsym(void *handle, const char *name)
{
	return (void*) GetProcAddress( (HMODULE)handle, name );
}

WISEPLATFORM_API char * dlerror(void)
{
	LPSTR lpMsgBuf;
	DWORD dw = GetLastError(); 
	char* result = NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) & lpMsgBuf,
		0, NULL );

	// Display the error message and exit the process
	result = (char*)calloc(strlen(lpMsgBuf)+1, 1);
	sprintf(result,"%s",lpMsgBuf);

	LocalFree(lpMsgBuf);

	return result;
}