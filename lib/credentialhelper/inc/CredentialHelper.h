/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/9/27 by Scott Chang									*/
/* Modified Date: 2017/9/27 by Scott Chang									*/
/* Abstract     :  															*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef _CREDENTIAL_HELPER_H_
#define _CREDENTIAL_HELPER_H_

#pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <windows.h>
#ifndef CREDENTIAL_HELPER_API
	#define CREDENTIAL_HELPER_CALL __stdcall
	#define CREDENTIAL_HELPER_API __declspec(dllexport)
#endif
#else
	#define CREDENTIAL_HELPER_CALL
	#define CREDENTIAL_HELPER_API
#endif

#ifdef  __cplusplus
extern "C" {
#endif



CREDENTIAL_HELPER_API char* Cred_GetCredentialWithDevId(char* url, char* auth, char *deviceId, char** response);
#define Cred_GetCredential(url, auth) Cred_GetCredentialWithDevId(url, auth, "", NULL)

CREDENTIAL_HELPER_API char* Cred_ParseCredential(char* credential, char* path);

CREDENTIAL_HELPER_API char* Cred_ZeroConfig(char *deviceId);

CREDENTIAL_HELPER_API void Cred_FreeBuffer(char* buf);

#ifdef  __cplusplus
}
#endif

#endif
