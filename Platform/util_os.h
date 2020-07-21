#ifndef _UTIL_OS_H
#define _UTIL_OS_H 
#include <stdbool.h>
#include <stdint.h>
#include "export.h"

#ifndef ULONGLONG
typedef unsigned long long  ULONGLONG;
#endif 


#ifdef __cplusplus
extern "C" {
#endif

	WISEPLATFORM_API bool util_os_get_os_name_reg_x64(char * pOSNameBuf, unsigned long * bufLen);
	WISEPLATFORM_API bool util_os_get_os_name_reg(char * pOSNameBuf, unsigned long * bufLen);
	WISEPLATFORM_API bool util_os_get_os_name(char * pOSNameBuf, unsigned long * bufLen);
	WISEPLATFORM_API bool util_os_get_processor_name(char * pProcessorNameBuf, unsigned long * bufLen);
	WISEPLATFORM_API bool util_os_get_architecture(char * pArchBuf, int *bufLen);
	WISEPLATFORM_API bool util_os_get_free_memory(uint64_t *totalPhysMemKB, uint64_t *availPhysMemKB);
	WISEPLATFORM_API unsigned long long util_os_get_tick_count();

#ifdef __cplusplus
}
#endif

#endif