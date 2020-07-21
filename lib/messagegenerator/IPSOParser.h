#ifndef _IPSO_PARSER_H_
#define _IPSO_PARSER_H_
#include <stdbool.h>
#include "MsgGenerator.h"
#ifdef __cplusplus
extern "C" {
#endif

bool transfer_parse_ipso(const char* data, MSG_CLASSIFY_T *pGroup);

bool transfer_get_ipso_handlername(const char* data, char* handlerName);

bool transfer_get_ipso_opts(const char* data, long long* opts);

#ifdef __cplusplus
}
#endif
#endif