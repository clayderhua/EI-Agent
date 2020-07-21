#include "network.h"
#include "SAGatherInfo.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <advcarehelper.h>
#include <hwmhelper.h>
#include "util_os.h"
#ifdef WIN32
#include "SUSIAuthenticator.h"
#endif
#define DIV                       (1024)
#define DEF_OS_NAME_LEN           34
#define DEF_OS_VERSION_LEN        64
#define DEF_BIOS_VERSION_LEN      256
#define DEF_PLATFORM_NAME_LEN     128
#define DEF_PROCESSOR_NAME_LEN    64

int SAGetInfo_GetMacList(char * macListStr)
{
	int iRet = -1;
	if(NULL == macListStr) return iRet;
	{
		char macsStr[10][20] = {{0}};
		int macCnt = 0;
		int count = 600;
		do{
			if(count <=0) return iRet;
			count--;
			macCnt = network_mac_list_get(macsStr, sizeof(macsStr)/20);
			usleep(100*1000);
		}while(macCnt == 0);
		if(macCnt > 0)
		{
			int i = 0;
			for(i = 0; i < macCnt; i++)
			{
				strcat(macListStr, macsStr[i]);
				if(i < macCnt -1) strcat(macListStr, ";");
			}
			iRet = 0;
		}
	}
	return iRet;
}

int SAGetInfo_GetMacAddr(char* mac)
{
	int iRet = -1;
	if(NULL == mac) return iRet;
	{
		int count = 600;
		do{
			if(count <=0) return iRet;
			count--;
			iRet = network_mac_get_ex(mac);
			usleep(100*1000);
		}while(iRet != 0);
	}
	return iRet;
}

int SAGATHERINFO_API sagetInfo_gatherplatforminfo(susiaccess_agent_profile_body_t * profile)
{
	char mac[DEF_MAC_LENGTH] = {0};
	//char osversion[DEF_OS_VERSION_LEN] = {0};
	char localip[DEF_MAX_STRING_LENGTH] = {0};
	char maclist[DEF_MAC_LENGTH*16] = {0};
	//char processorname[DEF_PROCESSOR_NAME_LEN] = {0};
	//char osarchitect[DEF_FILENAME_LENGTH] = {0};
	unsigned long osversionsize= DEF_OS_VERSION_LEN;
	unsigned long processnamesize = DEF_PROCESSOR_NAME_LEN;
	int osarchitectsize = DEF_FILENAME_LENGTH;
	uint64_t totalmemsize = 0;
	uint64_t availmemsize = 0;

	if(!profile)
		return false;

	if(strlen(profile->hostname)<=0)
	{
		char hostname[DEF_HOSTNAME_LENGTH] = {0};
		if(network_host_name_get(hostname, sizeof(hostname))==0)
			strncpy(profile->hostname, hostname, strlen(hostname)+1);
	}

	if(strlen(profile->mac)<=0)
	{
		if(SAGetInfo_GetMacAddr(mac) >= 0)
			strncpy(profile->mac, mac, strlen(mac)+1);
	}

	if(util_os_get_os_name(NULL, &osversionsize))
	{
		char *osversion = (char*)malloc(osversionsize);
		memset(osversion, 0, osversionsize);
		if(util_os_get_os_name(osversion, NULL))
			strncpy(profile->osversion, osversion, osversionsize>=sizeof(profile->osversion)?sizeof(profile->osversion):osversionsize);
		free(osversion);
	}

	if(network_ip_get(localip, sizeof(localip)) == 0)
		strncpy(profile->localip, localip, strlen(localip)+1);

	if(SAGetInfo_GetMacList(maclist)==0)
		strncpy(profile->maclist, maclist, strlen(maclist)+1);
	
	if(util_os_get_free_memory(&totalmemsize, &availmemsize))
		profile->totalmemsize = totalmemsize;

	if(util_os_get_processor_name(NULL, &processnamesize))
	{
		char *processorname = (char*)malloc(processnamesize);
		memset(processorname, 0, processnamesize);
		if(util_os_get_processor_name(processorname, NULL))
			strncpy(profile->processorname, processorname, processnamesize>=sizeof(profile->processorname)?sizeof(profile->processorname):processnamesize);
		free(processorname);
	}

	if(util_os_get_architecture(NULL, &osarchitectsize))
	{
		char *osarchitect = (char*)malloc(osarchitectsize);
		memset(osarchitect, 0, osarchitectsize);
		if(util_os_get_architecture(osarchitect, NULL))
			strncpy(profile->osarchitect, osarchitect, osarchitectsize>=sizeof(profile->osarchitect)?sizeof(profile->osarchitect):osarchitectsize);
		free(osarchitect);
	}
#if 0 //Disable SUSI Check for UNO, Scott Chang, 2020.06.10
	if (SUSIAuthenticator() == 0)
	{
		char getStr[MAX_PATH] = { 0 };
		int getStrLen = 0;
		int result = SUSICheckPlatform(getStr, &getStrLen);
		if (result != 0)
			return true;
	}
#endif
	if(hwm_StartupSUSILib() == true)
	{
		char platform[DEF_PLATFORM_NAME_LEN] = {0};
		char bios[DEF_BIOS_VERSION_LEN] = {0};
		if(hwm_GetPlatformName(platform, DEF_PLATFORM_NAME_LEN) == true)
		{
			strncpy(profile->platformname, platform, sizeof(profile->platformname));
			profile->platformname[sizeof(profile->platformname)-1] = '\0';
		}

		if(hwm_GetBIOSVersion(bios, DEF_BIOS_VERSION_LEN) == true)
		{
			strncpy(profile->biosversion, bios, sizeof(profile->biosversion));
			profile->biosversion[sizeof(profile->biosversion)-1] = '\0';
		}
		//hwm_CleanupSUSILib();
	}
	else
	if(advcare_StartupAdvCareLib())
	{
		char platform[DEF_PLATFORM_NAME_LEN] = {0};
		char bios[DEF_BIOS_VERSION_LEN] = {0};
		if(advcare_GetPlatformName(platform, DEF_PLATFORM_NAME_LEN))
		{
			strncpy(profile->platformname, platform, strlen(platform)+1);
		}

		if(advcare_GetBIOSVersion(bios, DEF_BIOS_VERSION_LEN))
		{
			strncpy(profile->biosversion, bios, strlen(bios)+1);
		}
		//advcare_CleanupAdvCareLib();
	} 

	return true;
}