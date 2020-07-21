#ifndef _NAMEDPIPE_CLIENT_H_
#define _NAMEDPIPE_CLIENT_H_

#include <stdbool.h>
#include "export.h"

typedef int (* PPIPERECVCB)(char*, int);

typedef struct NamedPipeClient{
#ifdef linux
   int hPipe;
#else
   void *  hPipe;
#endif
   unsigned long commID;
   bool isConnected;
   void * pipeRecvThreadHandle;
   bool   pipeRecvThreadRun;
   PPIPERECVCB pPipeRecvCB;
}NAMEDPIPECLIENT, *PNAMEDPIPECLIENT;

typedef PNAMEDPIPECLIENT PIPECLINETHANDLE;

#ifdef __cplusplus
extern "C" {
#endif

WISEPLATFORM_API PIPECLINETHANDLE NamedPipeClientConnect(char * pName, unsigned long commID, PPIPERECVCB pPipeRecvCB);

WISEPLATFORM_API void NamedPipeClientDisconnect(PIPECLINETHANDLE pipeClientHandle);

WISEPLATFORM_API int NamedPipeClientSend(PIPECLINETHANDLE pipeClientHandle, char * sendData, unsigned long sendToLen);

#ifdef __cplusplus
}
#endif

#endif