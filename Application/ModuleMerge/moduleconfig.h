#ifndef _CAGENT_MODULE_CONFIG_H_
#define _CAGENT_MODULE_CONFIG_H_
#include <stdbool.h>
#include "srp/susiaccess_def.h"

#define DEF_CONFIG_FILE_NAME	"module_config.xml"

#ifdef __cplusplus
extern "C" {
#endif

bool module_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen);
bool module_set(char const * const configFile, char const * const itemName, char const * const itemValue);

#ifdef __cplusplus
}
#endif

#endif