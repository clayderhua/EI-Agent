/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2018/06/07 by Hank Peng									*/
/* Modified Date: 2018/06/07 by HAnk Peng									*/
/* Abstract     : Advantech ElasticSearch API Library   			        */
/* Reference    : None														*/
/****************************************************************************/
#ifndef __ADV_ELSAPI_H__
#define __ADV_ELSAPI_H__

#if defined(WIN32) //windows
#pragma comment(lib, "Winmm.lib")
#define W_LIB_NAME "WAPI_4_advlog.dll"
#include <windows.h>
#include <pthread.h>
#define TaskSleep(x)		Sleep(x)
#else
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#define TaskSleep(x)		usleep(x*1000)
#define W_LIB_NAME "libAdvWebClientAPI_4_advlog.so"
#endif




#ifdef __cplusplus
extern "C" {
#endif


typedef enum 
{	
	// FAILED: < 0
	ELS_CODE_SERVICE_UNABAILABLE     = -503,
	ELS_CODE_FAILED									   = -1,

	// Successful : >= 0
	ELS_CODE_OK											    = 0,	
} ELSAPI_CODE ;



 ELSAPI_CODE  AdvEls_Init( const char* address, int port );
 void AdvEls_Uninit();

 ELSAPI_CODE AdvEls_Write( const char *uri, const char* logContent, char *logResult, int logResultSize );



#endif

#ifdef __cplusplus
}

#endif //__ADV_ELSAPI_H__