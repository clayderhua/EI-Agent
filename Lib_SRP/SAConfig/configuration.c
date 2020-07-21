#include "XMLBase.h"
#include "configuration.h"
#include "WISEPlatform.h"
#include <string.h>

#define XML_CONFIG_ROOT "XMLConfigSettings"
#define XML_CONFIG_BASIC "BaseSettings"

#define RUN_MODE_KEY			"RunMode"
#define LAUNCH_CONNECT_KEY		"LaunchConnect"
#define AUTO_START_KEY			"AutoStart"
#define IOT_AUTO_REPORT			"AutoReport"

#define CREDENTIAL_URL_KEY		"CredentialURL"
#define CREDENTIAL_IOTKEY_KEY	"IoTKey"

#define SERVER_IP_KEY			"ServerIP"
#define SERVER_PORT_KEY			"ServerPort"
#define SERVER_AUTH_KEY			"ConnAuth"
//#define USER_NAME_KEY			"UserName"
//#define USER_PASSWORD_KEY		"UserPassword"

#define SERVER_TLS_TYPE			"TLSType"
#define SERVER_TLS_CAFILE		"CAFile"
#define SERVER_TLS_CAPATH		"CAPath"
#define SERVER_TLS_CERTFILE		"CertFile"
#define SERVER_TLS_KEYFILE		"KeyFile"
#define SERVER_TLS_CERTPASS		"CertPW"

#define SERVER_TLS_PSK			"PSK"
#define SERVER_PSK_IDENTIFY		"PSKIdentify"
#define SERVER_PSK_CIPHERS		"PSKCiphers"

#define COMPRESSION_TUNNEL_SECOND	"CompressionTunnel_Second"
#define COMPRESSION_TUNNEL_METHOD	"CompressionTunnel_Method"

#ifndef SA30
#define LISTEN_PORT 1883
#else
#define LISTEN_PORT 10001
#endif

bool SACONFIG_API cfg_load(char const * configFile, susiaccess_agent_conf_body_t * conf)
{
	bool bRet = false;
	xml_doc_info * doc = NULL;
	bool bModify = false;
	char temp[256] = {0};

	if(configFile == NULL) 
		return bRet;
	if(conf == NULL)
		return bRet;
	
	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
		return bRet;

	memset(conf, 0, sizeof(susiaccess_agent_conf_body_t));

	if(!xml_GetItemValue(doc, RUN_MODE_KEY, conf->runMode, sizeof(conf->runMode)))
	{
		memset(conf->runMode, 0, sizeof(conf->runMode));
	}
	if(strlen(conf->runMode)<=0)
	{
		strncpy(conf->runMode, DEF_STANDALONE_RUN_MODE, strlen(DEF_STANDALONE_RUN_MODE)+1);
		xml_SetItemValue(doc, RUN_MODE_KEY, conf->runMode);
		bModify = true;
	}

	/*if(!xml_GetItemValue(doc, LAUNCH_CONNECT_KEY, conf->lunchConnect, sizeof(conf->lunchConnect)))
	{
		memset(conf->lunchConnect, 0, sizeof(conf->lunchConnect));
	}
	if(strlen(conf->lunchConnect)<=0)
	{
		strncpy(conf->lunchConnect, "False", strlen("False")+1);
		xml_SetItemValue(doc, LAUNCH_CONNECT_KEY, conf->lunchConnect);
		bModify = true;
	}*/

	if(!xml_GetItemValue(doc, AUTO_START_KEY, conf->autoStart, sizeof(conf->autoStart)))
	{
		memset(conf->autoStart, 0, sizeof(conf->autoStart));
	}
	if(strlen(conf->autoStart)<=0)
	{
		strncpy(conf->autoStart, "True", strlen("True")+1);
		xml_SetItemValue(doc, AUTO_START_KEY, conf->autoStart);
		bModify = true;
	}

	/*if(!xml_GetItemValue(doc, IOT_AUTO_REPORT, conf->autoReport, sizeof(conf->autoReport)))
	{
		memset(conf->autoReport, 0, sizeof(conf->autoReport));
	}
	if(strlen(conf->autoReport)<=0)
	{
		strncpy(conf->autoReport, "True", strlen("True")+1);
		xml_SetItemValue(doc, IOT_AUTO_REPORT, conf->autoReport);
		bModify = true;
	}*/

	if(!xml_GetItemValue(doc, CREDENTIAL_URL_KEY, conf->credentialURL, sizeof(conf->credentialURL)))
		memset(conf->credentialURL, 0, sizeof(conf->credentialURL));

	if (!xml_GetItemValue(doc, CREDENTIAL_IOTKEY_KEY, conf->iotKey, sizeof(conf->iotKey)))
		memset(conf->iotKey, 0, sizeof(conf->iotKey));

	memcpy(conf->solution, conf->iotKey, 16);
	snprintf(conf->solution + strlen(conf->solution), 2, "%s", ",");
	if (xml_GetItemValue(doc, COMPRESSION_TUNNEL_SECOND, conf->solution+strlen(conf->solution), sizeof(conf->solution) - strlen(conf->solution)))
	{
		snprintf(conf->solution + strlen(conf->solution), 4, ":s,");
	}

	if (xml_GetItemValue(doc, COMPRESSION_TUNNEL_METHOD, conf->solution + strlen(conf->solution), sizeof(conf->solution) - strlen(conf->solution)))
	{
		snprintf(conf->solution + strlen(conf->solution), 4, ":m,");
	}
	


	
	
	if(!xml_GetItemValue(doc, SERVER_IP_KEY, conf->serverIP, sizeof(conf->serverIP)))
		memset(conf->serverIP, 0, sizeof(conf->serverIP));

	if(!xml_GetItemValue(doc, SERVER_PORT_KEY, conf->serverPort, sizeof(conf->serverPort)))
	{
		memset(conf->serverPort, 0, sizeof(conf->serverPort));
	}
	if(strlen(conf->serverPort)<=0)
	{
		//strncpy(conf->serverPort, LISTEN_PORT, strlen(LISTEN_PORT)+1);
		sprintf(conf->serverPort,"%d", LISTEN_PORT);
		xml_SetItemValue(doc, SERVER_PORT_KEY, conf->serverPort);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, SERVER_AUTH_KEY, conf->serverAuth, sizeof(conf->serverAuth)))
	{
		memset(conf->serverAuth, 0, sizeof(conf->serverAuth));
	}
	if(strlen(conf->serverAuth)<=0)
	{
		strncpy(conf->serverAuth, "fENl4B7tnuwpIbs61I5xJQ==", strlen("fENl4B7tnuwpIbs61I5xJQ==")+1);
		xml_SetItemValue(doc, SERVER_AUTH_KEY, conf->serverAuth);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_TYPE, temp, sizeof(temp)))
	{
		conf->tlstype = 0;
	}
	else
	{
		conf->tlstype = atoi(temp);
	}
	if(strlen(temp)<=0)
	{
		sprintf(temp, "%d", conf->tlstype);
		xml_SetItemValue(doc,SERVER_TLS_TYPE, temp);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_CAFILE, conf->cafile, sizeof(conf->cafile)))
	{
		memset(conf->cafile, 0, sizeof(conf->cafile));
	}
	
	if(!xml_GetItemValue(doc, SERVER_TLS_CAPATH, conf->capath, sizeof(conf->capath)))
	{
		memset(conf->capath, 0, sizeof(conf->capath));
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_CERTFILE, conf->certfile, sizeof(conf->certfile)))
	{
		memset(conf->certfile, 0, sizeof(conf->certfile));
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_KEYFILE, conf->keyfile, sizeof(conf->keyfile)))
	{
		memset(conf->keyfile, 0, sizeof(conf->keyfile));
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_CERTPASS, conf->cerpasswd, sizeof(conf->cerpasswd)))
	{
		memset(conf->cerpasswd, 0, sizeof(conf->cerpasswd));
	}

	if(!xml_GetItemValue(doc, SERVER_TLS_PSK, conf->psk, sizeof(conf->psk)))
	{
		memset(conf->psk, 0, sizeof(conf->psk));
	}

	if(!xml_GetItemValue(doc, SERVER_PSK_IDENTIFY, conf->identity, sizeof(conf->identity)))
	{
		memset(conf->identity, 0, sizeof(conf->identity));
	}

	if(!xml_GetItemValue(doc, SERVER_PSK_CIPHERS, conf->ciphers, sizeof(conf->ciphers)))
	{
		memset(conf->ciphers, 0, sizeof(conf->ciphers));
	}

	/*if(!xml_GetItemValue(doc, USER_NAME_KEY, conf->loginID, sizeof(conf->loginID)))
	{
		memset(conf->loginID, 0, sizeof(conf->loginID));
	}
	if(strlen(conf->loginID)<=0)
	{
		strncpy(conf->loginID, "admin", strlen("admin")+1);
		xml_SetItemValue(doc,USER_NAME_KEY, conf->loginID);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, USER_PASSWORD_KEY, conf->loginPwd, sizeof(conf->loginPwd)))
	{
		memset(conf->loginPwd, 0, sizeof(conf->loginPwd));
	}
	if(strlen(conf->loginPwd)<=0)
	{
		strncpy(conf->loginPwd, "GP25qY7TjJA=", strlen("GP25qY7TjJA=")+1);
		xml_SetItemValue(doc,USER_PASSWORD_KEY, conf->loginPwd);
		bModify = true;
	}*/

	if(bModify)
	{
		xml_SaveFile(configFile, doc);
	}
	bRet = true;
	xml_FreeDoc(doc);
	return bRet;
}

bool SACONFIG_API cfg_save(char const * configFile, susiaccess_agent_conf_body_t const * const conf)
{
	bool bRet = false;
	xml_doc_info * doc = NULL;
	char temp[256] = {0};
	if(configFile == NULL) 
		return bRet;
	if(conf == NULL)
		return bRet;
	
	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
	{
		doc = xml_CreateDoc( XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	}

	if(!xml_SetItemValue(doc, RUN_MODE_KEY, conf->runMode))
		goto SAVE_EXIT;

	//if(!xml_SetItemValue(doc, LAUNCH_CONNECT_KEY, conf->lunchConnect))
	//	goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, AUTO_START_KEY, conf->autoStart))
		goto SAVE_EXIT;

	
	/*if(!xml_SetItemValue(doc, IOT_AUTO_REPORT, conf->autoReport))
		goto SAVE_EXIT;*/

	if(!xml_SetItemValue(doc, CREDENTIAL_URL_KEY, conf->credentialURL))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, CREDENTIAL_IOTKEY_KEY, conf->iotKey))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, SERVER_IP_KEY, conf->serverIP))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, SERVER_PORT_KEY, conf->serverPort))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc,SERVER_AUTH_KEY, conf->serverAuth))
		goto SAVE_EXIT;

	sprintf(temp, "%d", conf->tlstype);
	if(!xml_SetItemValue(doc,SERVER_TLS_TYPE, temp))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, SERVER_TLS_CAFILE, conf->cafile))
		goto SAVE_EXIT;
	
	if(!xml_SetItemValue(doc, SERVER_TLS_CAPATH, conf->capath))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, SERVER_TLS_CERTFILE, conf->certfile))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, SERVER_TLS_KEYFILE, conf->keyfile))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, SERVER_TLS_CERTPASS, conf->cerpasswd))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, SERVER_TLS_PSK, conf->psk))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, SERVER_PSK_IDENTIFY, conf->identity))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc, SERVER_PSK_CIPHERS, conf->ciphers))
		goto SAVE_EXIT;

	/*if(!xml_SetItemValue(doc,USER_NAME_KEY, conf->loginID))
		goto SAVE_EXIT;

	if(!xml_SetItemValue(doc,USER_PASSWORD_KEY, conf->loginPwd))
		goto SAVE_EXIT;*/

	xml_SaveFile(configFile, doc);
	bRet = true;
SAVE_EXIT:
	xml_FreeDoc(doc);
	return bRet;
}

bool SACONFIG_API cfg_create(char const * configFile, susiaccess_agent_conf_body_t * conf)
{
	bool bRet = false;
	xml_doc_info * doc = NULL;
	char temp[256] = {0};

	if(configFile == NULL) 
		return bRet;
	if(conf == NULL)
		return bRet;

	memset(conf, 0, sizeof(susiaccess_agent_conf_body_t));

	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
	{
		doc = xml_CreateDoc( XML_CONFIG_ROOT, XML_CONFIG_BASIC);

		strncpy(conf->runMode, DEF_STANDALONE_RUN_MODE, strlen(DEF_STANDALONE_RUN_MODE)+1);
		if(!xml_SetItemValue(doc, RUN_MODE_KEY, conf->runMode))
			goto CREATE_EXIT;

		/*strncpy(conf->lunchConnect, "False", strlen("False")+1);
		if(!xml_SetItemValue(doc, LAUNCH_CONNECT_KEY, conf->lunchConnect))
			goto CREATE_EXIT;*/

		strncpy(conf->autoStart, "True", strlen("True")+1);
		if(!xml_SetItemValue(doc, AUTO_START_KEY, conf->autoStart))
			goto CREATE_EXIT;

		/*strncpy(conf->autoReport, "True", strlen("True")+1);
		if(!xml_SetItemValue(doc, IOT_AUTO_REPORT, conf->autoReport))
			goto CREATE_EXIT;*/

		if(!xml_SetItemValue(doc, CREDENTIAL_URL_KEY, conf->credentialURL))
			goto CREATE_EXIT;

		if(!xml_SetItemValue(doc, CREDENTIAL_IOTKEY_KEY, conf->iotKey))
			goto CREATE_EXIT;

		strncpy(conf->serverIP, "127.0.0.1", strlen("127.0.0.1")+1);
		if(!xml_SetItemValue(doc, SERVER_IP_KEY, conf->serverIP))
			goto CREATE_EXIT;

		sprintf(conf->serverPort,"%d", LISTEN_PORT);
		//strncpy(conf->serverPort, LISTEN_PORT,  strlen(LISTEN_PORT));
		if(!xml_SetItemValue(doc, SERVER_PORT_KEY, conf->serverPort))
			goto CREATE_EXIT;

		strncpy(conf->serverAuth, "fENl4B7tnuwpIbs61I5xJQ==", strlen("fENl4B7tnuwpIbs61I5xJQ==")+1);
		if(!xml_SetItemValue(doc,SERVER_AUTH_KEY, conf->serverAuth))
			goto CREATE_EXIT;

		conf->tlstype = 0;
		sprintf(temp, "%d", conf->tlstype);
		if(!xml_SetItemValue(doc,SERVER_TLS_TYPE, temp))
			goto CREATE_EXIT;

		/*strncpy(conf->loginID, "ral", strlen("ral")+1);
		if(!xml_SetItemValue(doc,USER_NAME_KEY, conf->loginID))
			goto CREATE_EXIT;

		strncpy(conf->loginPwd, "123", strlen("123")+1);
		if(!xml_SetItemValue(doc,USER_PASSWORD_KEY, conf->loginPwd))
			goto CREATE_EXIT;*/

		xml_SaveFile(configFile, doc);
		bRet = true;
CREATE_EXIT:
		xml_FreeDoc(doc);
		return bRet;
	}
	else
	{
		xml_FreeDoc(doc);
		return cfg_load(configFile, conf);
	}
}

bool SACONFIG_API cfg_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen)
{
	bool bRet = false;
	if(NULL == configFile || NULL == itemName || NULL == itemValue) return bRet;
	{
		xml_doc_info * doc = NULL;
		doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
		if(doc)
		{
			bRet = xml_GetItemValue(doc, itemName, itemValue, valueLen);
			xml_FreeDoc(doc);
		}
	}
	return bRet;
}

bool SACONFIG_API cfg_set(char const * const configFile, char const * const itemName, char const * const itemValue)
{
	bool bRet = false;
   if(NULL == configFile || NULL == itemName || NULL == itemValue) return bRet;
	{
		xml_doc_info * doc = NULL;
		doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
		if(doc)
		{
			if(!xml_SetItemValue(doc, itemName, itemValue))
				return bRet;

			xml_SaveFile(configFile, doc);
			xml_FreeDoc(doc);
			bRet = true;
		}
	}
	return bRet;
}

/*
	return xml doc of configFile
*/
void* cfg_doc_open(char const * const configFile)
{
	xml_doc_info * doc = NULL;
	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);

	return doc;
}

/*
	close xml doc
*/
void cfg_doc_close(void* doc)
{
	xml_FreeDoc((xml_doc_info*) doc);
}

/*
	get xml item by xml doc instance
*/
bool SACONFIG_API cfg_doc_get(void* xmlDoc, char const * const itemName, char * itemValue, int valueLen)
{
	bool bRet = false;
	if(NULL == xmlDoc || NULL == itemName || NULL == itemValue) return bRet;
	{
		xml_doc_info* doc = (xml_doc_info*) xmlDoc;
		bRet = xml_GetItemValue(doc, itemName, itemValue, valueLen);
	}
	return bRet;
}