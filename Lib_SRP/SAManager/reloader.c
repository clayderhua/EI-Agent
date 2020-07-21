#include "reloader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include "util_path.h"
#include "util_libloader.h"
#include "SAManagerLog.h"

#ifdef WIN32
#define DEF_RUELENGINE_LIB_NAME	"RuleEngine.dll"
#else
#define DEF_RUELENGINE_LIB_NAME	"libRuleEngine.so"
#endif

void reloader_function_load(RuleEngine_Interface * pRELoader)
{
	if(pRELoader != NULL && pRELoader->Handler != NULL)
	{
		pRELoader->RE_Initialize_API = (RULEENGINE_INITIALIZE)util_dlsym(pRELoader->Handler, "Handler_Initialize");
		pRELoader->RE_Uninitialize_API = (RULEENGINE_UNINITIALIZE)util_dlsym(pRELoader->Handler, "Handler_Uninitialize");
		pRELoader->RE_HandleRecv_API = (RULEENGINE_HANDLERECV)util_dlsym(pRELoader->Handler, "Handler_Recv");
		pRELoader->RE_OnStatusChanges_API = (RULEENGINE_ONSTATUSCHANGE)util_dlsym(pRELoader->Handler, "Handler_OnStatusChange");
		pRELoader->RE_Start_API = (RULEENGINE_START)util_dlsym(pRELoader->Handler, "Handler_Start");
		pRELoader->RE_Stop_API = (RULEENGINE_STOP)util_dlsym(pRELoader->Handler, "Handler_Stop");
		pRELoader->RE_Get_Capability = (RULEENGINE_GET_CAPABILITY) util_dlsym( pRELoader->Handler, "Handler_Get_Capability");
		pRELoader->RE_MemoryFree_API = (RULEENGINE_MEMORYFREE) util_dlsym( pRELoader->Handler, "Handler_MemoryFree");
		pRELoader->RE_Recv_Capability = (RULEENGINE_RECVCAPABILITY)util_dlsym(pRELoader->Handler, "Handler_RecvCapability");
		pRELoader->RE_Recv_Data = (RULEENGINE_RECVDATA)util_dlsym(pRELoader->Handler, "Handler_RecvData");
	}
}

bool reloader_is_exist(const char* path)
{
	char file[MAX_PATH] = {0};
	util_path_combine(file, path, DEF_RUELENGINE_LIB_NAME);
	return util_dlexist(file);
}

bool reloader_load(const char* path, RuleEngine_Interface * pRELoader)
{
	bool bRet = false;
	void * hRULEENGINEDLL = NULL;
	char file[MAX_PATH] = {0};
	if(!pRELoader)
		return bRet;
	util_path_combine(file, path, DEF_RUELENGINE_LIB_NAME);
	if(util_dlopen(file, &hRULEENGINEDLL))
	{
		memset(pRELoader, 0, sizeof(RuleEngine_Interface));
		pRELoader->Handler = hRULEENGINEDLL;
		reloader_function_load(pRELoader);
		bRet = true;
	}
	return bRet;
}

bool reloader_release(RuleEngine_Interface * pRELoader)
{
	bool bRet = true;
	if(pRELoader != NULL)
	{
		if(pRELoader->Handler)
			util_dlclose(pRELoader->Handler);
		pRELoader->Handler = NULL;
	}
	return bRet;
}

char* reloader_get_error()
{
	char *error = util_dlerror();
	return error;
}

void reloader_free_error(char *error)
{
	util_dlfree_error(error);
}

RuleEngine_Interface* reloader_initialize(char const * pWorkdir, Handler_Loader_Interface* pHandlerInfo, void* pLogHandle)
{
	RuleEngine_Interface * pRELoader = NULL;

	if(!pWorkdir)
		return pRELoader;

	if(!pHandlerInfo)
		return pRELoader;

	if(reloader_is_exist(pWorkdir))
	{
		pRELoader = malloc(sizeof(RuleEngine_Interface));
		memset(pRELoader, 0, sizeof(RuleEngine_Interface));
		if(reloader_load(pWorkdir, pRELoader))
		{
			printf("Rule Engine loaded\r\n");
			pRELoader->LogHandle = pLogHandle;
			pHandlerInfo->type = core_handler;

			if(pRELoader->RE_Initialize_API)
			{
				pHandlerInfo->Handler_Init_API = (HANDLER_INITLIZE)pRELoader->RE_Initialize_API;
				pRELoader->RE_Initialize_API(pHandlerInfo->pHandlerInfo);
				strncpy(pHandlerInfo->Name, pHandlerInfo->pHandlerInfo->Name, sizeof(pHandlerInfo->Name));
			}

			if(pRELoader->RE_HandleRecv_API)
				pHandlerInfo->Handler_Recv_API = (HANDLER_RECV)pRELoader->RE_HandleRecv_API;

			if(pRELoader->RE_Start_API)
				pHandlerInfo->Handler_Start_API = (HANDLER_START)pRELoader->RE_Start_API;

			if(pRELoader->RE_Stop_API)
				pHandlerInfo->Handler_Stop_API = (HANDLER_STOP)pRELoader->RE_Stop_API;

			if(pRELoader->RE_OnStatusChanges_API)
				pHandlerInfo->Handler_OnStatusChange_API = (HANDLER_ONSTATUSCAHNGE)pRELoader->RE_OnStatusChanges_API;

			if(pRELoader->RE_Get_Capability)
				pHandlerInfo->Handler_Get_Capability_API = (HANDLER_GET_CAPABILITY)pRELoader->RE_Get_Capability;

			if(pRELoader->RE_MemoryFree_API)
				pHandlerInfo->Handler_MemoryFree_API = (HANDLER_MEMORYFREE)pRELoader->RE_MemoryFree_API;

			pHandlerInfo->Workable = true;
		}
		else
		{
			char *err = reloader_get_error();
			SAManagerLog(pLogHandle, Warning, "Load Rule Engine failed!\r\n  %s!!", err);
			reloader_free_error(err);
			free(pRELoader);
			pRELoader = NULL;
		}
	}
	else
	{
		char *err = reloader_get_error();
		SAManagerLog(pLogHandle, Warning, "Cannot find Rule Engine!\r\n  %s!!", err);
		reloader_free_error(err);
	}

	return pRELoader;
}

void reloader_uninitialize(RuleEngine_Interface * pRELoader)
{
	if(pRELoader)
	{
		if(pRELoader->RE_Uninitialize_API)
			pRELoader->RE_Uninitialize_API();
		reloader_release(pRELoader);
		free(pRELoader);
	}
}

void reloader_capability_recv(RuleEngine_Interface * pRELoader, HANDLE const handler, void const * const requestData, unsigned int const requestLen)
{
	if(pRELoader)
	{
		if(pRELoader->RE_Recv_Capability)
			pRELoader->RE_Recv_Capability(handler, requestData, requestLen);
	}
}

void reloader_data_recv(RuleEngine_Interface * pRELoader, HANDLE const handler, void const * const requestData, unsigned int const requestLen)
{
	if(pRELoader)
	{
		if(pRELoader->RE_Recv_Data)
			pRELoader->RE_Recv_Data(handler, requestData, requestLen);
	}
}