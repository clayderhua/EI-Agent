/*---------------------------------------------------------------------------*/
//       Author : Scott Chang
//         Mail : scott68.chang@advantech.com.tw
//      License : The MIT License
/*---------------------------------------------------------------------------*/

#ifndef _SIMPLE_SMART_H_
#define _SIMPLE_SMART_H_
#pragma once
#include "windows.h"
#ifdef MAKEDLL
	#define SSMART_CALL __stdcall
	#define SSMART_EXPORT __declspec(dllexport)
#else
	#define SSMART_CALL __stdcall
	#define SSMART_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SSMART_T void*

SSMART_EXPORT SSMART_T SSMART_CALL SSMART_Init();

SSMART_EXPORT void SSMART_CALL SSMART_Uninit(SSMART_T smart);

SSMART_EXPORT int SSMART_GetDeviceCount(SSMART_T smart);

SSMART_EXPORT int SSMART_GetDeviceModelName(SSMART_T smart, int index, TCHAR *pModelNameBuf);

SSMART_EXPORT int SSMART_UpdateSmartInfo(SSMART_T smart, int index);

SSMART_EXPORT int SSMART_GetSmartInfo(SSMART_T smart, int index, BYTE *pBuf);

SSMART_EXPORT int SSMART_GetThreshold(SSMART_T smart, int index, BYTE *pBuf);

SSMART_EXPORT int SSMART_GetFirmwareRevision(SSMART_T smart, int index, TCHAR* pFirmwareRev);

SSMART_EXPORT int SSMART_GetSerialNumber(SSMART_T smart, int index, TCHAR* pSerialNumber);

#ifdef __cplusplus
}
#endif
#endif
