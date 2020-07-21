#include "util_process.h"
#include <Windows.h>
#include <process.h>
#include <string.h>
#include <stdio.h>

#include <psapi.h>
#pragma comment(lib, "Psapi.lib")

#define DIV                       (1024)

WISEPLATFORM_API bool util_process_launch(char *appPath)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if (NULL == appPath)
		return -1;

	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;

	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(appPath, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		return true;
	}
	else
	{
		return false;
	}
}

WISEPLATFORM_API HANDLE util_process_cmd_launch(char *cmdline)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	if (cmdline == NULL)
		return NULL;

	memset(&si, 0, sizeof(si));
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(NULL, (char *)cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		//CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	return pi.hProcess;
}

WISEPLATFORM_API HANDLE util_process_cmd_launch_no_wait(char *cmdline)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	if (cmdline == NULL)
		return NULL;
	memset(&si, 0, sizeof(si));
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	if (CreateProcess(NULL, (char *)cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		return pi.hProcess;
	}
	else
	{
		return NULL;
	}
}

WISEPLATFORM_API bool util_is_process_running(HANDLE hProcess)
{
	unsigned long long ret = 0;
	if (hProcess == NULL)
		return false;

	ret = WaitForSingleObject(hProcess, 0);
	return ret == WAIT_TIMEOUT;
}

WISEPLATFORM_API void util_process_wait_handle(HANDLE hProcess)
{
	if (hProcess == NULL)
		return;
	WaitForSingleObject(hProcess, INFINITE);
	CloseHandle(hProcess);
}

WISEPLATFORM_API void util_process_kill_handle(HANDLE hProcess)
{
	if (hProcess == NULL)
		return;
	TerminateProcess(hProcess, 0); //asynchronous
	WaitForSingleObject(hProcess, 5000);
	CloseHandle(hProcess);
}

bool GetTokenByName(HANDLE* hToken, char* prcName)
{
	bool bRet = false;
	HANDLE hPrc = NULL;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = NULL;
	if (NULL == prcName || NULL == hToken)
		return bRet;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
		return bRet;
	while (true)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == false)
			break;
		if (_stricmp(pe.szExeFile, prcName) == 0)
		{
			hPrc = OpenProcess(PROCESS_QUERY_INFORMATION, false, pe.th32ProcessID);
			bRet = OpenProcessToken(hPrc, TOKEN_ALL_ACCESS, hToken);
			CloseHandle(hPrc);
			break;
		}
	}
	if (hSnapshot)
		CloseHandle(hSnapshot);
	return bRet;
}

bool GetLastLogonName(char* pNameBuf, unsigned long* bufLen)
{
	bool bRet = false;
	HKEY hk;
	char regName[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI";
	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_READ | KEY_WOW64_64KEY, &hk)) return false;
	else
	{
		char valueName[] = "LastLoggedOnSAMUser";
		char valueData[256] = { 0 };
		unsigned long  valueDataSize = sizeof(valueData);
		if (ERROR_SUCCESS != RegQueryValueEx(hk, valueName, 0, NULL, (LPBYTE)valueData, &valueDataSize))
		{
			RegCloseKey(hk);
			printf("GetLastLogonName Error %u\n", GetLastError());
			return false;
		}
		RegCloseKey(hk);
		bRet = valueDataSize == 0 ? false : true;
		if (bRet)
		{
			//unsigned int cpyLen = valueDataSize < *bufLen ? valueDataSize : *bufLen; 
			if (pNameBuf)
				memcpy(pNameBuf, valueData, valueDataSize);
			if (bufLen)
				*bufLen = valueDataSize;
		}
	}
	return bRet;
}

bool GetLogonFromToken(HANDLE hToken, char* strUser)
{
	unsigned long long dwSize = 256;
	bool bSuccess = false;
	unsigned long long dwLength = 0;

	SID_NAME_USE SidType;
	char lpName[256];
	char lpDomain[256];

	PTOKEN_USER ptu = NULL;
	//Verify the parameter passed in is not NULL.
	if (NULL == hToken)
		goto Cleanup;

	if (!GetTokenInformation(
		hToken,         // handle to the access token
		TokenUser,    // get information about the token's groups 
		NULL,   // pointer to PTOKEN_USER buffer
		dwLength,              // size of buffer
		&dwLength       // receives required buffer size
	))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			printf("GetTokenInformation Error %u\n", GetLastError());
			goto Cleanup;
		}
			

		ptu = (PTOKEN_USER)HeapAlloc(GetProcessHeap(),
			HEAP_ZERO_MEMORY, dwLength);

		if (ptu == NULL)
		{
			printf("HeapAlloc Error %u\n", GetLastError());
			goto Cleanup;
		}
	}

	if (!GetTokenInformation(
		hToken,         // handle to the access token
		TokenUser,    // get information about the token's groups 
		(LPVOID)ptu,   // pointer to PTOKEN_USER buffer
		dwLength,       // size of buffer
		&dwLength       // receives required buffer size
	))
	{
		printf("GetTokenInformation Error %u\n", GetLastError());
		goto Cleanup;
	}

	if (!LookupAccountSid(NULL, ptu->User.Sid, lpName, &dwSize, lpDomain, &dwSize, &SidType))
	{
		DWORD dwResult = GetLastError();
		if (dwResult == ERROR_NONE_MAPPED)
			strcpy(lpName, "NONE_MAPPED");
		else
		{
			printf("LookupAccountSid Error %u\n", GetLastError());
		}
	}
	else
	{
		printf("Current user is  %s\\%s\n",lpDomain, lpName);
		sprintf(strUser, "%s\\%s", lpDomain, lpName);
		bSuccess = true;
	}

Cleanup:

	if (ptu != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)ptu);
	return bSuccess;
}

bool GetTokenByNameWithLogon(HANDLE *hToken, char *prcName)
{
	bool bRet = false;
	HANDLE hPrc = NULL;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = NULL;
	char userName[256] = { 0 };

	if (NULL == prcName || NULL == hToken)
		return bRet;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
		return bRet;

	if(!GetLastLogonName(userName, NULL))
		return bRet;
	while (true)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == false)
			break;
		if (_stricmp(pe.szExeFile, prcName) == 0)
		{
			char strUser[256] = { 0 };
			hPrc = OpenProcess(PROCESS_QUERY_INFORMATION, false, pe.th32ProcessID);
			bRet = OpenProcessToken(hPrc, TOKEN_ALL_ACCESS, hToken);
			GetLogonFromToken(*hToken, strUser);
			CloseHandle(hPrc);
			if(_stricmp(strUser, userName) == 0)
				break;
			else
			{
				printf("Current user \"%s\" is different with \"%s\"(Reg)", strUser, userName);
			}
		}
	}
	if (hSnapshot)
		CloseHandle(hSnapshot);
	return bRet;
}

WISEPLATFORM_API bool util_process_as_user_launch(char *cmdLine, bool isAppNameRun, bool isShowWindow, unsigned long *newPrcID)
{
	bool bRet = false;
	if (NULL == cmdLine)
		return bRet;
	{
		HANDLE hToken;
		//if (!GetTokenByName(&hToken, "EXPLORER.EXE"))
		if (!GetTokenByNameWithLogon(&hToken, "EXPLORER.EXE"))
		{
			return bRet;
		}
		else
		{
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			DWORD dwCreateFlag = CREATE_NO_WINDOW;
			memset(&si, 0, sizeof(si));
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			if (isShowWindow)
			{
				si.wShowWindow = SW_SHOW;
				dwCreateFlag = CREATE_NEW_CONSOLE;
			}
			si.cb = sizeof(si);
			memset(&pi, 0, sizeof(pi));
			if (isAppNameRun)
			{
				bRet = CreateProcessAsUserA(hToken, cmdLine, NULL, NULL, NULL,
											false, dwCreateFlag, NULL, NULL, &si, &pi);
			}
			else
			{
				bRet = CreateProcessAsUserA(hToken, NULL, cmdLine, NULL, NULL,
											false, dwCreateFlag, NULL, NULL, &si, &pi);
			}

			if (!bRet)
			{
				printf("error code: %s  %d\n", cmdLine, GetLastError());
			}
			else
			{
				if (newPrcID != NULL)
					*newPrcID = pi.dwProcessId;
			}
			WaitForSingleObject(hToken, INFINITE);
			CloseHandle(hToken);
		}
	}
	return bRet;
}

bool adjust_privileges()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	TOKEN_PRIVILEGES oldtp;
	DWORD dwSize = sizeof(TOKEN_PRIVILEGES);
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
			return true;
		else
			return false;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
	{
		CloseHandle(hToken);
		return false;
	}

	memset(&tp, 0, sizeof(tp));
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize))
	{
		CloseHandle(hToken);
		return false;
	}

	CloseHandle(hToken);
	return true;
}

WISEPLATFORM_API bool util_process_kill(char *processName)
{
	bool bRet = false;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = NULL;
	if (NULL == processName)
		return bRet;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
	{
		CloseHandle(hSnapshot);
		return bRet;
	}

	while (true)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == false)
			break;
		if (strcmp(pe.szExeFile, processName) == 0)
		{
			HANDLE hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
			if (hPrc == NULL)
			{
				DWORD dwRet = GetLastError();
				if (dwRet == 5)
				{
					if (adjust_privileges())
					{
						hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
					}
				}
			}


			if (hPrc)
			{
				TerminateProcess(hPrc, 0); //asynchronous
				WaitForSingleObject(hPrc, 5000);
				bRet = true;
				CloseHandle(hPrc);
			}

			break;
		}
	}
	CloseHandle(hSnapshot);
	return bRet;
}

WISEPLATFORM_API int util_process_id_get(void)
{
	return _getpid();
}

WISEPLATFORM_API bool util_process_username_get(HANDLE hProcess, char *userNameBuf, int bufLen)
{
	bool bRet = false;
	if (hProcess == NULL || userNameBuf == NULL || bufLen <= 0)
		return bRet;
	{
		HANDLE hToken;
		if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
		{
			DWORD dwNeedLen = 0;
			GetTokenInformation(hToken, TokenUser, NULL, 0, &dwNeedLen);
			if (dwNeedLen > 0)
			{
				TOKEN_USER *pTokenUser = (TOKEN_USER *)malloc(dwNeedLen);
				if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwNeedLen, &dwNeedLen))
				{
					SID_NAME_USE sn;
					char szDomainName[MAX_PATH] = {0};
					DWORD dwDmLen = MAX_PATH;

					if (LookupAccountSid(NULL, pTokenUser->User.Sid, userNameBuf, (LPDWORD)&bufLen, szDomainName, &dwDmLen, &sn))
					{
						bRet = true;
					}
				}

				if (pTokenUser)
				{
					free(pTokenUser);
					pTokenUser = NULL;
				}
			}
			CloseHandle(hToken);
		}
	}

	return bRet;
}


WISEPLATFORM_API bool util_process_check(char *processName)
{
	bool bRet = false;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = NULL;
	if (NULL == processName)
		return bRet;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
	{
		CloseHandle(hSnapshot);
		return bRet;
	}

	while (true)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == false)
			break;
		if (_stricmp(pe.szExeFile, processName) == 0)
		{
			bRet = true;
			break;
		}
	}
	CloseHandle(hSnapshot);
	return bRet;
}

bool util_process_adjust_privileges()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	TOKEN_PRIVILEGES oldtp;

	DWORD dwSize = sizeof(TOKEN_PRIVILEGES);
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
			return true;
		else
			return false;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
	{
		CloseHandle(hToken);
		return false;
	}

	memset(&tp, 0, sizeof(tp));
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, false, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize))
	{
		CloseHandle(hToken);
		return false;
	}

	CloseHandle(hToken);
	return true;
}

void util_process_get_handles(HANDLE *eplHandleList, char *processName, int *logonUserCnt, int maxLogonUserCnt)
{
	HANDLE hPrc = NULL;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = NULL;
	if (NULL == eplHandleList || NULL == processName || NULL == logonUserCnt)
		return;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
		return;
	*logonUserCnt = 0;
	while (TRUE)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == FALSE)
			break;
		if (strcmp(pe.szExeFile, processName) == 0)
		{
			hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);

			if (hPrc == NULL)
			{
				DWORD dwRet = GetLastError();
				if (dwRet == 5)
				{
					if (util_process_adjust_privileges())
					{
						hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
					}
				}
			}

			if (*logonUserCnt < maxLogonUserCnt)
			{
				eplHandleList[*logonUserCnt] = hPrc;
				(*logonUserCnt)++;
			}
			//break;
		}
	}
	if (hSnapshot)
		CloseHandle(hSnapshot);
	//return hPrc;
}

WISEPLATFORM_API bool util_process_get_logon_users(char *logonUserList, int *logonUserCnt, int maxLogonUserCnt, int maxLogonUserNameLen)
{
	if (logonUserList == NULL || logonUserCnt == NULL)
		return false;
	{
		//HANDLE eplHandleList[maxLogonUserCnt] = {0};
		HANDLE *eplHandleList = (HANDLE *)malloc(sizeof(HANDLE) * maxLogonUserCnt);
		memset(eplHandleList, 0, sizeof(HANDLE) * maxLogonUserCnt);
		util_process_get_handles(eplHandleList, "explorer.exe", logonUserCnt, maxLogonUserCnt);
		if (*logonUserCnt > 0)
		{
			int i = 0;
			for (i = 0; i < *logonUserCnt; i++)
			{
				/*if(app_os_GetProcessUsername(eplHandleList[i], logonUserList[i], maxLogonUserNameLen))
				{
					continue
				}*/
				util_process_username_get(eplHandleList[i], &(logonUserList[i * maxLogonUserNameLen]), maxLogonUserNameLen);
				if (eplHandleList[i])
					CloseHandle(eplHandleList[i]);
			}
		}
		free(eplHandleList);
		//if(eplHandle) app_os_CloseHandle(eplHandle);
	}
	return true;
}

WISEPLATFORM_API HANDLE util_process_create_Toolhelp32Snapshot(unsigned long dwFlags, unsigned long th32ProcessID)
{
	return CreateToolhelp32Snapshot(dwFlags, th32ProcessID);
}

WISEPLATFORM_API bool util_process_close_Toolhelp32Snapshot_handle(HANDLE hSnapshot)
{
	return CloseHandle(hSnapshot);
}

WISEPLATFORM_API bool util_process_Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
	return Process32First(hSnapshot, lppe);
}

WISEPLATFORM_API bool util_process_Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
	return Process32Next(hSnapshot, lppe);
}

WISEPLATFORM_API bool util_process_GlobalMemoryStatusEx(__out LPMEMORYSTATUSEX lpBuffer)
{
	return GlobalMemoryStatusEx(lpBuffer);
}

WISEPLATFORM_API bool util_process_GetSystemTimes(__out_opt LPFILETIME lpIdleTime, __out_opt LPFILETIME lpKernelTime, __out_opt LPFILETIME lpUserTime)
{
	return GetSystemTimes(lpIdleTime, lpKernelTime, lpUserTime);
}

WISEPLATFORM_API bool util_process_GetProcessTimes(__in HANDLE hProcess, __out LPFILETIME lpCreationTime, __out LPFILETIME lpExitTime, __out LPFILETIME lpKernelTime, __out LPFILETIME lpUserTime)
{
	return GetProcessTimes(hProcess, lpCreationTime, lpExitTime, lpKernelTime, lpUserTime);
}

WISEPLATFORM_API void util_process_GetSystemInfo(__out LPSYSTEM_INFO lpSystemInfo)
{
	GetSystemInfo(lpSystemInfo);
}

WISEPLATFORM_API void util_process_handles_get_withid(HANDLE *eplHandleList, unsigned long pid)
{
	HANDLE hPrc = NULL;
	hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hPrc == NULL)
	{
		DWORD dwRet = GetLastError();
		if (dwRet == 5)
		{
			if (adjust_privileges())
			{
				hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
			}
		}
	}
	*eplHandleList = hPrc;
}

WISEPLATFORM_API void util_process_handles_close(HANDLE hProcess)
{
	CloseHandle(hProcess);
}

WISEPLATFORM_API bool util_process_memoryusage_withid(unsigned int prcID, long *memUsageKB)
{
	bool bRet = false;

	HANDLE hPrc = NULL;
	PROCESS_MEMORY_COUNTERS pmc;

	if (!prcID || NULL == memUsageKB)
		return bRet;

	util_process_handles_get_withid(&hPrc, prcID);
	memset(&pmc, 0, sizeof(pmc));

	if (GetProcessMemoryInfo(hPrc, &pmc, sizeof(pmc)))
	{
		*memUsageKB = (long)(pmc.WorkingSetSize / DIV);
		bRet = true;
	}
	CloseHandle(hPrc);

	return bRet;
}
