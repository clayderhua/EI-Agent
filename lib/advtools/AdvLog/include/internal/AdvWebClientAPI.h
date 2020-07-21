/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2016/12/14 by Eric Liang									*/
/* Modified Date: 2016/12/14 by Eric Liang									*/
/* Abstract     : Advantech WebClient API Library   				        */
/* Reference    : None														*/
/****************************************************************************/
#ifndef __ADV_WEB_CLIENT_API_H__
#define __ADV_WEB_CLIENT_API_H__

#if defined(WIN32)
	#pragma once
	#ifdef ADVLOG_EXPORTS
		#define ADV_CALL __stdcall
		#define ADV_EXPORT __declspec(dllexport)
	#else
		#define ADV_CALL __stdcall
		#define ADV_EXPORT 
	#endif
	#define __func__ __FUNCTION__

	//#include <AFXSOCK.H>	
#else
	#define ADV_CALL
	#define ADV_EXPORT
	
#endif





#ifdef __cplusplus
extern "C" {
#endif



typedef enum 
{
	WAPI_E_ESTABLISHED,			//
	WAPI_E_CONNECT_ERROR,
	WAPI_E_CLOSE_CONNECTION,
	WAPI_E_RECEIVE
}WEBAPI_EVENT ;


typedef enum
{
	WAPI_ST_INVILID_HANDLE = -1, // -1
	WAPI_ST_IDLE,                //  0
	WAPI_ST_CONNECTING,          //  1
	WAPI_ST_CONNECTED,           //  2
	WAPI_ST_CONNECTION_ERROR,    //  3
	WAPI_ST_LOST_CONNECTION,     //  4
	WAPI_ST_CLOSE_CONNECTION,    //  5
}WEBAPI_SERVICE_STATUS;

typedef enum 
{	
	// FAILED: < 0
	WAPI_CODE_SERVICE_UNABAILABLE = -503,
	WAPI_CODE_NOT_IMPLEMENTED     = -501,
	WAPI_CODE_SERVER_ERROR        = -500,
	WAPI_CODE_LOCKED			  = -426,
	WAPI_CODE_SYNTAX_ERROR	      = -422,
	WAPI_CODE_CONFLICT			  = -409,
	WAPI_CODE_REQUEST_TIMEOUT     = -408,
	WAPI_CODE_NOT_ACCEPTABLE	  = -406,
	WAPI_CODE_METHOD_NOT_ALLOWED  = -405,
	WAPI_CODE_NOT_FOUND	          = -404,
	WAPI_CODE_UNAUTHORIZED        = -401,
	WAPI_CODE_BAD_REQUEST         = -400,
	WAPI_NOT_SET_CALLBACK         = -3,
	WAPI_CODE_CANNOT_CONNECT      = -2,
	WAPI_CODE_FAILED              = -1,
	
	// Successful : >= 0
	WAPI_CODE_OK                  = 0,
	WAPI_CODE_CONNECTING          = 1
}WEBAPI_CODE ;

//typedef int callbackfn
typedef int WebApi_Cb( WEBAPI_EVENT e, void *in, int len, void *user );




// Advantech EIS API-Gateway Service
ADV_EXPORT WEBAPI_CODE ADV_CALL AdvWAPI_Initialize();
ADV_EXPORT void ADV_CALL AdvWAPI_UnInitialize();
ADV_EXPORT void ADV_CALL AdvWAPI_SetCallback( WebApi_Cb fn );

// REST API
ADV_EXPORT WEBAPI_CODE ADV_CALL AdvWAPI_Get( const char *URI, char *buffer, int bufsize );
ADV_EXPORT WEBAPI_CODE ADV_CALL AdvWAPI_Set( const char *URI, char *data, int len, char *buffer, int bufsize );

//ADV_EXPORT WEBAPI_CODE ADV_CALL AdvWAPI_GetAsync( const char *URI, char *buffer, int bufsize , void *user );
//ADV_EXPORT WEBAPI_CODE ADV_CALL AdvWAPI_SetAsync( const char *URI, char *data, int len, char *buffer, int bufsize, void *user );

// Event Service
//ADV_EXPORT WEBAPI_CODE ADV_CALL AdvWAPI_Query_Service( char *buffer, int bufsize );
ADV_EXPORT int         ADV_CALL AdvWAPI_Join_Service( const char *ServiceName , void *user );
ADV_EXPORT WEBAPI_CODE ADV_CALL AdvWAPI_Leave_Service( int handle );
ADV_EXPORT WEBAPI_SERVICE_STATUS ADV_CALL AdvWAPI_GetStatus_Service( int handle );

// Not in EIS Container Service -> Call to other WebService
ADV_EXPORT WEBAPI_CODE ADV_CALL WAPI_Get( const char *address, int port, const char *uri, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
ADV_EXPORT WEBAPI_CODE ADV_CALL WAPI_Get_With_Body( const char *address, int port, const char *uri, const char *data, int len, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
ADV_EXPORT WEBAPI_CODE ADV_CALL WAPI_Put( const char *address, int port, const char *uri, const char *data, int len, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
ADV_EXPORT WEBAPI_CODE ADV_CALL WAPI_Post( const char *address, int port, const char *uri, const char *data, int len, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
ADV_EXPORT WEBAPI_CODE ADV_CALL WAPI_Delete( const char *address, int port, const char *uri, const char *data, int len, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
//ADV_EXPORT WEBAPI_CODE ADV_CALL WAPI_Query_Service(char *address, int port, const char *uri);
ADV_EXPORT int         ADV_CALL WAPI_Join_Service( const char *address, int port, const char *ServiceName , void *user );
ADV_EXPORT WEBAPI_CODE ADV_CALL WAPI_Leave_Service( int handle );
ADV_EXPORT WEBAPI_SERVICE_STATUS ADV_CALL WAPI_GetStatus_Service( int handle );



//-----------------------------------------------------------------------------
// Dynamic Library Function Point 
//----------------------------------------------------------------------------- 

ADV_EXPORT int ADV_CALL GetAPILibFn(const char *LibPath, void **pFunInfo);

typedef WEBAPI_CODE (ADV_CALL *W_AdvWAPI_Initialize)     ( void );
typedef WEBAPI_CODE (ADV_CALL *W_AdvWAPI_UnInitialize)   ( void );
typedef WEBAPI_CODE (ADV_CALL *W_AdvWAPI_SetCallback)    ( WebApi_Cb fn );

// HTTP Functions
typedef WEBAPI_CODE (ADV_CALL *W_AdvWAPI_Get) ( const char *URI, char *buffer, int bufsize );
typedef WEBAPI_CODE (ADV_CALL *W_AdvWAPI_Set) ( const char *URI, char *data, int len, char *buffer, int bufsize );

typedef WEBAPI_CODE (ADV_CALL *W_WAPI_Get) ( const char *address, int port, const char *uri, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
typedef WEBAPI_CODE (ADV_CALL *W_WAPI_Get_With_Body) ( const char *address, int port, const char *uri, const char *data, int len, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
typedef WEBAPI_CODE (ADV_CALL *W_WAPI_Put) ( const char *address, int port, const char *uri, const char *data, int len, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
typedef WEBAPI_CODE (ADV_CALL *W_WAPI_Post) ( const char *address, int port, const char *uri, const char *data, int len, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
typedef WEBAPI_CODE (ADV_CALL *W_WAPI_Delete) ( const char *address, int port, const char *uri, const char *data, int len, char *buffer, int bufsize, char *user_psw, int TimeOut /*sec*/);
typedef int         (ADV_CALL *W_WAPI_Join_Service) ( const char *address, int port, const char *ServiceName , void *user );
typedef WEBAPI_CODE (ADV_CALL *W_WAPI_Leave_Service) ( int handle );
typedef WEBAPI_SERVICE_STATUS (ADV_CALL *W_WAPI_GetStatus_Service) ( int handle );


// Websocket
typedef int         (ADV_CALL *W_AdvWAPI_Join_Service) ( const char *ServiceName , void *user );
typedef WEBAPI_CODE (ADV_CALL *W_AdvWAPI_Leave_Service) ( int handle );
typedef WEBAPI_SERVICE_STATUS (ADV_CALL *W_AdvWAPI_GetStatus_Service) ( int handle );

typedef struct ADVWAPI_INTERFACE
{
	void*						Handler;                              
	W_AdvWAPI_Initialize	    AdvWAPI_Initialize;  
	W_AdvWAPI_UnInitialize		AdvWAPI_UnInitialize;
	W_AdvWAPI_SetCallback       AdvWAPI_SetCallback;
	W_AdvWAPI_Get               AdvWAPI_Get;    
	W_AdvWAPI_Set               AdvWAPI_Set;
	W_AdvWAPI_Join_Service      AdvWAPI_Join_Service;
	W_AdvWAPI_Leave_Service     AdvWAPI_Leave_Service;
	W_AdvWAPI_GetStatus_Service AdvWAPI_GetStatus_Service;
    int                         Workable;
	W_WAPI_Get                  WAPI_Get;
	W_WAPI_Get_With_Body        WAPI_Get_With_Body;
	W_WAPI_Put                  WAPI_Put;
	W_WAPI_Post                 WAPI_Post;
	W_WAPI_Delete               WAPI_Delete;
	W_WAPI_Join_Service         WAPI_Join_Service;
	W_WAPI_Leave_Service        WAPI_Leave_Service;
	W_WAPI_GetStatus_Service    WAPI_GetStatus_Service;

} WAPI_Interface;

extern WAPI_Interface    *g_pWAPI_Interface;




#endif

#ifdef __cplusplus
}
#endif


