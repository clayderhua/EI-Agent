#include <stdio.h>
#include <stdbool.h>
#include "service.h"

SERVICE_STATUS_HANDLE srvcStatusHandle = NULL;
SERVICE_STATUS srvcStatus;
char srvcName[64] = DEF_SERVICE_NAME;
char srvcVersion[32] = DEF_SERVICE_VERSION;
char srvcMoudleFileName[MAX_PATH] = {0};
bool isNSrvRun = false;
HANDLE srvcStopEvent = NULL;
APP_START_CB m_pStart = NULL;
APP_STOP_CB m_pStop = NULL;

static bool SetStatus(DWORD dwState);
static DWORD GetStatus();
static bool OnInit();
static bool OnRun();
static bool OnStop();
static bool OnPause();
static bool OnContinue();
static bool OnInterrogate();
static bool OnShutdown();
static bool OnUserControl(DWORD dwCtrlCode);
static void WINAPI ServiceMain(DWORD dwArgs, LPTSTR *lpArgv);
static void WINAPI CtrlHandler(DWORD dwCtrlCode);
static bool IsInstalled();
static bool InstallService();
static bool UninstallService();
static bool UserRun();
static bool UserStop();

WISEPLATFORM_API int ServiceInit(char * pSrvcName, char * pVersion, APP_START_CB pStart, APP_STOP_CB pStop, void * logHandle)
{
   int iRet = 0;

   memset(&srvcStatus, 0, sizeof(srvcStatus));
   if(pSrvcName)
   {
	   memset(srvcName, 0, sizeof(srvcName));
	   memcpy(srvcName, pSrvcName, strlen(pSrvcName)+1);
   }
   memset(srvcMoudleFileName, 0 , sizeof(srvcMoudleFileName));
   GetModuleFileName(NULL, srvcMoudleFileName, sizeof(srvcMoudleFileName));

   if(pVersion)
   {
	   memset(srvcVersion, 0, sizeof(srvcVersion));
	   memcpy(srvcVersion, pVersion, strlen(pVersion)+1);
   }

   srvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   srvcStatus.dwCurrentState = SERVICE_STOPPED;
   srvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
   srvcStatus.dwWin32ExitCode = 0;
   srvcStatus.dwServiceSpecificExitCode = 0;
   srvcStatus.dwCheckPoint = 0;
   srvcStatus.dwWaitHint = 0;

   m_pStart = pStart;
   m_pStop = pStop;

   srvcStopEvent = CreateEvent(NULL, true, false, NULL);
   if(srvcStopEvent == NULL)
   {
	   m_pStart = NULL;
	   m_pStop = NULL;
	   iRet = -1;
   }

   return iRet;
}

WISEPLATFORM_API int ServiceUninit()
{
   if(srvcStopEvent)
   {
      SetEvent(srvcStopEvent);
      CloseHandle(srvcStopEvent);
   }
   //if(g_logHandle) UninitLog(g_logHandle);
   return 0;
}

WISEPLATFORM_API int LaunchService()
{
   int iRet = -1;
   BOOL bRet = FALSE;
   SERVICE_TABLE_ENTRY st[] = {
      {srvcName, ServiceMain},
      {NULL, NULL}
   };
   bRet = StartServiceCtrlDispatcher(st);
   if(bRet)
   {
      printf("%s StartServiceCtrlDispatcher is successfully!", srvcName);
	  iRet = 0;
   }
   else
   {
      printf("%s StartServiceCtrlDispatcher is failed! Error code: %d", srvcName,GetLastError());
	  iRet = -1;
   }

   return iRet;
}

#define HelpPrintf() printf("\n"  \
   "-------------------------------------------------------------\n" \
   "|                     service control help                  |\n" \
   "-------------------------------------------------------------\n" \
   "-h    Control help command\n" \
   "-v    Display version command\n" \
   "-i    Install command\n" \
   "-u    Uninstall command\n" \
   "-n    Non service run command\n" \
   "-s    Stop non service run command\n" \
   "-q    Quit command\n" \
   ">>")

WISEPLATFORM_API int ExecuteCmd(char* cmd)
{
   int iRet = 0;
   if(cmd == NULL)
   {
	  HelpPrintf();
   }
   else if(_stricmp(cmd, HELP_SERVICE_CMD) == 0)
   {
      HelpPrintf();
   }
   else if(_stricmp(cmd, NSRV_RUN_CMD ) == 0 || _stricmp(cmd, "-m") == 0)
   {
      if(!isNSrvRun)
      {
         isNSrvRun = UserRun();
         if(!isNSrvRun)
         {
            printf("%s no-service is start failed!\n", srvcName);
         }
         else
         {
            printf("%s no-service is start successfully!\n", srvcName);
         }
      }
      else
      {
         printf("%s has been non-service running!\n", srvcName);
      }
      printf(">>");
   }
   else if(_stricmp(cmd, STOP_NSER_RUN_CMD) == 0)
   {
      if(!isNSrvRun)
      {
         printf("%s non-service is not running!\n", srvcName);
      }
      else
      {
         if(UserStop())
         {
            isNSrvRun = false;
            printf("%s non-service is stop successfully!\n", srvcName);
         }
         else
         {
            printf("%s non-service is stop failed!\n", srvcName);  
         }
      }    
      printf(">>");
   }
   else if(_stricmp(cmd, DSVERSION_SERVICE_CMD) == 0)
   {
      printf("%s Version: %s, the service is %s installed.\n", 
         srvcName, srvcVersion, IsInstalled() ? "currently":"not" );
      printf(">>");
   }
   else if(_stricmp(cmd, INSTALL_SERVICE_CMD) == 0)
   {
      if(!IsInstalled())
      {
         if(InstallService()) 
         {
            printf("%s installed!", srvcName);
         }
         else 
         {
            printf("%s failed to install. Error code: %d", srvcName, GetLastError());
         }
      }
      else
      {
         printf("%s is already installed!", srvcName);
      }
      printf(">>");
   }
   else if(_stricmp(cmd, UNINSTALL_SERVICE_CMD) == 0)
   {
      if(IsInstalled())
      {
         if(UninstallService()) 
         {
            printf("%s is  uninstall successfully!", srvcName);
         }
         else 
         {
            printf("%s is uninstall failed! Error code: %d",srvcName, GetLastError());
         }
      }
      else
      {
         printf("%s is not installed!", srvcName);
      }
      printf(">>");
   }
   else if(_stricmp(cmd, QUIT_CMD) == 0)
   {
      if(isNSrvRun)
      {
         UserStop();
      }
      iRet = -1;
   }
   else
   {
      printf("This command cannot be identified!\n");
      printf(">>");
   }
   return iRet;
}

static bool IsInstalled()
{
   bool bRet = false;

   SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if(hSCM)
   {
      SC_HANDLE hSrv = OpenService(hSCM, srvcName, SERVICE_QUERY_CONFIG);
      if(hSrv)
      {
         bRet = true;
         CloseServiceHandle(hSrv);
      }
      CloseServiceHandle(hSCM);
   }
   return bRet;
}

static bool InstallService()
{
   SC_HANDLE hSCM = NULL;
   SC_HANDLE hSrv = NULL;
   hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if(!hSCM) return false;
   //    hSrv = CreateService(hSCM, srvcName, srvcName, SERVICE_ALL_ACCESS, SERVICE_INTERACTIVE_PROCESS|SERVICE_WIN32_OWN_PROCESS,
   //       SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, srvcMoudleFileName, NULL, NULL, NULL, NULL, NULL);
   hSrv = CreateService(hSCM, srvcName, srvcName, SERVICE_ALL_ACCESS, SERVICE_INTERACTIVE_PROCESS|SERVICE_WIN32_OWN_PROCESS,
      SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, srvcMoudleFileName, NULL, NULL, NULL, NULL, NULL);  
   if(!hSrv)
   {
      CloseServiceHandle(hSCM);
      return false;
   }
   CloseServiceHandle(hSCM);
   CloseServiceHandle(hSrv);
   return true;
}

static bool UninstallService()
{
   bool bRet = false;
   SC_HANDLE hSCM = NULL;
   SC_HANDLE hSrv = NULL;
   hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if(!hSCM) return bRet;
   hSrv = OpenService(hSCM, srvcName, DELETE | SERVICE_QUERY_STATUS | SERVICE_STOP);
   if(hSrv)
   {
      SERVICE_STATUS srvStat;
      if(QueryServiceStatus(hSrv, &srvStat))
      {
         if(srvStat.dwCurrentState != SERVICE_STOPPED)
         {
            ControlService(hSrv, SERVICE_CONTROL_STOP, &srvStat);
         }

         if(DeleteService(hSrv))
         {
            bRet = true;
         }
      }
      CloseServiceHandle(hSrv);
   }
   CloseServiceHandle(hSCM);
   return bRet;
}

static bool SetStatus(DWORD dwState)
{
   srvcStatus.dwCurrentState = dwState;
   return SetServiceStatus(srvcStatusHandle, &srvcStatus);
}

static DWORD GetStatus()
{
   SC_HANDLE hSCM  = NULL;
   SC_HANDLE hSrv = NULL;
   hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if(!hSCM) return 0;
   hSrv = OpenService(hSCM, srvcName, SERVICE_QUERY_STATUS);
   if(hSrv)
   {
      SERVICE_STATUS srvStat;
      QueryServiceStatus(hSrv, &srvStat);
      return srvStat.dwCurrentState;
   }
   return 0;
}

static bool OnInit()
{
   SetStatus(SERVICE_START_PENDING);
   printf("%s  is start pending!", srvcName);
 
   return true;
}

static bool OnRun()
{
   UserRun();
   while(1)
   {
      WaitForSingleObject(srvcStopEvent, INFINITE);
      ResetEvent(srvcStopEvent);
      printf("%s  is get stop signal!", srvcName);
      break;
   }
   return true;
}

static bool OnStop()
{
   UserStop();
   printf("%s  is send stop signal!", srvcName);
   SetEvent(srvcStopEvent);
   return true;
}

static bool OnPause()
{
   return true;
}

static bool OnContinue()
{
   return true;
}

static bool OnInterrogate()
{
   return true;
}

static bool OnShutdown()
{
   return true;
}

static bool OnUserControl(DWORD dwCtrlCode)
{
   return true;
}

static void WINAPI ServiceMain(DWORD dwArgs, LPTSTR *lpArgv)
{
   srvcStatusHandle = RegisterServiceCtrlHandler(srvcName, CtrlHandler);
   if(srvcStatusHandle == NULL)
   {
      printf("%s RegisterServiceCtrlHandler is failed!", srvcName);
      return;
   }
   printf("%s RegisterServiceCtrlHandler is successfully!", srvcName);
   if(OnInit())
   {
      srvcStatus.dwWin32ExitCode = GetLastError();
      srvcStatus.dwCheckPoint = 0;
      srvcStatus.dwWaitHint = 0;
      SetStatus(SERVICE_RUNNING);
      printf("%s  is running!", srvcName);
      OnRun();
   }
   printf("%s  is stop successfully!", srvcName);
   SetStatus(SERVICE_STOPPED);
}

static void WINAPI CtrlHandler(DWORD dwCtrlCode)
{
   switch(dwCtrlCode)
   {
   case SERVICE_CONTROL_STOP:
      {
         SetStatus(SERVICE_STOP_PENDING);
         printf("%s  is stop pending!", srvcName);
         OnStop(); 
         SetStatus(SERVICE_STOPPED);
         break;
      }
   case SERVICE_CONTROL_PAUSE:
      {
         OnPause();
         break;
      }
   case SERVICE_CONTROL_CONTINUE:
      {
         OnContinue();
         break;
      }
   case SERVICE_CONTROL_INTERROGATE:
      {
         OnInterrogate();
         break;
      }
   case SERVICE_CONTROL_SHUTDOWN:
      {
         OnShutdown();
         break;
      }
   default:
      {
         if(dwCtrlCode >= SERVICE_CONTROL_USER)
         {
            OnUserControl(dwCtrlCode);
         }
         break;
      }
   }
}

static bool UserRun()
{
   bool bRet = false;
   if(m_pStart != NULL)
	   bRet = m_pStart()==0;
   return bRet;
}

static bool UserStop()
{
   bool bRet = false;
   if(m_pStop != NULL)
	   bRet = m_pStop()==0;
   return bRet;
}
