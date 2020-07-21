#include "ProcessList.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "util_process.h"
#include "util_path.h"
#include "ProcessMonitorLog.h"
#include <time.h>
#include <sys/time.h>

#ifdef WIN32
#include <Windows.h>
#include <process.h>
#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")
#else
#include <sys/stat.h>
#endif

prc_mon_info_node_t *CreatePrcMonInfoList()
{
	prc_mon_info_node_t *head = NULL;
	head = (prc_mon_info_node_t *)calloc(1, sizeof(prc_mon_info_node_t));
	if (head)
	{
		head->next = NULL;
		head->prcMonInfo.isValid = 1;
		head->prcMonInfo.prcName = NULL;
		head->prcMonInfo.ownerName = NULL;
		head->prcMonInfo.prcID = 0;
		head->prcMonInfo.memUsage = 0;
		head->prcMonInfo.vmemUsage = 0;
		head->prcMonInfo.cpuUsage = 0;
		head->prcMonInfo.isActive = 0;
		head->prcMonInfo.prcCpuUsageLastTimes.lastKernelTime = 0;
		head->prcMonInfo.prcCpuUsageLastTimes.lastUserTime = 0;
		head->prcMonInfo.prcCpuUsageLastTimes.lastTime = 0;
	}
	return head;
}

bool UpdatePrcList(prc_mon_info_node_t *head)
{
	bool bRet = false;
	if (head == NULL)
		return bRet;
	{
		prc_mon_info_node_t *frontNode = head;
		prc_mon_info_node_t *delNode = NULL;
		prc_mon_info_node_t *curNode = frontNode->next;
		while (curNode)
		{
			if (curNode->prcMonInfo.isValid == 0)
			{
				frontNode->next = curNode->next;
				delNode = curNode;
				if (delNode->prcMonInfo.prcName)
				{
					free(delNode->prcMonInfo.prcName);
					delNode->prcMonInfo.prcName = NULL;
				}
				if (delNode->prcMonInfo.ownerName)
				{
					free(delNode->prcMonInfo.ownerName);
					delNode->prcMonInfo.ownerName = NULL;
				}
				free(delNode);
				delNode = NULL;
				bRet = true; //change flag
							 //m_cntProcChanged++;
			}
			else
			{
				frontNode = curNode;
			}
			curNode = frontNode->next;
		}
	}
	return bRet;
}

bool ResetPrcList(prc_mon_info_node_t *head)
{
	bool bRet = false;
	if (head == NULL)
		return bRet;
	{
		prc_mon_info_node_t *curNode = head->next;
		while (curNode)
		{
			curNode->prcMonInfo.isValid = 0;
			curNode = curNode->next;
		}
	}
	return bRet = true;
}

prc_mon_info_node_t *FindPrcMonInfoNode(prc_mon_info_node_t *head, unsigned int prcID)
{
	prc_mon_info_node_t *findNode = NULL;
	if (head == NULL)
		return findNode;
	findNode = head->next;
	while (findNode)
	{
		if (findNode->prcMonInfo.prcID == prcID)
			break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

int InsertPrcMonInfoNode(prc_mon_info_node_t *head, prc_mon_info_t *prcMonInfo)
{
	int iRet = -1;
	prc_mon_info_node_t *newNode = NULL, *findNode = NULL;
	if (prcMonInfo == NULL || head == NULL)
		return iRet;
	findNode = FindPrcMonInfoNode(head, prcMonInfo->prcID);
	if (findNode == NULL && prcMonInfo->prcName)
	{
		newNode = (prc_mon_info_node_t *)calloc(1, sizeof(prc_mon_info_node_t));
		if (!newNode)
			return 1;
		if (prcMonInfo->prcName)
		{
			int prcNameLen = strlen(prcMonInfo->prcName);
			newNode->prcMonInfo.prcName = (char *)calloc(1,prcNameLen + 1);
			if (newNode->prcMonInfo.prcName)
			{
				memcpy(newNode->prcMonInfo.prcName, prcMonInfo->prcName, prcNameLen);
			}
		}
		if (prcMonInfo->ownerName)
		{
			int ownerNameLen = strlen(prcMonInfo->ownerName);
			newNode->prcMonInfo.ownerName = (char *)calloc(1, ownerNameLen + 1);
			if (newNode->prcMonInfo.ownerName)
			{
				memcpy(newNode->prcMonInfo.ownerName, prcMonInfo->ownerName, ownerNameLen);
			}
		}
		newNode->prcMonInfo.prcID = prcMonInfo->prcID;
		newNode->prcMonInfo.isValid = prcMonInfo->isValid;

		newNode->prcMonInfo.enGetStatus = prcMonInfo->enGetStatus;

		newNode->prcMonInfo.cpuUsage = 0;
		newNode->prcMonInfo.memUsage = 0;
		newNode->prcMonInfo.vmemUsage = 0;
		newNode->prcMonInfo.isActive = 0;
		newNode->prcMonInfo.prcCpuUsageLastTimes.lastKernelTime = 0;
		newNode->prcMonInfo.prcCpuUsageLastTimes.lastUserTime = 0;
		newNode->prcMonInfo.prcCpuUsageLastTimes.lastTime = 0;

		newNode->next = head->next;
		head->next = newNode;
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

int DeleteAllPrcMonInfoNode(prc_mon_info_node_t *head)
{
	int iRet = -1;
	prc_mon_info_node_t *delNode = NULL;
	if (head == NULL)
		return iRet;

	delNode = head->next;
	while (delNode)
	{
		head->next = delNode->next;
		if (delNode->prcMonInfo.prcName)
		{
			free(delNode->prcMonInfo.prcName);
			delNode->prcMonInfo.prcName = NULL;
		}
		if (delNode->prcMonInfo.ownerName)
		{
			free(delNode->prcMonInfo.ownerName);
			delNode->prcMonInfo.ownerName = NULL;
		}
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

void DestroyPrcMonInfoList(prc_mon_info_node_t *head)
{
	if (NULL == head)
		return;
	DeleteAllPrcMonInfoNode(head);
	free(head);
	head = NULL;
}

void UpdatePrcList_All(prc_mon_info_node_t *head, bool *isUserLogon, bool *bChangeFlag, bool bGatherStatus, char* listProcStatus)
{
	char procUserName[MAX_LOGON_USER_NAME_LEN] = {0};
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = (HANDLE)0;

	if (NULL == head)
		return;
	//printf("*****PMV2*****UpdatePrcList_All\r\n");
	*isUserLogon = true; //always
	hSnapshot = util_process_create_Toolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (util_process_Process32First(hSnapshot, &pe))
	{
		//printf("*****PMV2*****util_process_Process32First\r\n");
		while (true)
		{
			HANDLE procHandle = (HANDLE)0;
			memset(procUserName, 0, sizeof(procUserName));
			util_process_handles_get_withid(&procHandle, pe.th32ProcessID);
			if (procHandle)
			{
				util_process_username_get(procHandle, procUserName, sizeof(procUserName));
				//printf("*****PMV2*****ProcessUserName: %s\r\n", procUserName);
			}

			//if(strlen(procUserName) && CheckPrcUserFromList(logonUserList,procUserName,logonUserCnt, MAX_LOGON_USER_NAME_LEN))
			{
				prc_mon_info_node_t *findNode = NULL;
				findNode = FindPrcMonInfoNode(head, pe.th32ProcessID);
				if (findNode)
				{
					//printf("*****PMV2*****FindPrcMonInfoNode\r\n");
					findNode->prcMonInfo.isValid = 1;

					findNode->prcMonInfo.enGetStatus = false;
					if (bGatherStatus)
					{
						if (strstr(listProcStatus, findNode->prcMonInfo.prcName))
						{
							findNode->prcMonInfo.enGetStatus = true;
						}
					}
				}
				else
				{
					prc_mon_info_t prcMonInfo;
					int tmpLen = 0;
					memset(&prcMonInfo, 0, sizeof(prc_mon_info_t));
					prcMonInfo.isValid = 1;
					prcMonInfo.prcID = pe.th32ProcessID;
					tmpLen = strlen(pe.szExeFile);
					prcMonInfo.prcName = (char *)calloc(1, tmpLen + 1);
					if (prcMonInfo.prcName)
					{
						memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
					}
					tmpLen = strlen(procUserName);
					if (tmpLen)
					{
						prcMonInfo.ownerName = (char *)calloc(1, tmpLen + 1);
						if (prcMonInfo.ownerName)
						{
							memcpy(prcMonInfo.ownerName, procUserName, tmpLen);
						}
					}
					prcMonInfo.enGetStatus = false;
					if(bGatherStatus)
					{
						if (strstr(listProcStatus, prcMonInfo.prcName))
						{
							prcMonInfo.enGetStatus = true;
						}
					}
					
					//printf("UpdatePrcList_All/swPrcMonCS Enter\n");
					//printf("*****PMV2*****ProcessName: %s\r\n", prcMonInfo.prcName);
					InsertPrcMonInfoNode(head, &prcMonInfo);
					//printf("UpdatePrcList_All/swPrcMonCS Leave\n");

					if (prcMonInfo.prcName)
					{
						free(prcMonInfo.prcName);
						prcMonInfo.prcName = NULL;
					}
					if (prcMonInfo.ownerName)
					{
						free(prcMonInfo.ownerName);
						prcMonInfo.ownerName = NULL;
					}
					*bChangeFlag = true;
					//m_cntProcChanged++;
				}
			}
			if (procHandle)
				util_process_handles_close(procHandle);
			usleep(10000);
			pe.dwSize = sizeof(PROCESSENTRY32);
			if (util_process_Process32Next(hSnapshot, &pe) == false)
				break;
		}

		if (hSnapshot)
			util_process_close_Toolhelp32Snapshot_handle(hSnapshot);
	}
}

void UpdatePrcList_CheckUser(prc_mon_info_node_t *head, bool *isUserLogon, char *chkUserName, bool *bChangeFlag, bool bGatherStatus, char* listProcStatus)
{
	if (NULL == head || NULL == chkUserName)
		return;
	if (!strlen(chkUserName))
		return;
	{
		char procUserName[MAX_LOGON_USER_NAME_LEN] = {0};
		char logonUserList[MAX_LOGON_USER_CNT][MAX_LOGON_USER_NAME_LEN] = {0};
		int logonUserCnt = 0;
		bool isUserExist = false;
		util_process_get_logon_users((char *)logonUserList, &logonUserCnt, MAX_LOGON_USER_CNT, MAX_LOGON_USER_NAME_LEN);

		if (logonUserCnt > 0)
		{
			int i = 0;
			for (i = 0; i < logonUserCnt; i++)
			{
				//printf("*****PMV2*****LoginUsers: %s\r\n", logonUserList[i]);
				if (!strcmp(logonUserList[i], chkUserName))
				{
					isUserExist = true;
					break;
				}
			}
		}
		if (isUserExist)
		{
			PROCESSENTRY32 pe;
			HANDLE hSnapshot = (HANDLE)0;
			*isUserLogon = true;
			hSnapshot = util_process_create_Toolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			pe.dwSize = sizeof(PROCESSENTRY32);
			if (util_process_Process32First(hSnapshot, &pe))
			{
				while (true)
				{
					HANDLE procHandle = (HANDLE)0;
					memset(procUserName, 0, sizeof(procUserName));
					util_process_handles_get_withid(&procHandle, pe.th32ProcessID);
					if (procHandle)
					{
						util_process_username_get(procHandle, procUserName, sizeof(procUserName));
						//printf("*****PMV2*****ProcessUserName: %s\r\n", procUserName);
					}
					//printf("*****PMV2*****CheckUserName: %s\r\n", chkUserName);
					if (strlen(procUserName) && !strcmp(chkUserName, procUserName))
					{
						prc_mon_info_node_t *findNode = NULL;
						findNode = FindPrcMonInfoNode(head, pe.th32ProcessID);
						if (findNode)
						{
							findNode->prcMonInfo.isValid = 1;

							findNode->prcMonInfo.enGetStatus = false;
							if (bGatherStatus)
							{
								if (strstr(listProcStatus, findNode->prcMonInfo.prcName))
								{
									findNode->prcMonInfo.enGetStatus = true;
								}
							}
						}
						else
						{
							prc_mon_info_t prcMonInfo;
							int tmpLen = 0;
							memset(&prcMonInfo, 0, sizeof(prc_mon_info_t));
							prcMonInfo.isValid = 1;
							prcMonInfo.prcID = pe.th32ProcessID;
							tmpLen = strlen(pe.szExeFile);
							prcMonInfo.prcName = (char *)calloc(1, tmpLen + 1);
							if (prcMonInfo.prcName)
							{
								memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
							}
							tmpLen = strlen(procUserName);
							if (tmpLen)
							{
								prcMonInfo.ownerName = (char *)calloc(1, tmpLen + 1);
								if (prcMonInfo.ownerName)
								{
									memcpy(prcMonInfo.ownerName, procUserName, tmpLen);
								}
							}

							prcMonInfo.enGetStatus = false;
							if (bGatherStatus)
							{
								if (strstr(listProcStatus, prcMonInfo.prcName))
								{
									prcMonInfo.enGetStatus = true;
								}
							}
							//printf("UpdatePrcList_CheckUser/swPrcMonCS Enter\n");
							InsertPrcMonInfoNode(head, &prcMonInfo);
							//printf("UpdatePrcList_CheckUser/swPrcMonCS Leave\n");
							if (prcMonInfo.prcName)
							{
								free(prcMonInfo.prcName);
								prcMonInfo.prcName = NULL;
							}
							if (prcMonInfo.ownerName)
							{
								free(prcMonInfo.ownerName);
								prcMonInfo.ownerName = NULL;
							}
							*bChangeFlag = true;
							//m_cntProcChanged++;
						}
					}
					if (procHandle)
						util_process_handles_close(procHandle);
					usleep(10000);
					pe.dwSize = sizeof(PROCESSENTRY32);
					if (util_process_Process32Next(hSnapshot, &pe) == false)
						break;
				}
			}
			if (hSnapshot)
				util_process_close_Toolhelp32Snapshot_handle(hSnapshot);
		}
		else
			*isUserLogon = false;
	}
}

bool CheckPrcUserFromList(char *logonUserList, char *procUserName, int logonUserCnt, int maxLogonUserNameLen)
{
	bool bRet = false;
	int i = 0;
	if (logonUserList == NULL || procUserName == NULL || logonUserCnt == 0)
		return bRet;
	for (i = 0; i < logonUserCnt; i++)
	{
		if (!strcmp(&(logonUserList[i * maxLogonUserNameLen]), procUserName))
		{
			bRet = true;
			break;
		}
	}
	return bRet;
}

void UpdatePrcList_CheckAllUsers(prc_mon_info_node_t *head, bool *isUserLogon, bool *bChangeFlag, bool bGatherStatus, char* listProcStatus)
{
	if (NULL == head)
		return;
	{
		char procUserName[MAX_LOGON_USER_NAME_LEN] = {0};
		char logonUserList[MAX_LOGON_USER_CNT][MAX_LOGON_USER_NAME_LEN] = {0};
		int logonUserCnt = 0;

		util_process_get_logon_users((char *)logonUserList, &logonUserCnt, MAX_LOGON_USER_CNT, MAX_LOGON_USER_NAME_LEN);
		//printf("*****PMV2*****LoginUsers: %d/%s\r\n", logonUserCnt, logonUserList);
		if (logonUserCnt > 0)
		{
			PROCESSENTRY32 pe;
			HANDLE hSnapshot = (HANDLE)0;
			*isUserLogon = true;
			hSnapshot = util_process_create_Toolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			pe.dwSize = sizeof(PROCESSENTRY32);
			if (util_process_Process32First(hSnapshot, &pe))
			{
				//printf("*****PMV2*****util_process_Process32First\r\n");
				while (true)
				{
					HANDLE procHandle = (HANDLE)0;
					memset(procUserName, 0, sizeof(procUserName));
					util_process_handles_get_withid(&procHandle, pe.th32ProcessID);
					if (procHandle)
					{
						util_process_username_get(procHandle, procUserName, sizeof(procUserName));
						//printf("*****PMV2*****ProcessUserName: %s\r\n", procUserName);
					}
					//if(strlen(procUserName) && !strcmp(logonUserName, procUserName))
					if (strlen(procUserName) && CheckPrcUserFromList((char *)logonUserList, procUserName, logonUserCnt, MAX_LOGON_USER_NAME_LEN))
					{
						prc_mon_info_node_t *findNode = NULL;
						findNode = FindPrcMonInfoNode(head, pe.th32ProcessID);
						if (findNode)
						{
							findNode->prcMonInfo.isValid = 1;

							findNode->prcMonInfo.enGetStatus = false;
							if (bGatherStatus)
							{
								if (strstr(listProcStatus, findNode->prcMonInfo.prcName))
								{
									findNode->prcMonInfo.enGetStatus = true;
								}
							}
						}
						else
						{
							prc_mon_info_t prcMonInfo;
							int tmpLen = 0;
							memset(&prcMonInfo, 0, sizeof(prc_mon_info_t));
							prcMonInfo.isValid = 1;
							prcMonInfo.prcID = pe.th32ProcessID;
							tmpLen = strlen(pe.szExeFile);
							prcMonInfo.prcName = (char *)calloc(1, tmpLen + 1);
							if (prcMonInfo.prcName)
							{
								memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
							}
							tmpLen = strlen(procUserName);
							if (tmpLen)
							{
								prcMonInfo.ownerName = (char *)calloc(1, tmpLen + 1);
								if (prcMonInfo.ownerName)
								{
									memcpy(prcMonInfo.ownerName, procUserName, tmpLen);
								}
							}

							prcMonInfo.enGetStatus = false;
							if (bGatherStatus)
							{
								if (strstr(listProcStatus, prcMonInfo.prcName))
								{
									prcMonInfo.enGetStatus = true;
								}
							}
							//printf("UpdatePrcList_CheckAllUsers/swPrcMonCS Enter\n");
							InsertPrcMonInfoNode(head, &prcMonInfo);
							//printf("UpdatePrcList_CheckAllUsers/swPrcMonCS Leave\n");
							if (prcMonInfo.prcName)
							{
								free(prcMonInfo.prcName);
								prcMonInfo.prcName = NULL;
							}
							if (prcMonInfo.ownerName)
							{
								free(prcMonInfo.ownerName);
								prcMonInfo.ownerName = NULL;
							}
							*bChangeFlag = true;
							//m_cntProcChanged++;
						}

					}
					if (procHandle)
						util_process_handles_close(procHandle);
					usleep(10000);
					pe.dwSize = sizeof(PROCESSENTRY32);
					if (util_process_Process32Next(hSnapshot, &pe) == false)
						break;
				}
			}
			else
			{
				//printf("*****PMV2*****util_process_Process32First errno: %d(%s)\r\n", errno,strerror(errno));
			}

			if (hSnapshot)
				util_process_close_Toolhelp32Snapshot_handle(hSnapshot); //app_os_CloseHandle(hSnapshot);//Wei.Gang modified
		}
		else
		{
			*isUserLogon = false;
		}
	}
}

bool UpdateLogonUserPrcList(prc_mon_info_node_t *head, bool *isUserLogon, char *sysUserName, int gatherLevel, bool bGatherStatus, char* listProcStatus)
{
	bool bChangeFlag = false;
	if (NULL == head)
	{
		return bChangeFlag;
	}
	else
	{
		//printf("UpdateLogonUserPrcList/swPrcMonCS Enter\n");
		ResetPrcList(head);
		//printf("UpdateLogonUserPrcList/swPrcMonCS Leave\n");

		switch (gatherLevel)
		{
		case CFG_FLAG_SEND_PRCINFO_ALL:
			UpdatePrcList_All(head, isUserLogon, &bChangeFlag, bGatherStatus, listProcStatus);
			//printf("*****PMV2*****gatherLevel: %s\r\n", "ALL");
			break;
		case CFG_FLAG_SEND_PRCINFO_BY_USER:
			UpdatePrcList_CheckUser(head, isUserLogon, sysUserName, &bChangeFlag, bGatherStatus, listProcStatus);
			//printf("*****PMV2*****gatherLevel: %s\r\n", sysUserName);
			break;
		default: //CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS:
			UpdatePrcList_CheckAllUsers(head, isUserLogon, &bChangeFlag, bGatherStatus, listProcStatus);
			//printf("*****PMV2*****gatherLevel: %s\r\n", "AllUser");
			break;
		}

		//printf("UpdateLogonUserPrcList2/swPrcMonCS Enter\n");
		if (UpdatePrcList(head))
		{
			bChangeFlag = true;
		}
		//printf("UpdateLogonUserPrcList2/swPrcMonCS Leave\n");
	}
	return bChangeFlag;
}

bool GetSysCPUUsage(int *cpuUsage, sys_cpu_usage_time_t *pSysCpuUsageLastTimes)
{
	bool bRet = false;
	long long nowIdleTime = 0, nowKernelTime = 0, nowUserTime = 0;
	long long sysTime = 0, idleTime = 0;
	*cpuUsage = 0;
	if (cpuUsage == NULL || pSysCpuUsageLastTimes == NULL)
		return bRet;
	util_process_GetSystemTimes((FILETIME *)&nowIdleTime, (FILETIME *)&nowKernelTime, (FILETIME *)&nowUserTime);
	if (pSysCpuUsageLastTimes->lastUserTime == 0 && pSysCpuUsageLastTimes->lastKernelTime == 0 && pSysCpuUsageLastTimes->lastIdleTime == 0)
	{
		pSysCpuUsageLastTimes->lastIdleTime = nowIdleTime;
		pSysCpuUsageLastTimes->lastKernelTime = nowKernelTime;
		pSysCpuUsageLastTimes->lastUserTime = nowUserTime;
		return bRet;
	}

	sysTime = (nowKernelTime - pSysCpuUsageLastTimes->lastKernelTime) + (nowUserTime - pSysCpuUsageLastTimes->lastUserTime);
	idleTime = nowIdleTime - pSysCpuUsageLastTimes->lastIdleTime;

	if (sysTime)
	{
#ifdef WIN32
		*cpuUsage = (int)((sysTime - idleTime) * 100 / sysTime);
#else
		*cpuUsage = (int)((sysTime)*100 / (sysTime + idleTime));
#endif
		bRet = true;
	}

	pSysCpuUsageLastTimes->lastKernelTime = nowKernelTime;
	pSysCpuUsageLastTimes->lastUserTime = nowUserTime;
	pSysCpuUsageLastTimes->lastIdleTime = nowIdleTime;

	return bRet;
}

bool GetSysMemoryUsageKB(long *totalPhysMemKB, long *availPhysMemKB)
{
	MEMORYSTATUSEX memStatex;
	if (NULL == totalPhysMemKB || NULL == availPhysMemKB)
		return false;
	memStatex.dwLength = sizeof(memStatex);
	util_process_GlobalMemoryStatusEx(&memStatex);
#ifdef WIN32
	*totalPhysMemKB = (long)(memStatex.ullTotalPhys / DIV);
	*availPhysMemKB = (long)(memStatex.ullAvailPhys / DIV);
#else
	*totalPhysMemKB = (long)(memStatex.ullTotalPhys);
	*availPhysMemKB = (long)(memStatex.ullAvailPhys);
#endif
	return true;
}

#ifndef WIN32
void GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	struct timeval now;

	//Add code
	gettimeofday(&now, NULL);
	int sysTickHZ = sysconf(_SC_CLK_TCK);

	long long tmp_sec = now.tv_sec;
	long long tmp_usec = now.tv_usec;
	*((long long *)lpSystemTimeAsFileTime) = tmp_sec * sysTickHZ + (tmp_usec * sysTickHZ) / 1000000;
	return;
}
#endif

int GetProcessCPUUsageWithID(unsigned int prcID, prc_cpu_usage_time_t *pPrcCpuUsageLastTimes)
{
	HANDLE hPrc = (HANDLE)0;
	int prcCPUUsage = 0;
	bool bRet = false;
	long long nowKernelTime = 0, nowUserTime = 0, nowTime = 0, divTime = 0;
	long long creationTime = 0, exitTime = 0;
	SYSTEM_INFO sysInfo;
	int processorCnt = 1;
	if (pPrcCpuUsageLastTimes == NULL)
		return prcCPUUsage;
	util_process_GetSystemInfo(&sysInfo);
	if (sysInfo.dwNumberOfProcessors)
		processorCnt = sysInfo.dwNumberOfProcessors;
	util_process_handles_get_withid(&hPrc, prcID);
	if (hPrc == 0)
		return prcCPUUsage;
	GetSystemTimeAsFileTime((FILETIME *)&nowTime);
	bRet = util_process_GetProcessTimes(hPrc, (FILETIME *)&creationTime, (FILETIME *)&exitTime,
										(FILETIME *)&nowKernelTime, (FILETIME *)&nowUserTime);
	if (!bRet)
	{
		util_process_handles_close(hPrc);
		return prcCPUUsage;
	}
	if (pPrcCpuUsageLastTimes->lastKernelTime == 0 && pPrcCpuUsageLastTimes->lastUserTime == 0)
	{
		pPrcCpuUsageLastTimes->lastKernelTime = nowKernelTime;
		pPrcCpuUsageLastTimes->lastUserTime = nowUserTime;
		pPrcCpuUsageLastTimes->lastTime = nowTime;
		util_process_handles_close(hPrc);
		return prcCPUUsage;
	}
	divTime = nowTime - pPrcCpuUsageLastTimes->lastTime;

	prcCPUUsage = (int)((((nowKernelTime - pPrcCpuUsageLastTimes->lastKernelTime) + (nowUserTime - pPrcCpuUsageLastTimes->lastUserTime)) * 100 / divTime) / processorCnt);

	pPrcCpuUsageLastTimes->lastKernelTime = nowKernelTime;
	pPrcCpuUsageLastTimes->lastUserTime = nowUserTime;
	pPrcCpuUsageLastTimes->lastTime = nowTime;
	util_process_handles_close(hPrc);
	return prcCPUUsage;
}

bool GetProcessMemoryUsageKBWithID(unsigned int prcID, long *memUsageKB)
{
	bool bRet = false;
	if (!prcID || NULL == memUsageKB)
		return bRet;
	bRet = util_process_memoryusage_withid(prcID, memUsageKB);
	return bRet;
}

#ifdef VIRTUAL_MEM
bool GetProcessVirtualMemoryUsageKBWithID(DWORD prcID, long *memUsageKB)
{
	bool bRet = false;
	HANDLE hPrc = NULL;
	MEMORY_BASIC_INFORMATION mbi;
	DWORD dwStart = 0, dwEnd = 0;
	SYSTEM_INFO si;

#ifdef WIN32
	if (NULL == memUsageKB)
		return bRet;
	util_process_handles_get_withid(&hPrc, prcID);
	memset(&mbi, 0, sizeof(MEMORY_BASIC_INFORMATION));

	GetSystemInfo(&si);
	dwStart = (DWORD)si.lpMinimumApplicationAddress;
	dwEnd = (DWORD)si.lpMaximumApplicationAddress;
	*memUsageKB = 0;

	while (VirtualQueryEx(hPrc, (void *)dwStart, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == sizeof(MEMORY_BASIC_INFORMATION))
	{
		if (dwStart > dwEnd)
			break;
		if (!(mbi.State & MEM_FREE))
		{
			*memUsageKB += (long)(mbi.RegionSize / DIV);
		}
		dwStart = (DWORD)mbi.BaseAddress + mbi.RegionSize;
		bRet = true;
	}
	util_process_handles_close(hPrc);
#endif
	return bRet;
}
#endif

#ifdef WIN32
bool IsProcessActiveWithIDEx(DWORD prcID, bool* isActive, bool* isShutdown);
#else
bool IsProcessActiveWithIDEx(unsigned int prcID);
#endif

void GetPrcMonInfo(prc_mon_info_node_t *head)
{
	prc_mon_info_node_t *curNode;
	long prcMemUsage = -1;
#ifdef VIRTUAL_MEM
	long prcVMemUsage = -1;
#endif
	if (NULL == head)
		return;
	curNode = head->next;
	while (curNode)
	{
		//printf("GetPrcMonInfo/swPrcMonCS Enter\n");
		if (curNode->prcMonInfo.prcName)
		{
			//bool isActive = true;
			curNode->prcMonInfo.cpuUsage = GetProcessCPUUsageWithID(curNode->prcMonInfo.prcID, &curNode->prcMonInfo.prcCpuUsageLastTimes);
			//curNode->prcMonInfo.isActive = IsProcessActiveWithIDEx(curNode->prcMonInfo.prcID);
			
			if (curNode->prcMonInfo.enGetStatus)
			{
#ifdef WIN32
				bool isActive = true;
				bool isShutdown = false;
				IsProcessActiveWithIDEx(curNode->prcMonInfo.prcID, &isActive, &isShutdown);
				curNode->prcMonInfo.isActive = isActive;
#else
				curNode->prcMonInfo.isActive = IsProcessActiveWithIDEx(curNode->prcMonInfo.prcID);
#endif
			}

			if (GetProcessMemoryUsageKBWithID(curNode->prcMonInfo.prcID, &prcMemUsage))
			{
				curNode->prcMonInfo.memUsage = prcMemUsage;
			}
			else
			{
				curNode->prcMonInfo.memUsage = 0;
			}
#ifdef VIRTUAL_MEM
			if (GetProcessVirtualMemoryUsageKBWithID(curNode->prcMonInfo.prcID, &prcVMemUsage))
			{
				curNode->prcMonInfo.vmemUsage = prcVMemUsage;
			}
			else
			{
				curNode->prcMonInfo.vmemUsage = 0;
			}
#endif
		}
		curNode = curNode->next;
		//printf("GetPrcMonInfo/swPrcMonCS Leave\n");
		usleep(10000);
	}
}

#ifdef WIN32
bool AdjustPrivileges()
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

	if (!LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &luid))
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

bool ConverNativePathToWin32(char *nativePath, char *win32Path)
{
	bool bRet = false;
	if (NULL == nativePath || NULL == win32Path)
		return bRet;
	{
		char drv = 'A';
		char devName[3] = {drv, ':', '\0'};
		char tmpDosPath[MAX_PATH] = {0};
		while (drv <= 'Z')
		{
			devName[0] = drv;
			memset(tmpDosPath, 0, sizeof(tmpDosPath));
			if (QueryDosDeviceA(devName, tmpDosPath, sizeof(tmpDosPath) - 1) != 0)
			{
				if (strstr(nativePath, tmpDosPath))
				{
					strcat(win32Path, devName);
					strcat(win32Path, nativePath + strlen(tmpDosPath));
					bRet = true;
					break;
				}
			}
			drv++;
		}
	}
	return bRet;
}

bool GetTokenByName(HANDLE *hToken, char *prcName)
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
		if (stricmp(pe.szExeFile, prcName) == 0)
		{
			hPrc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
			bRet = OpenProcessToken(hPrc, TOKEN_ALL_ACCESS, hToken);
			CloseHandle(hPrc);
			break;
		}
	}
	if (hSnapshot)
		CloseHandle(hSnapshot);
	return bRet;
}

bool RunProcessAsUser(char *cmdLine, bool isAppNameRun, bool isShowWindow, unsigned long *newPrcID, char* errmsg)
{
	bool bRet = false;
	unsigned long errcode = -1;
	if (NULL == cmdLine)
	{
		if (errmsg)
			sprintf(errmsg, "command is empty!");
		if (newPrcID != NULL)
			*newPrcID = errcode;
		return bRet;
	}
	else
	{
		HANDLE hToken;
		if (!GetTokenByName(&hToken, "EXPLORER.EXE"))
		{
			if (errmsg)
				sprintf(errmsg, "OS not logged in!");
			if (newPrcID != NULL)
				*newPrcID = errcode;
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
			//AdjustPrivileges();
			if (isAppNameRun)
			{
				bRet = CreateProcessAsUserA(hToken, cmdLine, NULL, NULL, NULL,
											FALSE, dwCreateFlag, NULL, NULL, &si, &pi);
				errcode = GetLastError();
				if (!bRet && errcode == 1314)
				{
					bRet = CreateProcess(NULL, (char*)cmdLine, NULL, NULL, FALSE, dwCreateFlag, NULL, NULL, &si, &pi);
					errcode = GetLastError();
				}
			}
			else
			{
				
				bRet = CreateProcessAsUserA(hToken, NULL, cmdLine, NULL, NULL,
											FALSE, dwCreateFlag, NULL, NULL, &si, &pi);
				errcode = GetLastError();
				if (!bRet && errcode == 1314)
				{
					bRet = CreateProcess(NULL, (char*)cmdLine, NULL, NULL, FALSE, dwCreateFlag, NULL, NULL, &si, &pi);
					errcode = GetLastError();
				}
					
			}

			if (!bRet)
			{
				if (errmsg)
					sprintf(errmsg, "execute %s fail! error code: %d", cmdLine, errcode);
				else
					printf("execute %s fail! error code: %d\n", cmdLine, errcode);

				if (newPrcID != NULL)
					*newPrcID = errcode;
			}
			else
			{
				if (newPrcID != NULL)
					*newPrcID = pi.dwProcessId;
			}
			CloseHandle(hToken);
		}
	}
	return bRet;
}

unsigned int RestartProcessWithID(unsigned int prcID)
{
	unsigned int dwPrcID = 0;
	HANDLE hPrc = NULL;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = NULL;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
	{
		CloseHandle(hSnapshot);
		return dwPrcID;
	}
	while (true)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == FALSE)
			break;
		if (pe.th32ProcessID == prcID)
		{
			hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);

			if (hPrc == NULL)
			{
				DWORD dwRet = GetLastError();
				if (dwRet == 5)
				{
					if (AdjustPrivileges())
					{
						hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
					}
				}
			}
			if (hPrc)
			{
				char nativePath[MAX_PATH] = {0};
				char win32Path[MAX_PATH] = {0};
				if (GetProcessImageFileNameA(hPrc, nativePath, sizeof(nativePath)))
				{
					if (ConverNativePathToWin32(nativePath, win32Path))
					{
						TerminateProcess(hPrc, 0);
						{
							char cmdLine[BUFSIZ] = {0};
							unsigned int tmpPrcID = 0;
							sprintf(cmdLine, "%s", win32Path);
							//sprintf(cmdLine, "%s \"%s\"", "cmd.exe /c ", path);
							if (RunProcessAsUser(cmdLine, true, true, &tmpPrcID, NULL))
							//if(CreateProcess(cmdLine, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
							//if(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
							{
								dwPrcID = tmpPrcID;
							}
						}
					}
				}
				CloseHandle(hPrc);
			}
			break;
		}
	}
	if (hSnapshot)
		CloseHandle(hSnapshot);
	return dwPrcID;
}

bool KillProcessWithID(unsigned int prcID)
{
	bool bRet = false;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = NULL;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
	{
		CloseHandle(hSnapshot);
		return bRet;
	}
	while (TRUE)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == FALSE)
			break;
		if (pe.th32ProcessID == prcID)
		{
			HANDLE hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
			if (hPrc == NULL)
			{
				DWORD dwRet = GetLastError();
				if (dwRet == 5)
				{
					if (AdjustPrivileges())
					{
						hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
					}
				}
			}

			if (hPrc)
			{
				bRet = TerminateProcess(hPrc, 0); //asynchronous
				CloseHandle(hPrc);
			}

			break;
		}
	}
	CloseHandle(hSnapshot);
	return bRet;
}

BOOL RunProcessInfoHelperAsUser(char* cmdLine, char* prcUserName)
{
	BOOL bRet = FALSE;
	if (NULL == cmdLine || NULL == prcUserName) return bRet;
	{
		int errorNo = 0;
		BOOL isRun = TRUE;
		HANDLE hToken = NULL;
		HANDLE hPrc = NULL;
		PROCESSENTRY32 pe;
		HANDLE hSnapshot = NULL;

		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnapshot, &pe))
		{
			while (isRun)
			{
				pe.dwSize = sizeof(PROCESSENTRY32);
				isRun = Process32Next(hSnapshot, &pe);
				if (isRun)
				{
					if (_stricmp(pe.szExeFile, "EXPLORER.EXE") == 0)
					{
						hPrc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
						bRet = OpenProcessToken(hPrc, TOKEN_ALL_ACCESS, &hToken);
						if (bRet)
						{
							//bRet = ScreenshotAdjustPrivileges(hToken);
							//if(bRet)
							char tokUserName[128] = { 0 };
							util_process_username_get(hPrc, tokUserName, 128);
							if (!strcmp(prcUserName, tokUserName))
							{
								STARTUPINFO si;
								PROCESS_INFORMATION pi;
								DWORD dwCreateFlag = CREATE_NO_WINDOW;
								memset(&si, 0, sizeof(si));
								si.dwFlags = STARTF_USESHOWWINDOW;
								si.wShowWindow = SW_HIDE;
								si.cb = sizeof(si);
								memset(&pi, 0, sizeof(pi));
								bRet = FALSE;
								if (CreateProcessAsUserA(hToken, NULL, cmdLine, NULL, NULL,
									FALSE, dwCreateFlag, NULL, NULL, &si, &pi))
								{
									errorNo = GetLastError();
									bRet = TRUE;
									//isRun = FALSE;
									WaitForSingleObject(pi.hProcess, INFINITE);
									CloseHandle(pi.hProcess);
									CloseHandle(pi.hThread);
								}
								/*#ifndef DEBUG
																else
																{
																	ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###Create user process failed!\n",__FUNCTION__, __LINE__);
																}
								#endif*/
							}
							CloseHandle(hToken);
						}
						else
						{
							ProcessMonitorLog(Normal, "%s()[%d]###Get user token failed!\n", __FUNCTION__, __LINE__);
						}
						CloseHandle(hPrc);
					}
				}
			}
		}
		if (hSnapshot) CloseHandle(hSnapshot);
	}
	return bRet;
}

HANDLE GetProcessHandleWithID(DWORD prcID)
{
	HANDLE hPrc = NULL;
	hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, prcID);
	if (hPrc == NULL)
	{
		DWORD dwRet = GetLastError();
		if (dwRet == 5)
		{
			if (AdjustPrivileges())
			{
				hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, prcID);
			}
		}
	}
	return hPrc;
}

bool IsProcessActiveWithIDEx(DWORD prcID, bool* isActive, bool* isShutdown)
{
	bool bRet = true;
	if (prcID > 0) {
		HANDLE hMutex = NULL;
		HANDLE hFileMapping = NULL;
		LPVOID lpShareMemory = NULL;
		HANDLE hServerWriteOver = NULL;
		HANDLE hClientReadOver = NULL;
		msg_context_t* pShareBuf = NULL;
		SECURITY_ATTRIBUTES securityAttr;
		SECURITY_DESCRIPTOR secutityDes;

		HANDLE hPrc = NULL;
		char prcUserName[128] = { 0 };
		//SUSIAgentLog(ERROR, "---SoftwareMonitor S---");
		//FILE *fp = NULL;
		//if(fp = fopen("server_log.txt","a+"))
		//  fputs("Open ok!\n",fp);
		//create share memory 
		//LPSECURITY_ATTRIBUTES lpAttributes;

		if(!util_is_file_exist(g_ProcessInfoHelperFile))
			return false;

		InitializeSecurityDescriptor(&secutityDes, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&secutityDes, TRUE, NULL, FALSE);

		securityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		securityAttr.bInheritHandle = FALSE;
		securityAttr.lpSecurityDescriptor = &secutityDes;
		hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
			&securityAttr,
			PAGE_READWRITE,
			0,
			sizeof(msg_context_t),
			"Global\\Share-Mem-For-RMM-3-ID-0x0001");

		if (NULL == hFileMapping)
		{
			ProcessMonitorLog(Normal, "%s()[%d]###CreateFileMapping failed!\n", __FUNCTION__, __LINE__);
			goto SERVER_SHARE_MEMORY_END;
		}
		lpShareMemory = MapViewOfFile(hFileMapping,
			FILE_MAP_ALL_ACCESS,
			0,
			0,   //memory start address 
			0);   //all memory space 
		if (NULL == lpShareMemory)
		{
			ProcessMonitorLog(Normal, "%s()[%d]###MapViewOfFile failed!\n", __FUNCTION__, __LINE__);
			goto SERVER_SHARE_MEMORY_END;
		}
		pShareBuf = (msg_context_t*)lpShareMemory;
		memset(pShareBuf, 0, sizeof(msg_context_t));
		pShareBuf->procID = prcID;  //firefox for test.
		pShareBuf->isActive = true;
		pShareBuf->isShutdown = false;
		hPrc = GetProcessHandleWithID(prcID);
		if (hPrc)
		{
			util_process_username_get(hPrc, prcUserName, 128);

			RunProcessInfoHelperAsUser(g_ProcessInfoHelperFile, prcUserName);
		}

		bRet = *isActive = pShareBuf->isActive;
		*isShutdown = pShareBuf->isShutdown;
		if (hPrc) util_process_handles_close(hPrc);
	SERVER_SHARE_MEMORY_END:
		//if(fp) fclose(fp);
		if (NULL != lpShareMemory)   UnmapViewOfFile(lpShareMemory);
		if (NULL != hFileMapping)    CloseHandle(hFileMapping);
	}
	return bRet;
}
#else
#include <signal.h>
#include <sys/wait.h>

bool RunProcessAsUser(char *cmdLine, bool isAppNameRun, bool isShowWindow, unsigned long *newPrcID, char* errmsg)
{
	bool bRet = false;
	bRet = util_process_as_user_launch(cmdLine, isAppNameRun, isShowWindow, newPrcID);
	if (!bRet)
	{
		if (errmsg)
			sprintf(errmsg, "error code: %s  %d\n", cmdLine, errno);
		else
			printf("error code: %s  %d\n", cmdLine, errno);
		*newPrcID = errno;
	}
	return bRet;
}

unsigned int RestartProcessWithID(unsigned int prcID)
{
	unsigned int dwPrcID = 0;
	FILE *fp = NULL;
	char cmdLine[256];
	char cmdBuf[300];
	char file[128] = {0};
	char buf[1024] = {0};
	char logonUserName[32] = {0};
	pid_t retPid = -1;
	int i, status = 0;

	if (!prcID)
		return 0;

	sprintf(file, "/proc/%d/cmdline", prcID);
	if (!(fp = fopen(file, "r")))
	{
		printf("read %s file fail!\n", file);
		return dwPrcID;
	}
	if (fgets(buf, sizeof(buf), fp))
	{
		sscanf(buf, "%255s", cmdLine);
		fclose(fp);
		printf("cmd line is %s\n", cmdLine);
	}
	else
	{
		fclose(fp);
		return dwPrcID;
	}

	snprintf(cmdBuf, sizeof(cmdBuf), "which %s", cmdLine);
	if (!(fp = popen(cmdBuf, "r")))
	{
		fprintf(stderr, "poen [%s] fail!\n", cmdBuf);
		return dwPrcID;
	}
	if (fgets(buf, sizeof(buf), fp))
	{
		sscanf(buf, "%255s", cmdLine);
		pclose(fp);
		printf("cmd line after which is %s\n", cmdLine);
	}
	else
	{
		pclose(fp);
		return dwPrcID;
	}

	if (kill(prcID, SIGKILL) != 0)
		return dwPrcID;

	// waitpid
	for(i = 0; i < 30; i++) { // wait 30 * 100 ms.
		retPid = waitpid(prcID, &status, WNOHANG);
		if(retPid == 0) { // children still alive
			usleep(100000);
			continue;
		}
		break; // ignore child exit status
	}

	if (GetSysLogonUserName(logonUserName, sizeof(logonUserName)))
	{
		pid_t pid = fork();

		sleep(10);
		//fp = NULL;
		//sprintf(cmdBuf,"su - %s -c %s &",logonUserName,cmdLine);
		//sprintf(cmdBuf,"DISPLAY=:0 su -c %s %s &",cmdLine,logonUserName);
#ifdef ANDROID
		printf("RestartProcessWithID->\n");
		printf("cmdline=%s, username=%s\n", cmdLine, logonUserName);
		sprintf(cmdBuf, "%s &", cmdLine);
#else
		//sprintf(cmdBuf,"DISPLAY=:0 su -c 'xterm -e /bin/bash -c \"%s\"' %s &",cmdLine,logonUserName);
		sprintf(cmdBuf, "/usr/bin/sudo -H -u %s -- %s", logonUserName, cmdLine);
#endif
		if (pid == -1)
		{ // error
			perror("fork()");
			return dwPrcID;
		}
		else if (pid == 0)
		{ // children
			int uid;
			setpgid(0, 0);
			// get user id
			snprintf(cmdBuf, sizeof(cmdBuf), "id -u %s", logonUserName);
			if ((fp = popen(cmdBuf, "r")) == NULL)
			{
				fprintf(stderr, "error in popen(%s)\n", cmdBuf);
				exit(0);
			}
			if (!fgets(buf, sizeof(buf), fp))
			{
				fprintf(stderr, "error in fgets\n");
				pclose(fp);
				exit(0);
			}
			pclose(fp);

			// change to user
			uid = strtol(buf, NULL, 10);
			if (setuid(uid))
			{
				fprintf(stderr, "error in setuid\n");
				exit(0);
			}

			// set display to front screen
			setenv("DISPLAY", ":0", 1);
			// run user command

			fprintf(stderr, "cmdLine=[%s]\n", cmdLine);
			exit(execlp("/bin/sh", "sh", "-c", cmdLine, NULL));			

			// execl(cmdLine, cmdLine, NULL);
			// perror("execl()");
			// fprintf(stderr, "cmdLine=[%s]\n", cmdLine);
			// exit(0);
		}
		// parent
	}
	else
		printf("restart process failed,%s", cmdBuf);

	dwPrcID = getPIDByName(cmdLine);
	return dwPrcID;
}

bool KillProcessWithID(unsigned int prcID)
{
	int i, status = 0;
	if (!prcID)
		return false;
	if (kill(prcID, SIGKILL) != 0)
		return false;

		// waitpid
	for(i = 0; i < 30; i++) { // wait 30 * 100 ms.
		pid_t retPid = waitpid(prcID, &status, WNOHANG);
		if(retPid == 0) { // children still alive
			usleep(100000);
			continue;
		}
		break; // ignore child exit status
	}

	return true;
}

#define _POSIX_PATH_MAX 512

typedef struct statstruct_proc {
  int           pid;                      /** The process id. **/
  char          exName [_POSIX_PATH_MAX]; /** The filename of the executable **/
  char          state; /** 1 **/          /** R is running, S is sleeping, 
			   D is sleeping in an uninterruptible wait,
			   Z is zombie, T is traced or stopped **/
  unsigned      euid,                      /** effective user id **/
                egid;                      /** effective group id */					     
  int           ppid;                     /** The pid of the parent. **/
  int           pgrp;                     /** The pgrp of the process. **/
  int           session;                  /** The session id of the process. **/
  int           tty;                      /** The tty the process uses **/
  int           tpgid;                    /** (too long) **/
  unsigned int	flags;                    /** The flags of the process. **/
  unsigned int	minflt;                   /** The number of minor faults **/
  unsigned int	cminflt;                  /** The number of minor faults with childs **/
  unsigned int	majflt;                   /** The number of major faults **/
  unsigned int  cmajflt;                  /** The number of major faults with childs **/
  int           utime;                    /** user mode jiffies **/
  int           stime;                    /** kernel mode jiffies **/
  int		cutime;                   /** user mode jiffies with childs **/
  int           cstime;                   /** kernel mode jiffies with childs **/
  int           counter;                  /** process's next timeslice **/
  int           priority;                 /** the standard nice value, plus fifteen **/
  unsigned int  timeout;                  /** The time in jiffies of the next timeout **/
  unsigned int  itrealvalue;              /** The time before the next SIGALRM is sent to the process **/
  int           starttime; /** 20 **/     /** Time the process started after system boot **/
  unsigned int  vsize;                    /** Virtual memory size **/
  unsigned int  rss;                      /** Resident Set Size **/
  unsigned int  rlim;                     /** Current limit in bytes on the rss **/
  unsigned int  startcode;                /** The address above which program text can run **/
  unsigned int	endcode;                  /** The address below which program text can run **/
  unsigned int  startstack;               /** The address of the start of the stack **/
  unsigned int  kstkesp;                  /** The current value of ESP **/
  unsigned int  kstkeip;                 /** The current value of EIP **/
  int		signal;                   /** The bitmap of pending signals **/
  int           blocked; /** 30 **/       /** The bitmap of blocked signals **/
  int           sigignore;                /** The bitmap of ignored signals **/
  int           sigcatch;                 /** The bitmap of catched signals **/
  unsigned int  wchan;  /** 33 **/        /** (too long) **/
  int		sched, 		  /** scheduler **/
                sched_priority;		  /** scheduler priority **/
		
} procinfo;

int get_proc_info(unsigned int pid, procinfo* pinfo)
{
	char szFileName[_POSIX_PATH_MAX],
		szStatStr[2048],
		* s, * t;
	FILE* fp;
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

bool IsProcessActiveWithIDEx(unsigned int prcID)
{
	bool bRet = false;
	procinfo pinfo;
	
	if (get_proc_info(prcID, &pinfo)==0)
	{
		if (pinfo.state == 'R' || pinfo.state == 'S')
			bRet = true;
		//printf("[IsProcessActiveWithIDEx] pid %d is %c\n",m_procInfo.pid, m_procInfo.state);
	}
	//else
	//	printf("[IsProcessActiveWithIDEx] pid %d is invalid\n", prcID);
	return bRet;
}
#endif
