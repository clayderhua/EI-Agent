#include "ConfigReader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util_path.h"
#include "ReadINI.h"

bool ReadConfig(char* filepath, struct CONFIG* config)
{
	char tmp[2048] = {0};

	if(!util_is_file_exist(filepath))
		return false;

	if(config == NULL)
		return false;
	strcpy(config->credentialUrl, GetIniKeyString("Platform", "credentialUrl", filepath));
	strcpy(config->iotKey, GetIniKeyString("Platform", "iotKey", filepath));
	strcpy(config->strServerIP, GetIniKeyString("Platform","ip", filepath));
	config->iPort = GetIniKeyInt("Platform","port", filepath);
	//config->SSLMode = GetIniKeyInt("Platform","SSLMode", filepath);
	config->iTotalCount = GetIniKeyInt("Platform","total", filepath);
	strcpy(config->strseed, GetIniKeyString("Platform","seedID", filepath));
	strcpy(config->strAccount, GetIniKeyString("Platform","account", filepath));
	strcpy(config->strPasswd, GetIniKeyString("Platform","passwd", filepath));
	strcpy(config->strPrefix, GetIniKeyString("Platform","prefixname", filepath));
	strcpy(config->containpath, GetIniKeyString("Contain","source", filepath));
	config->frequency = GetIniKeyInt("Platform","frequency", filepath);
	config->launchinterval = GetIniKeyInt("Platform","launchinterval", filepath);
	strcpy(config->strProductTag, GetIniKeyString("Platform","producttag", filepath));
	
	memset(tmp, 0, sizeof(tmp));
	strcpy(tmp, GetIniKeyString("Platform","connid", filepath));
	if(strlen(tmp)>0)
		strcpy(config->strConnID, tmp);

	memset(tmp, 0, sizeof(tmp));
	strcpy(tmp, GetIniKeyString("Platform","connpw", filepath));
	if(strlen(tmp)>0)
		strcpy(config->strConnPW, tmp);

	memset(tmp, 0, sizeof(tmp));
	strcpy(tmp, GetIniKeyString("Platform","reportcountdown", filepath));
	if(strlen(tmp)>0)
		config->reportcountdown = atol(tmp);

	memset(tmp, 0, sizeof(tmp));
	strcpy(tmp, GetIniKeyString("Platform","reconnectdelay", filepath));
	if(strlen(tmp)>0)
		config->reconnectdelay = atol(tmp);

	strcpy(config->strCafile, GetIniKeyString("TLS", "cafile", filepath));
	strcpy(config->strCertfile, GetIniKeyString("TLS", "certfile", filepath));
	strcpy(config->strKeyfile, GetIniKeyString("TLS", "keyfile", filepath));
	strcpy(config->strKeypwd, GetIniKeyString("TLS", "keypwd", filepath));

	return true;
}

MSG_CLASSIFY_T* ParseCapability(char* buff)
{
	MSG_CLASSIFY_T* root = NULL;
	char handler[256] = {0};
	if(!transfer_get_ipso_handlername(buff, handler))
		return root;
	root = IoT_CreateRoot(handler);
	transfer_parse_ipso(buff, root);
	return root;
}

struct CAPABILITY* LoadCapability(char* path)
{
	struct CAPABILITY* pCapabilities = NULL;
	struct CAPABILITY* pLast = NULL;
	int size = 0;
	char* buff = NULL; 
	char* data = NULL;
	MSG_CLASSIFY_T* root = NULL;
	size = util_file_size_get(path);
	buff = calloc(1, size+1);
	if(buff == NULL)
		return pCapabilities;
	if(util_file_read(path, buff, size)==0)
	{
		if(sizeof(buff)==0)
			return pCapabilities;
	}

	data = strtok(buff, "\n");
	while(data)
	{
		root = ParseCapability(data);
		if(root != NULL)
		{
			struct CAPABILITY* pCapab = (struct CAPABILITY*)calloc(1, sizeof(struct CAPABILITY));
			pCapab->pCapability = root;
			if(pLast != NULL)
				pLast->next = pCapab;
			pLast = pCapab;
			if(pCapabilities == NULL)
				pCapabilities = pCapab;
		}
		data = strtok(NULL, "\n");
	}
		
	free(buff);
	return pCapabilities;
}

void FreeCapability(struct CAPABILITY* pCapabilities)
{
	struct CAPABILITY* pNext = pCapabilities;
	while(pNext)
	{
		struct CAPABILITY* pCurrent = pNext;
		pNext = pCurrent->next;

		if(pCurrent->pCapability)
			IoT_ReleaseAll(pCurrent->pCapability);
		free(pCurrent);
	}
}