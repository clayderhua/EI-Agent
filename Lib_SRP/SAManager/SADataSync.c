#include "SADataSync.h"
#include "util_path.h"
#include "util_libloader.h"
#include <stdio.h>

#ifdef WIN32
#define DEF_DATASYNC_LIB_NAME	"DataSync.dll"
#else
#define DEF_DATASYNC_LIB_NAME	"libDataSync.so"
#endif

#define DATASYNC_MODULE_NAME	"DataSync"

bool SADataSync_Is_Exist(const char* path)
{
	char file[MAX_PATH] = {0};
	util_path_combine(file, path, DEF_DATASYNC_LIB_NAME);
	return util_dlexist(file);
}

bool SADataSync_Load(SADataSync_Interface* pSADataSync)
{
	if(pSADataSync == NULL && pSADataSync->Handler == NULL) {
		return false;
	}

	pSADataSync->DataSync_Initialize_API = (DATASYNC_INITIALIZE)util_dlsym(pSADataSync->Handler, "DataSync_Initialize");
	pSADataSync->DataSync_Uninitialize_API = (DATASYNC_UNINITIALIZE)util_dlsym(pSADataSync->Handler, "DataSync_Uninitialize");
	pSADataSync->DataSync_SetFuncCB_API = (DATASYNC_SETFUNCCB)util_dlsym(pSADataSync->Handler, "DataSync_SetFuncCB");
	pSADataSync->DataSync_Insert_Cap_API = (DATASYNC_INSERT_CAP)util_dlsym(pSADataSync->Handler, "DataSync_Insert_Cap");
	pSADataSync->DataSync_Insert_Data_API = (DATASYNC_INSERT_DATA)util_dlsym(pSADataSync->Handler, "DataSync_Insert_Data");
	pSADataSync->DataSync_Set_LostTime_API = (DATASYNC_SET_LOSTTIME)util_dlsym(pSADataSync->Handler, "DataSync_Set_LostTime");
	pSADataSync->DataSync_Set_ReConTime_API = (DATASYNC_SET_RECONTIME)util_dlsym(pSADataSync->Handler, "DataSync_Set_ReConTime");

	return true;
}

SADataSync_Interface* SADataSync_Initialize(SALoader_Interface* pSALoader, char const * pWorkdir, Handler_List_t* pLoaderList, void* pLogHandle)
{
	SADataSync_Interface * pSADataSync = NULL;
	char moduleFile[MAX_PATH]={0};

	SAManagerLog(pLogHandle, Normal, "SADataSync_Initialize()\n");

	if(!pWorkdir)
		return pSADataSync;

	if(SADataSync_Is_Exist(pWorkdir))
	{
		Handler_Loader_Interface* pLoader = NULL;
		// load DataSync as mdule
		sprintf(moduleFile, "%s/%s", pWorkdir, DEF_DATASYNC_LIB_NAME);
		if (pSALoader->Loader_LoadHandler_API)
			pSALoader->Loader_LoadHandler_API(pLoaderList, moduleFile, DATASYNC_MODULE_NAME);

		// load DataSync hook function
		// get DataSync loader from list that create by Loader_LoadHandler_API()
		pLoader = pSALoader->Loader_FindHandler_API(pLoaderList, DATASYNC_MODULE_NAME);
		if (!pLoader) {
			SAManagerLog(pLogHandle, Warning, "[DataSync] find [%s] from list fail!\n", DATASYNC_MODULE_NAME);
			return pSADataSync;
		}

		pSADataSync = malloc(sizeof(SADataSync_Interface));
		memset(pSADataSync, 0, sizeof(SADataSync_Interface));
		// copy Handler info
		pSADataSync->Handler = pLoader->Handler;
		if(SADataSync_Load(pSADataSync))
		{
			SAManagerLog(pLogHandle, Normal, "SADataSync loaded\r\n");
			pSADataSync->LogHandle = pLogHandle;
		}
		else
		{
			SAManagerLog(pLogHandle, Warning, "Load SADataSync failed!\r\n");
			free(pSADataSync);
			pSADataSync = NULL;
		}
	}
	else
	{
		SAManagerLog(pLogHandle, Warning, "Cannot find SADataSync!\r\n");
	}

	return pSADataSync;
}

bool SADataSync_Uninitialize(SADataSync_Interface * pSADataSync)
{
	bool bRet = true;
	if(pSADataSync != NULL)
	{
		// handler already close by SALoader.c that trigger by hlloader_handler_release() in SAManager_Uninitialize()
//		if(pSADataSync->Handler)
//			util_dlclose(pSADataSync->Handler);
		pSADataSync->Handler = NULL;
		free(pSADataSync);
		pSADataSync=NULL;
	}
	return bRet;
}