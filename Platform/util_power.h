#ifndef _UTIL_POWER_H
#define _UTIL_POWER_H 
#include <stdbool.h>
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

WISEPLATFORM_API bool util_power_off();

WISEPLATFORM_API bool util_power_restart();

WISEPLATFORM_API bool util_power_suspend();

WISEPLATFORM_API bool util_power_hibernate();

WISEPLATFORM_API void util_resume_passwd_disable();

WISEPLATFORM_API bool util_power_suspend_check();

WISEPLATFORM_API bool util_power_hibernate_check();


#ifdef __cplusplus
}
#endif

#endif