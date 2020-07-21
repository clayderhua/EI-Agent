#ifndef _DLFCN_H
#define _DLFCN_H 

#include <windows.h>
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RTLD_LAZY 0

WISEPLATFORM_API void* dlopen(const char *filename, int flags);

WISEPLATFORM_API int dlclose(void *handle);

WISEPLATFORM_API void* dlsym(void *handle, const char *name); 

WISEPLATFORM_API char* dlerror(void);

#ifdef __cplusplus
}
#endif


#endif