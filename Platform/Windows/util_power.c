#include "util_power.h"
#include <stdio.h>
#include <PowrProf.h>
#pragma comment(lib,"PowrProf.lib")

WISEPLATFORM_API bool util_power_off()
{
	bool bRet = true;
	char cmdLine[BUFSIZ] = {0};
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;  
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	sprintf(cmdLine, "%s", "cmd.exe /c shutdown /s /f /t 0");
	if(!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
	{
		bRet = false;
	}
	return bRet;
}

WISEPLATFORM_API bool util_power_restart()
{
	bool bRet = true;
	char cmdLine[BUFSIZ] = {0};
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;  
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	sprintf(cmdLine, "%s", "cmd.exe /c shutdown /r /f /t 0");
	if(!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
	{
		bRet = false;
	}
	return bRet;
}

WISEPLATFORM_API bool util_power_suspend()
{
	return SetSuspendState(FALSE, FALSE, FALSE);
}

WISEPLATFORM_API bool util_power_hibernate()
{
	return SetSuspendState(TRUE, FALSE, FALSE);
}

WISEPLATFORM_API void util_resume_passwd_disable()
{
	char cmdLine[BUFSIZ] = {0};
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;  
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	sprintf(cmdLine, "%s", "cmd.exe /c powercfg /globalpowerflag off /option:resumepassword");
	if(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

WISEPLATFORM_API bool util_power_suspend_check()
{
	return IsPwrSuspendAllowed();
}

WISEPLATFORM_API bool util_power_hibernate_check()
{
	return IsPwrHibernateAllowed();
}