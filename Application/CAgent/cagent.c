/* This provides a crude manner of testing the performance of a broker in messages/s. */

#include <configuration.h>
#include <profile.h>
#include <XMLBase.h>
#include <SAGatherInfo.h>
#include <SAClient.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "util_path.h"
#include "WISEPlatform.h"

#include "cagentlog.h"
#include "version.h"
#include "service.h"
#include "cagent.h"

#define DEF_SUSIACCESSAGENT_CONFIG_NAME "agent_config.xml"
#define DEF_SUSIACCESSAGENT_DEFAULT_CONFIG_NAME "agent_config_def.xml"
#define DEF_SUSIACCESSAGENT_STATUS_NAME "agent_status"

#ifdef WIN32
	#define CREDENTIAL_CHECKER "CredentialChecker.exe"
#else
	#define CREDENTIAL_CHECKER "CredentialChecker"
#endif
typedef struct{
	pthread_t threadHandler;
	int iReconnDelay;
	int iPrevDelay;
	bool isThreadRunning;
	bool isSAClientInit;
	bool isConnectResponsed;
	pthread_mutex_t ConnectMutex;
	pthread_cond_t ConnectCond;
	susiaccess_agent_conf_body_t config;
	susiaccess_agent_profile_body_t profile;
}connect_context_t;

bool g_bConnectRetry = false;
connect_context_t g_ConnContex;
char g_CAgentStatusPath[MAX_PATH] = {0};

void on_connect_cb();
void on_lost_connect_cb();
void on_disconnect_cb();

bool RestoreConfigDefault(char* workdir)
{
	char CAgentConfigPath[MAX_PATH] = {0};
	char CAgentDefConfigPath[MAX_PATH] = {0};
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

bool LoadConfig(char const * workdir, susiaccess_agent_conf_body_t * conf)
{
	bool bCfgRetry = true;
	char CAgentConfigPath[MAX_PATH] = {0};
	if(workdir == NULL) return false;
	if(conf == NULL) return false;
	util_path_combine(CAgentConfigPath, workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
CFG_RETRY:
	if(!cfg_load(CAgentConfigPath, conf))
	{
		if(bCfgRetry)
		{
			SUSIAccessAgentLog(Error, "CFG load failed: %s, restore default!", CAgentConfigPath);
			RestoreConfigDefault(workdir);
			bCfgRetry = false;
			goto CFG_RETRY;
		}
		SUSIAccessAgentLog(Error, "Resotire CFG file failed, create new one!");
		if(!cfg_create(CAgentConfigPath, conf))
			SUSIAccessAgentLog(Error, "CFG create failed: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Normal, "CFG Created: %s", CAgentConfigPath);
	}
	return bCfgRetry;
}

bool LoadProfile(char const * workdir, susiaccess_agent_profile_body_t * profile)
{
	bool bCfgRetry = true;
	char CAgentConfigPath[MAX_PATH] = {0};
	if(workdir == NULL) return false;
	if(profile == NULL) return false;
	util_path_combine(CAgentConfigPath, workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
PROFILE_RETRY:
	if(!profile_load(CAgentConfigPath, profile))
	{
		if(bCfgRetry)
		{
			SUSIAccessAgentLog(Error, "Profile load failed: %s, restore default!", CAgentConfigPath);
			RestoreConfigDefault(g_ConnContex.profile.workdir);
			bCfgRetry = false;
			goto PROFILE_RETRY;
		}
		SUSIAccessAgentLog(Error, "Resotire Profile file failed, create new one!");

		if(!profile_create(CAgentConfigPath, profile))
			SUSIAccessAgentLog(Error, "Profile create failed: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Normal, "Profile Created: %s", CAgentConfigPath);
	}
	return true;
}

bool CheckCredential(char const * workdir, susiaccess_agent_conf_body_t * conf, susiaccess_agent_profile_body_t * profile)
{
	FILE *fd = NULL;
	char path[256] = {0};
	char cmd[1024] = {0};
	char buffer[2048] = {0};
	char protocol[32] = {0};
	char credential[768] = {0};
	/*if(strlen(conf->credentialURL) == 0 || strlen(conf->iotKey) == 0)
	{
		SUSIAccessAgentLog(Warning, "CredentialURL or IoTKey is empty!");
		return false;
	}*/

	util_path_combine(path, workdir, CREDENTIAL_CHECKER);
	if(!util_is_file_exist(path))
	{
		SUSIAccessAgentLog(Warning, "%s not found!", path);
		return false;
	}

	/*casting URL*/
	if(strlen(conf->credentialURL) == 0 || strlen(conf->iotKey) == 0)
	{
		sprintf(cmd, "\"%s\"", path);
	}
	else
	{
		const char *last_char, *temp = conf->credentialURL;
		while(*temp != '\0')
		{
			last_char = temp;
			temp++;        
		}

		if(strcmp(last_char, "/") != 0) 
		{
			sprintf(credential, "%s/%s", conf->credentialURL, conf->iotKey);
		}
		else
		{
			sprintf(credential, "%s%s", conf->credentialURL, conf->iotKey);
		}

		if(conf->tlstype <= tls_type_none)
			sprintf(cmd, "\"%s\" %s mqtt %s", path, credential, profile->devId);
		else
			sprintf(cmd, "\"%s\" %s mqtt+ssl %s", path, credential, profile->devId);
	} 

	fd = popen(cmd, "r");
	if(fd == NULL)
	{
		SUSIAccessAgentLog(Debug, "Execute (popen) fail: %s [%s]", cmd, strerror(errno));
		return false;
	}

	while(fgets(buffer, sizeof(buffer), fd))
	{
		if(strstr(buffer, "mqtt")==0)
		{
			SUSIAccessAgentLog(Debug, "Readline: %s", buffer);
		}		
		else
		{
			break;
		}
	}
	pclose(fd);
	if(strstr(buffer, "mqtt")!=0)
	{
		SUSIAccessAgentLog(Normal, "Agent Credential: %s", buffer);
		{/*remove '\n'*/
			char *pos;
			if ((pos = strchr(buffer, '\n')) != NULL)
				*pos = '\0';
		}

		if(strcmp(conf->runMode,"Auto") == 0)
		{/*Reload Config*/
			susiaccess_agent_conf_body_t tmp_conf;
			susiaccess_agent_profile_body_t tmp_profile;
			LoadConfig(workdir, &tmp_conf);
			strncpy(conf->credentialURL, tmp_conf.credentialURL, sizeof(conf->credentialURL));
			strncpy(conf->iotKey, tmp_conf.iotKey, sizeof(conf->iotKey));

			LoadProfile(workdir, &tmp_profile);
			strncpy(profile->account, tmp_profile.account, sizeof(profile->account));
		}

		{/*Parse credential data*/
			char* token = NULL;
			char* protocol = strtok_r(buffer, ",", &token);
			char* name = strtok_r(NULL, ",", &token);
			char* pass = strtok_r(NULL, ",", &token);
			char* host = strtok_r(NULL, ",", &token);
			char* port = strtok_r(NULL, ",", &token);
			char* ssl = strtok_r(NULL, ",", &token);
			printf("%s\n", protocol);
			printf("%s\n", name);
			printf("%s\n", pass);
			printf("%s\n", host);
			printf("%s\n", port);
			printf("%s\n", ssl);
			sprintf(conf->serverAuth,"%s;%s", name, pass);
			sprintf(conf->serverIP, "%s", host);
			sprintf(conf->serverPort, "%s", port);
			if(!strcasecmp(ssl, "true"))
			{
				if(conf->tlstype == tls_type_none)
					conf->tlstype = tls_type_psk;
			}
			else
				conf->tlstype = tls_type_none;
		}
		return true;
	}
	else if(strlen(buffer) > 0)
	{
		char* token = NULL;
		char* protocol = strtok_r(buffer, ",", &token);
		SUSIAccessAgentLog(Warning, "%s", protocol);
		return false;
	}
	else
	{		
		SUSIAccessAgentLog(Warning, "Credential no response!");
		return false;
	}
}

bool GatherInfo(char const * workdir, susiaccess_agent_conf_body_t * conf, susiaccess_agent_profile_body_t * profile)
{
	char version[DEF_MAX_STRING_LENGTH] = {0};
	char CAgentConfigPath[MAX_PATH] = {0};
	if(workdir == NULL) return false;
	if(conf == NULL) return false;
	if(profile == NULL) return false;
	util_path_combine(CAgentConfigPath, workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	sagetInfo_gatherplatforminfo(profile);
	snprintf(version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);
	if(strcmp(profile->version, version))
	{
		strncpy(profile->version, version, sizeof(profile->version));
		profile_set(CAgentConfigPath, "SWVersion", profile->version);
		SUSIAccessAgentLog(Warning, "Software Version %s!", profile->version);
	}

	if(strlen(conf->identity)==0)
		strncpy(conf->identity, profile->devId, sizeof(conf->identity));
	return true;
}

static void reload_agent_credentail(connect_context_t *pConnContex)
{
	char CAgentConfigPath[MAX_PATH] = {0};
	void* doc;

	util_path_combine(CAgentConfigPath, g_ConnContex.profile.workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	doc = cfg_doc_open(CAgentConfigPath);

	cfg_doc_get(doc, "CredentialURL",  pConnContex->config.credentialURL, sizeof(pConnContex->config.credentialURL));
	cfg_doc_get(doc, "IoTKey",  pConnContex->config.iotKey, sizeof(pConnContex->config.iotKey));
	cfg_doc_get(doc, "TLSType",  CAgentConfigPath, sizeof(CAgentConfigPath));
	pConnContex->config.tlstype = strtol(CAgentConfigPath, NULL, 10);
	cfg_doc_get(doc, "CAFile",  pConnContex->config.cafile, sizeof(pConnContex->config.cafile));
	cfg_doc_get(doc, "CAPath",  pConnContex->config.capath, sizeof(pConnContex->config.capath));
	cfg_doc_get(doc, "CertFile",  pConnContex->config.certfile, sizeof(pConnContex->config.certfile));
	cfg_doc_get(doc, "KeyFile",  pConnContex->config.keyfile, sizeof(pConnContex->config.keyfile));
	cfg_doc_get(doc, "CertPW",  pConnContex->config.cerpasswd, sizeof(pConnContex->config.cerpasswd));

	cfg_doc_close(doc);
}

static void* ConnectThreadStart(void* args)
{
	int iConnResult = saclient_false;
	connect_context_t *pConnContex = (connect_context_t *)args;
	if(pConnContex == NULL)
	{
		pConnContex->isThreadRunning = false;
		pthread_exit(0);
		return 0;
	}
	//pConnContex->isThreadRunning = true;

	saclient_disconnect();
	//SUSIAccessAgentLog(Normal, "Agent Disconnected!");

	if(pConnContex->iReconnDelay>0)
	{
		int count = g_ConnContex.iReconnDelay/10;
		while(count>0)
		{
			count--;
			if(!pConnContex->isThreadRunning)
				break;
			usleep(10000);
		}
	}
	if(!pConnContex->isThreadRunning )
	{
		pthread_exit(0);
		return 0;
	}
	while(pConnContex->isThreadRunning)
	{
		iConnResult = saclient_false;
		if(strlen(pConnContex->profile.mac) <=0 )
		{
			char CAgentConfigPath[MAX_PATH] = {0};
			util_path_combine(CAgentConfigPath, g_ConnContex.profile.workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	GAT_INFO:
			GatherInfo(pConnContex->profile.workdir, &pConnContex->config, &pConnContex->profile);

			if(strlen(pConnContex->profile.mac) <=0 )
			{
				SUSIAccessAgentLog(Error, "No MAC Address retrieved!");	
				usleep(100*1000);
				if(pConnContex->isThreadRunning)
					goto GAT_INFO;
				else
					break;
			}
			else
			{
				if(strlen(pConnContex->profile.devId) <=0 )
				{
					sprintf(pConnContex->profile.devId, "00000001-0000-0000-0000-%s", pConnContex->profile.mac);
					profile_set(CAgentConfigPath, "DevID",pConnContex->profile.devId);
				}
				if(strlen(pConnContex->profile.sn) <=0 )
				{
					strncpy(pConnContex->profile.sn,pConnContex->profile.mac, sizeof(pConnContex->profile.sn));
					profile_set(CAgentConfigPath, "SN",pConnContex->profile.sn);
				}
			}
			SUSIAccessAgentLog(Normal, "Agent Reinitialize");
		}

		if (strcmp(pConnContex->config.runMode,"Auto")!=0) { // reload config if it is not auto mode
			reload_agent_credentail(pConnContex);
		}

		if(strcmp(pConnContex->config.runMode,"Auto")!=0 && (strlen(pConnContex->config.credentialURL) == 0 || strlen(pConnContex->config.iotKey) == 0))
		{
			SUSIAccessAgentLog(Warning, "CredentialURL or IoTKey is empty! Skip Credential query");
			iConnResult = saclient_server_connect_config(&pConnContex->config, &pConnContex->profile);
		}
		else
		{
			if(CheckCredential(pConnContex->profile.workdir, &pConnContex->config, &pConnContex->profile))
			{
				iConnResult = saclient_server_connect_config(&pConnContex->config, &pConnContex->profile);
			}
		}
		if(iConnResult == saclient_success)
			break;
		else
		{
			int count = 0;
			int currentDelay = pConnContex->iReconnDelay;
			SUSIAccessAgentLog(Debug, "Agent Connect fail, reconnect after %d sec.", currentDelay / 1000);
			if (pConnContex->iReconnDelay < 300000)
			{
				pConnContex->iReconnDelay += pConnContex->iPrevDelay;
				pConnContex->iPrevDelay = currentDelay;
			}
				
			count = pConnContex->iReconnDelay/10;
			while(count>0)
			{
				count--;
				if(!pConnContex->isThreadRunning)
					break;
				usleep(10000);
			}
		}
	}
	pConnContex->isThreadRunning = false;
	pthread_exit(0);
	return 0;
}
bool Reconnect();
static void* ConnectTimeoutThread(void* args)
{
	connect_context_t* pConnContex = (connect_context_t*)args;
	struct timespec timeToWait;
	/*int countdown = 60;
	while (countdown > 0)
	{
		usleep(1000000);
		if (pConnContex->isConnectResponsed)
			break;
		countdown--;
	}*/
	timeToWait.tv_sec = time(NULL) + 60; // wait 1 minute
	timeToWait.tv_nsec = 0;

	pthread_mutex_lock(&pConnContex->ConnectMutex);
	pthread_cond_timedwait(&pConnContex->ConnectCond, &pConnContex->ConnectMutex, &timeToWait);
	pthread_mutex_unlock(&pConnContex->ConnectMutex);

	if (!pConnContex->isConnectResponsed)
		Reconnect();
	pthread_exit(0);
	return 0;
}

bool ConnectToServer(connect_context_t *pConnContex)
{
	bool bRet =false;
	pthread_t threadHandler;
	if(pConnContex->isThreadRunning)
		return bRet;

	pConnContex->isThreadRunning = true;
	pConnContex->isConnectResponsed = false;
	g_bConnectRetry = strcasecmp(pConnContex->config.autoStart, "True") == 0;
	if (pthread_create(&pConnContex->threadHandler, NULL, ConnectThreadStart, pConnContex) != 0)
	{
		pConnContex->isThreadRunning = false;
		SUSIAccessAgentLog(Error, "start Connecting thread failed!\n");	
		bRet = false;
	}
	bRet = true;
	if (pthread_create(&threadHandler, NULL, ConnectTimeoutThread, pConnContex) != 0)
	{
		pConnContex->isConnectResponsed = true;
		SUSIAccessAgentLog(Error, "start Connecting timeout thread failed!\n");
	}
	else
	{
		pthread_detach(threadHandler);
		threadHandler = 0;
	}

	return bRet;
}

bool Disconnect()
{
	bool bRet = false;
	g_bConnectRetry = false;

	if(g_ConnContex.isThreadRunning == true)
	{
		g_ConnContex.isThreadRunning = false;
		if(g_ConnContex.threadHandler)
			pthread_join(g_ConnContex.threadHandler, NULL);
		g_ConnContex.threadHandler = 0;
	}
	saclient_disconnect();
	SUSIAccessAgentLog(Normal, "Agent Disconnected!");
	return bRet;
}

bool Reconnect()
{
	bool bRet = false;
	int current = g_ConnContex.iReconnDelay;
	g_bConnectRetry = false;

	if(g_ConnContex.isThreadRunning == true)
	{
		return bRet;
	}
	else
	{
		if (g_ConnContex.threadHandler)
		{
			SUSIAccessAgentLog(Debug, "Wait Connect Thread Stop!");
			pthread_join(g_ConnContex.threadHandler, NULL);
		}
		SUSIAccessAgentLog(Debug, "Connect Thread Stopped!");
		g_ConnContex.threadHandler = 0;
		g_ConnContex.isThreadRunning = false;
	}
	if (g_ConnContex.iReconnDelay < 300000)
	{
		g_ConnContex.iReconnDelay += g_ConnContex.iPrevDelay;
		g_ConnContex.iPrevDelay = current;
		current = g_ConnContex.iReconnDelay;
	}
	SUSIAccessAgentLog(Normal, "Agent reconnect after %d sec.", current / 1000);
	return ConnectToServer(&g_ConnContex);
}

void WriteStatus(char * statusFileName, char* status)
{
	FILE * statusFile = NULL;
    statusFile = fopen(statusFileName, "w+");
	if(statusFile)
	{
		//fputs(status, statusFile);
		fprintf(statusFile, "%s\r\n", status);
		fflush(statusFile);
		fclose(statusFile);
	}
}

void on_connect_cb()
{
	pthread_mutex_lock(&g_ConnContex.ConnectMutex);
	g_ConnContex.isConnectResponsed = true;
	pthread_cond_signal(&g_ConnContex.ConnectCond);
	pthread_mutex_unlock(&g_ConnContex.ConnectMutex);
	
	WriteStatus(g_CAgentStatusPath, "1");

	g_ConnContex.iReconnDelay = 0;
	g_ConnContex.iPrevDelay = 5000;
	//g_ConnContex.isThreadRunning == false;
	//SUSIAccessAgentLog(Normal, "Broker connected!");
}

void on_lost_connect_cb()
{
	pthread_mutex_lock(&g_ConnContex.ConnectMutex);
	g_ConnContex.isConnectResponsed = true;
	pthread_cond_signal(&g_ConnContex.ConnectCond);
	pthread_mutex_unlock(&g_ConnContex.ConnectMutex);

	WriteStatus(g_CAgentStatusPath, "0");
	//Disconnect();
	Reconnect();
	/*if(g_bConnectRetry)
	{
		SUSIAccessAgentLog(Normal, "Broker Lost connect, retry!");
	}
	else
	{
		SUSIAccessAgentLog(Normal, "Broker Lost connect!");
	}*/
}

void on_disconnect_cb()
{
	pthread_mutex_lock(&g_ConnContex.ConnectMutex);
	g_ConnContex.isConnectResponsed = true;
	pthread_cond_signal(&g_ConnContex.ConnectCond);
	pthread_mutex_unlock(&g_ConnContex.ConnectMutex);

	WriteStatus(g_CAgentStatusPath, "0");
	//g_ConnContex.isThreadRunning == false;
	//SUSIAccessAgentLog(Normal, "Broker Disconnected!");
}

int CAgentStart()
{
	int iRet = -1;
	char CAgentConfigPath[MAX_PATH] = {0};
	bool bCfgRetry = true;

	memset(&g_ConnContex, 0, sizeof(connect_context_t));
	util_module_path_get(g_ConnContex.profile.workdir);

	if(!SUSIAccessAgentLogHandle)
	{
		SUSIAccessAgentLogHandle = InitLog(g_ConnContex.profile.workdir);
		SUSIAccessAgentLog(Normal, "Current path: %s", g_ConnContex.profile.workdir);
	}
	util_path_combine(g_CAgentStatusPath, g_ConnContex.profile.workdir, DEF_SUSIACCESSAGENT_STATUS_NAME);
	WriteStatus(g_CAgentStatusPath, "0");

	pthread_mutex_init(&g_ConnContex.ConnectMutex, NULL);
	pthread_cond_init(&g_ConnContex.ConnectCond, NULL);

	util_path_combine(CAgentConfigPath, g_ConnContex.profile.workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	
	LoadConfig(g_ConnContex.profile.workdir, &g_ConnContex.config);

	LoadProfile(g_ConnContex.profile.workdir, &g_ConnContex.profile);

	GatherInfo(g_ConnContex.profile.workdir, &g_ConnContex.config, &g_ConnContex.profile);

	if(strlen(g_ConnContex.profile.mac) <=0 )
	{
		SUSIAccessAgentLog(Error, "No MAC Address retrieved!");	
	}
	else
	{
		if(strlen(g_ConnContex.profile.devId) <=0 )
		{
			sprintf(g_ConnContex.profile.devId, "00000001-0000-0000-0000-%s", g_ConnContex.profile.mac);
			profile_set(CAgentConfigPath, "DevID",g_ConnContex.profile.devId);
		}
		if(strlen(g_ConnContex.profile.sn) <=0 )
		{
			strncpy(g_ConnContex.profile.sn,g_ConnContex.profile.mac, sizeof(g_ConnContex.profile.sn));
			profile_set(CAgentConfigPath, "SN",g_ConnContex.profile.sn);
		}
	}
	/*
	if(strlen(g_ConnContex.config.serverIP) <=0 )
	{
		SUSIAccessAgentLog(Error, "No Server IP!");	
		goto EXIT_START;
	}
	*/
	if(saclient_initialize(&g_ConnContex.config, &g_ConnContex.profile, SUSIAccessAgentLogHandle) != saclient_success)
	{
		g_ConnContex.isSAClientInit = false;
		SUSIAccessAgentLog(Error, "Unable to initialize AgentCore.");		
		saclient_uninitialize();
		SUSIAccessAgentLog(Error, "Agent initialize failed!");
		goto EXIT_START;
	}
	SUSIAccessAgentLog(Normal, "Agent Initialized");
	g_ConnContex.isSAClientInit = true;
	saclient_connection_callback_set(on_connect_cb, on_lost_connect_cb, on_disconnect_cb);

	g_ConnContex.iReconnDelay = 0;
	g_ConnContex.iPrevDelay = 5000;

	iRet = ConnectToServer(&g_ConnContex)?0:-1;
EXIT_START:
	return iRet;
}

int CAgentStop()
{
	pthread_mutex_unlock(&g_ConnContex.ConnectMutex);

	WriteStatus(g_CAgentStatusPath, "0");

	if(g_ConnContex.threadHandler)
	{
		g_ConnContex.isThreadRunning = false;
		if(g_ConnContex.threadHandler)
			pthread_join(g_ConnContex.threadHandler, NULL);
		g_ConnContex.threadHandler = 0;
	}
	Disconnect();

	if(g_ConnContex.isSAClientInit)
		saclient_uninitialize();
	
	if(SUSIAccessAgentLogHandle!=NULL)
	{
		UninitLog(SUSIAccessAgentLogHandle);
		SUSIAccessAgentLogHandle = NULL;
	}

	pthread_mutex_lock(&g_ConnContex.ConnectMutex);
	g_ConnContex.isConnectResponsed = true;
	pthread_cond_signal(&g_ConnContex.ConnectCond);
	pthread_mutex_destroy(&g_ConnContex.ConnectMutex);
	pthread_cond_destroy(&g_ConnContex.ConnectCond);
	return 0;
}

int LoadServerName(char* configFile, char* name, int size)
{
	int iRet = 0;
	xml_doc_info * doc = xml_Loadfile(configFile, "XMLConfigSettings", "Customize");
	
	if(doc == NULL)
		return iRet;

	memset(name, 0, size);

	if(!xml_GetItemValue(doc, "ServiceName", name, size))
		memset(name, 0, size);
	else
		iRet = strlen(name);
	xml_FreeDoc(doc);
	return iRet;
}

int main(int argc, char *argv[])
{
	bool isSrvcInit = false;
	char moudlePath[MAX_PATH] = {0};
	char CAgentConfigPath[MAX_PATH] = {0};
	char serverName[DEF_OSVERSION_LEN] = {0};
	char version[DEF_VERSION_LENGTH] = {0};
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif  
	sprintf(version, "%d.%d.%d.%d",VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);

	memset(moudlePath, 0 , sizeof(moudlePath));
	
	util_module_path_get(moudlePath);

	
	/*Enable Log*/
	/*{
		FILE * logFile = NULL;
		util_path_combine(CAgentLogPath, "c:\\", DEF_SUSIACCESSAGENT_LOG_NAME);
		logFile = fopen(CAgentLogPath, "ab");
		if(logFile != NULL)
		{
			char logstring[MAX_PATH] = {0};
			sprintf(logstring, "Path: %s\r\n", moudlePath);
			fputs(logstring, logFile);
			fflush(logFile);
			fclose(logFile);
		}
	}*/
	SUSIAccessAgentLogHandle = InitLog(moudlePath);
	SUSIAccessAgentLog(Normal, "Current path: %s", moudlePath);

	/*Load service name fomr config and init service*/
	util_path_combine(CAgentConfigPath, moudlePath, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	if(LoadServerName(CAgentConfigPath, serverName, sizeof(serverName))>0)
	{
		isSrvcInit = ServiceInit(serverName, version, CAgentStart, CAgentStop, SUSIAccessAgentLogHandle)==0?true:false;
	}
	else 
	{
		isSrvcInit = ServiceInit(NULL, version, CAgentStart, CAgentStop, SUSIAccessAgentLogHandle)==0?true:false;
	}

	
	if(!isSrvcInit) return -1;

	if(argv[1] != NULL)
	{
		char cmdChr[MAX_CMD_LEN] = {'\0'};
		memcpy(cmdChr, argv[1], strlen(argv[1]));
		do 
		{
			char* token = NULL;
			bool bRet = ExecuteCmd(strtok_r(cmdChr, "\n", &token))==0?true:false;
			if(!bRet) break;
			memset(cmdChr, 0, sizeof(cmdChr));
			fgets(cmdChr, sizeof(cmdChr), stdin);			
		} while (true);
	}
	else
	{
		if(isSrvcInit)
		{
			LaunchService();
		}
	}
	if(isSrvcInit) ServiceUninit();

	if(SUSIAccessAgentLogHandle!=NULL)
	{
		UninitLog(SUSIAccessAgentLogHandle);
		SUSIAccessAgentLogHandle = NULL;
	}

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif
	return 0;
}
