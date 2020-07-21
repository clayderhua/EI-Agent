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
#ifdef _WINDOWS
#include <windows.h>
#endif
#include "Service_API.h"
#include "util_libloader.h"

// Global Variable

SVAPI_Interface           g_SVAPI_Interface;
SVAPI_Interface           *pSVAPI_Interface = NULL ;
//-----------------------------------------------------------------------------
// function:
//-----------------------------------------------------------------------------
//

int LoadServiceSDKLib( const char *path, SVAPI_Interface *pAPI_Loader )
{
	if( util_dlopen( path, &pAPI_Loader->Handler ) )  {
		pAPI_Loader->SV_Initialize			          =  ( SV_Initialize_API ) util_dlsym( pAPI_Loader->Handler, "SV_Initialize");
		pAPI_Loader->SV_Uninitialize			  =  ( SV_Uninitialize_API ) util_dlsym( pAPI_Loader->Handler, "SV_Uninitialize");
		pAPI_Loader->SV_GetVersion             =  ( SV_GetVersion_API ) util_dlsym( pAPI_Loader->Handler, "SV_GetVersion");
		pAPI_Loader->SV_GetServiceStatus   =  ( SV_GetServiceStatus_API ) util_dlsym( pAPI_Loader->Handler, "SV_GetServiceStatus");
	    pAPI_Loader->SV_Query_Service        =  ( SV_Query_Service_API ) util_dlsym( pAPI_Loader->Handler, "SV_Query_Service");
        pAPI_Loader->SV_GetCapability         =  ( SV_GetCapability_API ) util_dlsym( pAPI_Loader->Handler, "SV_GetCapability");

        pAPI_Loader->SV_AutoReportStart    =  ( SV_AutoReportStart_API ) util_dlsym( pAPI_Loader->Handler, "SV_AutoReportStart");
        pAPI_Loader->SV_AutoReportStop    =  ( SV_AutoReportStop_API ) util_dlsym( pAPI_Loader->Handler, "SV_AutoReportStop");
        pAPI_Loader->SV_Action                       =  ( SV_Action_API ) util_dlsym( pAPI_Loader->Handler, "SV_Action");
	}else {
		printf("00000000=%s\r\n",path );
		return 0;
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
	} else if( 1 == LoadServiceSDKLib( LibPath, &g_SVAPI_Interface ) ) {
		*pFunInfo = (void*)&g_SVAPI_Interface;
		Ret = 1;
	}
	return Ret;
}