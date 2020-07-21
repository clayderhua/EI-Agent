/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								       */
/* Create Date  : 2014/10/28 by hailong.dang								          */
/* Modified Date: 2014/10/28 by hailong.dang								          */
/* Abstract     : Handler API                                     			 */
/* Reference    : None														             */
/****************************************************************************/
#include "susiaccess_handler_api.h"
#include "ScreenshotHandler.h"
#include "ScreenshotLog.h"
#include "Parser.h"
#include "base64.h"

#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "WISEPlatform.h"
#include "util_path.h"
#include "util_process.h"
//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct{
   pthread_t threadHandler;
   char sessionID[MAX_SESSION_LEN];
   bool isThreadRunning;
   bool threadSyncFlag;
}handler_context_t;

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
const char strPluginName[MAX_TOPIC_LEN] = MyTopic;
const int iRequestID = cagent_request_screenshot;
const int iActionID = cagent_reply_screenshot;
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Sample handler variables define
//-----------------------------------------------------------------------------
static Handler_info  g_PluginInfo;
static handler_context_t g_HandlerContex;
static void* g_loghandle = NULL;
static bool g_bEnableLog = true;
static char g_FlagCode = 0;
static HandlerSendCbf           g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf       g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf     g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf  g_subscribecustcbf = NULL;

//------------------------------User variables define--------------------------
#ifdef WIN32
#define DEF_SCREENSHOT_HELPER      "ScreenshotHelper.exe"
#elif defined ANDROID
#define DEF_SCREENSHOT_HELPER      "screencap"
#else
#define DEF_SCREENSHOT_HELPER      "ScreenshotHelper"
#endif
static bool IsHandlerStart = false;
static char TempPath[MAX_PATH] = {0};
static char ModulePath[MAX_PATH] = {0};
static char DevID[128]={0};
static char ScreenshotHelperPath[MAX_PATH] = {0};
static char ScreenshotHelperTempPath[MAX_PATH] = {0};
//-----------------------------------------------------------------------------

//------------------------------user func define-------------------------------
static bool ScreenshotRun(char * cmdLine);
static void* ScreenshotTransferThreadStart(void* args);
static void ScreenshotTransfer();
//-----------------------------------------------------------------------------

#ifdef ANDROID
typedef pthread_t sp_pthread_t;
static int pthread_cancel(sp_pthread_t thread) {
        return (kill(thread, SIGTERM));
}
#endif

void Handler_Uninitialize();
#ifdef _MSC_VER
bool WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		printf("DllInitializer\r\n");
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL) // Dynamic load
		{
			// Initialize your stuff or whatever
			// Return false if you don't want your module to be dynamically loaded
		}
		else // Static load
		{
			// Return false if you don't want your module to be statically loaded
			return false;
		}
	}

	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
	{
		printf("DllFinalizer\r\n");
		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize();
		}
		else // Process is terminating
		{
			// Cleanup
			Handler_Uninitialize();
		}
	}
	return true;
}
#else
__attribute__((constructor))
/**
 * initializer of the shared lib.
 */
static void Initializer(int argc, char** argv, char** envp)
{
    fprintf(stderr, "DllInitializer\r\n");
}

__attribute__((destructor))
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer()
{
    fprintf(stderr, "DllFinalizer\r\n");
	Handler_Uninitialize();
}
#endif

#ifdef WIN32
bool IsWorkstationLocked()
{
	// note: we can't call OpenInputDesktop directly because it's not
	// available on win 9x
	typedef HDESK (WINAPI *PFNOPENDESKTOP)(LPSTR lpszDesktop, DWORD dwFlags, bool fInherit, ACCESS_MASK dwDesiredAccess);
	typedef bool (WINAPI *PFNCLOSEDESKTOP)(HDESK hDesk);
	typedef bool (WINAPI *PFNSWITCHDESKTOP)(HDESK hDesk);

	// load user32.dll once only
	HMODULE hUser32 = LoadLibrary("user32.dll");

	if (hUser32)
	{
		PFNOPENDESKTOP fnOpenDesktop = (PFNOPENDESKTOP)GetProcAddress(hUser32, "OpenDesktopA");
		PFNCLOSEDESKTOP fnCloseDesktop = (PFNCLOSEDESKTOP)GetProcAddress(hUser32, "CloseDesktop");
		PFNSWITCHDESKTOP fnSwitchDesktop = (PFNSWITCHDESKTOP)GetProcAddress(hUser32, "SwitchDesktop");

		if (fnOpenDesktop && fnCloseDesktop && fnSwitchDesktop)
		{
			HDESK hDesk = fnOpenDesktop("Default", 0, false, DESKTOP_SWITCHDESKTOP);

			if (hDesk)
			{
				bool bLocked = !fnSwitchDesktop(hDesk);

				// cleanup
				fnCloseDesktop(hDesk);
				FreeLibrary(hUser32);
				return bLocked;
			}
		}
		FreeLibrary(hUser32);
	}

	// must be win9x
	return false;
}
#elif defined ANDROID
#else
bool GetGUILogonUserName(char *userNameBuf, unsigned int bufLen)
{
	int i = 0;
	FILE *fp = NULL;
	char cmdline[128] = {0};
	char cmdbuf[32] = {0};
	char display[32] = {0};

	if (userNameBuf == NULL || bufLen == 0)
		return false;

	sprintf(cmdline, "ls /tmp/.X11-unix/* | sed 's#/tmp/.X11-unix/X##' | head -n 1"); // Detect the name of the display in use
	fp = popen(cmdline, "r");
	if (NULL != fp)
	{
		if (fgets(cmdbuf, sizeof(cmdbuf), fp) == NULL)
		{ // if error
			cmdbuf[0] = '\0';
		}
		pclose(fp);
	}

	i = strlen(cmdbuf);
	if (i > 0)
	{
		char* idx = index(cmdbuf, '\r');
		if(idx == 0)
			idx = index(cmdbuf, '\n');
		if(idx > 0)
		{
			strncpy(display, cmdbuf, (int)idx-(int)cmdbuf);
		}
		else
			strcpy(display, cmdbuf);
	}
	else
		return false;

	memset(cmdline, 0, sizeof(cmdline));
	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdline, "who | grep '(:%s)' | awk '{ print $3, $4, $1 }' | sort | awk 'END { print $3 }' | tr -d '\n'", display); // get last logon user
	//ScreenshotLog(g_loghandle, Debug, "Check User %s", cmdline);
	fp = popen(cmdline, "r");
	if (NULL != fp)
	{
		if (fgets(cmdbuf, sizeof(cmdbuf), fp) == NULL)
		{ // if error
			cmdbuf[0] = '\0';
		}
		pclose(fp);
	}

	i = strlen(cmdbuf);
	if (i > 0 && i < bufLen)
		strcpy(userNameBuf, cmdbuf);
	else
		return false;
	ScreenshotLog(g_loghandle, Debug, "Login user %s (:%s)", userNameBuf, display);
	return true;
}

bool IsUserLogonWithGUI() 
{
	bool bRet = false;
	char logonUserName[32] = {0};
	if (GetGUILogonUserName(logonUserName, sizeof(logonUserName)))
	{
		bRet = true;
	}
 	return bRet;
}


#endif

static bool ScreenshotRun(char * cmdLine)
{
	bool bRet = false;
	char logonUserName[32] = {0};
	if(cmdLine == NULL) return bRet;
#ifdef ANDROID
	bRet = util_process_cmd_launch(cmdLine);
	return bRet;
#elif defined WIN32
	bRet = util_process_as_user_launch(cmdLine, false, false, NULL);
	if (!bRet)
	{
		HANDLE pHandle = util_process_cmd_launch(cmdLine);
		bRet = pHandle != NULL ? true : false;
		util_process_handles_close(pHandle);
	}
	return bRet;
#else
	if (GetGUILogonUserName(logonUserName, sizeof(logonUserName)))
	{
		FILE *fp = NULL;
		char cmdBuf[256] = {0};
		
		sprintf(cmdBuf, "su -c '%s' %s", cmdLine, logonUserName);

		if ((fp = popen(cmdBuf, "r")) == NULL)
		{
			pclose(fp);
			return false;
		}
		{
			char result[260];
			while (fgets(result, 260, fp) != NULL)
    			printf("%s", result);
		}
		pclose(fp);
		return true;
	}
	return false;
#endif
}

static void* ScreenshotTransferThreadStart(void* args)
{
#define DEF_BASE64_PER_SIZE    1200   //Common multiple of 6 and 8
	handler_context_t *pHandlerContex = (handler_context_t*)args;
	char* sessionID = pHandlerContex->sessionID;
	pHandlerContex->threadSyncFlag = false;
	if(strlen(ScreenshotHelperTempPath) == 0 || !util_is_file_exist(ScreenshotHelperTempPath))
	{
		//memset(ModulePath, 0, sizeof(ModulePath));
		//memset(TempPath, 0, sizeof(TempPath));
		//app_os_get_module_path(ModulePath);
		//util_temp_pathl_get(TempPath, sizeof(TempPath));
		if(strlen(TempPath)&&strlen(ModulePath))
		{
			memset(ScreenshotHelperPath, 0, sizeof(ScreenshotHelperPath));
#ifdef ANDROID
			util_path_combine(ScreenshotHelperPath, "/system/bin/", DEF_SCREENSHOT_HELPER);
#else
			util_path_combine(ScreenshotHelperPath, ModulePath, DEF_SCREENSHOT_HELPER);
#endif
			//sprintf(ScreenshotHelperPath, "%s%s", ModulePath, DEF_SCREENSHOT_HELPER);
			memset(ScreenshotHelperTempPath, 0, sizeof(ScreenshotHelperTempPath));
			util_path_combine(ScreenshotHelperTempPath, TempPath, DEF_SCREENSHOT_HELPER);
			//sprintf(ScreenshotHelperTempPath, "%s/%s", TempPath, DEF_SCREENSHOT_HELPER);
		}
		if(util_is_file_exist(ScreenshotHelperTempPath))
		{
			util_remove_file(ScreenshotHelperTempPath);
		}
		if(util_is_file_exist(ScreenshotHelperPath))
		{
			util_copy_file(ScreenshotHelperPath, ScreenshotHelperTempPath);
		}
	}
	if(pHandlerContex->isThreadRunning)
	{
		bool bRet = false;
		ScreenshotUploadRep screenshotUploadRep;
		memset(&screenshotUploadRep, 0, sizeof(ScreenshotUploadRep));
		strcpy(screenshotUploadRep.status, "False");
		if(strlen(ScreenshotHelperTempPath) && util_is_file_exist(ScreenshotHelperTempPath))
		{
			char cmdLine[512] = {0};
			char localFilePath[MAX_PATH] = {0};
			char screenshotFileName[64] = {0};
#ifdef ANDROID
			sprintf(screenshotFileName, "%s.png", DevID);
#else
			sprintf(screenshotFileName, "%s.jpg", DevID);
#endif
			//sprintf(localFilePath, "%s/%s", TempPath, screenshotFileName);
			util_path_combine(localFilePath, TempPath, screenshotFileName);
			//sprintf_s(cmdLine, sizeof(cmdLine), "cmd.exe /c \"%s\" %s", screenshotHelperTempPath, screenshotFileName);
#ifdef ANDROID
			sprintf(cmdLine, "\"%s\" %s", ScreenshotHelperTempPath, localFilePath);
#else
			sprintf(cmdLine, "\"%s\" %s", ScreenshotHelperTempPath, screenshotFileName);
#endif
#ifdef ANDROID
			if (!util_process_check("screencap"))
#elif defined WIN32 
			if (!IsWorkstationLocked() /*&& !util_process_check("LogonUI.exe")*/ && util_process_check("explorer.exe") && !util_process_check("rdpclip.exe"))
#else
			if (IsUserLogonWithGUI())
#endif
			{
				//bRet = util_process_as_user_launch(cmdLine, false, false, NULL);
				ScreenshotLog(g_loghandle, Normal, "cmdLine:%s, ScreenshotHelperTempPath:%s,localFilePath:%s", 
					cmdLine,ScreenshotHelperTempPath,localFilePath);
				bRet = ScreenshotRun(cmdLine);
				if(bRet)
				{
					int count = 3;
					while(!util_is_file_exist(localFilePath))
					{
						if(count <=0)
							break;
						count--;
						usleep(1000*1000);
					}
					if(util_is_file_exist(localFilePath))
					{
						FILE * fp = NULL;
						bRet = false;
						fp = fopen(localFilePath, "rb+");
						if(fp != NULL)
						{
							unsigned long fSize;
							fseek( fp, 0, SEEK_END );
							fSize = ftell(fp);
							fseek(fp, 0, SEEK_SET);
							if(fSize > 0)
							{
								int realReadLen = 0;
								char * base64Buf = NULL;
								int base64Len = 0;
								char srcBuf[DEF_BASE64_PER_SIZE] = {0};
								char * base64Str = NULL;
								int base64StrLen = fSize*8/6+8;
								base64Str = (char *)malloc(base64StrLen);
								memset(base64Str, 0, base64StrLen);
								bRet = true;
								while((realReadLen = fread(srcBuf, 1, sizeof(srcBuf), fp)) != 0)
								{
									int iRet = Base64Encode(srcBuf, realReadLen, &base64Buf, &base64Len);
									if(iRet == 0 && base64Buf != NULL)
									{
										if(strlen(base64Buf))
										{
											strcat(base64Str, base64Buf);
										}
										free(base64Buf);
										base64Buf = NULL;
									}
									else
									{
										bRet = false;
										break;
									}
									memset(srcBuf, 0, sizeof(srcBuf));
								}
								if(bRet && strlen(base64Str))
								{
									screenshotUploadRep.base64Str = base64Str;
									sprintf(screenshotUploadRep.uplFileName, "%s", screenshotFileName);
									memset(screenshotUploadRep.status, 0, sizeof(screenshotUploadRep.status));
									strcpy(screenshotUploadRep.status, "True");
									memset(screenshotUploadRep.uplMsg, 0, sizeof(screenshotUploadRep.uplMsg));
									sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot success!");
									{
										char * uploadRepJsonStr = NULL;
										int jsonStrlen = Parser_PackScrrenshotUploadRep(&screenshotUploadRep, sessionID, &uploadRepJsonStr);
										if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
										{
											g_sendcbf(&g_PluginInfo, screenshot_transfer_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
										}
										if(uploadRepJsonStr)free(uploadRepJsonStr);
									}
								}
								free(base64Str);
							}
							fclose(fp);
						}
						if(!bRet)
						{
							sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error!Base64 encode error!");
						}
					}
					else
					{
						ScreenshotLog(g_loghandle, Error, "%s File not ready!", localFilePath);
						sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error!");
					}
				}
				else
				{
					ScreenshotLog(g_loghandle, Error, "%s", "Screenshot error!");
					sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error!");
				}
			}
			else
			{
				if (util_process_check("rdpclip.exe"))
				{
					sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error! Not support login with RDP (Windows Remote Desktop)!");
				}
				else
				{
					sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error! OS not logged in!");
				}
				
			}
			if(util_is_file_exist(localFilePath))
			{
				util_remove_file(localFilePath);
			}
		}
		else
		{
			sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error!");
		}

		if(!bRet && strlen(screenshotUploadRep.uplMsg))
		{
			char * uploadRepJsonStr = NULL;
			int jsonStrlen = Parser_PackScrrenshotUploadRep(&screenshotUploadRep, sessionID, &uploadRepJsonStr);
			if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, screenshot_transfer_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
			}
			if(uploadRepJsonStr)free(uploadRepJsonStr);
		}
	}
	pHandlerContex->isThreadRunning = false;
	pthread_exit(0);
	return 0;
}

static void ScreenshotTransfer(char *sessionID)
{
	ScreenshotUploadRep screenshotUploadRep;
	memset(&screenshotUploadRep, 0, sizeof(ScreenshotUploadRep));
	strcpy(screenshotUploadRep.status, "False");

	if(!g_HandlerContex.isThreadRunning)
	{
		g_HandlerContex.isThreadRunning = true;
		g_HandlerContex.threadSyncFlag = true;
		if(sessionID && strlen(sessionID)>0)
			strncpy(g_HandlerContex.sessionID, sessionID, sizeof(g_HandlerContex.sessionID));
		else
			memset(g_HandlerContex.sessionID, 0, sizeof(g_HandlerContex.sessionID));
		if (pthread_create(&g_HandlerContex.threadHandler, NULL, ScreenshotTransferThreadStart, &g_HandlerContex) != 0)
		{
			g_HandlerContex.isThreadRunning = false;
			sprintf(screenshotUploadRep.uplMsg, "%s", "Create Screenshot thread failed!");
		}
		else
		{
			while(g_HandlerContex.threadSyncFlag && g_HandlerContex.isThreadRunning)
				usleep(10*1000);
		}
	}
	else
	{
		sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot running!");
	}
	if(strlen(screenshotUploadRep.uplMsg))
	{
		char * uploadRepJsonStr = NULL;
		int jsonStrlen = Parser_PackScrrenshotUploadRep(&screenshotUploadRep, sessionID, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, screenshot_transfer_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	if(pluginfo == NULL)
	{
		return handler_fail;
	}

	//g_HandlerContex.threadHandler = NULL;
	g_HandlerContex.isThreadRunning = false;
   g_HandlerContex.threadSyncFlag = false;
   if(g_bEnableLog)
	{
		/*char MonitorLogPath[MAX_PATH] = {0};
		path_combine(MonitorLogPath, pluginfo->WorkDir, DEF_LOG_NAME);
		printf(" %s> Log Path: %s", MyTopic, MonitorLogPath);
		g_loghandle = InitLog(MonitorLogPath);*/
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

	strcpy(DevID, g_PluginInfo.agentInfo->devId);

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf           = g_PluginInfo.sendcbf          = pluginfo->sendcbf;
	g_sendcustcbf       = g_PluginInfo.sendcustcbf      = pluginfo->sendcustcbf;
	g_subscribecustcbf  = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf     = g_PluginInfo.sendreportcbf    = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;

	memset(ModulePath, 0, sizeof(ModulePath));
	strncpy(ModulePath, pluginfo->WorkDir, sizeof(ModulePath));

	memset(TempPath, 0, sizeof(TempPath));
	util_temp_path_get(TempPath, sizeof(TempPath));
	
	return handler_success;
}

void Handler_Uninitialize()
{
	if(IsHandlerStart)
	{
		if(g_HandlerContex.isThreadRunning == true)
		{
			g_HandlerContex.isThreadRunning = false;
			pthread_cancel(g_HandlerContex.threadHandler);
			pthread_join(g_HandlerContex.threadHandler, NULL);
			g_HandlerContex.threadHandler = NULL;
		}
		g_HandlerContex.threadSyncFlag = false;
		IsHandlerStart = false;
	}
}
/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Status 
 *  Input :
 *  Output: char * pOutReply ( JSON )
 *  Return:  int  : Length of the status information in JSON format
 *                       :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus ) // JSON Format
{
	int len = 0; // Data length of the pOutReply 
	return len;
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
}


/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	bool bRet = false;
	if(IsHandlerStart)
	{
		bRet = true;
	}
	else
	{
		IsHandlerStart = true;
		{
			//app_os_get_module_path(ModulePath);
			//util_temp_pathl_get(TempPath, sizeof(TempPath));
			if(strlen(TempPath)&&strlen(ModulePath))
			{
				//sprintf(ScreenshotHelperPath,  "%s%s", ModulePath, DEF_SCREENSHOT_HELPER);
#ifdef ANDROID
				util_path_combine(ScreenshotHelperPath, "/system/bin/", DEF_SCREENSHOT_HELPER);
#else
				util_path_combine(ScreenshotHelperPath, ModulePath, DEF_SCREENSHOT_HELPER);
#endif
				//sprintf(ScreenshotHelperTempPath,  "%s/%s", TempPath, DEF_SCREENSHOT_HELPER);
				util_path_combine(ScreenshotHelperTempPath, TempPath, DEF_SCREENSHOT_HELPER);
			}
			if(util_is_file_exist(ScreenshotHelperTempPath))
			{
				remove(ScreenshotHelperTempPath);
			}
			if(util_is_file_exist(ScreenshotHelperPath))
			{
				util_copy_file(ScreenshotHelperPath, ScreenshotHelperTempPath);
				g_FlagCode = SCREENSHOT_FLAGCODE_INTERNAL;
			}
			else {
				g_FlagCode = SCREENSHOT_FLAGCODE_NONE;
			}
		}
		bRet = true;

	}

	if(!bRet)
	{
		Handler_Stop();
		return handler_fail;
	}
	else
	{
		return handler_success;
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Stop
 *  Description: Stop the handler
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	if(IsHandlerStart)
	{
		if(g_HandlerContex.isThreadRunning == true)
		{
			g_HandlerContex.isThreadRunning = false;
			pthread_join(g_HandlerContex.threadHandler, NULL);
			g_HandlerContex.threadHandler = NULL;
		}
		g_HandlerContex.threadSyncFlag = false;
		IsHandlerStart = false;
	}
	return handler_success;
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
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int cmdID = 0;
	char sessionID[MAX_SESSION_LEN] = {0};
	ScreenshotLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedCMDWithSessoinID(data, datalen, &cmdID, sessionID))
		return;
	switch(cmdID)
	{
	case screenshot_transfer_req:
		{
			ScreenshotTransfer(sessionID);
			break;
		}
	case screenshot_capability_req:
		{
			int len = 0;
			char * pOutReply = NULL;
			len = Parser_CreateCapabilityRep(g_FlagCode, & pOutReply);
			if (len > 0 && NULL != pOutReply ){
				g_sendcbf( & g_PluginInfo, screenshot_capability_rep, pOutReply, len + 1, NULL, NULL);
				free(pOutReply);
			}
			break;
		}
	default: 
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128] = {0};
			int jsonStrlen = 0;
			strcpy(errorStr, "Unknown cmd!");
			jsonStrlen = Parser_PackScrrenshotError(errorStr, sessionID, &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, screenshot_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);
			break;
		}
	}
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
	/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["all"],"commCmd":2053,"type":"WSN"}}*/
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

}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char * : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{
	int len = 0;
	* pOutReply = NULL;
	len = Parser_CreateCapabilityRep(g_FlagCode, pOutReply);
	return len;
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
