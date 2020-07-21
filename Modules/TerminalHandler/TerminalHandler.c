/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								       */
/* Create Date  : 2014/12/11 by lige                                        */
/* Modify Date  : 2015/02/14 by hailong.dang                                */
/* Abstract     : Terminal API interface definition   					       */
/* Reference    : None														             */
/****************************************************************************/

#include "susiaccess_handler_api.h"
#ifdef USE_WEB_SHELL_CLIENT
#include "./WebShellClient/SSHClient.h"
#endif
#include "TerminalHandler.h"
#include "TerminalLog.h"
//#include <configuration.h>
#include "Parser.h"
#include "cJSON.h"
#include <stdio.h>
#include <unistd.h>
#include "WISEPlatform.h"
#include "util_string.h"
#include "util_pipe.h"


//-----------------------------------------------------------------------------
// Sample handler variables define
//-----------------------------------------------------------------------------
#define DEF_HANDLER_NAME         "terminal"
const char strPluginName[MAX_TOPIC_LEN] = {"terminal"};
const int iRequestID = cagent_request_terminal;
const int iActionID = cagent_reply_terminal;
static bool g_bEnableLog = true;
static void* g_loghandle = NULL;
static Handler_info  g_PluginInfo;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;

#ifndef WIN32
#define LINUX_DF_PS_CMD   "export HOME=/root ; echo $(uname -n):$(pwd) \\#"
#endif
//-----------------------------------------------------------------------------

//------------------------------User variables define--------------------------
static bool IsHandlerStart = false;
tmn_session_context_list SesContextList = NULL;
pthread_mutex_t SesContextListMutex;
//CAGENT_MUTEX_TYPE SesContextListMutex = NULL;
bool IsTmnSesMngThreadRunning = false;
//CAGENT_THREAD_HANDLE TmnSesMngThreadHandle = NULL;
pthread_t TmnSesMngThreadHandle;
//-----------------------------------------------------------------------------

//------------------------------user func define------------------------------------
//----------------------session context list function define-------------------
#ifndef USE_WEB_SHELL_CLIENT
static tmn_session_context_list CreateSesContextList();
static void DestroySesContextList(tmn_session_context_list sesContextList);
static int InsertSesContextNode(tmn_session_context_list sesContextList, tmn_session_context_node_t * pSesContextNode);
static tmn_session_context_node_t * FindSesContextNodeWithID(tmn_session_context_list sesContextList, char* id);
static int DeleteSesContextNodeWithID(tmn_session_context_list sesContextList, char* id);
static void UpdateSesContextList(tmn_session_context_list sesContextList);
static int DeleteAllSesContextNode(tmn_session_context_list sesContextList);
static bool IsSesContextListEmpty(tmn_session_context_list sesContextList);
//-----------------------------------------------------------------------------
static bool TmnStartNewSession(tmn_session_context_t * sesContext);
static bool TmnStopSession(tmn_session_context_t * sesContext);
static void TmnStopAllSession(tmn_session_context_list sesContextList);
static void* TmnSesMngThreadStart(void* args);
static void json_escape_str(char *str, char *out);
#endif
static void CollectCpbInfo(tmn_capability_info_t * pCpbInfo);
static void GetCapability();
//----------------------------------------------------------------------------------

#ifdef ANDROID
typedef pthread_t sp_pthread_t;
static int pthread_cancel(sp_pthread_t thread) {
        return (kill(thread, SIGTERM));
}
#endif

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
#ifndef USE_WEB_SHELL_CLIENT
static tmn_session_context_list CreateSesContextList()
{
	tmn_session_context_node_t * head = NULL;
	head = (tmn_session_context_node_t *)malloc(sizeof(tmn_session_context_node_t));
	if(head)
	{
		memset(head, 0, sizeof(tmn_session_context_node_t));
		//head->sesContext.sesID = DEF_INVALID_SESID;
		head->next = NULL;
	}
	return head;
}

static void DestroySesContextList(tmn_session_context_list sesContextList)
{
	if(NULL == sesContextList) return;
	DeleteAllSesContextNode(sesContextList);
	free(sesContextList); 
}

static int InsertSesContextNode(tmn_session_context_list sesContextList, tmn_session_context_node_t * pSesContextNode)
{
	int iRet = -1;
	tmn_session_context_node_t * newNode = NULL, * findNode = NULL, *head = NULL;
	if(pSesContextNode == NULL || sesContextList == NULL) return iRet;
	head = sesContextList;
	findNode = FindSesContextNodeWithID(head, pSesContextNode->sesContext.sesID);
	if(findNode == NULL)
	{
		newNode = pSesContextNode;
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

static tmn_session_context_node_t * FindSesContextNodeWithID(tmn_session_context_list sesContextList, char* id)
{
	tmn_session_context_node_t * findNode = NULL, *head = NULL;
	if(sesContextList == NULL) return findNode;
	head = sesContextList;
	findNode = head->next;
	while(findNode)
	{
		if(strcmp(findNode->sesContext.sesID, id) == 0) break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

static int DeleteSesContextNodeWithID(tmn_session_context_list sesContextList, char* id)
{
	int iRet = -1;
	tmn_session_context_node_t * delNode = NULL, *head = NULL;
	tmn_session_context_node_t * p = NULL;
	if(sesContextList == NULL) return iRet;
	head = sesContextList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(strcmp(delNode->sesContext.sesID, id) == 0)
		{
			p->next = delNode->next;
			if(delNode->sesContext.cmdStr != NULL)
			{
				free(delNode->sesContext.cmdStr);
				delNode->sesContext.cmdStr = NULL;
			}
			if(delNode->sesContext.cmdInputPipeReadHandle)
			{
				util_pipe_close(delNode->sesContext.cmdInputPipeReadHandle);
				delNode->sesContext.cmdInputPipeReadHandle = NULL;
			}
			if(delNode->sesContext.cmdInputPipeWriteHandle)
			{
				util_pipe_close(delNode->sesContext.cmdInputPipeWriteHandle);
				delNode->sesContext.cmdInputPipeWriteHandle = NULL;
			}
			if(delNode->sesContext.retOutputPipeReadHandle)
			{
				util_pipe_close(delNode->sesContext.retOutputPipeReadHandle);
				delNode->sesContext.retOutputPipeReadHandle = NULL;
			}
			if(delNode->sesContext.retOutputPipeWriteHandle)
			{
				util_pipe_close(delNode->sesContext.retOutputPipeWriteHandle);
				delNode->sesContext.retOutputPipeWriteHandle = NULL;
			}
			if(delNode->sesContext.sesWriteThreadHandle)
			{
				pthread_cancel(delNode->sesContext.sesWriteThreadHandle);
				pthread_join(delNode->sesContext.sesWriteThreadHandle, NULL);
				delNode->sesContext.sesWriteThreadHandle = 0;
			}
			if(delNode->sesContext.sesReadThreadHandle)
			{
				pthread_cancel(delNode->sesContext.sesReadThreadHandle);
				pthread_join(delNode->sesContext.sesReadThreadHandle, NULL);
				delNode->sesContext.sesReadThreadHandle = 0;
			}
			if(delNode->sesContext.sesPrcHandle)
			{
				util_pipe_close(delNode->sesContext.sesPrcHandle);
				delNode->sesContext.sesPrcHandle = NULL;
			}
			//if(delNode->sesContext.sesMutex)
			{
				pthread_mutex_destroy(&delNode->sesContext.sesMutex);
				//delNode->sesContext.sesMutex = NULL;
			}
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		else
		{
			p = delNode;
			delNode = delNode->next;
		}
	}
	if(iRet == -1) iRet = 1;
	return iRet;
}

static void UpdateSesContextList(tmn_session_context_list sesContextList)
{
	tmn_session_context_node_t * head = NULL, * delNode = NULL, *preNode = NULL;
	head = sesContextList;
	preNode = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->sesContext.isStop && !delNode->sesContext.isReady)
		{
			preNode->next = delNode->next;
			if(delNode->sesContext.cmdStr != NULL)
			{
				free(delNode->sesContext.cmdStr);
				delNode->sesContext.cmdStr = NULL;
			}
			if(delNode->sesContext.cmdInputPipeReadHandle)
			{
				util_pipe_close(delNode->sesContext.cmdInputPipeReadHandle);
				delNode->sesContext.cmdInputPipeReadHandle = NULL;
			}
			if(delNode->sesContext.cmdInputPipeWriteHandle)
			{
				util_pipe_close(delNode->sesContext.cmdInputPipeWriteHandle);
				delNode->sesContext.cmdInputPipeWriteHandle = NULL;
			}
			if(delNode->sesContext.retOutputPipeReadHandle)
			{
				util_pipe_close(delNode->sesContext.retOutputPipeReadHandle);
				delNode->sesContext.retOutputPipeReadHandle = NULL;
			}
			if(delNode->sesContext.retOutputPipeWriteHandle)
			{
				util_pipe_close(delNode->sesContext.retOutputPipeWriteHandle);
				delNode->sesContext.retOutputPipeWriteHandle = NULL;
			}
			if(delNode->sesContext.sesWriteThreadHandle)
			{
				pthread_cancel(delNode->sesContext.sesWriteThreadHandle);
				pthread_join(delNode->sesContext.sesWriteThreadHandle, NULL);
				delNode->sesContext.sesWriteThreadHandle = 0;
			}
			if(delNode->sesContext.sesReadThreadHandle)
			{
				pthread_cancel(delNode->sesContext.sesReadThreadHandle);
				pthread_join(delNode->sesContext.sesReadThreadHandle, NULL);
				delNode->sesContext.sesReadThreadHandle = 0;
			}
			if(delNode->sesContext.sesPrcHandle)
			{
				util_pipe_close(delNode->sesContext.sesPrcHandle);
				delNode->sesContext.sesPrcHandle = NULL;
			}
			//if(delNode->sesContext.sesMutex)
			{
				pthread_mutex_destroy(&delNode->sesContext.sesMutex);
				//delNode->sesContext.sesMutex = NULL;
			}

			free(delNode);
			delNode = preNode->next;
		}
		else
		{
			preNode = delNode;
			delNode = delNode->next;
		}
	}
}

static int DeleteAllSesContextNode(tmn_session_context_list sesContextList)
{
	int iRet = -1;
	tmn_session_context_node_t * delNode = NULL, *head = NULL;
	if(sesContextList == NULL) return iRet;
	head = sesContextList;
	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->sesContext.cmdStr != NULL)
		{
			free(delNode->sesContext.cmdStr);
			delNode->sesContext.cmdStr = NULL;
		}
		if(delNode->sesContext.cmdInputPipeReadHandle)
		{
			util_pipe_close(delNode->sesContext.cmdInputPipeReadHandle);
			delNode->sesContext.cmdInputPipeReadHandle = NULL;
		}
		if(delNode->sesContext.cmdInputPipeWriteHandle)
		{
			util_pipe_close(delNode->sesContext.cmdInputPipeWriteHandle);
			delNode->sesContext.cmdInputPipeWriteHandle = NULL;
		}
		if(delNode->sesContext.retOutputPipeReadHandle)
		{
			util_pipe_close(delNode->sesContext.retOutputPipeReadHandle);
			delNode->sesContext.retOutputPipeReadHandle = NULL;
		}
		if(delNode->sesContext.retOutputPipeWriteHandle)
		{
			util_pipe_close(delNode->sesContext.retOutputPipeWriteHandle);
			delNode->sesContext.retOutputPipeWriteHandle = NULL;
		}
		if(delNode->sesContext.sesWriteThreadHandle)
		{
			pthread_cancel(delNode->sesContext.sesWriteThreadHandle);
			pthread_join(delNode->sesContext.sesWriteThreadHandle, NULL);
			delNode->sesContext.sesWriteThreadHandle = 0;
		}
		if(delNode->sesContext.sesReadThreadHandle)
		{
			pthread_cancel(delNode->sesContext.sesReadThreadHandle);
			pthread_join(delNode->sesContext.sesReadThreadHandle, NULL);
			delNode->sesContext.sesReadThreadHandle = 0;
		}
		if(delNode->sesContext.sesPrcHandle)
		{
			util_pipe_close(delNode->sesContext.sesPrcHandle);
			delNode->sesContext.sesPrcHandle = NULL;
		}
		//if(delNode->sesContext.sesMutex)
		{
			pthread_mutex_destroy(&delNode->sesContext.sesMutex);
			//delNode->sesContext.sesMutex = NULL;
		}
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

static bool IsSesContextListEmpty(tmn_session_context_list sesContextList)
{
	bool bRet = true;
	tmn_session_context_node_t * curNode = NULL, *head = NULL;
	if(sesContextList == NULL) return bRet;
	head = sesContextList;
	curNode = head->next;
	if(curNode != NULL) bRet = false;
	return bRet;
}

static void* TmnSesWriteThreadStart(void* args)
{
   tmn_session_context_t * sesContext = (tmn_session_context_t *)args;
	if(sesContext != NULL)
	{
		while(sesContext->isSesWriteThreadRunning)
		{
			if(sesContext->isReady)
			{
				pthread_mutex_lock(&sesContext->sesMutex);
				if(sesContext->hasNewCmdStr && sesContext->cmdStr) //input cmd
				{
					unsigned int nBytesWrote = 0;
					char * tmpAnsiCmdStr = NULL;
					char * realAnsiCmdStr = NULL;
					int len = 0;
#ifdef WIN32
					char enterCh[] = {"\r\n"};
					//tmpAnsiCmdStr = UTF8ToANSI(sesContext->cmdStr);
					tmpAnsiCmdStr = sesContext->cmdStr;
#else
					char enterCh[] = {"\n"};
					tmpAnsiCmdStr = sesContext->cmdStr;
					sesContext->synWRFlag = 1;
#endif
					len = strlen(tmpAnsiCmdStr) + strlen(enterCh) + 1;
					realAnsiCmdStr = (char *)calloc(len, 1);
					strcpy(realAnsiCmdStr, tmpAnsiCmdStr);
					strcat(realAnsiCmdStr, enterCh);
					//printf("realAnsiCmdStr:%s\n", realAnsiCmdStr);
					util_pipe_write(sesContext->cmdInputPipeWriteHandle, realAnsiCmdStr, strlen(realAnsiCmdStr), &nBytesWrote);
#ifdef WIN32
					//free(tmpAnsiCmdStr);
					//tmpAnsiCmdStr = NULL;
#endif
					free(realAnsiCmdStr);
					realAnsiCmdStr = NULL;

					sesContext->hasNewCmdStr = false;
				}
				pthread_mutex_unlock(&sesContext->sesMutex);
#ifndef WIN32
				{
					int j = 0;
					for(j = 0; j<100; j++)
					{
						if(!sesContext->synWRFlag)break;
						usleep(10*1000);
					}
					pthread_mutex_lock(&sesContext->sesMutex);
					if(strcmp(sesContext->cmdStr, LINUX_DF_PS_CMD) && sesContext->synWRFlag)
					{
						int len = 0; 
						if(sesContext->cmdStr)
						{
							free(sesContext->cmdStr);
							sesContext->cmdStr = NULL;
						}
						len = strlen(LINUX_DF_PS_CMD) + 1;
						sesContext->cmdStr = (char *)malloc(len);
						memset(sesContext->cmdStr, 0, len);
						strcpy(sesContext->cmdStr, LINUX_DF_PS_CMD);
						sesContext->hasNewCmdStr = true;
					}
					pthread_mutex_unlock(&sesContext->sesMutex);
				}
#endif
			}
			usleep(10*1000);
		}
	}
	pthread_exit(0);
	return 0;
}

//#define INTERNAL_TRANS_UTF8

static void* TmnSesReadThreadStart(void* args)
{
	tmn_session_context_t * sesContext = (tmn_session_context_t *)args;
	if(sesContext != NULL)
	{
		while(sesContext->isSesWriteThreadRunning)
		{
			if(sesContext->isReady)
			{
				pthread_mutex_lock(&sesContext->sesMutex);
				{//output result
					wchar_t wText[40960] = {0};
					unsigned int nBytesRead = 0;
					if (util_pipe_read(sesContext->retOutputPipeReadHandle, (char *)wText, sizeof(wText), &nBytesRead) && nBytesRead > 0)
					{
						char * utf8RetStr = NULL;
#ifdef WIN32
						bool isTrans = false;
#ifdef INTERNAL_TRANS_UTF8
						int tmpLen = 0;
						utf8RetStr = ANSIToUTF8(wText);
						tmpLen = utf8RetStr==NULL?0:strlen(utf8RetStr);
						if(tmpLen == 1)
						{
							if(utf8RetStr) free(utf8RetStr);
							utf8RetStr = UnicodeToUTF8(wText);
						}
						isTrans = true;
#else   
						utf8RetStr = (char *)wText;
						/*if(strlen(utf8RetStr)==1)
						{
							utf8RetStr = UnicodeToANSI(wText);
							isTrans = true;
						} */
#endif
#else
						utf8RetStr = (char *)wText;
						sesContext->synWRFlag = 0;
#endif
						if(utf8RetStr)
						{
							char * repJsonStr = NULL;
							int jsonStrlen = 0;
							//char * escapeJsonText = NULL;
							//int len = strlen(utf8RetStr)+512;
							//escapeJsonText = (char*)malloc(len);
							//memset(escapeJsonText, 0, len);
							//json_escape_str(utf8RetStr, escapeJsonText);
							jsonStrlen = Parser_PackSesRet(sesContext->sesID, utf8RetStr, &repJsonStr);
							//if(escapeJsonText)free(escapeJsonText);
							if(jsonStrlen > 0 && repJsonStr != NULL)
							{
								g_sendcbf(&g_PluginInfo, terminal_session_ret_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);	
							}
							if(repJsonStr)free(repJsonStr);
#ifdef WIN32
							if(isTrans)free(utf8RetStr);
#endif
						}
					}
				}
				pthread_mutex_unlock(&sesContext->sesMutex);
			}
			usleep(10*1000);
		}
	}
	pthread_exit(0);
	return 0;
}

static bool TmnStopSession(tmn_session_context_t * sesContext)
{
	if(sesContext == NULL) return false;
	{
		if(sesContext->isSesWriteThreadRunning)
		{
			sesContext->isSesWriteThreadRunning = false;
			pthread_join(sesContext->sesWriteThreadHandle, NULL);
			sesContext->sesWriteThreadHandle = 0;
		}
		if(sesContext->sesPrcHandle)
		{
			unsigned int nBytesWrote;
			char exitStr[32] = {0};
			strcpy(exitStr, "exit\r\n");
			util_pipe_write(sesContext->cmdInputPipeWriteHandle, exitStr, strlen(exitStr),&nBytesWrote);
			util_pipe_close(sesContext->sesPrcHandle);
			sesContext->sesPrcHandle = NULL;
		}
		if(sesContext->isSesReadThreadRunning)
		{
			sesContext->isSesReadThreadRunning = false;
			pthread_join(sesContext->sesReadThreadHandle, NULL);
			sesContext->sesReadThreadHandle = 0;
		}
		if(sesContext->cmdInputPipeReadHandle)
		{
			util_pipe_close(sesContext->cmdInputPipeReadHandle);
			sesContext->cmdInputPipeReadHandle = NULL;
		}
		if(sesContext->cmdInputPipeWriteHandle)
		{
			util_pipe_close(sesContext->cmdInputPipeWriteHandle);
			sesContext->cmdInputPipeWriteHandle = NULL;
		}
		if(sesContext->retOutputPipeReadHandle)
		{
			util_pipe_close(sesContext->retOutputPipeReadHandle);
			sesContext->retOutputPipeReadHandle = NULL;
		}
		if(sesContext->retOutputPipeWriteHandle)
		{
			util_pipe_close(sesContext->retOutputPipeWriteHandle);
			sesContext->retOutputPipeWriteHandle = NULL;
		}
		if(sesContext->errorWriteHandle)
		{
			util_pipe_close(sesContext->errorWriteHandle);
			sesContext->errorWriteHandle = NULL;
		}
		sesContext->hasNewCmdStr = false;
		if(sesContext->cmdStr)
		{
			free(sesContext->cmdStr);
			sesContext->cmdStr = NULL;
		}
		memset(sesContext->sesID, 0, sizeof(sesContext->sesID));
		sesContext->isReady = false;
		sesContext->isStop = false;
	}
	return true;
}

static bool TmnStartNewSession(tmn_session_context_t * sesContext)
{
	bool bRet = false;
	if(sesContext == NULL) return bRet;
	{
		if(util_pipe_create(&sesContext->cmdInputPipeReadHandle, &sesContext->cmdInputPipeWriteHandle))
		{
			if(util_pipe_create(&sesContext->retOutputPipeReadHandle, &sesContext->retOutputPipeWriteHandle))
			{
				if (util_pipe_duplicate(sesContext->retOutputPipeWriteHandle, &sesContext->errorWriteHandle))
				{
					//printf("stdin:%d, stdout:%d\n", sesContext->cmdInputPipeReadHandle, sesContext->retOutputPipeWriteHandle);
					sesContext->sesPrcHandle = util_pipe_PrepAndLaunchRedirectedChild(sesContext->retOutputPipeWriteHandle,
						sesContext->cmdInputPipeReadHandle, sesContext->errorWriteHandle);
					//sesContext->sesPrcHandle = app_os_PrepAndLaunchRedirectedChild(sesContext->retOutputPipeWriteHandle,
					//	sesContext->cmdInputPipeReadHandle, sesContext->retOutputPipeWriteHandle);
					if(sesContext->sesPrcHandle)
					{
						sesContext->isSesWriteThreadRunning = true;
						if(pthread_create(&sesContext->sesWriteThreadHandle, NULL, TmnSesWriteThreadStart, sesContext) != 0)
						{
							sesContext->isSesWriteThreadRunning = false;
						}
						else
						{
							sesContext->isSesReadThreadRunning = true;
							if(pthread_create(&sesContext->sesReadThreadHandle, NULL, TmnSesReadThreadStart, sesContext) != 0)
							{
								sesContext->isSesReadThreadRunning = false;
							}
							else
							{
								sesContext->isReady = true;
								bRet = true;
							}
						}
					}
					else
					{
						printf("sesContext->sesPrcHandle: %d\n", (int)sesContext->sesPrcHandle);
					}
				}
			}
		}
		if(!bRet)
		{
			TmnStopSession(sesContext);
			sesContext->isValid = false;
		}
	}
	return bRet;
}

static void TmnStopAllSession(tmn_session_context_list sesContextList)
{
	if(sesContextList == NULL) return;
	{
		tmn_session_context_node_t * curNode = sesContextList->next;
		while(curNode)
		{
			curNode->sesContext.isStop = true;
			curNode = curNode->next;
		}
	}
}

static void* TmnSesMngThreadStart(void* args)
{
	tmn_session_context_list sesContextList = (tmn_session_context_list)args;
	tmn_session_context_node_t * curNode = sesContextList->next, *preNode = sesContextList;
	while(IsTmnSesMngThreadRunning)
	{
		pthread_mutex_lock(&SesContextListMutex);
		preNode = sesContextList;
		curNode = sesContextList->next;
		while(curNode)
		{
			usleep(10*1000);
			if(!curNode->sesContext.isReady && !curNode->sesContext.isStop && curNode->sesContext.isValid) //create
			{
				bool bRet;
				pthread_mutex_lock(&curNode->sesContext.sesMutex);
				bRet = TmnStartNewSession(&curNode->sesContext);
				pthread_mutex_unlock(&curNode->sesContext.sesMutex);
				if(bRet)
				{
					curNode->sesContext.isReady = true;
				}
				else
				{
					char repStr[128] = {0};
					char * repJsonStr = NULL;
					int jsonStrlen = 0;
					strcpy(repStr, "Failed");
					jsonStrlen = Parser_PackSessionStartRep(curNode->sesContext.sesID, repStr, &repJsonStr);
					if(jsonStrlen > 0 && repJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, terminal_session_cmd_run_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
					}
					if(repJsonStr)free(repJsonStr);
				}
			}
			if(curNode->sesContext.isReady && curNode->sesContext.isStop && curNode->sesContext.isValid) //stop
			{
				preNode->next = curNode->next;
				pthread_mutex_lock(&curNode->sesContext.sesMutex);
				TmnStopSession(&curNode->sesContext);
				curNode->sesContext.isValid = false;
				curNode->sesContext.isReady = false;
				pthread_mutex_unlock(&curNode->sesContext.sesMutex);
				{
					char repStr[128];
					char * repJsonStr = NULL;
					int jsonStrlen = 0;
					strcpy(repStr, "SUCCESS");
					jsonStrlen = Parser_PackSessionStopRep(curNode->sesContext.sesID, repStr, &repJsonStr);
					if(jsonStrlen > 0 && repJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, terminal_session_stop_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
					}
					if(repJsonStr)free(repJsonStr);
				}
				pthread_mutex_destroy(&curNode->sesContext.sesMutex);
				//curNode->sesContext.sesMutex = NULL;
				free(curNode);
				curNode = preNode->next;
				continue;
			}
			preNode = curNode;
			curNode = curNode->next;
		}
		pthread_mutex_unlock(&SesContextListMutex);
		usleep(10*1000);
	}
	pthread_exit(0);
	return 0;
}

static void json_escape_str(char *str, char *out)
{
	char *json_hex_chars = "0123456789abcdef";
	//char *results = (char*)malloc(1024);
	char *resultsPt = out;
	int pos = 0, start_offset = 0;
	unsigned char c;
	do {
		c = str[pos]; 
		switch(c) 
		{
			case '\0': break;
			case '\b':
			case '\n':
			case '\r':
			case '\t':
			case '\\':
			case '/':
			case 'r':
			case 'n':
				{
					if((c == 'r' && str[pos - 1] != '\\') || (c== 'n' && str[pos - 1] != '\\'))
					{
						if(c < ' ') {
							if(pos - start_offset > 0)
							{
								memcpy(resultsPt, str + start_offset, pos - start_offset); resultsPt+=pos-start_offset;
							} 
							sprintf(resultsPt, "\\u00%c%c",
								json_hex_chars[c >> 4],
								json_hex_chars[c & 0xf]);
							start_offset = ++pos;
						} else pos++;
					}	
					else 
					{
						if(pos - start_offset > 0)
						{
							memcpy(resultsPt, str + start_offset, pos - start_offset); resultsPt+=pos - start_offset;
						} 
						if(c == '\b')  
						{
							memcpy(resultsPt, "\\b", 2); resultsPt+=2;
						} 
						else if(c == '\n') 
						{
							memcpy(resultsPt, "\\n", 2); resultsPt+=2;
						} 
						else if(c == '\r') 
						{
							memcpy(resultsPt, "\\r", 2); resultsPt+=2;
						} 
						else if(c == '\t') 
						{
							memcpy(resultsPt, "\\t", 2); resultsPt+=2;
						} 
						else if(c == '"') 
						{
							memcpy(resultsPt, "\\\"", 2); resultsPt+=2;
						} 
						else if(c == '\\' && str[pos + 1] != 'r' && str[pos + 1] != 'n')
						{
							memcpy(resultsPt, "\\\\", 2); resultsPt+=2;
						} 
						else if(c == '/') 
						{
							memcpy(resultsPt, "\\/", 2); resultsPt+=2;
						} 
						else if(c == '\\' && str[pos + 1] == 'r') 
						{
							memcpy(resultsPt, "\\r", 2);
							resultsPt+=2;
						} 
						else if(c == 'r' && str[pos - 1] == '\\') 
						{
							memcpy(resultsPt, str + start_offset, pos - start_offset); resultsPt+=pos - start_offset;
							//resultsPt+=2;
						} 
						else if(c == '\\' && str[pos + 1] == 'n') 
						{
							memcpy(resultsPt, "\\n", 2);
							resultsPt+=2;
						} 
						else if(c == 'n' && str[pos - 1] == '\\') 
						{
							memcpy(resultsPt, str + start_offset, pos - start_offset); resultsPt+=pos - start_offset;
							//resultsPt+=2;
						} 
						start_offset = ++pos;
					}
					break;
				}
		default:
			{
				if(c < ' ') 
				{
					if(pos - start_offset > 0)
					{
						memcpy(resultsPt, str + start_offset, pos - start_offset); resultsPt+=pos-start_offset;
					} 
					sprintf(resultsPt, "\\u00%c%c",
						json_hex_chars[c >> 4],
						json_hex_chars[c & 0xf]);
					start_offset = ++pos;
				} 
				else pos++;
			}
		}
	} while(c);
	if(pos - start_offset > 0)
	{
		memcpy(resultsPt, str + start_offset, pos - start_offset); resultsPt+=pos-start_offset;
	} 
	memcpy(resultsPt, "\0", 1);
}
#endif

static void CollectCpbInfo(tmn_capability_info_t * pCpbInfo)
{
	if(NULL == pCpbInfo) return;
	{
		//add code about ssh
		pCpbInfo->funcsCode = TMN_INTERNAL_FUNC_FLAG;
		sprintf(pCpbInfo->funcsStr, "%s", TMN_INTERNAL_FUNC_STR);
	}
}

static void GetCapability()
{
	char * cpbStr = NULL;
	int jsonStrlen = 0;
	tmn_capability_info_t cpbInfo;
	memset((char*)&cpbInfo, 0, sizeof(tmn_capability_info_t));
	CollectCpbInfo(&cpbInfo);
	jsonStrlen = Parser_PackCpbInfo(&cpbInfo, &cpbStr);

	if(jsonStrlen>0 && cpbStr != NULL)
	{
		char * repJsonStr = NULL;
		jsonStrlen = Parser_PackSpecInfoRep(cpbStr, DEF_HANDLER_NAME, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, terminal_get_capability_rep, repJsonStr, jsonStrlen+1, NULL, NULL);
			if(repJsonStr)free(repJsonStr);
		}
		if(cpbStr)free(cpbStr);
	}
	else
	{
		char * errorRepJsonStr = NULL;
		char errorStr[128];
		int jsonStrlen = 0;
		sprintf(errorStr, "Command(%d), Get capability error!", terminal_get_capability_req);
		jsonStrlen = Parser_PackTerminalError(errorStr, &errorRepJsonStr);
		if(jsonStrlen > 0 && errorRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, terminal_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
		}
		if(errorRepJsonStr)free(errorRepJsonStr);
	}
}

static int WebShellClientSend(char * sessionKey, char * buf)
{
	if(sessionKey && buf && strlen(buf))
	{
		char * repJsonStr = NULL;
		int jsonStrlen = 0;
		printf("-------SendHandle, sessionKey: %s, buf:%s--------\n",sessionKey, buf);
		jsonStrlen = Parser_PackSesRet(sessionKey, buf, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, terminal_session_ret_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
		}
		if(repJsonStr)free(repJsonStr);
	}
	return 0;
}

void Handler_Uninitialize()
{
	if(IsHandlerStart)
	{
#ifndef USE_WEB_SHELL_CLIENT
		pthread_mutex_lock(&SesContextListMutex);
		TmnStopAllSession(SesContextList);
		pthread_mutex_unlock(&SesContextListMutex);
		while(!IsSesContextListEmpty(SesContextList))
		{
			usleep(10*1000);
		}
		if(IsTmnSesMngThreadRunning)
		{
			IsTmnSesMngThreadRunning = false;
			pthread_cancel(TmnSesMngThreadHandle);
			pthread_join(TmnSesMngThreadHandle, NULL);
			TmnSesMngThreadHandle = NULL;
		}
		DestroySesContextList(SesContextList);
		pthread_mutex_destroy(&SesContextListMutex);
#else
		SSHClientStop();
#endif
		IsHandlerStart = false;
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
	return 0;
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
*  Return:  handler_success : Success to Start Handler
*           handler_fail : Fail to Start Handler
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
#ifndef USE_WEB_SHELL_CLIENT
		pthread_mutex_init(&SesContextListMutex, NULL);
		
		SesContextList = CreateSesContextList();
		IsTmnSesMngThreadRunning = true;
		if(pthread_create(&TmnSesMngThreadHandle, NULL, TmnSesMngThreadStart, SesContextList) != 0)
		{
			IsTmnSesMngThreadRunning = false;
			pthread_mutex_destroy(&SesContextListMutex);
			return handler_fail;
		}
		bRet = true;
#else
		int iRet = 0;
		iRet = SSHClientStart();
		RegisterDataSendHandle(WebShellClientSend);
		if(iRet == 0)
		{
			bRet = true;
		}
#endif
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
*  Return:  handler_success : Success to Stop
*           handler_fail: Fail to Stop handler
* ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	if(IsHandlerStart)
	{
#ifndef USE_WEB_SHELL_CLIENT
		pthread_mutex_lock(&SesContextListMutex);
		TmnStopAllSession(SesContextList);
		pthread_mutex_unlock(&SesContextListMutex);
		while(!IsSesContextListEmpty(SesContextList))
		//if(!IsSesContextListEmpty(SesContextList))
		{
			usleep(10*1000);
		}
		if(IsTmnSesMngThreadRunning)
		{
			IsTmnSesMngThreadRunning = false;
			//pthread_cancel(TmnSesMngThreadHandle);
			pthread_join(TmnSesMngThreadHandle, NULL);
			TmnSesMngThreadHandle = NULL;
		}
		DestroySesContextList(SesContextList);
		pthread_mutex_destroy(&SesContextListMutex);
#else
		SSHClientStop();
#endif
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
void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2)
{
	int commCmd = unknown_cmd;
	TerminalLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &commCmd))
		return;

	switch(commCmd)
	{
	case terminal_get_capability_req:
		{
			GetCapability();
			break;
		}
	case terminal_session_cmd_run_req:
		{
			char tmpSesID[37] = {0};
			char * tmpSesCmdStr = NULL;
#ifndef USE_WEB_SHELL_CLIENT
			if(Parser_ParseSessionCmd((char *)data, tmpSesID, &tmpSesCmdStr))
			{
				tmn_session_context_node_t * findSesContextNode = NULL;
				pthread_mutex_lock(&SesContextListMutex);
				findSesContextNode = FindSesContextNodeWithID(SesContextList, tmpSesID);
				if(findSesContextNode == NULL)
				{
					tmn_session_context_node_t * newSesContextNode = (tmn_session_context_node_t *)malloc(sizeof(tmn_session_context_node_t));
					memset(newSesContextNode, 0, sizeof(tmn_session_context_node_t));
					strcpy(newSesContextNode->sesContext.sesID, tmpSesID);
					if(tmpSesCmdStr && strlen(tmpSesCmdStr))
					{
						int len = strlen(tmpSesCmdStr)+1;
						newSesContextNode->sesContext.cmdStr = (char *)malloc(len);
						memset(newSesContextNode->sesContext.cmdStr, 0, len);
						strcpy(newSesContextNode->sesContext.cmdStr, tmpSesCmdStr);
						newSesContextNode->sesContext.hasNewCmdStr = true;
					}
#ifndef WIN32
					else
					{
						int len = strlen(LINUX_DF_PS_CMD)+1;
						newSesContextNode->sesContext.cmdStr = (char *)malloc(len);
						memset(newSesContextNode->sesContext.cmdStr, 0, len);
						strcpy(newSesContextNode->sesContext.cmdStr, LINUX_DF_PS_CMD);
						newSesContextNode->sesContext.hasNewCmdStr = true;
					}
#endif
					newSesContextNode->sesContext.isValid = true;
					InsertSesContextNode(SesContextList, newSesContextNode);
				}
				else
				{
					pthread_mutex_lock(&findSesContextNode->sesContext.sesMutex);
					if(findSesContextNode->sesContext.cmdStr)
					{
						free(findSesContextNode->sesContext.cmdStr);
						findSesContextNode->sesContext.cmdStr = NULL;
					}
					if(tmpSesCmdStr)
					{
						int len = strlen(tmpSesCmdStr) + 1;
						findSesContextNode->sesContext.cmdStr = (char *)malloc(len);
						memset(findSesContextNode->sesContext.cmdStr, 0, len);
						strcpy(findSesContextNode->sesContext.cmdStr, tmpSesCmdStr);
						findSesContextNode->sesContext.hasNewCmdStr = true;
					}
					pthread_mutex_unlock(&findSesContextNode->sesContext.sesMutex);
				}
				pthread_mutex_unlock(&SesContextListMutex);
			}
#else
			int width = 0, height = 0;
			if(Parser_ParseSessionCmdEx((char *)data, tmpSesID, &width, &height, &tmpSesCmdStr))
			{
				char tmpSesStr[128] = {0}; 
				sprintf(tmpSesStr, "%s", tmpSesID);
				//itoa(tmpSesID, tmpSesStr,10);
				RecvData(tmpSesStr, tmpSesCmdStr, width, height);
			}
#endif
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128];
				int jsonStrlen = 0;
				strcpy(errorStr, "Parser params failed!");
				jsonStrlen = Parser_PackTerminalError(errorStr, &errorRepJsonStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, terminal_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			if(tmpSesCmdStr)
			{
				free(tmpSesCmdStr);
				tmpSesCmdStr = NULL;
			}
			break;
		}
	case terminal_session_stop_req:
		{
			char repStr[256] = {0};
			char tmpSesID[37] = {0};
			if(Parser_ParseSessionStopParams((char *)data, tmpSesID))
			{
#ifndef USE_WEB_SHELL_CLIENT
				tmn_session_context_node_t * findSesContextNode = NULL;
				pthread_mutex_lock(&SesContextListMutex);
				findSesContextNode = FindSesContextNodeWithID(SesContextList, tmpSesID);
				if(findSesContextNode)
				{
					pthread_mutex_lock(&findSesContextNode->sesContext.sesMutex);
					findSesContextNode->sesContext.isStop = true;
					pthread_mutex_unlock(&findSesContextNode->sesContext.sesMutex);
				}
				else
				{
					strcpy(repStr, "Not found!");
				}
				pthread_mutex_unlock(&SesContextListMutex);
#else
				char tmpSesStr[37] = {0}; 
				sprintf(tmpSesStr, "%s", tmpSesID);
				CloseSession(tmpSesStr);
				strcpy(repStr, "SUCCESS");
#endif
				if(strlen(repStr))
				{
					char * repJsonStr = NULL;
					int jsonStrlen = 0;
					jsonStrlen = Parser_PackSessionStopRep(tmpSesID, repStr, &repJsonStr);
					if(jsonStrlen > 0 && repJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, terminal_session_stop_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
					}
					if(repJsonStr)free(repJsonStr);
				}
			}
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128] = {0};
				int jsonStrlen = 0;
				strcpy(errorStr, "Parser params failed!");
				jsonStrlen = Parser_PackTerminalError(errorStr, &errorRepJsonStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, terminal_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			break;
		}
	default:
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128] = {0};
			int jsonStrlen = 0;
			strcpy(errorStr, "Unknown cmd!");
			jsonStrlen = Parser_PackTerminalError(errorStr, &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, terminal_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);
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
	tmn_capability_info_t cpbInfo;
	memset((char*)&cpbInfo, 0, sizeof(tmn_capability_info_t));
	CollectCpbInfo(&cpbInfo);
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
*  Input : char *pInQuery, if *pInQuery = NULL, then stop all upload message.
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
