#ifndef _RUELENGINE_SENHUB_SDK_H_
#define _RUELENGINE_SENHUB_SDK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <Windows.h>
#ifndef WAPI_API
	#define WAPI_API WINAPI
#endif
#else
	#include <stdlib.h>
	#define WAPI_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (WAPI_API *SenHub_RecvCapability_Cb) ( void* handler, void const * const requestData, unsigned int const requestLen );

typedef void (WAPI_API *SenHub_RecvData_Cb) (void* handler, void const * const requestData, unsigned int const requestLen);

int InitSenHubHandle(SenHub_RecvCapability_Cb recv_capability, SenHub_RecvData_Cb recv_data, void* loghandle);
void UninitSenHubHandle();

#ifdef __cplusplus
}
#endif

#endif /* _RUELENGINE_SENHUB_SDK_H_ */
