#include "ghparser.h"
#include <stdio.h>
#include <string.h>
#include "cJSON.h"

#include "util_path.h"

#define AGENTINFO_BODY_STRUCT	"susiCommData"
#define AGENTINFO_CMDTYPE		"commCmd"
#define AGENTINFO_AUTOREPORT	"autoReport"
#define AGENTINFO_REPORTDATALEN	"reportDataLength"
#define AGENTINFO_REPORTDATA	"reportData"
#define AGENTINFO_SESSION_ID	"sessionID"
#define AGENTINFO_CONTENT		"content"

#define GBL_UPDATE_PARMAS                "params"
#define GBL_UPDATE_USERNAME              "userName"
#define GBL_UPDATE_PASSWORD              "pwd"
#define GBL_UPDATE_PORT                  "port"
#define GBL_UPDATE_PATH                  "path"
#define GBL_UPDATE_MD5                   "md5"

#define GBL_RENAME_DEVNAME				 "devName"

#define GBL_SERVER_RESPONSE				 "response"
#define GBL_SERVER_STATUSCODE			 "statuscode"
#define GBL_SERVER_RESPONSEMESSAGE		 "msg"
#define GBL_SERVER_SERVER_NODE			 "server"
#define GBL_SERVER_SERVER_ADDRESS		 "address"
#define GBL_SERVER_SERVER_PORT			 "port"
#define GBL_SERVER_SERVER_AUTH			 "auth"
#define GBL_SERVER_SERVER_IP_LIST		 "serverIPList"
#define GBL_SERVER_N_FLAG				 "n"
#define GBL_SERVER_CREDENTIAL_NODE		 "credential"
#define GBL_SERVER_CREDENTIAL_URL		 "url"
#define GBL_SERVER_CREDENTIAL_IOTKEY	 "iotkey"

bool ParseUpdateCMD(void* data, int datalen, download_params_t *pDownloadParams)
{
	/*{"commCmd":111,"catalogID":4,"requestID":16,"params":{"userName":"sa30Read","pwd":"sa30Read","port":2121,"path":"/upgrade/SA30Agent_V3.0.15.exe","md5":"758C9D0A8654A93D09F375D33E262507"}}*/
	cJSON * root = NULL, *body = NULL, *pSubItem = NULL; 
	cJSON *pDownloadParamsItem = NULL;
	bool bRet = false;
	if(!data) return false;
	if(datalen<=0) return false;
	if(!pDownloadParams) return false;

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		body = root;
	}

	pDownloadParamsItem = cJSON_GetObjectItem(body, GBL_UPDATE_PARMAS);
	if(pDownloadParams && pDownloadParamsItem)
	{
		pSubItem = cJSON_GetObjectItem(pDownloadParamsItem, GBL_UPDATE_USERNAME);
		if(pSubItem)
		{
			strncpy(pDownloadParams->ftpuserName, pSubItem->valuestring, sizeof(pDownloadParams->ftpuserName));
			pSubItem = cJSON_GetObjectItem(pDownloadParamsItem, GBL_UPDATE_PASSWORD);
			if(pSubItem)
			{
				strncpy(pDownloadParams->ftpPassword, pSubItem->valuestring, sizeof(pDownloadParams->ftpPassword));
				pSubItem = cJSON_GetObjectItem(pDownloadParamsItem, GBL_UPDATE_PORT);
				if(pSubItem)
				{
					pDownloadParams->port = pSubItem->valueint;
					pSubItem = cJSON_GetObjectItem(pDownloadParamsItem, GBL_UPDATE_PATH);
					if(pSubItem)
					{
						strncpy(pDownloadParams->installerPath, pSubItem->valuestring, sizeof(pDownloadParams->installerPath));
						pSubItem = cJSON_GetObjectItem(pDownloadParamsItem, GBL_UPDATE_MD5);
						if(pSubItem)
						{
							strncpy(pDownloadParams->md5, pSubItem->valuestring, sizeof(pDownloadParams->md5));
							bRet = true;
						}
					}
				}
			}
		}
	}
	cJSON_Delete(root);
	return bRet;
}

bool ParseReceivedCMD(void* data, int datalen, int * cmdID, char* pSessionID)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10, "sessionID":"XXX"}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return false;
	if(datalen<=0) return false;
	//MonitorLog(g_loghandle, Normal, " %s>Parser_ParseReceivedData [%s]\n", MyTopic, data );
	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		body = root;
	}

	target = cJSON_GetObjectItem(body, AGENTINFO_CMDTYPE);
	if(target)
	{
		*cmdID = target->valueint;
	}

	if(pSessionID != NULL)
	{
		target = cJSON_GetObjectItem(body, AGENTINFO_SESSION_ID);
		if(target)
		{
			strcpy(pSessionID, target->valuestring);
		}
		else
		{
			target = cJSON_GetObjectItem(body, AGENTINFO_CONTENT);
			if(target)
			{
				target = cJSON_GetObjectItem(target, AGENTINFO_SESSION_ID);
				if(target)
				{
					strcpy(pSessionID, target->valuestring);
				}
			}
		}		
	}

	cJSON_Delete(root);
	return true;
}

bool ParseRenameCMD(void* data, int datalen, char* pNewName)
{
	/*{"susiCommData":{"devName":"pc-test1","commCmd":113,"requestID":1001,"agentID":"","handlerName":"","sendTS":1434447015}}*/
	cJSON * root = NULL, *body = NULL, *pSubItem = NULL; 
	bool bRet = false;
	if(!data) return false;
	if(datalen<=0) return false;
	if(!pNewName) return false;

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		body = root;
	}

	pSubItem = cJSON_GetObjectItem(body, GBL_RENAME_DEVNAME);
	if(pSubItem)
	{
		strcpy(pNewName, pSubItem->valuestring);
		bRet = true;
	}
	cJSON_Delete(root);
	return bRet;
}

int ParseServerCtrl(void* data, int datalen, char* workdir, GENERAL_CTRL_MSG *pMessage)
{
	/*{"susiCommData":{"commCmd":125,"handlerName":"general","catalogID":4,"response":{"statuscode":0,"msg":"Server losyconnection"}}}*/
	cJSON *root = NULL, *body = NULL, *pSubItem = NULL, *pTarget = NULL, *pServer = NULL,*pServerIPList = NULL; 
	bool bRet = false;
	if(!data) return false;
	if(datalen<=0) return false;
	if(!pMessage) return false;

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		body = root;
	}

	pSubItem = cJSON_GetObjectItem(body, GBL_SERVER_RESPONSE);
	if(pSubItem)
	{
		pTarget = cJSON_GetObjectItem(pSubItem, GBL_SERVER_STATUSCODE);
		if(pTarget)
		{
			pMessage->statuscode = pTarget->valueint;
		}

		pTarget = cJSON_GetObjectItem(pSubItem, GBL_SERVER_RESPONSEMESSAGE);
		if(pTarget)
		{
			if(pTarget->valuestring)
			{
				if(strlen(pTarget->valuestring)<=0)
					pMessage->msg = NULL;
				else
				{
					pMessage->msg = malloc(strlen(pTarget->valuestring)+1);
					memset(pMessage->msg, 0, strlen(pTarget->valuestring)+1);
					strcpy(pMessage->msg, pTarget->valuestring);
				}
			}
		}

		pServer = cJSON_GetObjectItem(pSubItem, GBL_SERVER_SERVER_NODE);
		if(pServer)
		{
			pTarget = cJSON_GetObjectItem(pServer, GBL_SERVER_SERVER_ADDRESS);
			if(pTarget)
			{
				strncpy(pMessage->serverIP, pTarget->valuestring, sizeof(pMessage->serverIP));
			}

			pTarget = cJSON_GetObjectItem(pServer, GBL_SERVER_SERVER_PORT);
			if(pTarget)
			{
				pMessage->serverPort = pTarget->valueint;
			}

			pTarget = cJSON_GetObjectItem(pServer, GBL_SERVER_SERVER_AUTH);
			if(pTarget)
			{
				strncpy(pMessage->serverAuth, pTarget->valuestring, sizeof(pMessage->serverAuth));
			}
		}
		pServerIPList = cJSON_GetObjectItem(pSubItem, GBL_SERVER_SERVER_IP_LIST);
		if(pServerIPList)
		{
			cJSON * subItem = NULL;
			cJSON * valItem = NULL;
			int i = 0;
			FILE *fp = NULL;
			int nCount = cJSON_GetArraySize(pServerIPList);
			char filepath[MAX_PATH] = {0};
			util_path_combine(filepath, workdir, DEF_SERVER_IP_LIST_FILE);
			if(fp=fopen(filepath,"w+"))
			{
				int i = 0;
				for(i = 0; i<nCount; i++)
				{
					cJSON *subItem = cJSON_GetArrayItem(pServerIPList, i);
					if(subItem)
					{
						cJSON *valItem = cJSON_GetObjectItem(subItem, GBL_SERVER_N_FLAG);
						if(valItem)
						{
							fputs(valItem->valuestring,fp);
							fputc('\n',fp);
						}
					}
				}
				fclose(fp);
			}
		}
		
		/*Connect with New Credential format
		*{
		*	"agentID": "00000001-0000-0000-0000-305A3A77B1DA",
		*	"handlerName": "general",
		*	"commCmd": 125,
		*	"content": {
		*		"response": {
		*			"statuscode": 9,
		*			"msg": "Connect with New Credential",
		*			"credential": {
		*				"url": "https://api-dccs.wise-paas.com/v1/serviceCredentials/",
		*				"iotkey": "ebb54a95dd7aa48cbe74030fd2daa5js"
		*			}
		*		}
		*	}
		*}*/

		pServer = cJSON_GetObjectItem(pSubItem, GBL_SERVER_CREDENTIAL_NODE);
		if(pServer)
		{
			pTarget = cJSON_GetObjectItem(pServer, GBL_SERVER_CREDENTIAL_URL);
			if(pTarget)
			{
				strncpy(pMessage->credentialURL, pTarget->valuestring, sizeof(pMessage->credentialURL));
			}

			pTarget = cJSON_GetObjectItem(pServer, GBL_SERVER_CREDENTIAL_IOTKEY);
			if(pTarget)
			{
				strncpy(pMessage->iotKey, pTarget->valuestring, sizeof(pMessage->iotKey));
			}
		}
		bRet = true;
	}
	cJSON_Delete(root);
	return bRet;
}