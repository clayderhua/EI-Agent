#ifndef _CP_FUN_H
#define _CP_FUN_H

#include <stdbool.h>

#ifdef linux
	#define CP_PID_T    pid_t
#ifdef ANDROID
	#define WIFCONTINUED(s) ((s) == 0xffff)
#endif
#else 
	#include "windows.h"

	typedef struct cp_pid_t {
		HANDLE hProc;
		DWORD procID;
	}cp_pid_t, *cp_p_pid_t;

	#define CP_PID_T    cp_p_pid_t

	#define OS_UNKNOW                 "unknow"
	#define OS_WINDOWS                "win"                       //"Windows"
	#define OS_WINDOWS_95             "win95"                     //"Windows 95"
	#define OS_WINDOWS_98             "win98"                     //"Windows 98"
	#define OS_WINDOWS_ME             "winME"                     //"Windows ME"
	#define OS_WINDOWS_NT_3_51        "winNT351"                  //"Windows NT 3.51"
	#define OS_WINDOWS_NT_4           "winNT4"                    //"Windows NT 4"
	#define OS_WINDOWS_2000           "win2000"                   //"Windows 2000"
	#define OS_WINDOWS_XP             "winXP"                     //"Windows XP"
	#define OS_WINDOWS_SERVER_2003    "winS2003"                  //"Windows Server 2003"
	#define OS_WINDOWS_VISTA          "winVista"                  //"Windows Vista"
	#define OS_WINDOWS_7              "win7"                      //"Windows 7"
	#define OS_WINDOWS_8              "win8"                      //"Windows 8"
	#define OS_WINDOWS_SERVER_2012    "winS2012"                  //"Windows Server 2012"
	#define OS_WINDOWS_8_1            "win8"                     //"Windows 8.1"
	#define OS_WINDOWS_SERVER_2012_R2 "winS2012R2"                //"Windows Server 2012 R2"
	#define OS_WINDOWS_10			    "win10"                     //"Windows 10"
	#define OS_WINDOWS_SERVER_2016	 "winS2016"                  //"Windows Server 2016"
#endif

#define ARCH_UNKNOW               "unknow"
#define ARCH_X64                  "x86_64"                    //"X64"
#define ARCH_ARM                  "arm"
#define ARCH_IA64                 "ia64"
#define ARCH_X86                  "x86_32"                    //"X86"    

#ifdef __cplusplus
extern "C" {
#endif

CP_PID_T cp_process_launch_as_user(char * appPath, char * argvStr, char * workDir);
void cp_close_pid(CP_PID_T pid);
int cp_wait_pid(CP_PID_T pid, int* status, unsigned int timeoutMs);
int cp_tidy_file_path(char * filePath);
int cp_kill_pid(CP_PID_T pid);
char * cp_strlwr(char *s);
int cp_del_match_file(char * dirPath, char * matchStr);
char * cp_find_file(const char * dirPath, const char * fileName, unsigned int depth, bool isSpecLayer);

#ifdef __cplusplus
}
#endif

#endif
