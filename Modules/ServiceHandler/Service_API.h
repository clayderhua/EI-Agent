/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2017/06/28 by Eric Liang															     */
/* Modified Date: 2017/07/02 by Eric Liang															 */
/* Abstract       :  SENSOR NETWORK API     													             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __SERVICE_API_H__
#define __SERVICE_API_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef SVCALL
#define SVCALL __stdcall
#endif

#ifndef EXPORT
#define EXPORT __declspec(dllexport)
#endif

#else
#ifndef SVCALL
#define SVCALL
#endif

#ifndef EXPORT
#define EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif



typedef enum {														
	SV_ER_NOT_IMPLEMENT				    = -13,		  /*	Does Not Support this command		(501)	*/
	SV_ER_TIMEOUT								    = -12,       /*	Request Timeout									(408)	*/
	SV_ER_SYS_BUSY								= -11,       /*	System is busy										(503)		*/
	SV_ER_VALUE_OUT_OF_RNAGE	    = -10,       /*	Value is out of range							(416)		*/
	SV_ER_SYNTAX_ERROR					    =   -9,		  /*	Format is correct but syntax error		(422)	*/
	SV_ER_FORMAT_ERROR					=   -8,	     /*	Format error											(415)		*/
	SV_ER_REQUEST_ERROR					=   -7,	     /*	Request error										(400)		*/
	SV_ER_RESOURCE_LOSE					=   -6,		/*		SenHub disconnect								(410)	*/
	SV_ER_RESOURCE_LOCKED			    =   -5,		/* 	Resource is in setting							(426)		*/
	SV_ER_NOT_FOUND						    =   -4,		/*		Resource Not Found							(404)		*/
	SV_ER_WRITE_ONLY						    =   -3,		/*		Read Only												(405)		*/
	SV_ER_READ_ONLY							=   -2,		/*		Write Only												(405)	*/
	SV_ER_FAILED									    =   -1,		/*		Failed														(500)		*/
	SV_OK												    =    0,		/*		Success													(200)		*/
	SV_INITILIZED									    =    1,		/*		Library had initilized											*/
} SV_CODE;

typedef enum 
{
	SV_E_LeaveServiceSystem            = 0,  // 0. Disconnected from the Service System
	SV_E_JoinServiceSystem                = 1,  // 1. Connected to the Service System
	SV_E_RegisterService                      = 2, // 2. Registed a new Service ( ex: HDD_PMQ Plugin )
	SV_E_DeregisterService                  = 3, // 3. Deregisted a Service
	SV_E_UpdateServiceCapability     = 4,  // 4. Update Service's Capability
	SV_E_UpdateData                            = 5,  // 5. Update Service's data value
	SV_E_ActionResult                            = 6, // 6. Reply Message of Get / Set
	SV_E_EventNotify                              = 7, // 7. EventNotify
}SV_EVENT ;


typedef enum
{
	SV_UNINIT   =  0,
	SV_INIT			=  1,
	SV_JOINED   =  2,
	SV_LEAVED	=  3,
}SV_STATUS;

//-----------------------------------------------------------------------------
//  Callback Function 
//-----------------------------------------------------------------------------
//typedef int callbackfn
typedef SV_CODE (SVCALL *Service_Cb) ( SV_EVENT e, char *ServiceName, void *inData, int dataLen, void *UserData );



//-----------------------------------------------------------------------------
// Sensor Netowrk Function Define
//-----------------------------------------------------------------------------
SV_CODE SVCALL SV_Initialize( Service_Cb fn, void *pInUserData );

SV_CODE SVCALL SV_Uninitialize( );  

SV_CODE SVCALL SV_GetVersion( char *outVersion, int bufSize ); 

SV_STATUS SVCALL SV_GetServiceStatus( );

SV_CODE SVCALL SV_Query_Service( char *outBuf, int bufSize  );

SV_CODE SVCALL SV_GetCapability( const char *ServiceName, char *outBuffer,  int bufSize  );

SV_CODE SVCALL SV_AutoReportStart( const char *ServiceName , char *inData, int inDataLen );

SV_CODE SVCALL SV_AutoReportStop( const char *ServiceName, char *inData, int inDataLen );

SV_CODE SVCALL SV_Action( const char *ServiceName , char *szAction,  void *pUserData );

SV_CODE SVCALL SV_ReSyncData( const char *ServiceName );

//SV_CODE SVCALL SV_SetCallback( Service_Cb fn );

//-----------------------------------------------------------------------------
// Dynamic Library Function Point 
//-----------------------------------------------------------------------------

EXPORT int SVCALL GetServiceSDKLibFn(const char *LibPath, void **pFunInfo);

int GetSNAPILibFn( const char *LibPath, void **pFunInfo );

typedef SV_CODE(SVCALL *SV_Initialize_API )							( Service_Cb fn, void *pInUserData );
typedef SV_CODE(SVCALL *SV_Uninitialize_API )						( );
typedef SV_CODE(SVCALL *SV_GetVersion_API)						( char *outVersion, int bufSize );
typedef SV_STATUS(SVCALL *SV_GetServiceStatus_API) 		(  );
typedef SV_CODE(SVCALL *SV_Query_Service_API)					( char *outBuf, int bufSize  );
typedef SV_CODE(SVCALL *SV_GetCapability_API)					( const char *ServiceName, char *outBuffer,  int bufSize );
typedef SV_CODE(SVCALL *SV_AutoReportStart_API)				( const char *ServiceName , char *inData, int inDataLen  );
typedef SV_CODE(SVCALL *SV_AutoReportStop_API)				( const char *ServiceName, char *inData, int inDataLen );
typedef SV_CODE(SVCALL *SV_Action_API)								( const char *ServiceName , char *szAction,  void *pUserData );
typedef SV_CODE(SVCALL *SV_ReSyncData_API)                      ( const char *ServiceName );

//typedef SV_CODE(SVCALL *SV_SetCallback_API)						( Service_Cb pCbfn );

typedef struct SVAPI_INTERFACE
{
	void*																			Handler;                              
	SV_Initialize_API												 SV_Initialize;  
	SV_Uninitialize_API									SV_Uninitialize;
	SV_GetVersion_API									SV_GetVersion;
    //SV_SetCallback_API									SV_SetCallback;
	SV_GetServiceStatus_API			SV_GetServiceStatus;
	SV_Query_Service_API							SV_Query_Service;
	SV_GetCapability_API						     SV_GetCapability;
	SV_AutoReportStart_API					SV_AutoReportStart;
	SV_AutoReportStop_API					SV_AutoReportStop;
	SV_Action_API													      SV_Action;
	SV_ReSyncData_API	                                SV_ReSyncData;               
	int    																	      Workable;
}SVAPI_Interface;

extern SVAPI_Interface    *pSVAPI_Interface;
#ifdef __cplusplus
}
#endif

#endif // __SERVICE_API_H__


