#ifndef _UTIL_LIBLOADER_H_
#define _UTIL_LIBLOADER_H_
#include <stdbool.h>
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

WISEPLATFORM_API bool util_dlexist(char* path);
WISEPLATFORM_API bool util_dlopen(char* path, void ** lib);
WISEPLATFORM_API bool util_dlclose(void * lib);
WISEPLATFORM_API char* util_dlerror();
WISEPLATFORM_API void util_dlfree_error(char* err);
WISEPLATFORM_API void* util_dlsym( void * handle, const char *name );

#ifdef __cplusplus
}
#endif

#endif
