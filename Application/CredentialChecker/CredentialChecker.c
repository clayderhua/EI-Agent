/* standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "CredentialHelper.h"
#include "OsDeclarations.h"
#include "Susi4.h"
#include "util_libloader.h"
#include "util_string.h"
#include "util_path.h"
#include "configuration.h"
#include "profile.h"

#define LOG_TAG	"CredentialChk"
#include "Log.h"
//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------
#define USERNAME_ITEM "/credential/protocols/%s/username"
#define PASSWORD_ITEM "/credential/protocols/%s/password"
#define PORT_ITEM "/credential/protocols/%s/port"
#define HOST_ITEM "/serviceHost"
#define SSL_ITEM "/credential/protocols/%s/ssl"

#define DEF_SUSIACCESSAGENT_CONFIG_NAME "agent_config.xml"
#define DEF_SUSIACCESSAGENT_DEFAULT_CONFIG_NAME "agent_config_def.xml"

#ifdef WIN32
#define DEF_SUSI4_LIB_NAME    "Susi4.dll"
#else
#define DEF_SUSI4_LIB_NAME    "libSUSI-4.00.so"
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <windows.h>
#ifndef SUSI_API
#define SUSI_API WINAPI
#endif
#else
#define SUSI_API
#endif
typedef uint32_t (SUSI_API *PSusiLibInitialize)();
typedef uint32_t (SUSI_API *PSusiLibUninitialize)();
typedef uint32_t (SUSI_API *PSusiBoardGetUniqueID)(char **pszUUID);

void * hSUSI4Dll = NULL;
PSusiLibInitialize pSusiLibInitialize = NULL;
PSusiLibUninitialize pSusiLibUninitialize = NULL;
PSusiBoardGetUniqueID pSusiBoardGetUniqueID = NULL;


#define CREDENTIALCHK_LOG_ENABLE
//#define DEF_CREDENTIALCHK_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_CREDENTIALCHK_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_CREDENTIALCHK_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
LOGHANDLE CredentialLogHandle = NULL;
#ifdef CREDENTIALCHK_LOG_ENABLE
#define CredentialChkLog(level, fmt, ...)  do { if (CredentialLogHandle != NULL)   \
	WriteLog(CredentialLogHandle, DEF_CREDENTIALCHK_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define CredentialChkLog(level, fmt, ...)
#endif

bool CredChecker_RestoreConfigDefault(char* workdir)
{
	char CAgentConfigPath[DEF_MAX_PATH] = {0};
	char CAgentDefConfigPath[DEF_MAX_PATH] = {0};
	FILE *pDefFile = NULL;
	FILE *pToFile = NULL;

	util_path_combine(CAgentConfigPath, workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	util_path_combine(CAgentDefConfigPath, workdir, DEF_SUSIACCESSAGENT_DEFAULT_CONFIG_NAME);

	pToFile = fopen( CAgentConfigPath, "w+" );
	if ( pToFile == 0 )
	{
		return false;
	}

	pDefFile = fopen( CAgentDefConfigPath, "r" );
	if ( pDefFile == 0 )
	{
		fclose( pToFile );
		return false;
	}
	else
	{
		int x;
		while  ( ( x = fgetc( pDefFile ) ) != EOF )
		{
			fputc( x, pToFile);
		}
		fclose( pDefFile );
	}
	fclose( pToFile );
	return true;
}

bool CredChecker_LoadConfig(char const * workdir, susiaccess_agent_conf_body_t * conf)
{
	bool bCfgRetry = true;
	char CAgentConfigPath[DEF_MAX_PATH] = {0};
	if(workdir == NULL) return false;
	if(conf == NULL) return false;
	util_path_combine(CAgentConfigPath, workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
CFG_RETRY:
	if(!cfg_load(CAgentConfigPath, conf))
	{
		if(bCfgRetry)
		{
			fprintf(stderr, "CFG load failed: %s, restore default!\n", CAgentConfigPath);
			CredChecker_RestoreConfigDefault(workdir);
			bCfgRetry = false;
			goto CFG_RETRY;
		}
		fprintf(stderr, "Resotire CFG file failed, create new one!\n");
		if(!cfg_create(CAgentConfigPath, conf))
			fprintf(stderr, "CFG create failed: %s\n", CAgentConfigPath);
	}
	return bCfgRetry;
}

bool CredChecker_UpdateConfig(char const * workdir, susiaccess_agent_conf_body_t * conf)
{
	char CAgentConfigPath[DEF_MAX_PATH] = {0};
	if(workdir == NULL) return false;
	if(conf == NULL) return false;
	util_path_combine(CAgentConfigPath, workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	
	return cfg_save(CAgentConfigPath, conf);
}

bool CredChecker_LoadProfile(char const * workdir, susiaccess_agent_profile_body_t * profile)
{
	bool bCfgRetry = true;
	char CAgentConfigPath[DEF_MAX_PATH] = {0};
	if(workdir == NULL) return false;
	if(profile == NULL) return false;
	util_path_combine(CAgentConfigPath, workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
PROFILE_RETRY:
	if(!profile_load(CAgentConfigPath, profile))
	{
		if(bCfgRetry)
		{
			fprintf(stderr, "Profile load failed: %s, restore default!\n", CAgentConfigPath);
			CredChecker_RestoreConfigDefault(workdir);
			bCfgRetry = false;
			goto PROFILE_RETRY;
		}
		fprintf(stderr, "Resotire Profile file failed, create new one!\n");

		if(!profile_create(CAgentConfigPath, profile))
			fprintf(stderr, "Profile create failed: %s\n", CAgentConfigPath);
	}
	return true;
}

bool CredChecker_UpdateProfile(char const * workdir, susiaccess_agent_profile_body_t * profile)
{
	char CAgentConfigPath[DEF_MAX_PATH] = {0};
	if(workdir == NULL) return false;
	if(profile == NULL) return false;
	util_path_combine(CAgentConfigPath, workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	
	return profile_save(CAgentConfigPath, profile);
}

void GetSUSI4Function(void * hSUSI4DLL)
{
	if(hSUSI4Dll!=NULL)
	{
		pSusiLibInitialize = (PSusiLibInitialize)util_dlsym(hSUSI4Dll, "SusiLibInitialize");
		pSusiLibUninitialize = (PSusiLibUninitialize)util_dlsym(hSUSI4Dll, "SusiLibUninitialize");
#ifdef WIN32
		pSusiBoardGetUniqueID = (PSusiBoardGetUniqueID)util_dlsym(hSUSI4Dll, "SusiBoardGetUniqueID");
#else
		pSusiBoardGetUniqueID = (PSusiBoardGetUniqueID)util_dlsym(hSUSI4Dll, "_Z20SusiBoardGetUniqueIDPPc");
#endif
	}
}

bool StartupSUSI4Lib()
{
	bool bRet = false;
	if(util_dlopen(DEF_SUSI4_LIB_NAME, &hSUSI4Dll))
	{
		GetSUSI4Function(hSUSI4Dll);
		if(pSusiLibInitialize)
		{
			uint32_t iRet = pSusiLibInitialize();
			if(iRet != SUSI_STATUS_NOT_INITIALIZED)
			{
				bRet = true;
			}
		}
	}
//#ifdef DISABLE_SUSI
//	bRet = true;
//#endif
	return bRet;
}

bool CleanupSUSI4Lib()
{
	bool bRet = false;
	if(pSusiLibUninitialize)
	{
		uint32_t iRet = pSusiLibUninitialize();
		if(iRet == SUSI_STATUS_SUCCESS)
		{
			bRet = true;
		}
	}
	if(hSUSI4Dll != NULL)
	{
		util_dlclose(hSUSI4Dll);
		hSUSI4Dll = NULL;
		pSusiLibInitialize = NULL;
		pSusiLibUninitialize = NULL;
		pSusiBoardGetUniqueID = NULL;
	}
	return bRet;
}


bool QueryCredentailInfo(char* url, char* protocol, char* devId)
{
	bool bRet = false;
	char workdir[DEF_MAX_PATH] = {0};
	susiaccess_agent_conf_body_t conf;
	susiaccess_agent_profile_body_t profile;

	util_module_path_get(workdir);

	memset(&conf, 0, sizeof(susiaccess_agent_conf_body_t));
	CredChecker_LoadConfig(workdir, &conf);

	if(strcmp(conf.runMode, "Auto")!=0)
		return bRet;

	memset(&profile, 0, sizeof(susiaccess_agent_profile_body_t));
	CredChecker_LoadProfile(workdir, &profile);
	
	if(StartupSUSI4Lib())
	{
		if(pSusiBoardGetUniqueID)
		{
			char *strMac = NULL;
			uint32_t iRet = pSusiBoardGetUniqueID(&strMac);
			if(iRet == SUSI_STATUS_SUCCESS)
			{
				char* data = NULL;
				char* idlist = NULL;
				char* content = NULL;
				//printf("%s\n", strMac);
				idlist = StringReplace(strMac, ",", "\",\"");
				content = calloc(1, strlen(idlist)+5);
				sprintf(content, "[\"%s\"]", idlist);
				free(idlist);	
				//printf("%s\n", content);
				data = Cred_ZeroConfig(content);
				free(content);
				if(data)
				{
					char* name = Cred_ParseCredential(data, "/credential");

					if(name)
					{
						if(strcmp(name, "null")!=0)
						{
							char* host = NULL;
							char* pass = NULL;
							host = Cred_ParseCredential(data, "/credential/url");
							//printf("host: %s \n", host);
							
							pass = Cred_ParseCredential(data, "/credential/connectionKey");
							//printf("password: %s \n", pass);

							/*casting URL*/
							if(host && pass)
							{
								const char *last_char, *temp = host;
								while(*temp != '\0')
								{
									last_char = temp;
									temp++;        
								}

								if(strcmp(last_char, "/") != 0) 
								{
									sprintf(url, "%s/%s", host, pass);
								}
								else
								{
									sprintf(url, "%s%s", host, pass);
								}

								if(conf.tlstype == 0)
									strcpy(protocol, "mqtt");
								else
									strcpy(protocol, "mqtt+ssl");

								strncpy(conf.credentialURL, host, sizeof(conf.credentialURL));
								strncpy(conf.iotKey, pass, sizeof(conf.iotKey));
								CredChecker_UpdateConfig(workdir, &conf);

								strcpy(devId, profile.devId);
								
								strncpy(profile.account, pass, sizeof(profile.account));
								CredChecker_UpdateProfile(workdir, &profile);

							} 

							if(host)
								Cred_FreeBuffer(host);

							if(pass)
								Cred_FreeBuffer(pass);
						}
						Cred_FreeBuffer(name);
					}
					Cred_FreeBuffer(data);
				}
				bRet = true;
			}
		}
		CleanupSUSI4Lib();
	} else {
		printf("%s,%s,%s,%s,%s,%s", "", "", "", "Device Not Support Zero-touch Onboard!", "0", "false");
	}
	return bRet;
}


int main(int argc, char *argv[]) {
	char url[1024] = {0};
	char item[256] = {0};
	char protocol[32] = "mqtt+ssl";
	char deviceId[256] = { 0 };
	char* data = NULL;
	char* name = NULL;
	char* pass = NULL;
	char* host = NULL;
	char* port = NULL;
	char* ssl = NULL;
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif

	char mdPath[MAX_PATH] = { 0 };
	char* response = NULL;
	util_module_path_get(mdPath);
	CredentialLogHandle = InitLog(mdPath);

	if(argc <= 1)
		goto CREDENTIAL_QUERY;

	strncpy(url, argv[1], sizeof(url));

	if (argc >= 3)
		strncpy(protocol, argv[2], sizeof(protocol));

	if (argc >= 4)
		strncpy(deviceId, argv[3], sizeof(deviceId));

	CredentialChkLog(Debug, "First Cred_GetCredentialWithDevId: %s", url);
	data = Cred_GetCredentialWithDevId(url, NULL, deviceId, &response);
CREDENTIAL_QUERY:
	if(data == NULL || strlen(data) <= 0)
	{
		CredentialChkLog(Error, "First Cred_GetCredentialWithDevId response: %s", response);
		if (response)
			free(response);
		response = NULL;
		if(data)
			Cred_FreeBuffer(data);
		CredentialChkLog(Debug, "QueryCredentailInfo: %s", url);
		if(!QueryCredentailInfo(url, protocol, deviceId))
			return -1;
		CredentialChkLog(Debug, "Second Cred_GetCredentialWithDevId: %s", url);
		data = Cred_GetCredentialWithDevId(url, NULL, deviceId, &response);
		if(data == NULL || strlen(data) <= 0)
		{
			CredentialChkLog(Error, "Second Cred_GetCredentialWithDevId response: %s", response);
			if (response)
				free(response);
			response = NULL;
			if(data)
				Cred_FreeBuffer(data);
			return -1;
		}
	}
	CredentialChkLog(Debug, "Cred_GetCredentialWithDevId response: %s", response);
	if (response)
		free(response);
	response = NULL;
	sprintf(item, USERNAME_ITEM, protocol);
	name = Cred_ParseCredential(data, item);
	
	sprintf(item, PASSWORD_ITEM, protocol);
	pass = Cred_ParseCredential(data, item);
		
	sprintf(item, PORT_ITEM, protocol);
	port = Cred_ParseCredential(data, item);

	//sprintf(item, HOST_ITEM, protocol);
	strcpy(item, HOST_ITEM);
	host = Cred_ParseCredential(data, item);

	sprintf(item, SSL_ITEM, protocol);
	ssl = Cred_ParseCredential(data, item);

	printf("%s,%s,%s,%s,%s,%s", protocol, name, pass, host, port, ssl?ssl:"false");

	if(name)
		Cred_FreeBuffer(name);
	if(pass)
		Cred_FreeBuffer(pass);
	if(port)
		Cred_FreeBuffer(port);
	if(host)
		free(host);
	if(ssl)
		Cred_FreeBuffer(ssl);
    /* exit */
	if(data)
		Cred_FreeBuffer(data);
	

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif


    return 0;
}

