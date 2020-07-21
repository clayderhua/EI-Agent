#include "SUSIAuthenticator.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <stdio.h>
#include <tchar.h>

#pragma comment(lib, "crypt32.lib")
#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)
#define CHECK_SIGN_FILE_NANE TEXT("SUSIPlt.dll")

int SUSIAuthenticator()
{
	int result = -1;
	WCHAR szFileName[MAX_PATH];
	HCERTSTORE hStore = NULL;
	HCRYPTMSG hMsg = NULL;
	PCCERT_CONTEXT pCertContext = NULL;
	BOOL fResult;
	DWORD dwEncoding, dwContentType, dwFormatType;
	PCMSG_SIGNER_INFO pSignerInfo = NULL;
	DWORD dwSignerInfo;
	CERT_INFO CertInfo;

	LPTSTR szName = NULL;
	DWORD dwData;

	__try
	{

		lstrcpynW(szFileName, CHECK_SIGN_FILE_NANE, MAX_PATH);

		// Get message handle and store handle from the signed file.
		//_tprintf(_T("SignedFileInfo <%s>\n"), szFileName);
		fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
			szFileName,
			CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
			CERT_QUERY_FORMAT_FLAG_BINARY,
			0,
			&dwEncoding,
			&dwContentType,
			&dwFormatType,
			&hStore,
			&hMsg,
			NULL);
		if (!fResult)
		{
			_tprintf(_T("File not signed.\nCryptQueryObject failed with %x\n"), GetLastError());
			__leave;
		}

		// Get signer information size.
		fResult = CryptMsgGetParam(hMsg,
			CMSG_SIGNER_INFO_PARAM,
			0,
			NULL,
			&dwSignerInfo);
		if (!fResult)
		{
			_tprintf(_T("CryptMsgGetParam failed with %x\n"), GetLastError());
			__leave;
		}

		// Allocate memory for signer information.
		pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
		if (!pSignerInfo)
		{
			_tprintf(_T("Unable to allocate memory for Signer Info.\n"));
			__leave;
		}

		// Get Signer Information.
		fResult = CryptMsgGetParam(hMsg,
			CMSG_SIGNER_INFO_PARAM,
			0,
			(PVOID)pSignerInfo,
			&dwSignerInfo);
		if (!fResult)
		{
			_tprintf(_T("CryptMsgGetParam failed with %x\n"), GetLastError());
			__leave;
		}

		// Search for the signer certificate in the temporary 
		// certificate store.
		CertInfo.Issuer = pSignerInfo->Issuer;
		CertInfo.SerialNumber = pSignerInfo->SerialNumber;

		pCertContext = CertFindCertificateInStore(hStore,
			ENCODING,
			0,
			CERT_FIND_SUBJECT_CERT,
			(PVOID)&CertInfo,
			NULL);
		if (!pCertContext)
		{
			_tprintf(_T("CertFindCertificateInStore failed with %x\n"),
				GetLastError());
			__leave;
		}

		// Get Subject name size.
		if (!(dwData = CertGetNameString(pCertContext,
			CERT_NAME_SIMPLE_DISPLAY_TYPE,
			0,
			NULL,
			NULL,
			0)))
		{
			_tprintf(_T("CertGetNameString failed.\n"));
			__leave;
		}

		// Allocate memory for subject name.
		szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
		if (!szName)
		{
			_tprintf(_T("Unable to allocate memory for subject name.\n"));
			__leave;
		}

		// Get subject name.
		if (!(CertGetNameString(pCertContext,
			CERT_NAME_SIMPLE_DISPLAY_TYPE,
			0,
			NULL,
			szName,
			dwData)))
		{
			_tprintf(_T("CertGetNameString failed.\n"));
			__leave;
		}
		_tprintf(_T("Subject name = %s\n"), szName);
		if (wcscmp(szName, L"Advantech Co., Ltd.") == 0)
			result = 0;
	}
	__finally
	{
		if (pSignerInfo != NULL) LocalFree(pSignerInfo);
		if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
		if (hStore != NULL) CertCloseStore(hStore, 0);
		if (hMsg != NULL) CryptMsgClose(hMsg);
		if (szName != NULL) LocalFree(szName);
	}
	return result;
}

typedef int(WINAPI *SUSIPlatformCheck)(char*, int*);


int SUSICheckPlatform(char* getStr, int* getStrLen)
{
	int result = -1;
	WCHAR szFileName[MAX_PATH];
	HMODULE ghSusiLib = NULL;
	SUSIPlatformCheck _SUSIPlatformCheck = NULL;

	lstrcpynW(szFileName, CHECK_SIGN_FILE_NANE, MAX_PATH);

	SetErrorMode(SEM_FAILCRITICALERRORS);
	ghSusiLib = ::LoadLibrary(szFileName);
	SetErrorMode((UINT)NULL);

	if (ghSusiLib != NULL)
	{
		_SUSIPlatformCheck = (SUSIPlatformCheck)::GetProcAddress(ghSusiLib, "SUSIPlatformCheck");
		if (_SUSIPlatformCheck == NULL)
		{
			printf("_SUSIPlatformCheck is null\n");
			goto exit;
		}
	}
	else
	{
		printf("Load library fail.\n");
		goto exit;
	}

	result = _SUSIPlatformCheck(getStr, getStrLen);
	if (result == 0)
		printf("Platform = %s\n", getStr);
	else
		printf("_SUSIPlatformCheck fail. error code: 0x%x\n", result);

exit:
	if (ghSusiLib != NULL)
	{
		::FreeLibrary(ghSusiLib);
		ghSusiLib = NULL;
	}
	return result;
}
