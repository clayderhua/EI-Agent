/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								       */
/* Create Date  : 2014/11/25 by li.ge								                */
/* Modified Date: 2014/11/25 by li.ge								                */
/* Abstract     : RemoteKVM  Handler API                                    */
/* Reference    : None														             */
/****************************************************************************/
#include "network.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "susiaccess_handler_api.h"
#include "RemoteKVMHandler.h"
#include "RemoteKVMLog.h"
#include "Parser.h"
#include "cJSON.h"
#include "kvmconfig.h"

#include "WISEPlatform.h"
#include "util_path.h"
#include "util_string.h"
#include "util_process.h"
#ifdef ANDROID
#define KVM_LINUX_OS_VERSION "na"
#endif
//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define DEF_HANDLER_NAME             "remote_kvm"
const char strPluginName[MAX_TOPIC_LEN] = {"remote_kvm"};
const int iRequestID = cagent_request_remote_kvm;
const int iActionID = cagent_reply_remote_kvm;
static Handler_info  g_PluginInfo;
static void* g_loghandle = NULL;
static bool g_bEnableLog = true;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;

kvm_capability_info_t cpbInfo;
susiaccess_kvm_conf_body_t kvm_conf;

#define  vncServerIP                         vncConnectParams.vnc_server_ip      
#define  vncServerPort                       vncConnectParams.vnc_server_port    
#define  vncPassword                         vncConnectParams.vnc_password 
 
#define  DEF_VNC_SERVER_PORT                 5902
#define  DEF_VNC_SERVER_PWD                  "na"
#define  DEF_VNC_FOLDER_NAME                 "VNC"
static pthread_t KVMGetConnectParamsThreadHandle = 0;
static BOOL IsKVMGetConnectParamsThreadRunning = FALSE;
#ifdef WIN32
#define  DEF_VNC_INI_FILE_NAME               "ultravnc.ini"
#define  DEF_VNC_SERVICE_NAME                "uvnc_sa30"
#define  DEF_VNC_SERVER_EXE_NAME             "winvnc.exe"
#define  DEF_VNC_CHANGE_PWD_EXE_NAME         "vncpwdchg.exe"
#else
#include <sys/wait.h>
#ifdef ANDROID
#define  DEF_VNC_SERVER_EXE_NAME             "androidvncserver"
#else
#define  DEF_VNC_SERVER_EXE_NAME             "x11vnc"
#endif
static int X11vncPid = 0;
#define KVM_LINUX_OS_SUPPORT_LIST		"CentOS6.5,CentOS7.6,Ubuntu16.04"
#define KVM_FUNC_TEST  strstr(KVM_LINUX_OS_SUPPORT_LIST, KVM_LINUX_OS_VERSION)
#endif
#define  DEF_CONFIG_FILE_NAME         "agent_config.xml"

#define  DEF_VNC_SERVER_INSTALL              0
#define  DEF_VNC_SERVER_START				 1
#define  DEF_VNC_SERVER_STOP                 2
#define  DEF_VNC_SERVER_UNINSTALL            3

char VNCServerPath[MAX_PATH] = {0};
char ConfigFilePath[MAX_PATH] = {0};
#ifdef WIN32
char VNCIniFilePath[MAX_PATH] = {0};
char VNCChangePwdExePath[MAX_PATH] = {0};
#endif

kvm_vnc_server_start_params* g_pkvmVNCServerStartParms = NULL;

static void InitRemoteKVMPath();
static void KVMGetConnectParams(kvm_vnc_server_start_params * pVNCStartServerParams);
#ifdef WIN32
static BOOL IsSvcRun(LPCTSTR  lpszSvcName);
static BOOL IsSvcStop(LPCTSTR  lpszSvcName);
static void VNCServiceAction(int action);
static BOOL GetIniValue(char * iniPath, char * keyWord, char * valueStr);
static BOOL GetVNCServerPort(unsigned int * srvPort);
static BOOL StopWinVNCServer();
static BOOL ChangeVNCPassword(char* vncPwd, kvm_vnc_server_start_params* pVNCStartServerParams);
static void GenVNCPassword(kvm_vnc_server_start_params * pVNCStartServerParams);
#else

#endif

static void CollectCpbInfo(kvm_capability_info_t * pCpbInfo);
static void GetCapability();

#ifdef WIN32
static void InitRemoteKVMPath()
{
	char modulePath[MAX_PATH] = {0};
	char vncPath[MAX_PATH] = {0};
	if(strlen(g_PluginInfo.WorkDir)<=0) return;
	strcpy(modulePath, g_PluginInfo.WorkDir);
	util_path_combine(vncPath, modulePath, DEF_VNC_FOLDER_NAME);

	util_path_combine(VNCIniFilePath, vncPath, DEF_VNC_INI_FILE_NAME);
	util_path_combine(VNCServerPath, vncPath, DEF_VNC_SERVER_EXE_NAME);
	util_path_combine(VNCChangePwdExePath, vncPath, DEF_VNC_CHANGE_PWD_EXE_NAME);

	util_path_combine(ConfigFilePath, modulePath, DEF_CONFIG_FILE_NAME);
	/*sprintf(VNCIniFilePath, "%s\\%s\\%s", modulePath, DEF_VNC_FOLDER_NAME, DEF_VNC_INI_FILE_NAME);
	sprintf(VNCServerPath, "%s\\%s\\%s", modulePath, DEF_VNC_FOLDER_NAME, DEF_VNC_SERVER_EXE_NAME);
	sprintf(VNCChangePwdExePath, "%s\\%s\\%s", modulePath, DEF_VNC_FOLDER_NAME, DEF_VNC_CHANGE_PWD_EXE_NAME);*/
}

bool VNCCreateProcess(const char * cmdLine, bool isShowWindow)
{
	HANDLE pHandle = NULL;
	bool bRet = false;
	/*STARTUPINFO si;
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
	if(CreateProcess(NULL, (char*)cmdLine, NULL, NULL, FALSE, dwCreateFlag, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		bRet = true;
	}
	return bRet;*/
	if(isShowWindow)
		bRet = util_process_as_user_launch(cmdLine, false, false, NULL);

	if(!bRet)
	{
		pHandle = util_process_cmd_launch(cmdLine);
		if(pHandle)
		{
			util_process_wait_handle(pHandle);
			bRet = true;
		}
	}
	
	pHandle = NULL;
	return bRet;
}

static void VNCServiceAction(int action)
{
	char cmdLine[BUFSIZ] = {0};

	switch(action)
	{
	case DEF_VNC_SERVER_INSTALL:
		{
			sprintf(cmdLine, "cmd.exe /c \"%s\"  %s", VNCServerPath, "-install");
			break;
		}
	case DEF_VNC_SERVER_UNINSTALL:
		{
			sprintf(cmdLine, "cmd.exe /c \"%s\"  %s", VNCServerPath, "-uninstall");
			break;
		}
	case DEF_VNC_SERVER_START:
		{
			sprintf(cmdLine, "cmd.exe /c \"%s\"  %s", VNCServerPath, "-startservice");
			break;
		}
	case DEF_VNC_SERVER_STOP:
		{
			sprintf(cmdLine, "cmd.exe /c \"%s\"  %s", VNCServerPath, "-stopservice");
			break;
		}
	}

	VNCCreateProcess(cmdLine, false);

	switch(action)
	{
	case DEF_VNC_SERVER_START:
		while(IsSvcRun(DEF_VNC_SERVICE_NAME) != 1)
		{
			usleep(100*1000);
		}
		break;
	case DEF_VNC_SERVER_STOP:
		while(IsSvcStop(DEF_VNC_SERVICE_NAME) != 1)
		{
			usleep(100*1000);
		}
		break;
	}
}

static BOOL IsSvcRun(LPCTSTR  lpszSvcName)
{	
	SERVICE_STATUS svcStatus = {0};
	return QueryServiceStatus(OpenService(OpenSCManager(NULL, NULL, GENERIC_READ), lpszSvcName, GENERIC_READ), &svcStatus) ? (svcStatus.dwCurrentState == SERVICE_RUNNING) : 0;	
}

static BOOL IsSvcStop(LPCTSTR  lpszSvcName)
{	
	SERVICE_STATUS svcStatus = {0};
	return QueryServiceStatus(OpenService(OpenSCManager(NULL, NULL, GENERIC_READ), lpszSvcName, GENERIC_READ), &svcStatus) ? (svcStatus.dwCurrentState == SERVICE_STOPPED) : 0;	
}

static BOOL GetIniValue(char * iniPath, char * keyWord, char * valueStr)
{
	BOOL bRet = FALSE;
	if(NULL == iniPath || NULL == keyWord || NULL == valueStr) return bRet;
	if(util_is_file_exist(iniPath))
	{
		FILE * pVNCIniFile = NULL;
		pVNCIniFile = fopen(iniPath, "rb");
		if(pVNCIniFile)
		{
			char lineBuf[512] = {0};

			while(!feof(pVNCIniFile))
			{
				memset(lineBuf, 0, sizeof(lineBuf));
				if(fgets(lineBuf, sizeof(lineBuf), pVNCIniFile))
				{
					if(strstr(lineBuf, keyWord))
					{
						char * word[16] = {NULL};
						char *buf = lineBuf;
						int i = 0, wordIndex = 0;
						char *token = NULL;
						while((word[i] = strtok_r(buf, "=", &token)) != NULL)
						{
							i++;
							buf=NULL; 
						}
						while(word[wordIndex] != NULL)
						{
							TrimStr(word[wordIndex]);
							wordIndex++;
						}
						if(wordIndex < 2) continue;
						if(!strcmp(keyWord, word[0]))
						{
							strcpy(valueStr, word[1]);
							bRet = TRUE;
							break;
						}
					}
				}
			}
			fclose(pVNCIniFile);
		}
	}
	return bRet;
}

static BOOL GetVNCServerPort(unsigned int * srvPort)
{
	BOOL bRet = FALSE;
	if(NULL == srvPort) return bRet;
	{
		char portStr[16] = {0};
		if(GetIniValue(VNCIniFilePath, "PortNumber", portStr))
		{
			if(strlen(portStr))
			{
				*srvPort = atoi(portStr);
				bRet = TRUE;
			}
		}
	}
	return bRet;
}

static BOOL ChangeVNCPassword(char* vncPwd, kvm_vnc_server_start_params* pVNCStartServerParams)
{
	BOOL bRet = FALSE;
	char cmdLine[BUFSIZ] = {0};
	bool isShowWindow = false;
	if(NULL == vncPwd) return bRet;

	if(pVNCStartServerParams->mode == 1)
	{
		//sprintf(cmdLine, "%s \"%s\"  %s", cmdLine, VNCChangePwdExePath, vncPwd);
		sprintf(cmdLine, "cmd.exe /c \"%s\"  %s", VNCChangePwdExePath, vncPwd);
	}
	else if(pVNCStartServerParams->mode == 3)
	{	
		char server_url[256] = {0};

		if( strlen(pVNCStartServerParams->vnc_server_start_repeater_ip)>0 )
			strncpy(server_url, pVNCStartServerParams->vnc_server_start_repeater_ip, sizeof(server_url));
		else
			strncpy(server_url, g_PluginInfo.ServerIP, sizeof(server_url));

		if(pVNCStartServerParams->need_change_password == 1)
		{
			//sprintf(cmdLine, "%s \"%s\" %s %d %s %d", cmdLine, VNCChangePwdExePath, vncPwd, pVNCStartServerParams->vnc_server_repeater_id, server_url, pVNCStartServerParams->vnc_server_repeater_port);
			sprintf(cmdLine, "cmd.exe /c \"%s\" %s %d %s %d", VNCChangePwdExePath, vncPwd, pVNCStartServerParams->vnc_server_repeater_id, server_url, pVNCStartServerParams->vnc_server_repeater_port);
		}
		else
		{
			//sprintf(cmdLine, "cmd.exe /c \"%s\" -autoreconnect -id:%d -connect %s:%d", VNCServerPath, pVNCStartServerParams->vnc_server_repeater_id, server_url, pVNCStartServerParams->vnc_server_repeater_port);
			sprintf(cmdLine, "cmd.exe /c \"%s\" -id:%d -connect %s:%d", VNCServerPath, pVNCStartServerParams->vnc_server_repeater_id, server_url, pVNCStartServerParams->vnc_server_repeater_port);
			isShowWindow = true;
		}
	}
	RemoteKVMLog(g_loghandle, Debug, "%s()[%d]###CMD: %s!\n", __FUNCTION__, __LINE__, cmdLine);
	if(!VNCCreateProcess(cmdLine, isShowWindow))
	{
		RemoteKVMLog(g_loghandle, Normal, "%s()[%d]###Create restore bat process failed!\n", __FUNCTION__, __LINE__);
	}
	return bRet;
}

static void GenVNCPassword(kvm_vnc_server_start_params * pVNCStartServerParams)
{
	char logonUserList[32][32] = { 0 };
	int logonUserCnt = 0;

	remote_kvm_handler_context_t *pRemoteKVMHandlerContext = (remote_kvm_handler_context_t *)&RemoteKVMHandlerContext; 

	if(!util_process_check(DEF_VNC_SERVER_EXE_NAME))
		pVNCStartServerParams->need_change_password = 1;

	util_process_get_logon_users((char*)logonUserList, &logonUserCnt, 32, 32);
	RemoteKVMLog(g_loghandle, Debug, "%s()[%d]Logon User Count: %d!\n", __FUNCTION__, __LINE__, logonUserCnt);
	if (logonUserCnt <= 0)
		pVNCStartServerParams->need_change_password = 1;

	if (util_process_check("rdpclip.exe"))
	{
		RemoteKVMLog(g_loghandle, Debug, "%s()[%d] rdpclip.exe exist!\n", __FUNCTION__, __LINE__);
		pVNCStartServerParams->need_change_password = 1;
	}

	if(pVNCStartServerParams->need_change_password == 1)
	{
		VNCServiceAction(DEF_VNC_SERVER_UNINSTALL);
		memset(pRemoteKVMHandlerContext->vncPassword, 0, sizeof(pRemoteKVMHandlerContext->vncPassword));
		GetRandomStr(pRemoteKVMHandlerContext->vncPassword, sizeof(pRemoteKVMHandlerContext->vncPassword));
		ChangeVNCPassword(pRemoteKVMHandlerContext->vncPassword, pVNCStartServerParams);
		VNCServiceAction(DEF_VNC_SERVER_INSTALL);
	}	
	else
	{
		ChangeVNCPassword(pRemoteKVMHandlerContext->vncPassword, pVNCStartServerParams);
	}
	
}

static void* KVMGetConnectParamsThreadStart(void* args)
{
	if(IsKVMGetConnectParamsThreadRunning)
	{
		remote_kvm_handler_context_t *pRemoteKVMHandlerContext = (remote_kvm_handler_context_t *)&RemoteKVMHandlerContext;
        kvm_vnc_server_start_params * pVNCStartServerParams = (kvm_vnc_server_start_params *)args;

		GenVNCPassword(pVNCStartServerParams);

		if(strlen(pRemoteKVMHandlerContext->vncServerIP) == 0)
		{
			//app_os_GetHostIP(pRemoteKVMHandlerContext->vncServerIP);
			network_ip_get(pRemoteKVMHandlerContext->vncServerIP, sizeof(pRemoteKVMHandlerContext->vncServerIP));
		}

		if(pRemoteKVMHandlerContext->vncServerPort == 0)
		{
			pRemoteKVMHandlerContext->vncServerPort = DEF_VNC_SERVER_PORT;
			GetVNCServerPort(&pRemoteKVMHandlerContext->vncServerPort);
		}

		{
			char * paramsRepJsonStr = NULL;
			kvm_vnc_connect_params * pConnectParams = &pRemoteKVMHandlerContext->vncConnectParams;
			int jsonStrlen = Parser_PackKVMGetConnectParamsRep(pConnectParams, &paramsRepJsonStr);
			if(jsonStrlen > 0 && paramsRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, kvm_get_connect_params_rep, paramsRepJsonStr, strlen(paramsRepJsonStr)+1, NULL, NULL);
			}
			if(paramsRepJsonStr)free(paramsRepJsonStr);	
		}
	}
	if(g_pkvmVNCServerStartParms)
	{
		free(g_pkvmVNCServerStartParms);
		g_pkvmVNCServerStartParms = NULL;
	}
	IsKVMGetConnectParamsThreadRunning = FALSE;
	pthread_exit(0);
	return 0;
}

static void KVMGetConnectParams(kvm_vnc_server_start_params * pVNCStartServerParams)
{
	if(!IsKVMGetConnectParamsThreadRunning)
	{
		IsKVMGetConnectParamsThreadRunning = TRUE;
		if(!g_pkvmVNCServerStartParms)
			g_pkvmVNCServerStartParms = malloc(sizeof(kvm_vnc_server_start_params));
		memset(g_pkvmVNCServerStartParms, 0, sizeof(kvm_vnc_server_start_params));
		memcpy(g_pkvmVNCServerStartParms, pVNCStartServerParams,  sizeof(kvm_vnc_server_start_params));

		if (pthread_create(&KVMGetConnectParamsThreadHandle, NULL, KVMGetConnectParamsThreadStart, g_pkvmVNCServerStartParms) != 0)
		{
			IsKVMGetConnectParamsThreadRunning = FALSE;
		}
		else
		{
			pthread_detach(KVMGetConnectParamsThreadHandle);
			KVMGetConnectParamsThreadHandle = NULL;
		}
	}
}
#else
static void InitRemoteKVMPath()
{
	char modulePath[MAX_PATH] = {0};
	/*if(!app_os_get_module_path(modulePath)) return;
	sprintf(VNCServerPath, "%s/%s/%s", modulePath, DEF_VNC_FOLDER_NAME, DEF_VNC_SERVER_EXE_NAME);*/
	char vncPath[MAX_PATH] = {0};
	if(strlen(g_PluginInfo.WorkDir)<=0) return;
	strcpy(modulePath, g_PluginInfo.WorkDir);
	util_path_combine(vncPath, modulePath, DEF_VNC_FOLDER_NAME);

	util_path_combine(VNCServerPath, vncPath, DEF_VNC_SERVER_EXE_NAME);

	util_path_combine(ConfigFilePath, modulePath, DEF_CONFIG_FILE_NAME);
}

static void KillX11Vnc()
{
	FILE *fp = NULL;
#ifdef ANDROID
	fp = popen("ps | grep 'androidvncserver' | sed 's/ [ ]*/:/g'| cut -d: -f2 | xargs kill -9", "r");
#else
	fp = popen("ps aux | grep 'x11vnc' | cut -c 9-15 | xargs kill -9", "r");
#endif
	pclose(fp);
}

static int RunX11vnc(char * x11vncPath, char * runParams)
{
	if(x11vncPath == NULL || runParams == NULL) return -1;
	{
		pid_t fpid = 0;
		char * word[32] = {NULL};
		char *buf = runParams;
		char *token = NULL;
		int i = 0;
#ifdef ANDROID
		word[i]="androidvncserver";
#else
		word[i]="x11vnc";
#endif
		i++;
		while((word[i] = strtok_r(buf, " ", &token)) != NULL)
		{
			i++;
			buf=NULL; 
		}
		if(i>1)
		{
			fpid = fork();
			if(fpid == 0)
			{
				execvp(x11vncPath, word);
			}
		}
		return fpid;
	}
}

static char * GetVncAuth()
{
	char *vncauth = NULL;
	char line[64] = {0};
	FILE *fp = NULL;

	fp = popen("ps wwaux | grep '/X.*-auth' | grep -v grep | sed -e 's/^.*-auth *//' -e 's/ .*$//' | head -n 1", "r");
	while(fgets(line, sizeof(line), fp) != NULL)
	{
		vncauth = strndup(line, strlen(line) - 1);
	}
	pclose(fp);

	return vncauth;
}

static int StartX11Vnc(kvm_vnc_server_start_params * pVNCStartServerParams)
{	
	if(pVNCStartServerParams == NULL)
		return -1;
	{
		remote_kvm_handler_context_t *pRemoteKVMHandlerContext = (remote_kvm_handler_context_t *)&RemoteKVMHandlerContext;
		int ret = 0;
		char runVncParams[128] = {0};
 		char * vncAuth = NULL;
 		vncAuth = GetVncAuth();
		switch(pVNCStartServerParams->mode)
		{
		case 1: //normal
			{
				if(vncAuth != NULL)
				{
					sprintf(runVncParams, "-display :0 -auth %s -rfbport %d -passwd %s -shared", 
						vncAuth, pRemoteKVMHandlerContext->vncServerPort, pRemoteKVMHandlerContext->vncPassword);
				} else { // error
					RemoteKVMLog(g_loghandle, Debug, "KVM error! OS not logged in!");
					return -2;
				}
				/*
				else if(pRemoteKVMHandlerContext->vncModeParams.vnc_mode && !strcasecmp(KVM_CUSTOM_MODE_STR, pRemoteKVMHandlerContext->vncModeParams.vnc_mode))
				{
					sprintf(runVncParams, "-rfbport %d -passwd %s", 
						pRemoteKVMHandlerContext->vncModeParams.custvnc_port, pRemoteKVMHandlerContext->vncModeParams.custvnc_pwd);
				}
				else
				{
					sprintf(runVncParams, "-rfbport %d -passwd %s", 
						pRemoteKVMHandlerContext->vncServerPort, pRemoteKVMHandlerContext->vncPassword);
				}
				*/
				break;
			}
		case 2://listen ./x11vnc -connect 172.21.73.71:5901
			{
				if(vncAuth != NULL)
				{
					sprintf(runVncParams, "-display :0 -auth %s -connect %s:%d", 
						vncAuth, pVNCStartServerParams->vnc_server_start_listen_ip, pVNCStartServerParams->vnc_server_listen_port);
				}
				else
				{
					sprintf(runVncParams, "-connect %s:%d", 
						pVNCStartServerParams->vnc_server_start_listen_ip, pVNCStartServerParams->vnc_server_listen_port);
				}
				break;
			}
		case 3://repeater ./x11vnc -connect repeater://172.21.73.148:5501+ID:2222
			{
				char server_url[256] = {0};
				if( strlen(pVNCStartServerParams->vnc_server_start_repeater_ip)>0 )
					strncpy(server_url, pVNCStartServerParams->vnc_server_start_repeater_ip, sizeof(server_url));
				else
					strncpy(server_url, g_PluginInfo.ServerIP, sizeof(server_url));
				if(vncAuth != NULL)
				{
					if(strstr(KVM_LINUX_OS_VERSION, "CentOS"))
					{
						sprintf(runVncParams, "-display :0 -connect repeater://%s:%d+ID:%d -passwd %s", server_url, pVNCStartServerParams->vnc_server_repeater_port, pVNCStartServerParams->vnc_server_repeater_id, pRemoteKVMHandlerContext->vncPassword);
					}
					else 
					{
						sprintf(runVncParams, "-display :0 -auth %s -connect repeater://%s:%d+ID:%d -passwd %s", vncAuth, server_url, pVNCStartServerParams->vnc_server_repeater_port, pVNCStartServerParams->vnc_server_repeater_id, pRemoteKVMHandlerContext->vncPassword);
					}
				} else { // error
					RemoteKVMLog(g_loghandle, Debug, "KVM error! OS not logged in!");
					return -2;
				}
				break;
			}
		default: 
			break;
		}
		if(vncAuth) free(vncAuth);
		RemoteKVMLog(g_loghandle, Debug,"VNCServerPath:%s, runVncParams:%s, pVNCStartServerParams->mode:%d",VNCServerPath, runVncParams,pVNCStartServerParams->mode);
		ret = RunX11vnc(VNCServerPath, runVncParams);
		return ret;
	}
}

static void* KVMGetConnectParamsThreadStart(void* args)
{
	if(IsKVMGetConnectParamsThreadRunning)
	{
		remote_kvm_handler_context_t *pRemoteKVMHandlerContext = (remote_kvm_handler_context_t *)&RemoteKVMHandlerContext;
		kvm_vnc_server_start_params * pVNCStartServerParams = (kvm_vnc_server_start_params *)args;
		if(pVNCStartServerParams->need_change_password == 1)
		{
			//KillX11Vnc();
			memset(pRemoteKVMHandlerContext->vncPassword, 0, sizeof(pRemoteKVMHandlerContext->vncPassword));
			GetRandomStr(pRemoteKVMHandlerContext->vncPassword, sizeof(pRemoteKVMHandlerContext->vncPassword));
		}
		if(strlen(pRemoteKVMHandlerContext->vncServerIP) == 0)
		{
			//strcpy(pRemoteKVMHandlerContext->vncServerIP, "172.21.73.200");
			//app_os_GetHostIP(pRemoteKVMHandlerContext->vncServerIP);
			network_ip_get(pRemoteKVMHandlerContext->vncServerIP, sizeof(pRemoteKVMHandlerContext->vncServerIP));
			printf("ip:%s\n", pRemoteKVMHandlerContext->vncServerIP);
		}
		X11vncPid = StartX11Vnc(pVNCStartServerParams);
		if(X11vncPid > 0)
		{
			char * paramsRepJsonStr = NULL;
			kvm_vnc_connect_params * pConnectParams = &pRemoteKVMHandlerContext->vncConnectParams;
			int jsonStrlen = Parser_PackKVMGetConnectParamsRep(pConnectParams, &paramsRepJsonStr);
			int wPid = 0;

			if(jsonStrlen > 0 && paramsRepJsonStr != NULL) {
				g_sendcbf(&g_PluginInfo, kvm_get_connect_params_rep, paramsRepJsonStr, strlen(paramsRepJsonStr)+1, NULL, NULL);
			}
			if(paramsRepJsonStr)
				free(paramsRepJsonStr);	
		
			wPid = waitpid(X11vncPid,NULL, WNOHANG);
			if(wPid == X11vncPid)
				X11vncPid = 0;
		} else if(X11vncPid == -2) {
			KVMSendError("OS not logged in!");
		} else { // unknown error
			KVMSendError("Unknown error!");
		}
	}
	if(g_pkvmVNCServerStartParms)
	{
		free(g_pkvmVNCServerStartParms);
		g_pkvmVNCServerStartParms = NULL;
	}
	IsKVMGetConnectParamsThreadRunning = FALSE;
	return 0;
}

static void KVMGetConnectParams(kvm_vnc_server_start_params * pVNCStartServerParams)
{
	if(!IsKVMGetConnectParamsThreadRunning)
	{
		if(!g_pkvmVNCServerStartParms)
			g_pkvmVNCServerStartParms = malloc(sizeof(kvm_vnc_server_start_params));
		memset(g_pkvmVNCServerStartParms, 0, sizeof(kvm_vnc_server_start_params));
		memcpy(g_pkvmVNCServerStartParms, pVNCStartServerParams,  sizeof(kvm_vnc_server_start_params));

		KillX11Vnc();

		/*if(X11vncPid > 0)
		{
			int wPid = 0;
			kill(X11vncPid, SIGKILL);
			wPid = waitpid(X11vncPid,NULL, WNOHANG);
			//if(wPid == X11vncPid) X11vncPid = 0;
			X11vncPid = 0;
		}*/
		IsKVMGetConnectParamsThreadRunning = TRUE;
		if (pthread_create(&KVMGetConnectParamsThreadHandle, NULL, KVMGetConnectParamsThreadStart, g_pkvmVNCServerStartParms) != 0)
		{
			printf("KVMGetConnectParamsThreadStart failed!\n");
			IsKVMGetConnectParamsThreadRunning = FALSE;
		}
		else
		{
			pthread_detach(KVMGetConnectParamsThreadHandle);
			printf("KVMGetConnectParamsThreadStart ok!\n");
		}
	}
}
#endif

void KVMSendError(char* msg)
{
	char* errorRepJsonStr = NULL;
	char errorStr[128];
	int jsonStrlen = 0;
	strncpy(errorStr, msg, sizeof(errorStr) - 1);
	jsonStrlen = Parser_PackKVMErrorRep(errorStr, &errorRepJsonStr);
	if (jsonStrlen > 0 && errorRepJsonStr != NULL) {
		g_sendcbf(&g_PluginInfo, kvm_error_rep, errorRepJsonStr, strlen(errorRepJsonStr) + 1, NULL, NULL);
	}
	if (errorRepJsonStr)
		free(errorRepJsonStr);
}

void GetVNCMode() 
{
	char * paramsRepJsonStr = NULL;
	int jsonStrlen = 0;
	/*susiaccess_kvm_conf_body_t kvm_conf;
	memset(&kvm_conf, 0, sizeof(susiaccess_kvm_conf_body_t));
	kvm_load(ConfigFilePath, &kvm_conf);*/
	jsonStrlen = Parser_PackVNCModeParamsRep(&kvm_conf, &paramsRepJsonStr);
	if(jsonStrlen > 0 && paramsRepJsonStr != NULL)
	{
		g_sendcbf(&g_PluginInfo, kvm_get_vnc_mode_rep, paramsRepJsonStr, strlen(paramsRepJsonStr)+1, NULL, NULL);
	}
	if(paramsRepJsonStr)free(paramsRepJsonStr);	
}

static void CollectCpbInfo(kvm_capability_info_t * pCpbInfo)
{
	if(NULL == pCpbInfo) return;
	{
		//add code about ssh
		//susiaccess_kvm_conf_body_t kvm_conf;
		memset(&kvm_conf, 0, sizeof(susiaccess_kvm_conf_body_t));
		kvm_load(ConfigFilePath, &kvm_conf);
		strcpy(pCpbInfo->vncMode, kvm_conf.kvmMode);
		strcpy(pCpbInfo->vncPwd, kvm_conf.custVNCPwd);
		pCpbInfo->vncPort = (unsigned int)atoi(kvm_conf.custVNCPort);
		if(!strcasecmp(KVM_DEFAULT_MODE_STR, pCpbInfo->vncMode))
		{
#ifdef WIN32
			sprintf(pCpbInfo->funcsStr, "%s,%s,%s", KVM_DEFAULT_MODE_STR, KVM_REPEATER_FUNC_STR, KVM_SPEEDUP_FUNC_STR);
			pCpbInfo->funcsCode = KVM_DEFAULT_FUNC_FLAG|KVM_REPEATER_FUNC_FLAG|KVM_SPEEDUP_FUNC_FLAG;
#else
			if(KVM_FUNC_TEST > 0)
			{
				sprintf(pCpbInfo->funcsStr, "%s,%s,%s", KVM_DEFAULT_MODE_STR, KVM_REPEATER_FUNC_STR, KVM_SPEEDUP_FUNC_STR);
				pCpbInfo->funcsCode = KVM_DEFAULT_FUNC_FLAG|KVM_REPEATER_FUNC_FLAG|KVM_SPEEDUP_FUNC_FLAG;
			}
			else
			{
				sprintf(pCpbInfo->funcsStr, "%s,%s", KVM_DEFAULT_MODE_STR, KVM_REPEATER_FUNC_STR);
				pCpbInfo->funcsCode = KVM_DEFAULT_FUNC_FLAG|KVM_REPEATER_FUNC_FLAG;
			}
#endif
		}
		else if(!strcasecmp(KVM_CUSTOM_MODE_STR, pCpbInfo->vncMode))
		{
			sprintf(pCpbInfo->funcsStr, "%s", KVM_CUSTOM_MODE_STR);
			pCpbInfo->funcsCode = KVM_CUSTOM_FUNC_FLAG;
		}
		else if(!strcasecmp(KVM_DISABLE_MODE_STR, pCpbInfo->vncMode))
		{
			sprintf(pCpbInfo->funcsStr, "%s", KVM_DISABLE_MODE_STR);
			pCpbInfo->funcsCode = KVM_NONE_FUNC_FLAG;
		}
//RemoteKVMLog(g_loghandle, Debug,"KVMCapability< Mode:%s, custVNCPwd:%s, vncPort:%d",pCpbInfo->vncMode, pCpbInfo->vncPwd, pCpbInfo->vncPort);
	}
}

static void GetCapability()
{
	char * cpbStr = NULL;
	int jsonStrlen = 0;
	/*kvm_capability_info_t cpbInfo;
	memset((char*)&cpbInfo, 0, sizeof(kvm_capability_info_t));
	CollectCpbInfo(&cpbInfo);*/
	jsonStrlen = Parser_PackCpbInfo(&cpbInfo, &cpbStr);
	if(jsonStrlen > 0 && cpbStr != NULL)
	{
		char * repJsonStr = NULL;
		jsonStrlen = Parser_PackSpecInfoRep(cpbStr, DEF_HANDLER_NAME, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, kvm_get_capability_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			if(repJsonStr)free(repJsonStr);
		}
		if(cpbStr)free(cpbStr);
	}
	else
	{
		KVMSendError("Get capability error!");
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  handler_success  : Success Init Handler
 *           handler_fail : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	if(pluginfo == NULL)
	{
		return handler_fail;
	}

	if(g_bEnableLog)
	{
		g_loghandle = pluginfo->loghandle;
	}

	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", strPluginName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" >Name: %s\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;

	InitRemoteKVMPath();

	memset(RemoteKVMHandlerContext.vncPassword, 0, sizeof(RemoteKVMHandlerContext.vncPassword));
	strcpy(RemoteKVMHandlerContext.vncPassword, DEF_VNC_SERVER_PWD);
	{
		//susiaccess_kvm_conf_body_t kvm_conf;
		/*memset(&kvm_conf, 0, sizeof(susiaccess_kvm_conf_body_t));
		kvm_load(ConfigFilePath, &kvm_conf);*/
		memset((char*)&cpbInfo, 0, sizeof(kvm_capability_info_t));
		CollectCpbInfo(&cpbInfo);
		RemoteKVMHandlerContext.vncServerPort= atoi(kvm_conf.custVNCPort);
	}
	//RemoteKVMLog(g_loghandle, Debug,"RemoteKVMHandlerContext< vncPassword:%s, vncServerPort:%d>",RemoteKVMHandlerContext.vncPassword, RemoteKVMHandlerContext.vncServerPort);

	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
 *  Input :  None
 *  Output: char * : pOutStatus       // cagent handler status
 *  Return:  handler_success  : Success Init Handler
 *			 handler_fail : Fail Init Handler
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	return handler_success;
}


/* **************************************************************************************
 *  Function Name: Handler_OnStatusChange
 *  Description: Agent can notify handler the status is changed.
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  None
 * ***************************************************************************************/
void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	printf(" %s> Update Status", strPluginName);
	if(pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strPluginName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
	}
	if(pluginfo->agentInfo->status == AGENT_STATUS_ONLINE)
	{
		//GetVNCMode();
	}
}


/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  handler_success : Success to Start Handler
 *           handler_fail : Fail to Start Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	kvm_vnc_server_start_params kvmVNCServerStartParms;

	if(g_PluginInfo.agentInfo->status == AGENT_STATUS_ONLINE)
	{
		GetVNCMode();
	}


RemoteKVMLog(g_loghandle, Debug,"cpbInfo.funcsCode %d\n", cpbInfo.funcsCode);
	if((cpbInfo.funcsCode & KVM_SPEEDUP_FUNC_FLAG) != 0)
	{
		
		memset(&kvmVNCServerStartParms, 0, sizeof(kvm_vnc_server_start_params));
		kvmVNCServerStartParms.need_change_password=1;
		kvmVNCServerStartParms.mode=1;
		KVMGetConnectParams(&kvmVNCServerStartParms);
	}
	
	return handler_success;
}


/* **************************************************************************************
 *  Function Name: Handler_Stop
 *  Description: Stop the handler
 *  Input :  None
 *  Output: None
 *  Return:  handler_success : Success to Stop
 *           handler_fail: Fail to Stop handler
 * ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	BOOL bRet = TRUE;
	memset(&RemoteKVMHandlerContext, 0, sizeof(remote_kvm_handler_context_t));
#ifndef WIN32
	KillX11Vnc();
#endif
	return bRet;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input : char * const topic, 
 *			void* const data, 
 *			const size_t datalen
 *  Output: void *pRev1, 
 *			void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int commCmd = unknown_cmd;

	RemoteKVMLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &commCmd))
		return;

	switch(commCmd)
	{
	case kvm_get_capability_req:
		{
			GetCapability();
			break;
		}
	case kvm_get_vnc_mode_req:
		{	
			GetVNCMode();
			break;
		}
	case kvm_get_connect_params_req:
		{
			kvm_vnc_server_start_params kvmVNCServerStartParms;
			memset(&kvmVNCServerStartParms, 0, sizeof(kvm_vnc_server_start_params));
			if(ParseKVMRecvCmd((char*)data, g_PluginInfo.ServerIP, g_PluginInfo.ServerPort, &kvmVNCServerStartParms))
			{
				KVMGetConnectParams(&kvmVNCServerStartParms);
			}
			else
			{
				KVMSendError("kvm_get_connect_params_req parsing error!");
			}
			break;
		}
	default: 
		{
			KVMSendError("Unknown cmd!");
			break;
		}
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char * : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	int len = 0; // Data length of the pOutReply 
	char * cpbStr = NULL;
	int jsonStrlen = 0;
	/*kvm_capability_info_t cpbInfo;
	memset((char*)&cpbInfo, 0, sizeof(kvm_capability_info_t));
	CollectCpbInfo(&cpbInfo);*/
	jsonStrlen = Parser_PackCpbInfo(&cpbInfo, &cpbStr);
	if(jsonStrlen>0 && cpbStr != NULL)
	{
		char * repJsonStr = NULL;
		int jsonStrlen = Parser_PackSpecInfoRep(cpbStr, DEF_HANDLER_NAME, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			len = strlen(repJsonStr);
			*pOutReply = (char *)malloc(len + 1);
			memset(*pOutReply, 0, len + 1);
			strcpy(*pOutReply, repJsonStr);
		}
		if(repJsonStr)free(repJsonStr);
	}
	if(cpbStr) free(cpbStr);
	return len;
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	return;
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : char *pInQuery
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	return;
}

/* **************************************************************************************
 *  Function Name: Handler_MemoryFree
 *  Description: free the mamory allocated for Handler_Get_Capability
 *  Input : char *pInData.
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_MemoryFree(char *pInData)
{
	if(pInData)
	{
		free(pInData);
		pInData = NULL;
	}
}
