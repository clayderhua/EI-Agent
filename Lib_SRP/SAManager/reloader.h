#ifndef _RE_LOADER_H_
#define _RE_LOADER_H_
#include "srp/susiaccess_def.h"
#include "srp/susiaccess_handler_mgmt.h"
#include <stdbool.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <Windows.h>
#ifndef RELOADER_API
#define RELOADER_API WINAPI
#endif
#else
#define RELOADER_API
#endif

typedef int (RELOADER_API *RULEENGINE_INITIALIZE)(HANDLER_INFO *pluginfo);
typedef void (RELOADER_API *RULEENGINE_UNINITIALIZE)(void);
typedef void (RELOADER_API *RULEENGINE_HANDLERECV)( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2 );
typedef void (RELOADER_API *RULEENGINE_ONSTATUSCHANGE)(HANDLER_INFO *pluginfo);
typedef handler_result (HANDLER_API *RULEENGINE_GET_CAPABILITY) ( char ** pOutReply );
typedef void (RELOADER_API *RULEENGINE_START)(void);
typedef void (RELOADER_API *RULEENGINE_STOP)(void);
typedef void (RELOADER_API *RULEENGINE_MEMORYFREE)(char *pInData);
typedef void (RELOADER_API *RULEENGINE_RECVCAPABILITY)(HANDLE const handler, void const * const requestData, unsigned int const requestLen);
typedef void (RELOADER_API *RULEENGINE_RECVDATA)(HANDLE const handler, void const * const requestData, unsigned int const requestLen);

typedef struct RUELENGINE_INTERFACE
{
	void*						Handler;               // handle of to load so library
	RULEENGINE_INITIALIZE		RE_Initialize_API;
	RULEENGINE_UNINITIALIZE		RE_Uninitialize_API;
	RULEENGINE_HANDLERECV		RE_HandleRecv_API;
	RULEENGINE_ONSTATUSCHANGE	RE_OnStatusChanges_API;
	RULEENGINE_GET_CAPABILITY	RE_Get_Capability;	
	RULEENGINE_START			RE_Start_API;
	RULEENGINE_STOP				RE_Stop_API;
	RULEENGINE_MEMORYFREE		RE_MemoryFree_API;
	RULEENGINE_RECVCAPABILITY	RE_Recv_Capability;
	RULEENGINE_RECVDATA			RE_Recv_Data;

	void*						LogHandle;
} RuleEngine_Interface;
#ifdef __cplusplus
extern "C" {
#endif

bool reloader_is_exist(const char* path);
bool reloader_load(const char* path, RuleEngine_Interface * pRELoader);
bool reloader_release(RuleEngine_Interface * pRELoader);
char* reloader_get_error();
void reloader_free_error(char *error);

RuleEngine_Interface* reloader_initialize(char const * pWorkdir, Handler_Loader_Interface* pHandlerInfo, void* ruleengine);
void reloader_uninitialize(RuleEngine_Interface * pRELoader);
void reloader_capability_recv(RuleEngine_Interface * pRELoader, HANDLE const handler, void const * const requestData, unsigned int const requestLen);
void reloader_data_recv(RuleEngine_Interface * pRELoader, HANDLE const handler, void const * const requestData, unsigned int const requestLen);

#ifdef __cplusplus
}
#endif

#endif
