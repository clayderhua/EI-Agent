#ifndef _UTIL_STRING_H
#define _UTIL_STRING_H 

#include <stdlib.h>
#include <stdbool.h>
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif
WISEPLATFORM_API void TrimStr(char * str);

WISEPLATFORM_API wchar_t * ANSIToUnicode(const char * str);

WISEPLATFORM_API char * UnicodeToANSI(const wchar_t * str);

WISEPLATFORM_API wchar_t * UTF8ToUnicode(const char * str);

WISEPLATFORM_API char * UnicodeToUTF8(const wchar_t * str);

WISEPLATFORM_API char * ANSIToUTF8(const char * str);

WISEPLATFORM_API char * UTF8ToANSI(const char * str);

WISEPLATFORM_API bool IsUTF8(const char * string);

WISEPLATFORM_API bool GetRandomStr(char *randomStrBuf, int bufSize);

WISEPLATFORM_API char* StringReplace(const char *orig, const char *rep, const char *with);

WISEPLATFORM_API void StringFree(char *str);

#ifdef __cplusplus
}
#endif

#endif
