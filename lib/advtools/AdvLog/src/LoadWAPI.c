/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/06 by Eric Liang															     */
/* Modified Date: 2015/03/06 by Eric Liang															 */
/* Abstract     : Sensor Network API                       													*/
/* Reference    : None																									 */
/****************************************************************************/
#include <stdio.h>
#ifdef _WINDOWS
#pragma comment(lib, "Winmm.lib")
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include "AdvWebClientAPI.h"

// Global Variable

WAPI_Interface           g_WAPI_Interface;
WAPI_Interface           *g_pWAPI_Interface = NULL ;


//-----------------------------------------------------------------------------
// function:
//-----------------------------------------------------------------------------
//

void* OpenLib(const char* path )
{
#ifdef _WINDOWS
	return LoadLibrary(path); // HMODULE  WINAPI LoadLibrary( LPCTSTR lpFileName );
#else
	return dlopen( path, RTLD_LAZY );
#endif
}

void* GetLibFnAddress( void * handle, const char *name )
{
#ifdef _WINDOWS
	return (void*) GetProcAddress( (HMODULE) handle, name ); // FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR lpProcName ); 
#else
	return dlsym( handle, name );
#endif
}

//-----------------------------------------------------------------------------
// function:
//-----------------------------------------------------------------------------
//
int LoadAPILib( const char *path, WAPI_Interface *pAPI_Loader )
{
	pAPI_Loader->Handler    = OpenLib( path );

	if( pAPI_Loader->Handler )  {
		pAPI_Loader->AdvWAPI_Initialize             = ( W_AdvWAPI_Initialize ) GetLibFnAddress( pAPI_Loader->Handler, "AdvWAPI_Initialize");
		pAPI_Loader->AdvWAPI_UnInitialize        = ( W_AdvWAPI_UnInitialize ) GetLibFnAddress( pAPI_Loader->Handler, "AdvWAPI_UnInitialize");
		pAPI_Loader->AdvWAPI_SetCallback       = ( W_AdvWAPI_SetCallback ) GetLibFnAddress( pAPI_Loader->Handler, "AdvWAPI_SetCallback");

		pAPI_Loader->AdvWAPI_Get                      = ( W_AdvWAPI_Get ) GetLibFnAddress( pAPI_Loader->Handler, "AdvWAPI_Get");
		pAPI_Loader->AdvWAPI_Set                       = ( W_AdvWAPI_Set ) GetLibFnAddress( pAPI_Loader->Handler, "AdvWAPI_Set");

        pAPI_Loader->AdvWAPI_Join_Service      = ( W_AdvWAPI_Join_Service ) GetLibFnAddress( pAPI_Loader->Handler, "AdvWAPI_Join_Service");
		pAPI_Loader->AdvWAPI_Leave_Service   = ( W_AdvWAPI_Leave_Service ) GetLibFnAddress( pAPI_Loader->Handler, "AdvWAPI_Leave_Service");
		pAPI_Loader->AdvWAPI_GetStatus_Service  = ( W_AdvWAPI_GetStatus_Service ) GetLibFnAddress( pAPI_Loader->Handler, "AdvWAPI_GetStatus_Service");

		pAPI_Loader->WAPI_Get                             = ( W_WAPI_Get ) GetLibFnAddress( pAPI_Loader->Handler, "WAPI_Get");
		pAPI_Loader->WAPI_Get_With_Body       = ( W_WAPI_Get_With_Body ) GetLibFnAddress( pAPI_Loader->Handler, "WAPI_Get_With_Body");
		pAPI_Loader->WAPI_Put                             = ( W_WAPI_Put ) GetLibFnAddress( pAPI_Loader->Handler, "WAPI_Put");
		pAPI_Loader->WAPI_Post                           = ( W_WAPI_Post ) GetLibFnAddress( pAPI_Loader->Handler, "WAPI_Post");
		pAPI_Loader->WAPI_Delete                       = ( W_WAPI_Post ) GetLibFnAddress( pAPI_Loader->Handler, "WAPI_Delete");
		pAPI_Loader->WAPI_Join_Service             = ( W_WAPI_Join_Service ) GetLibFnAddress( pAPI_Loader->Handler, "WAPI_Join_Service");
		pAPI_Loader->WAPI_Leave_Service          = ( W_WAPI_Leave_Service ) GetLibFnAddress( pAPI_Loader->Handler, "WAPI_Leave_Service");
		pAPI_Loader->WAPI_GetStatus_Service  = ( W_WAPI_GetStatus_Service ) GetLibFnAddress( pAPI_Loader->Handler, "WAPI_GetStatus_Service");   
	}
	else {
		printf("00000000=%s\r\n",path );
		pAPI_Loader->Workable = 0;		
	}

	if( pAPI_Loader->Handler != NULL && pAPI_Loader->AdvWAPI_Initialize != NULL ) {
		printf("Load Lib=%s Successful\r\n",path );
		pAPI_Loader->Workable = 1;
		return 1;
	}

	return 0;
}


int ADV_CALL GetAPILibFn(const char *LibPath, void **pFunInfo)
{
	int Ret = 0;
	if( g_WAPI_Interface.Workable == 1 ) {
		*pFunInfo = (void*)&g_WAPI_Interface;
		Ret = 1;
	} else if( 1 == LoadAPILib( LibPath, &g_WAPI_Interface ) ) {
		*pFunInfo = (void*)&g_WAPI_Interface;
		Ret = 1;
	}
	return Ret;
}