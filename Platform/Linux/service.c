#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "service.h"

#define MAX_PATH 260
#define FILE_SEPARATOR   '/'
#define FILE_APPRUNDIR	"/var/run"

char srvcName[64] = DEF_SERVICE_NAME;
char srvcVersion[32] = DEF_SERVICE_VERSION;
char pidPath[MAX_PATH] = {0};
bool isNSrvRun = false;
bool isSrvRun = false;
pthread_cond_t  srvcStopEvent;
pthread_mutex_t srvcStopMux;

APP_START_CB m_pStart = NULL;
APP_STOP_CB m_pStop = NULL;

static bool OnRun();
static bool OnStop();
static bool UserRun();
static bool UserStop();

static void hdl (int sig)
{ 
	OnStop();
}

int ServiceInit(char * pSrvcName, char * pVersion, APP_START_CB pStart, APP_STOP_CB pStop, void * loghandle)
{
   int iRet = -1;
   int rcCond = 0, rcMux = 0;
   struct sigaction act;
   
   if(pSrvcName) memcpy(srvcName, pSrvcName, strlen(pSrvcName)+1);
   
   if(pVersion) memcpy(srvcVersion, pVersion, strlen(pVersion)+1);

#ifndef ANDROID
   sprintf(pidPath, "%s%c%s.pid", FILE_APPRUNDIR, FILE_SEPARATOR, srvcName);
    if(access( pidPath, F_OK ) != -1)
    {
		FILE *stream = NULL;
		stream = fopen(pidPath,"r");
		if(stream)
		{
			char spid[64] = {0};
			if(fgets(spid, sizeof(spid), stream))
			{
				int pid = atoi(spid);
				if(kill(pid,0)==0)
				{
					printf("Service is running.!");     
					fclose(stream);
					return iRet;
				}
			}
			fclose(stream);
		}
		remove(pidPath);
    }
#endif // ANDROID
   m_pStart = pStart;
   m_pStop = pStop;

   memset (&act, '\0', sizeof(act));
   act.sa_handler = &hdl;
   if (sigaction(SIGTERM, &act, NULL) < 0) {
	   printf("sigaction error!");                                                                                                                                                                                                                                                 
	   return iRet;
   }

   rcMux = pthread_mutex_init(&srvcStopMux, NULL);
   rcCond = pthread_cond_init(&srvcStopEvent, NULL);
   
   if(rcCond != 0 || rcMux != 0)
   {
	iRet = -1;
	printf("Event Create failed");  
   }
   else
   {
#ifndef ANDROID
    FILE *fp;
	char buf[100];
	
	fp = fopen(pidPath, "w");
	if (fp != NULL) {
		snprintf(buf, sizeof(buf), "%ld\n", (long) getpid());
		fputs(buf, fp);
		fclose(fp);
		printf("Create pid file: %s!", pidPath);
	} else {
		printf("Couldn't open %s for writing.\n", pidPath);
	}

#endif // ANDROID
	iRet = 0;
   }
   return iRet;
}

int ServiceUninit()
{
   pthread_mutex_lock(&srvcStopMux);
   pthread_cond_signal(&srvcStopEvent);
   pthread_mutex_unlock(&srvcStopMux);

   pthread_mutex_destroy(&srvcStopMux);
   pthread_cond_destroy(&srvcStopEvent);

   if( access( pidPath, F_OK ) != -1 ) {
   	remove(pidPath);
	printf("Remove pid file: %s!", pidPath);
   } else {
    printf("pid file %s not found!", pidPath);
   }
   
   //if(g_logHandle) UninitLog(g_logHandle);
   return 0;
}

int LaunchService()
{
   int iRet = -1;

  bool bRet = OnRun();

   if(bRet)
   {
      printf("%s Start successfully!", srvcName);
      iRet = 0;
   }
   else
   {
      printf("%s Start failed!", srvcName);
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
   "-n    Non service run command\n" \
   "-q    Quit command\n" \
   ">>")

int ExecuteCmd(char* cmd)
{
   int iRet = 0;
   if(cmd == NULL)
   {
   	printf("This command cannot be identified!\n");
       printf(">>");
   }
   else if(strcasecmp(cmd, HELP_SERVICE_CMD) == 0)
   {
      HelpPrintf();
   }
   else if(strcmp(cmd, NSRV_RUN_CMD ) == 0 || strcmp(cmd, "-m") == 0)
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
   else if(strcasecmp(cmd, STOP_NSER_RUN_CMD) == 0)
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
   else if(strcasecmp(cmd, DSVERSION_SERVICE_CMD) == 0)
   {
      printf("%s Version: %s.\n", srvcName, srvcVersion);
      printf(">>");
   }
   else if(strcasecmp(cmd, QUIT_CMD) == 0)
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

static bool OnRun()
{
   bool bRet;
   pthread_mutex_lock(&srvcStopMux);
   bRet = isSrvRun =  UserRun();
   pthread_mutex_unlock(&srvcStopMux);
   while(bRet)
   {
      printf("%s is running!", srvcName);
      pthread_mutex_lock(&srvcStopMux);
      pthread_cond_wait(&srvcStopEvent, &srvcStopMux);
      pthread_mutex_unlock(&srvcStopMux);
      printf("%s is get stop signal!", srvcName);
      break;
   }
   return bRet;
}

static bool OnStop()
{
   bool bRet = false;
   pthread_mutex_lock(&srvcStopMux);
   if(isSrvRun)
   	bRet = UserStop();
   isSrvRun = false;
   printf("%s is send stop signal!", srvcName);
   pthread_cond_signal(&srvcStopEvent);
   pthread_mutex_unlock(&srvcStopMux);
   return bRet;
}

static bool UserRun()
{
   bool bRet = false;
   if(m_pStart != NULL)
	   bRet = m_pStart() == 0;
   return bRet;
}

static bool UserStop()
{
   bool bRet = false;
   if(m_pStop != NULL)
	bRet = m_pStop() == 0;
   return bRet;
}
