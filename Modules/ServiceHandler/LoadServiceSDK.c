/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/06 by Eric Liang															     */
/* Modified Date: 2015/03/06 by Eric Liang															 */
/* Abstract     : Sensor Network API                       													*/
/* Reference    : None																									 */
/****************************************************************************/
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include "Service_API.h"

// Global Variable

SVAPI_Interface           g_SVAPI_Interface;
SVAPI_Interface           *pSVAPI_Interface = NULL ;
//-----------------------------------------------------------------------------
// function:
//-----------------------------------------------------------------------------
//
void* OpenLib(char* path )
{
#ifdef _WINDOWS
	return LoadLibrary(path); // HMODULE  WINAPI LoadLibrary( LPCTSTR lpFileName );
#else
	return dlopen( path, RTLD_LAZY );
#endif
}

void* GetLibFnAddress( void * handle, char *name )
{
#ifdef _WINDOWS
	return (void*) GetProcAddress( handle, name ); // FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR lpProcName ); 
#else
	return dlsym( handle, name );
#endif
}

int LoadAPILib( const char *path, SVAPI_Interface *pAPI_Loader )
{
	char modulepath[512]={0};
	pAPI_Loader->Handler    = OpenLib( (char*)path );

	if(pAPI_Loader->Handler == NULL )
	{
#ifdef _WINDOWS
		sprintf(modulepath, "module\\%s",path);
#else
		sprintf(modulepath, "module/%s",path);
#endif
		pAPI_Loader->Handler = OpenLib(modulepath);
	}

	if( pAPI_Loader->Handler )  {
		pAPI_Loader->SV_Initialize			          =  ( SV_Initialize_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_Initialize");
		pAPI_Loader->SV_Uninitialize			  =  ( SV_Uninitialize_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_Uninitialize");
		pAPI_Loader->SV_GetVersion             =  ( SV_GetVersion_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_GetVersion");
		pAPI_Loader->SV_GetServiceStatus   =  ( SV_GetServiceStatus_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_GetServiceStatus");
	    pAPI_Loader->SV_Query_Service        =  ( SV_Query_Service_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_Query_Service");
        pAPI_Loader->SV_GetCapability         =  ( SV_GetCapability_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_GetCapability");

        pAPI_Loader->SV_AutoReportStart    =  ( SV_AutoReportStart_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_AutoReportStart");
        pAPI_Loader->SV_AutoReportStop    =  ( SV_AutoReportStop_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_AutoReportStop");
        pAPI_Loader->SV_Action                       =  ( SV_Action_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_Action");
		pAPI_Loader->SV_ReSyncData             =  ( SV_ReSyncData_API ) GetLibFnAddress( pAPI_Loader->Handler, "SV_ReSyncData");
	}else {
		printf("00000000=%s\r\n",path );
	}

	if( pAPI_Loader->Handler != NULL && pAPI_Loader->SV_Initialize != NULL ) {
		printf("Load Lib=%s Successful\r\n",path );
		pAPI_Loader->Workable = 1;
		return 1;
	}

	return 0;
}


int SVCALL GetServiceSDKLibFn(const char *LibPath, void **pFunInfo)
{
	int Ret = 0;
	if( g_SVAPI_Interface.Workable == 1 ) {
		*pFunInfo = (void*)&g_SVAPI_Interface;
		Ret = 1;
	} else if( 1 == LoadAPILib( LibPath, &g_SVAPI_Interface ) ) {
		*pFunInfo = (void*)&g_SVAPI_Interface;
		Ret = 1;
	}
	return Ret;
}
