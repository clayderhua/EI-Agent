#ifndef  _NAMEDPIPE_SERVER_H_
#define  _NAMEDPIPE_SERVER_H_

#include <stdbool.h>
#include "export.h"

typedef bool (* PPIPERECVCB)(char*, int);

#ifdef __cplusplus
extern "C" {
#endif

WISEPLATFORM_API int NamedPipeServerInit(char * pName, unsigned long instCnt);

WISEPLATFORM_API void NamedPipeServerUninit();

WISEPLATFORM_API int IsChannelConnect(unsigned long commID);

WISEPLATFORM_API int NamedPipeServerRegRecvCB(unsigned long commID, PPIPERECVCB pipeRecvCB);

WISEPLATFORM_API int NamedPipeServerSend(unsigned long commID, char * sendData, unsigned long sendToLen);

#ifdef __cplusplus
}
#endif

#endif
