#ifndef _RUELENGINE_SERVICEHANDLE_SDK_H_
#define _RUELENGINE_SERVICEHANDLE_SDK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <Windows.h>
#ifndef SERVICEHANDLE_API
	#define SERVICEHANDLE_API WINAPI
#endif
#else
	#include <stdlib.h>
	#define SERVICEHANDLE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (SERVICEHANDLE_API *ServiceSDK_RecvCapability_Cb) ( void* handler, void const * const requestData, unsigned int const requestLen );

typedef void (SERVICEHANDLE_API *ServiceSDK_RecvData_Cb) (void* handler, void const * const requestData, unsigned int const requestLen);

int InitServiceSDKHandler(ServiceSDK_RecvCapability_Cb recv_capability, ServiceSDK_RecvData_Cb recv_data, void* loghandle);
void UninitServiceSDKHandler();

#ifdef __cplusplus
}
#endif

#endif /* _RUELENGINE_SERVICEHANDLE_SDK_H_ */
