#include "util_power.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

bool util_power_off()
{
	//"/sbin/poweroff"
	bool bRet = true;
	pid_t fpid;
	fpid = fork();
    if(fpid == 0)
	{
#ifdef ANDROID
                if(execlp("/system/bin/reboot", "reboot", "-p", NULL) < 0)
#else
		if(execlp("/sbin/poweroff", "poweroff",  NULL) < 0)
#endif
		{
			bRet = false;
		}
	}
	else if(fpid < 0)
	{
		bRet = false;
	}
   return bRet;
}

bool util_power_restart()
{
	//"/sbin/reboot"
	bool bRet = true;
	pid_t fpid;
	fpid = fork();
	if(fpid == 0)
	{
#ifdef ANDROID
                if(execlp("/system/bin/reboot", "reboot",  NULL) < 0)
#else
		if(execlp("/sbin/reboot", "reboot",  NULL) < 0)
#endif
		{
			bRet = false;
		}
	}
	else if(fpid < 0)
	{
		bRet = false;
	}
	return bRet;
}

bool util_power_suspend()
{
	//"/usr/sbin/pm-suspend"
	bool bRet = true;
	pid_t fpid;
	fpid = fork();
	if(fpid == 0)
	{
		if(execlp("/usr/sbin/pm-suspend", "pm-suspend", "-h", NULL) < 0)
		{
			bRet = false;
		}
	}
	else if(fpid < 0)
	{
		bRet = false;
	}
	return bRet;
}

bool util_power_hibernate()
{
	//"/usr/sbin/pm-hibernate"
	bool bRet = true;
	pid_t fpid;
	fpid = fork();
	if(fpid == 0)
	{
		if(execlp("/usr/sbin/pm-hibernate", "pm-hibernate", NULL) < 0)
		{
			bRet = false;
		}
	}
	else if(fpid < 0)
	{
		bRet = false;
	}
	return bRet;
}

void util_resume_passwd_disable()
{
	return;
}

bool util_power_suspend_check()
{
	bool bRet1 = false;
	bool bRet2 = false;
	FILE *fp = NULL;
#ifdef ANDROID
        fp = popen("/system/bin/cat  /sys/power/state ", "r");
#else
	fp = popen("/bin/cat  /sys/power/state ", "r");
#endif
	if(fp)
	{
		char tmpLineStr[512] = {0};
		while(fgets(tmpLineStr, sizeof(tmpLineStr), fp)!=NULL)
		{
			if(strstr(tmpLineStr, "mem"))
			{
				bRet1 = true;
				break;
			}
			memset(tmpLineStr, 0, sizeof(tmpLineStr));
		}
		pclose(fp);
	}
	fp = popen("if [ -f \"/usr/sbin/pm-suspend\" ]; then echo yes; else echo no; fi", "r");
	if(fp)
	{
		char tmpLineStr[512] = {0};
		while(fgets(tmpLineStr, sizeof(tmpLineStr), fp)!=NULL)
		{
			if(strstr(tmpLineStr, "yes"))
			{
				bRet2 = true;
				break;
			}
			memset(tmpLineStr, 0, sizeof(tmpLineStr));
		}
		pclose(fp);
	}	
	return bRet1&&bRet2;
}

bool util_power_hibernate_check()
{
	bool bRet1 = false;
	bool bRet2 = false;
	bool bRet3 = false;
	FILE *fp = NULL;
#ifdef ANDROID
        fp = popen("/system/bin/cat  /sys/power/state ", "r");
#else
	fp = popen("/bin/cat /sys/power/state ", "r");
#endif
	if(fp)
	{
		char tmpLineStr[512] = {0};
		while(fgets(tmpLineStr, sizeof(tmpLineStr), fp)!=NULL)
		{
			if(strstr(tmpLineStr, "disk"))
			{
				bRet1 = true;
				break;
			}
			memset(tmpLineStr, 0, sizeof(tmpLineStr));
		}
		pclose(fp);
	}	
#ifndef ANDROID
	fp = popen("fdisk -l | grep swap ", "r");
	if(fp)
	{
		char tmpLineStr[512] = {0};
		while(fgets(tmpLineStr, sizeof(tmpLineStr), fp)!=NULL)
		{
			if(strstr(tmpLineStr, "swap"))
			{
				bRet2 = true;
				break;
			}
			memset(tmpLineStr, 0, sizeof(tmpLineStr));
		}
		pclose(fp);
	}
#endif
	fp = popen("if [ -f \"/usr/sbin/pm-hibernate\" ]; then echo yes; else echo no; fi", "r");
	if(fp)
	{
		char tmpLineStr[512] = {0};
		while(fgets(tmpLineStr, sizeof(tmpLineStr), fp)!=NULL)
		{
			if(strstr(tmpLineStr, "yes"))
			{
				bRet3 = true;
				break;
			}
			memset(tmpLineStr, 0, sizeof(tmpLineStr));
		}
		pclose(fp);
	}
	return bRet1&&bRet2&&bRet3;
}
