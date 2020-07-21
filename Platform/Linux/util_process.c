#include "util_process.h"
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include "util_os.h"

#define PROC_NAME_LINE 1
#define PROC_PID_LINE 5
#define PROC_UID_LINE 8
#define BUFF_LEN 1024
#define CPU_USAGE_INFO_LINE 1
#define MEM_TOTAL_LINE 1
#define MEM_FREE_LINE 2

#define DIV (1024)

typedef struct statstruct_proc
{
	int pid;					   /** The process id. **/
	char exName[_POSIX_PATH_MAX];  /** The filename of the executable **/
	char state; /** 1 **/		   /** R is running, S is sleeping, 
			   D is sleeping in an uninterruptible wait,
			   Z is zombie, T is traced or stopped **/
	unsigned euid,				   /** effective user id **/
		egid;					   /** effective group id */
	int ppid;					   /** The pid of the parent. **/
	int pgrp;					   /** The pgrp of the process. **/
	int session;				   /** The session id of the process. **/
	int tty;					   /** The tty the process uses **/
	int tpgid;					   /** (too long) **/
	unsigned int flags;			   /** The flags of the process. **/
	unsigned int minflt;		   /** The number of minor faults **/
	unsigned int cminflt;		   /** The number of minor faults with childs **/
	unsigned int majflt;		   /** The number of major faults **/
	unsigned int cmajflt;		   /** The number of major faults with childs **/
	int utime;					   /** user mode jiffies **/
	int stime;					   /** kernel mode jiffies **/
	int cutime;					   /** user mode jiffies with childs **/
	int cstime;					   /** kernel mode jiffies with childs **/
	int counter;				   /** process's next timeslice **/
	int priority;				   /** the standard nice value, plus fifteen **/
	unsigned int timeout;		   /** The time in jiffies of the next timeout **/
	unsigned int itrealvalue;	  /** The time before the next SIGALRM is sent to the process **/
	int starttime; /** 20 **/	  /** Time the process started after system boot **/
	unsigned int vsize;			   /** Virtual memory size **/
	unsigned int rss;			   /** Resident Set Size **/
	unsigned int rlim;			   /** Current limit in bytes on the rss **/
	unsigned int startcode;		   /** The address above which program text can run **/
	unsigned int endcode;		   /** The address below which program text can run **/
	unsigned int startstack;	   /** The address of the start of the stack **/
	unsigned int kstkesp;		   /** The current value of ESP **/
	unsigned int kstkeip;		   /** The current value of EIP **/
	int signal;					   /** The bitmap of pending signals **/
	int blocked; /** 30 **/		   /** The bitmap of blocked signals **/
	int sigignore;				   /** The bitmap of ignored signals **/
	int sigcatch;				   /** The bitmap of catched signals **/
	unsigned int wchan; /** 33 **/ /** (too long) **/
	int sched,					   /** scheduler **/
		sched_priority;			   /** scheduler priority **/

} procinfo;

bool util_process_read_proc(PROCESSENTRY32 *info, const char *c_pid);
//PROCESSENTRY32_NODE *m_curNode = NULL;

bool util_process_launch(char *appPath)
{
	bool bRet = true;
	pid_t pid = fork();
	if (0 == pid)
	{ /* Child process */
#ifdef ANDROID
		exit(execlp("/system/bin/sh", "sh", "-c", appPath, NULL));
#else
		exit(execlp("/bin/sh", "sh", "-c", appPath, NULL));
#endif
	}
	else if (pid < 0)
	{ /* fork() failed */
		bRet = false;
	}
	return bRet;
}

HANDLE util_process_cmd_launch(char *cmdline)
{
	if (cmdline == NULL)
		return 0;
	pid_t pid = fork();
	if (0 == pid) /* Child process */
	{
#ifdef ANDROID
		execlp("/system/bin/sh", "sh", "-c", cmdline, NULL);
#else
		execlp("/bin/sh", "sh", "-c", cmdline, NULL);
#endif
	}
	if (-1 == pid)
		return pid; //Means failed -- like 'FALSE'
	else
	{
		int status;
		waitpid(pid, &status, 0); /* Wait for the child to terminate. */
		return pid;
	}
}

HANDLE util_process_cmd_launch_no_wait(char *cmdline)
{
	if (cmdline == NULL)
		return 0;
	pid_t pid = fork();
	if (0 == pid) /* Child process */
	{
#ifdef ANDROID
		execlp("/system/bin/sh", "sh", "-c", cmdline, NULL);
#else
		execlp("/bin/sh", "sh", "-c", cmdline, NULL);
#endif
	}
	return pid;
}

WISEPLATFORM_API bool util_is_process_running(HANDLE hProcess)
{
	if (-1 == hProcess)
		return false;
	else
	{
		int result = kill(hProcess, 0);
		return result == 0;
	}
}

void util_process_wait_handle(HANDLE hProcess)
{
	if (-1 == hProcess)
		return;
	else
	{
		int status;
		waitpid(hProcess, &status, 0); /* Wait for the child to terminate. */
	}
}

void util_process_kill_handle(HANDLE hProcess)
{
	if (-1 == hProcess)
		return;
	else
	{
		int status;
		kill(hProcess, SIGABRT);
		wait(&status); /* Wait for the child to terminate. */
	}
}

unsigned long getPIDByName(char *prcName)
{
	FILE *fopen = NULL;
	unsigned int pid = 0;
	char buf[16];
	char cmdLine[256];
	sprintf(cmdLine, "pidof -s %s", prcName);
	if ((fopen = popen(cmdLine, "r")) == NULL)
		return 0;
	if (!fgets(buf, sizeof(buf), fopen))
	{
		pclose(fopen);
		return 0;
	}
	sscanf(buf, "%ud", &pid);
	pclose(fopen);
	return pid;
}

/*
	Find last logon user
*/
bool GetSysLogonUserName2(char *userNameBuf, unsigned int bufLen)
{
	int i = 0;
	FILE *fp = NULL;
	char cmdline[128] = {0};
	char cmdbuf[32] = {0};

	if (userNameBuf == NULL || bufLen == 0)
		return false;
#ifdef ANDROID
	sprintf(cmdline, "whoami");
	fp = popen(cmdline, "r");
	if (NULL != fp)
	{
		char buf[512] = {0};
		if (fgets(buf, sizeof(buf), fp))
		{
			sscanf(buf, "%31s", cmdbuf);
		}
		pclose(fp);
	}
#else
	sprintf(cmdline, "who | awk '{ print $3, $4, $1 }' | sort | awk 'END { print $3 }' | tr -d '\n'"); // get last logon user
	fp = popen(cmdline, "r");
	if (NULL != fp)
	{
		if (fgets(cmdbuf, sizeof(cmdbuf), fp) == NULL)
		{ // if error
			cmdbuf[0] = '\0';
		}
		pclose(fp);
	}
#endif

	i = strlen(cmdbuf);
	if (i > 0 && i < bufLen)
		strcpy(userNameBuf, cmdbuf);
	else
		return false;
	return true;
}

bool GetSysLogonUserName(char *userNameBuf, unsigned int bufLen)
{
	int i = 0;
	FILE *fp = NULL;
	char cmdline[128];
	char cmdbuf[12][32] = {{0}};

	bool bRet = GetSysLogonUserName2(userNameBuf, bufLen);
	if (bRet)
		return bRet;

	if (userNameBuf == NULL || bufLen == 0)
		return false;
#ifdef ANDROID
	sprintf(cmdline, "whoami");
#else
	sprintf(cmdline, "last|grep still"); //for opensusi kde desktop
#endif
	fp = popen(cmdline, "r");
	if (NULL != fp)
	{
		char buf[512] = {0};
		while (fgets(buf, sizeof(buf), fp))
		{
			sscanf(buf, "%31s", cmdbuf[0]);
			if (strcmp(cmdbuf[0], "reboot"))
				break;
		}
	}
	pclose(fp);

	i = strlen(cmdbuf[0]);
	if (i > 0 && i < bufLen)
		strcpy(userNameBuf, cmdbuf[0]);
	else
		return false;

	return true;
}

bool util_process_as_user_launch(char *cmdLine, bool isAppNameRun, bool isShowWindow, unsigned long *newPrcID)
{
	char logonUserName[32] = {0};
	if (!cmdLine)
		return false;
	if (GetSysLogonUserName(logonUserName, sizeof(logonUserName)))
	{
		FILE *fp = NULL;
		char cmdBuf[256] = {0};
		//sprintf(cmdBuf,"su - %s -c %s &",logonUserName,cmdLine);
		//sprintf(cmdBuf,"DISPLAY=:0 su -c %s %s &",cmdLine,logonUserName);
#ifdef ANDROID
		printf("util_process_as_user_launch ->\n");
		printf("cmdline=%s, username=%s\n", cmdLine, logonUserName);
		sprintf(cmdBuf, "%s &", cmdLine);
#else
		if (isShowWindow)
		{
			sprintf(cmdBuf, "DISPLAY=:0 su -c 'xterm -e /bin/bash -c \"%s\"' %s &", cmdLine, logonUserName);
		}
		else
		{
			//sprintf(cmdBuf, "/usr/bin/sudo -u %s -- %s", logonUserName, cmdLine); //not work for "Defaults    requiretty" in /etc/sudoer
			sprintf(cmdBuf, "su -c '%s' %s", cmdLine, logonUserName);
		}
#endif
		printf("util_process_as_user_launch%s\n", cmdBuf);
		if ((fp = popen(cmdBuf, "r")) == NULL)
		{
			//printf("restart process failed,%s",cmdBuf);
			pclose(fp);
			return false;
		}
		{
			char result[260];
			while (fgets(result, 260, fp) != NULL)
    			printf("%s", result);
		}
		pclose(fp);
	}
	if (newPrcID != NULL)
		*newPrcID = getPIDByName(cmdLine);
	return true;
}

bool util_process_kill(char *processName)
{
	pid_t p;
	size_t i, j;
	char *s = (char *)malloc(264);
	char buf[128];
	FILE *st;
	DIR *d = opendir("/proc");
	if (d == NULL)
	{
		free(s);
		return false;
	}
	struct dirent *f;
	while ((f = readdir(d)) != NULL)
	{
		if (f->d_name[0] == '.')
			continue;
		for (i = 0; isdigit(f->d_name[i]); i++)
			;
		if (i < strlen(f->d_name))
			continue;
		strcpy(s, "/proc/");
		strcat(s, f->d_name);
		strcat(s, "/status");
		st = fopen(s, "r");
		if (st == NULL)
		{
			closedir(d);
			free(s);
			return false;
		}
		do
		{
			if (fgets(buf, 128, st) == NULL)
			{
				fclose(st);
				closedir(d);
				free(s);
				return -1;
			}
		} while (strncmp(buf, "Name:", 5));
		fclose(st);
		for (j = 5; isspace(buf[j]); j++)
			;
		*strchr(buf, '\n') = 0;
		if (!strcmp(&(buf[j]), processName))
		{
			sscanf(&(s[6]), "%d", &p);
			kill(p, SIGKILL);
		}
	}
	closedir(d);
	free(s);
	return true;
}

int util_process_id_get(void)
{
	return getpid();
}

bool util_process_username_get(HANDLE hProcess, char *userNameBuf, int bufLen)
{
	struct passwd *pw;
	unsigned int strLength = 0;
	PROCESSENTRY32 prcMonInfo;
	char pid[12] = {0};

	if (!hProcess)
		return false;

	sprintf(pid, "%ld", hProcess);
	//printf("*****util_process*****util_process_username_get %d \r\n", hProcess);
	memset(&prcMonInfo, 0, sizeof(PROCESSENTRY32));
	if (!util_process_read_proc(&prcMonInfo, pid))
		return false;
	//printf("*****util_process*****UID %d \r\n", prcMonInfo.dwUID);
	pw = getpwuid(prcMonInfo.dwUID);
	if (!pw)
		return false;
	//printf("*****util_process*****pw name %s \r\n", pw->pw_name);
	strLength = strlen((char *)(pw->pw_name));
	if (strLength >= bufLen)
		return false;

	strcpy(userNameBuf, (char *)(pw->pw_name));
	return true;
}

bool util_process_check(char *processName)
{
	bool isFind = false;
	if (processName && strlen(processName) > 0)
	{
		FILE *fd = NULL;
		char buf[BUFSIZ];
#ifdef ANDROID
		sprintf(buf, "ps | grep %s | grep -v grep", processName);
#else
		sprintf(buf, "ps -ely | grep %s | grep -v grep", processName);
#endif
		fd = popen(buf, "r");
		while (fgets(buf, sizeof(buf), fd))
		{
			if (strstr(buf, processName) && !strstr(buf, "<defunct>")) //not a zombie process
			{
				isFind = true;
				break;
			}
		}
		pclose(fd);
	}
	return isFind;
}

bool util_process_get_logon_users(char *logonUserList, int *logonUserCnt, int maxLogonUserCnt, int maxLogonUserNameLen)
{
	FILE *fp = NULL;
	char cmdline[128];
	if (logonUserList == NULL || logonUserCnt == NULL)
		return false;
	*logonUserCnt = 0;
#ifdef ANDROID
	sprintf(cmdline, "whoami");
#else
	sprintf(cmdline, "who");
#endif
	fp = popen(cmdline, "r");
	if (NULL != fp)
	{
		char buf[512] = {0};
		while (fgets(buf, sizeof(buf), fp))
		{
			int i = 0;
			char cmdbuf[32] = {0};
			bool isAddIn = false;
			sscanf(buf, "%31s", cmdbuf);
			if (strcmp(cmdbuf, "reboot") == 0)
				continue;
			for (i = 0; i < *logonUserCnt && i < maxLogonUserCnt; i++)
			{
				if (!strcmp(&(logonUserList[maxLogonUserNameLen * i]), cmdbuf))
				{
					isAddIn = true;
					break;
				}
			}
			if (!isAddIn)
			{
				if (*logonUserCnt < maxLogonUserCnt)
				{
					strcpy(&(logonUserList[maxLogonUserNameLen * (*logonUserCnt)]), cmdbuf);
					(*logonUserCnt)++;
				}
			}
		}
	}
	pclose(fp);

	//Wei.gang add to debug
	//	int i = 0;
	//	for (i = 0; i< *logonUserCnt; i++)
	//		printf("[common] app_os_GetSysLogonUserList. get user %d: %s\n",i, &(logonUserList[maxLogonUserNameLen * i]));
	//Wei.Gang add end
	return true;
}

bool read_line(FILE *fp, char *buff, int b_l, int l);

/*
bool read_line(FILE* fp,char* buff,int b_l,int l)
{
    char line_buff[b_l];
    int i;

    if (!fp)
        return false;
    
    for (i = 0; i < l-1; i++)
    {
        if (!fgets (line_buff, sizeof(line_buff), fp))
        {
            return false;
        }
    }

    if (!fgets (line_buff, sizeof(line_buff), fp))
    {
        return false;
    }

    memcpy(buff,line_buff,b_l);

    return true;
}
*/
bool util_process_read_proc(PROCESSENTRY32 *info, const char *c_pid)
{
	FILE *fp = NULL;
	char file[512] = {0};
	char line_buff[BUFF_LEN] = {0};
	char name[32] = {0};
	unsigned long dwID_tmp = 0;
	int line = 0;
	int count = 3;
	bool bRet = false;
	sprintf(file, "/proc/%s/status", c_pid);
	if (!(fp = fopen(file, "r")))
	{
		return bRet;
	}

	while (true)
	{
		line++;
		if (!fgets(line_buff, sizeof(line_buff), fp))
		{
			break;
		}
		if (strncmp(line_buff, "Name:", strlen("Name:")) == 0)
		{
			//printf("*****util_process*****Name Line %d:%s \r\n", line, line_buff);
			sscanf(line_buff, "%31s %259s", name, (info->szExeFile));
			count--;
		}
		else if (strncmp(line_buff, "Pid:", strlen("Pid:")) == 0)
		{
			dwID_tmp = 0;
			//printf("*****util_process*****PID Line %d:%s \r\n", PROC_PID_LINE, line_buff);
			sscanf(line_buff, "%31s %lu", name, &dwID_tmp);
			info->th32ProcessID = dwID_tmp;
			count--;
		}
		else if (strncmp(line_buff, "Uid:", strlen("Uid:")) == 0)
		{
			dwID_tmp = 0;
			//printf("*****util_process*****UID Line %d:%s \r\n", PROC_UID_LINE, line_buff);
			sscanf(line_buff, "%31s %lu", name, &dwID_tmp);
			info->dwUID = dwID_tmp;
			count--;
		}
		if (count <= 0)
		{
			bRet = true;
		}
	}

	fclose(fp);
	return bRet;
}

HANDLE util_process_create_Toolhelp32Snapshot(unsigned long dwFlags, unsigned long th32ProcessID)
{
	PROCESSENTRY32_NODE *headNodeSnapshot = NULL;
	PROCESSENTRY32_NODE *curInfoNode = NULL;
	PROCESSENTRY32_NODE *prcInfoNode = NULL;
	DIR *dir;
	struct dirent *ptr;
	if (!(dir = opendir("/proc")))
		return 0;
	headNodeSnapshot = (PROCESSENTRY32_NODE *)malloc(sizeof(PROCESSENTRY32_NODE));
	memset(headNodeSnapshot, 0, sizeof(PROCESSENTRY32_NODE));
	curInfoNode = headNodeSnapshot;
	//printf("*****util_process*****util_process_create_Toolhelp32Snapshot\r\n");
	while ((ptr = readdir(dir)))
	{
		if (ptr->d_name[0] > '0' && ptr->d_name[0] <= '9')
		{
			//printf("*****util_process*****path:%s \r\n", ptr->d_name);
			prcInfoNode = (PROCESSENTRY32_NODE *)malloc(sizeof(PROCESSENTRY32_NODE));
			memset(prcInfoNode, 0, sizeof(PROCESSENTRY32_NODE));
			if (!util_process_read_proc(&(prcInfoNode->prcMonInfo), ptr->d_name))
			{
				//printf("*****util_process*****util_process_read_proc return false\r\n");
				free(prcInfoNode);
				continue;
			}
			curInfoNode->next = prcInfoNode;
			curInfoNode = prcInfoNode;
			//printf("[ProcessMonitorHandler] proc name %s pid %d uid %d.\n",prcInfoNode->prcMonInfo.szExeFile, prcInfoNode->prcMonInfo.th32ProcessID, prcInfoNode->prcMonInfo.dwUID);
		}
	}
	curInfoNode = headNodeSnapshot;
	closedir(dir);
	return (HANDLE)headNodeSnapshot;
}

bool util_process_close_Toolhelp32Snapshot_handle(HANDLE hSnapshot)
{
	PROCESSENTRY32_NODE *headInfoNode = NULL;
	PROCESSENTRY32_NODE *curInfoNode = NULL;
	PROCESSENTRY32_NODE *nextInfoNode = NULL;
	if (!hSnapshot)
		return false;
	headInfoNode = (PROCESSENTRY32_NODE *)hSnapshot;
	curInfoNode = headInfoNode->next;
	while (curInfoNode != NULL)
	{
		nextInfoNode = curInfoNode->next;
		free(curInfoNode);
		curInfoNode = nextInfoNode;
	}
	headInfoNode->current = NULL;
	free(headInfoNode);
	headInfoNode = NULL;
	return true;
}

bool util_process_Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
	PROCESSENTRY32_NODE *headInfoNode = NULL;
	if (!hSnapshot)
		return false;
	headInfoNode = (PROCESSENTRY32_NODE *)hSnapshot;
	headInfoNode->current = headInfoNode->next;
	if (headInfoNode->current != NULL)
		memcpy(lppe, &(headInfoNode->current->prcMonInfo), sizeof(PROCESSENTRY32));
	else
		return false;
	//printf("[ProcessMonitorHandler]0 proc name %s pid %d\n",lppe->szExeFile, lppe->th32ProcessID);
	//lppe = &(curNode->prcMonInfo);
	return true;
}

bool util_process_Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
	PROCESSENTRY32_NODE *headInfoNode = NULL;
	if (!hSnapshot)
		return false;
	headInfoNode = (PROCESSENTRY32_NODE *)hSnapshot;
	headInfoNode->current = headInfoNode->current->next;
	if (headInfoNode->current != NULL)
		memcpy(lppe, &(headInfoNode->current->prcMonInfo), sizeof(PROCESSENTRY32));
	else
		return false;
	//printf("[ProcessMonitorHandler]1 proc name %s pid %d\n",m_curNode->prcMonInfo.szExeFile, m_curNode->prcMonInfo.th32ProcessID);
	//printf("[ProcessMonitorHandler]2 proc name %s pid %d\n",lppe->szExeFile, lppe->th32ProcessID);
	//lppe = &(m_curNode->prcMonInfo);
	return true;
}

bool util_process_GlobalMemoryStatusEx(LPMEMORYSTATUSEX lpBuffer)
{
	char proc_pic_path[128] = {0};
	FILE *fp = NULL;
	if (!lpBuffer)
		return false;
	sprintf(proc_pic_path, "/proc/meminfo");

	fp = fopen(proc_pic_path, "r");
	if (NULL != fp)
	{
		int line = 0;
		char line_buff[256] = {0};
		int count = 3;
		bool bHasMemAvailable = false;

		while (true)
		{
			line++;
			if (!fgets(line_buff, sizeof(line_buff), fp))
			{
				break;
			}
			if (strncmp(line_buff, "MemTotal:", strlen("MemTotal:")) == 0)
			{
				//printf("*****util_process_GlobalMemoryStatusEx*****MemTotal Line %d:%s \r\n", line, line_buff);
				char name[32] = {0};
				sscanf(line_buff, "%s %llu", name, &(lpBuffer->ullTotalPhys));
				count--;
			}
			else if (strncmp(line_buff, "MemFree:", strlen("MemFree:")) == 0)
			{
				//printf("*****util_process_GlobalMemoryStatusEx*****MemFree Line %d:%s \r\n", line, line_buff);
				char name[32] = {0};
				if(!bHasMemAvailable)
					sscanf(line_buff, "%s %llu", name, &(lpBuffer->ullAvailPhys));
				count--;
			}
			else if (strncmp(line_buff, "MemAvailable:", strlen("MemAvailable:")) == 0)
			{
				//printf("*****util_process_GlobalMemoryStatusEx*****MemAvailable Line %d:%s \r\n", line, line_buff);
				char name[32] = {0};
				bHasMemAvailable = true;
				sscanf(line_buff, "%s %llu", name, &(lpBuffer->ullAvailPhys));
				count--;
			}
			if (count <= 0)
			{
				break;
			}
		}
	}
	fclose(fp);
	return true;
}

bool util_process_GetSystemTimes(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime)
{
	char proc_pic_path[128] = {0};
	FILE *fp = NULL;

	if (!lpIdleTime || !lpKernelTime || !lpUserTime)
		return false;
	sprintf(proc_pic_path, "/proc/stat");
	fp = fopen(proc_pic_path, "r");
	if (NULL != fp)
	{
		char buf[256] = {0};
		if (read_line(fp, buf, sizeof(buf), CPU_USAGE_INFO_LINE))
		{
			char name[32] = {0};
			long long niceTime;
			sscanf(buf, "%s %lld %lld %lld %lld", name, (long long *)lpUserTime, &niceTime, (long long *)lpKernelTime, (long long *)lpIdleTime);
			//printf("[app_os_GetSystemTime] name:%s, utime:%lld, NTime:%lld, ktime:%lld, itime:%lld.\n",name,*lpUserTime,niceTime,*lpKernelTime,*lpIdleTime);
			//*(long long *)lpUserTime =*(long long *)lpUserTime + niceTime;
		}
	}
	fclose(fp);
	return true;
}

static int get_proc_info(pid_t pid, procinfo *pinfo)
{
	char szFileName[_POSIX_PATH_MAX],
		szStatStr[2048],
		*s, *t;
	FILE *fp;
	struct stat st;

	if (NULL == pinfo)
	{
		errno = EINVAL;
		return -1;
	}

	sprintf(szFileName, "/proc/%u/stat", (unsigned)pid);

	if (-1 == access(szFileName, R_OK))
	{
		return (pinfo->pid = -1);
	} /** if **/

	if (-1 != stat(szFileName, &st))
	{
		pinfo->euid = st.st_uid;
		pinfo->egid = st.st_gid;
	}
	else
	{
		pinfo->euid = pinfo->egid = -1;
	}

	if ((fp = fopen(szFileName, "r")) == NULL)
	{
		return (pinfo->pid = -1);
	} /** IF_NULL **/

	if ((s = fgets(szStatStr, 2048, fp)) == NULL)
	{
		fclose(fp);
		return (pinfo->pid = -1);
	}

	/** pid **/
	sscanf(szStatStr, "%u", &(pinfo->pid));
	s = strchr(szStatStr, '(') + 1;
	t = strchr(szStatStr, ')');
	strncpy(pinfo->exName, s, t - s);
	pinfo->exName[t - s] = '\0';

	sscanf(t + 2, "%c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u",
		   /*       1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33*/
		   &(pinfo->state),
		   &(pinfo->ppid),
		   &(pinfo->pgrp),
		   &(pinfo->session),
		   &(pinfo->tty),
		   &(pinfo->tpgid),
		   &(pinfo->flags),
		   &(pinfo->minflt),
		   &(pinfo->cminflt),
		   &(pinfo->majflt),
		   &(pinfo->cmajflt),
		   &(pinfo->utime),
		   &(pinfo->stime),
		   &(pinfo->cutime),
		   &(pinfo->cstime),
		   &(pinfo->counter),
		   &(pinfo->priority),
		   &(pinfo->timeout),
		   &(pinfo->itrealvalue),
		   &(pinfo->starttime),
		   &(pinfo->vsize),
		   &(pinfo->rss),
		   &(pinfo->rlim),
		   &(pinfo->startcode),
		   &(pinfo->endcode),
		   &(pinfo->startstack),
		   &(pinfo->kstkesp),
		   &(pinfo->kstkeip),
		   &(pinfo->signal),
		   &(pinfo->blocked),
		   &(pinfo->sigignore),
		   &(pinfo->sigcatch),
		   &(pinfo->wchan));
	fclose(fp);
	return 0;
}

bool util_process_GetProcessTimes(HANDLE hProcess, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime)
{
	if (!hProcess || !lpCreationTime || !lpKernelTime || !lpUserTime)
		return false;
	procinfo procInfo;
	int ret = get_proc_info(hProcess, &procInfo);

	if (ret < 0)
		return false;
	//printf("[app_os_GetProcessTimes]  procid:%d,realid%d, utime: %d, stime: %d, start_time: %d\n",hPRocess, m_procInfo.pid ,m_procInfo.utime, m_procInfo.stime, m_procInfo.starttime);

	*((long long *)lpCreationTime) = procInfo.starttime;
	*((long long *)lpKernelTime) = procInfo.stime;
	*((long long *)lpUserTime) = procInfo.utime;

	return true;
}

void util_process_GetSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
	if (lpSystemInfo == NULL)
		return;
	lpSystemInfo->dwNumberOfProcessors = sysconf(_SC_NPROCESSORS_CONF);
	return;
}

void util_process_handles_get_withid(HANDLE *eplHandleList, unsigned long pid)
{
	*eplHandleList = (HANDLE)pid;
}

void util_process_handles_close(HANDLE hProcess)
{
	return;
}

bool util_process_memoryusage_withid(unsigned int prcID, long *memUsageKB)
{
	bool bRet = false;
	procinfo porcinfo;
	int pageSize = getpagesize();

	if (!prcID || NULL == memUsageKB)
		return bRet;

	get_proc_info(prcID, &porcinfo);
	if (prcID == porcinfo.pid)
	{
		*memUsageKB = porcinfo.rss * (pageSize / DIV); // rss is page count, 4KB/Page.
		bRet = true;
	}
	return bRet;
}
