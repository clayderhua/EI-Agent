#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#ifdef PCN_OTA_BASH_DEBUG
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#define LOG_TAG "OTA"
#include "Log.h"
#include "SUEClientCore.h"

#ifdef linux
#include <dirent.h>
#include <sys/wait.h>
#include <fnmatch.h>
#include <sys/utsname.h>
#else
#include <windows.h>
#include <iphlpapi.h>
#include <io.h>
#include <direct.h>
#include <lm.h>

#include <tlhelp32.h>
#include <UserEnv.h>
#include "wrapper.h"

#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "netapi32.lib")
#endif

#include "cp_fun.h"
#include "util_path.h"

#ifdef linux

#define MATCH_CHAR(c1, c2, igCase)  ((c1 == c2) || ((igCase == 1) && (tolower(c1) == tolower(c2))))
bool WildcardMatch(const char *src, const char *pattern, int igCase)
{
	bool asterisk = false;
	int sa = 0, pa = 0;
	char cp = '\0';
	char cs = '\0';
	int i = 0;
	int si = 0, pi = 0;
	while(si < strlen(src))
	{
		if(pi == strlen(pattern))
		{
			if(asterisk)
			{
				sa++;
				si = sa;
				pi = pa;
				continue;
			}
			else
			{
				return false;
			}
		}

		cp = *(pattern+pi);
		cs = *(src+si);

		if(cp == '*')
		{
			asterisk = true;
			pi++;
			pa = pi;
			sa = si;
		}
		else if(cp == '?')
		{
			si++;
			pi++;
		}
		else
		{
			if(MATCH_CHAR(cs,cp,igCase))
			{
				si++;
				pi++;
			}
			else if(asterisk)
			{
				pi = pa;
				sa++;
				si = sa;
			}
			else
			{
				return false;
			}
		}
	}

	for (i = pi; i < strlen(pattern); i++)
	{
		if (*(pattern+i) != '*') return false;
	}
	return true;
}

// Search the spacfiy name file form the directory.
//
// depth
// 		 0		-- unlimited, seaerch all it can rearched.
// 		 n  	-- search 1 to n depth.
// isJustSpacLayer
//      [If depth == 0, ignore this parameter, else effect]
// 		 false	-- search 1 to n depth, or all it can rearched depends on 'depth'.
// 		 true	-- just search the 'N' layer.
// Return
// 		 first file path it found or NULL when not found.
//
/* Note: If return value is not NULL, caller should free the value when don't need it any more. */
char * cp_find_file(const char * dirPath, const char * fileName, unsigned int depth, bool isJustSpacLayer)
{
	char * distFilePath = NULL;
	if (dirPath && fileName) {
		DIR * dp = opendir(dirPath);
		if (dp) {
			struct dirent entry;
			struct dirent * result;

			while (0 == readdir_r(dp, &entry, &result) && result != NULL) {
				const char * tname = result->d_name;

				if (!strcmp(tname, ".") || !strcmp(tname, "..")) {
					continue;
				}
				if (DT_DIR == result->d_type && 1 != depth) {
					unsigned int tdepth = depth > 1 ? (depth - 1) : 0; // Note: depth can't be 1 here!
                    char path[PATH_MAX];

                    sprintf(path, "%s/%s", dirPath, tname);
					distFilePath = cp_find_file(path, fileName, tdepth, isJustSpacLayer);
					if (distFilePath)
						break;
					continue;
				}

				if (depth > 1 && isJustSpacLayer) {
					continue;
				}

				if (strcmp(tname, fileName) == 0) {
					//char buf[PATH_MAX];
					//char * actualPath = getcwd(buf, PATH_MAX);
					int len = strlen(dirPath) + strlen(tname) + 2; // +1 for '/', +1 for '\0'

					distFilePath = malloc(len); /* Not free, caller do that! */
					sprintf(distFilePath, "%s/%s", dirPath, tname);
					break;
				}
			}
			closedir(dp);
		}
	}
	return distFilePath;
}

int cp_del_match_file(char * dirPath, char * matchStr)
{
	int iRet = 0;
	if(NULL != dirPath)
	{
		DIR *dp;
		struct dirent *entry;
		if((dp = opendir(dirPath)) != NULL)
		{
			while((entry = readdir(dp)) != NULL)
			{
                char *path = (char *)malloc(PATH_MAX);
                sprintf(path, "%s/%s", dirPath, entry->d_name);
				if(fnmatch(matchStr, entry->d_name, FNM_PATHNAME|FNM_PERIOD) == 0)
				{
					iRet = remove(path);
				}
                free(path);
			}
			closedir(dp);
		}
		else iRet = -1;
	}
	else iRet = -1;
	return iRet;
}

char * cp_strlwr(char *s)
{
    if (s) {
        int i;
        for(i = 0; s[i]; i++) {
            s[i] = tolower(s[i]);
        }
    }
    return s;
}

 /*
  * return  0 if appPath is NULL;
  *         -1 if fork(), chdir() failed;
  *         > 0, success and this value is the child process pid.
  */
CP_PID_T cp_process_launch(char * appPath, char * argvStr, char * workDir)
{
    CP_PID_T pidRet = 0;
    if(appPath)
    {
        pid_t pid = fork();
        if (0 == pid) /* Child process */
        {
            char cmdLine[MAX_PATH] = {0};
            int proRetCode = 0;


            // Change work directory
            if(workDir) {
                if (chdir(workDir)) {
					LOGE("chdir workDir=[%s] fail", workDir);
					exit(ENOENT);
				}
			}

            // Add read & execute permission
            struct stat buf;
            stat(appPath, &buf);
            chmod(appPath, buf.st_mode | S_IRUSR | S_IRGRP | S_IXUSR | S_IXGRP);

            /*
             * The'appPath' may contain space/wildcard/shell-meta
             * character, use double quotes(") wrap it to escape
             * except $ and `.
             * So make $ and ` escape manually.
             */
            {
                char * p1 = appPath;
                char * p2 = cmdLine;
                *p2++ = '"';
                while (*p1 != '\0') {
                    if (*p1 == '$' || *p1 == '`') {
                        *p2++ = '\\';
                    }
                    *p2 = *p1;
                    p1++;
                    p2++;
                }
                *p2++ = '"';
            }
            if (argvStr)
            {
                strcat(cmdLine, " ");
                strcat(cmdLine, argvStr);
            }
#ifdef PCN_OTA_BASH_DEBUG
			int outfd = open("/tmp/pkg/ota.log", O_CREAT|O_WRONLY|O_TRUNC, 0644);
			dup2(outfd, 1);
			dup2(outfd, 2);
			close(outfd);
#endif
#ifdef ANDROID
            proRetCode = execlp("/system/bin/sh", "sh", "-c", cmdLine, NULL);
#else
            proRetCode = execlp("/bin/sh", "sh", "-c", cmdLine, NULL);
#endif
			LOGE("cp_process_launch: error in launch, proRetCode=%d, errno=%d", proRetCode, errno);
            exit(proRetCode);
        }
        pidRet = pid;
    }
	LOGD("cp_process_launch: return pidRet=%d", pidRet);
    return pidRet;
}

CP_PID_T cp_process_launch_as_user(char * appPath, char * argvStr, char * workDir)
{
	return cp_process_launch(appPath, argvStr, workDir);
}


int cp_tidy_file_path(char * filePath)
{
	int iRet = -1;
	if(filePath != NULL)
	{
		char * tmpStr = filePath, *pos = NULL;
		while(tmpStr != NULL && *tmpStr!='\0')
		{
			pos = strchr(tmpStr, '\\');
			if(pos)
			{
				*pos = FILE_SEPARATOR;
				tmpStr = pos + 1;
			}
			else tmpStr = NULL;
		}
		iRet = 0;
	}
	return iRet;
}

int cp_wait_pid(CP_PID_T pid, int* status, unsigned int timeoutMs)
{
	int iRet = -1;
	CP_PID_T retPid = -1;
	int tmpStatus = 0;
	int cnt = timeoutMs/10+1, i = 0;
	iRet = 1;  //Timeout
	for(i = 0; i < cnt; i++)
	{
		retPid = waitpid(pid, &tmpStatus, WNOHANG);
		if(retPid == pid)
		{
			iRet = 0;  //WaitObject
			if(WIFEXITED(tmpStatus)) {
				*status = WEXITSTATUS(tmpStatus);
				LOGD("cp_process_launch: WIFEXITED, status=%d", *status);
			} else if(WIFSIGNALED(tmpStatus)) {
				*status = WTERMSIG(tmpStatus);
				LOGD("cp_process_launch: WIFSIGNALED, status=%d", *status);
			} else if(WIFSTOPPED(tmpStatus)) {
				*status = WSTOPSIG(tmpStatus);
				LOGD("cp_process_launch: WIFSTOPPED, status=%d", *status);
			} else if(WIFCONTINUED(tmpStatus)) {
				LOGD("cp_process_launch: WIFCONTINUED, status=%d", *status);
				continue;
			} else {
				LOGD("cp_process_launch: others");
				iRet = -1;  //WaitObject
			}
			break;
		}
		else if(retPid == 0)
		{
			usleep(1000*10);
		}
		else
		{
			LOGD("cp_process_launch: waitpid error, retPid=%d, errno=%d", retPid, errno);
			iRet = -1; //Failed
			break;
		}
	}
	return iRet;
}


void cp_close_pid(CP_PID_T pid)
{
	return;
}

int cp_kill_pid(CP_PID_T pid)
{
	return kill(pid, SIGKILL);
}

#else // Windows

#define MATCH_CHAR(c1, c2, igCase)  ((c1 == c2) || ((igCase == 1) && (tolower(c1) == tolower(c2))))
bool WildcardMatch(const char *src, const char *pattern, int igCase)
{
	bool asterisk = false;
	int sa = 0, pa = 0;
	char cp = '\0';
	char cs = '\0';
	int i = 0;
	int si = 0, pi = 0;
	while(si < strlen(src))
	{
		if(pi == strlen(pattern))
		{
			if(asterisk)
			{
				sa++;
				si = sa;
				pi = pa;
				continue;
			}
			else
			{
				return false;
			}
		}

		cp = *(pattern+pi);
		cs = *(src+si);

		if(cp == '*')
		{
			asterisk = true;
			pi++;
			pa = pi;
			sa = si;
		}
		else if(cp == '?')
		{
			si++;
			pi++;
		}
		else
		{
			if(MATCH_CHAR(cs,cp,igCase))
			{
				si++;
				pi++;
			}
			else if(asterisk)
			{
				pi = pa;
				sa++;
				si = sa;
			}
			else
			{
				return false;
			}
		}
	}

	for (i = pi; i < strlen(pattern); i++)
	{
		if (*(pattern+i) != '*') return false;
	}
	return true;
}


char * cp_find_file(const char * dirPath, const char * fileName, unsigned int depth, bool isSpecLayer)
{
	char * findFilePath = NULL;
	if(NULL == dirPath || NULL == fileName) return findFilePath;
	{
		long findHandle = -1;
		struct _finddata_t fileInfo;
		int len = strlen(dirPath)+3;
		char * tmpFindPath = (char*)malloc(len);
		memset(tmpFindPath, 0, len);
		sprintf(tmpFindPath, "%s%c*", dirPath, FILE_SEPARATOR);
		findHandle = _findfirst(tmpFindPath, &fileInfo);
		if(findHandle != -1)
		{
			do
			{
				if (fileInfo.attrib & _A_SUBDIR )
				{
					if(depth != 1)
					{
						if((strcmp(fileInfo.name,".") != 0 ) &&(strcmp(fileInfo.name,"..") != 0))
						{
							unsigned int curDepth = depth >1?depth-1:depth;
							char * curDirPath = NULL;
							len = strlen(dirPath)+strlen(fileInfo.name)+2;
							curDirPath = (char *)malloc(len);
							memset(curDirPath, 0, len);
							sprintf(curDirPath, "%s%c%s", dirPath, FILE_SEPARATOR, fileInfo.name);
							findFilePath = cp_find_file(curDirPath, fileName, curDepth, isSpecLayer);
							free(curDirPath);
						}
					}
				}

				if((depth>1 && !isSpecLayer) || depth <= 1)
				{
					if(!strcasecmp(fileName, fileInfo.name))
					{
						len = strlen(dirPath)+strlen(fileInfo.name)+16;
						findFilePath = (char *)malloc(len);
						memset(findFilePath, 0, len);
						sprintf(findFilePath, "%s%c%s", dirPath, FILE_SEPARATOR, fileInfo.name);
					}
				}
			}while (_findnext(findHandle, &fileInfo) == 0 && findFilePath == NULL);
			_findclose(findHandle);
		}
		free(tmpFindPath);
	}
	return findFilePath;
}

int cp_del_match_file(char * dirPath, char * matchStr)
{
	int iRet = 0;
	if(NULL != dirPath && NULL != matchStr)
	{
		long findHandle = -1;
		struct _finddata_t fileInfo;
		int len = strlen(dirPath)+strlen(matchStr)+2;
		char * tmpFindPath = (char*)malloc(len);
		memset(tmpFindPath, 0, len);
		sprintf(tmpFindPath, "%s%c%s", dirPath, FILE_SEPARATOR, matchStr);
		findHandle = _findfirst(tmpFindPath, &fileInfo);
		if(findHandle != -1)
		{
			do
			{
				if (fileInfo.attrib & _A_ARCH)
				{
					char * curFilePath = NULL;
					len = strlen(dirPath)+strlen(fileInfo.name)+2;
					curFilePath = (char *)malloc(len);
					memset(curFilePath, 0, len);
					sprintf(curFilePath, "%s%c%s", dirPath, FILE_SEPARATOR, fileInfo.name);
					iRet = remove(curFilePath);
					free(curFilePath);
				}
			}while (_findnext(findHandle, &fileInfo) == 0 && iRet == 0);
			_findclose(findHandle);
		}
		else iRet = -1;
		free(tmpFindPath);
	}
	else iRet = -1;
	return iRet;
}

char * cp_strlwr(char *s)
{
    return strlwr(s);
}

static int adjust_privileges()
{
	int iRet = -1;
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	TOKEN_PRIVILEGES oldtp;
	DWORD dwSize = sizeof(TOKEN_PRIVILEGES);
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) iRet = 0;
		goto done;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
	{
		CloseHandle(hToken);
		goto done;
	}

	memset(&tp, 0, sizeof(tp));
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize))
	{
		CloseHandle(hToken);
		goto done;
	}
	iRet = 0;
done:
	CloseHandle(hToken);
	return iRet;
}

static BOOL adjust_privileges_ex(HANDLE hToken,
	LPCTSTR lpszPrivilege,
	BOOL bEnablePrivilege)
{
	BOOL bRet = FALSE;
	if (hToken != NULL && lpszPrivilege != NULL)
	{
		TOKEN_PRIVILEGES tp;
		DWORD dwSize = sizeof(TOKEN_PRIVILEGES);
		LUID luid;
		if (LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
		{
			memset(&tp, 0, sizeof(tp));
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			if (AdjustTokenPrivileges(hToken, bEnablePrivilege, &tp, dwSize, (PTOKEN_PRIVILEGES)NULL,
				(PDWORD)NULL))
			{
				bRet = TRUE;
			}
		}
	}
	return bRet;
}

CP_PID_T cp_process_launch_as_user(char * appPath, char * argvStr, char * workDir)
{
    CP_PID_T pidRet = 0;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char * cmdStr = NULL;
    HANDLE hToken = NULL;
    HANDLE hCurProcToken = NULL;
    HANDLE hTokenDup = NULL;
    LPVOID pEnv = NULL;
    DWORD dwSessionId = 0xFFFFFFFF;
    DWORD dwCurProcSessionId = 0xFFFFFFFF;
    if (NULL == appPath) return pidRet;
    dwSessionId = WTSGetActiveConsoleSessionId();
    //printf("cp_process_launch_as_user Active Session Id:%d", dwSessionId);
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hCurProcToken))
    {
        DWORD retLen = 0;
        if (GetTokenInformation(hCurProcToken, TokenSessionId, &dwCurProcSessionId, sizeof(DWORD), &retLen))
        {
            //printf("cp_process_launch_as_user Current process Session Id:%d", dwCurProcSessionId);
            if (dwSessionId != dwCurProcSessionId)
            {
                if (DuplicateTokenEx(hCurProcToken, TOKEN_ALL_ACCESS, NULL, SecurityIdentification, TokenPrimary, &hTokenDup))
                {
                    BOOL bFlag = FALSE;
                    if (!SetTokenInformation(hTokenDup, TokenSessionId, &dwSessionId, sizeof(DWORD)))
                    {
                        if (GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
                        {
                            //printf("cp_process_launch_as_user adjust privileges SE_TCB_NAME");
                            adjust_privileges_ex(hTokenDup, SE_TCB_NAME, FALSE);
                            if (SetTokenInformation(hTokenDup, TokenSessionId, &dwSessionId, sizeof(DWORD)))
                            {
                                if (CreateEnvironmentBlock(&pEnv, hTokenDup, FALSE)) bFlag = TRUE;
                            }
                        }
                    }
                    else bFlag = TRUE;
                    if (bFlag = TRUE) hToken = hTokenDup;
                }
            }
            else hToken = hCurProcToken;
        }
    }
    if (hToken != NULL)
    {
        memset(&si, 0, sizeof(si));
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWNORMAL;
        si.cb = sizeof(si);
        si.lpDesktop = "WinSta0\\Default";
        memset(&pi, 0, sizeof(pi));
        if (CreateProcessAsUser(hToken, appPath, argvStr, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, workDir, &si, &pi))
        {
            CloseHandle(pi.hThread);
            //CloseHandle(pi.hProcess);
            pidRet = (CP_PID_T)malloc(sizeof(cp_pid_t));
            memset(pidRet, 0, sizeof(cp_pid_t));
            pidRet->hProc = pi.hProcess;
            pidRet->procID = pi.dwProcessId;
        }
    }

    if (pEnv != NULL) DestroyEnvironmentBlock(pEnv);
    if (hTokenDup != NULL) CloseHandle(hTokenDup);
    if (hCurProcToken != NULL) CloseHandle(hCurProcToken);

    //printf("cp_process_launch_as_user appPath:%s,argvStr:%s,Error:%d", appPath, argvStr, GetLastError());
    return pidRet;
}

int cp_tidy_file_path(char * filePath)
{
	int iRet = -1;
	if(filePath != NULL)
	{
		char * tmpStr = filePath, *pos = NULL;
		while(tmpStr != NULL && *tmpStr!='\0')
		{
			pos = strchr(tmpStr, '/');
			if(pos)
			{
				*pos = FILE_SEPARATOR;;
				tmpStr = pos + 1;
			}
			else tmpStr = NULL;
		}
		iRet = 0;
	}
	return iRet;
}

int cp_wait_pid(CP_PID_T pid, int* status, unsigned int timeoutMs)
{
	int iRet = -1;
    if (pid != NULL && NULL != status)
    {
        DWORD dwRet = WaitForSingleObject(pid->hProc, timeoutMs);
        if (dwRet == WAIT_OBJECT_0)
        {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(pid->hProc, &exitCode))
            {
                *status = exitCode;
                iRet = 0;
            }
        }
        else if (dwRet == WAIT_TIMEOUT)
        {
            iRet = 1;
        }
    }
	return iRet;
}

int cp_kill_pid(CP_PID_T pid)
{
    int iRet = -1;
    PROCESSENTRY32 pe;
    HANDLE hSnapshot = NULL;
    DWORD procID = pid->procID;
    if (NULL == pid) return iRet;
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hSnapshot, &pe))
    {
        CloseHandle(hSnapshot);
        return iRet;
    }
    do
    {
        if (pe.th32ParentProcessID == procID)
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
                TerminateProcess(hPrc, 0x7f);
                CloseHandle(hPrc);
            }
        }
    } while (Process32Next(hSnapshot, &pe));
    CloseHandle(hSnapshot);
    if (pid->hProc)
    {
        TerminateProcess(pid->hProc, 0x7f);    //asynchronous
        iRet = 0;
    }
    return iRet;
}

void cp_close_pid(CP_PID_T pid)
{
    if (pid != NULL)
    {
        CloseHandle(pid->hProc);
        free(pid);
    }
}

#endif

