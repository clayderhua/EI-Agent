#ifndef _CAGENT_AMT_CONFIG_H_
#define _CAGENT_AMT_CONFIG_H_
#include "srp/susiaccess_def.h"

#define DEF_CONFIG_FILE_NAME	"agent_config.xml"

typedef struct {
	/*Intel AMT*/
	char amtEn[DEF_ENABLE_LENGTH];
	char amtID[DEF_USER_PASS_LENGTH];
	char amtPwd[DEF_USER_PASS_LENGTH];
}susiaccess_amt_conf_body_t;

#ifdef __cplusplus
extern "C" {
#endif

int amt_load(char const * configFile, susiaccess_amt_conf_body_t * conf);
int amt_save(char const * configFile, susiaccess_amt_conf_body_t const * const conf);
int amt_create(char const * configFile, susiaccess_amt_conf_body_t * conf);
int amt_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen);
int amt_set(char const * const configFile, char const * const itemName, char const * const itemValue);

#ifdef __cplusplus
}
#endif

#endif