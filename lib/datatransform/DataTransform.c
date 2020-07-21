/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/11/20 by Scott Chang								    */
/* Modified Date: 2017/11/20 by Scott Chang									*/
/* Abstract     : WISE-PaaS data transform									*/
/* Reference    : None														*/
/****************************************************************************/
#include "DataTransform.h"
#include <stdio.h>
#include <string.h>
// ------------------------- WISE-PaaS 1.0 -----------------------------
#define WP1_IDENTIFY_FLAG					"susiCommData"
#define WP1_AGENTID_FLAG					"agentID"
#define WP1_CMDID_FLAG						"commCmd"
#define WP1_REQID_FLAG						"requestID"
#define WP1_HANDLERNAME_FLAG				"handlerName"
#define WP1_TS_FLAG							"sendTS"
#define WP1_CATALOGID_FLAG					"catalogID"

// ------------------------- WISE-PaaS 2.0 -----------------------------
#define WP2_CONTENT_FLAG					"content"

bool GetNewID(char* oldID, char* newID)
{
	bool bRet = false;
	int length = 0;
	char *p = 0;
	char ID1[9] = {0};
	char ID2[5] = {0};
	char ID3[5] = {0};
	char ID4[5] = {0};
	char ID5[13] = {0};
	char IDFull[33] = {0};
	if(oldID == NULL) return bRet;
	length = strlen(oldID);
	if(length <= 0) return bRet;

	if(strcmp(oldID, "NULL")==0 || strcmp(oldID, "null")==0)
		return bRet;

	else if(length == 1)
	{
		if(strcmp(oldID, "+")==0)
		{
			strcpy(newID, oldID);
			return true;
		}
		else
			return bRet;
	}
	
	if(length == 36)
	{
		strcpy(newID, oldID);
		return true;
	}
	else if(length<32)
	{
		int i=0;
		int append = 32-length;
		for(i=0; i<append; i++)
			strcat(IDFull, "0");
		strcat(IDFull, oldID);
	}
	else
	{
		int offset = length - 32;
		p = oldID + offset;
		strncpy(IDFull, p, 32);
	}
	strncpy(ID1, IDFull, 8);
	if(strcmp(ID1, "00000000") == 0)
		strcpy(ID1, "00000001");

	p = IDFull;
	p += 8;

	strncpy(ID2, p, 4);
	p += 4;

	strncpy(ID3, p, 4);
	p += 4;

	strncpy(ID4, p, 4);
	p += 4;

	strncpy(ID5, p, 12);

	sprintf(newID, "%s-%s-%s-%s-%s", ID1 ,ID2, ID3, ID4, ID5);
	bRet = true;
	return bRet;
}

bool GetOldID(char* newID, char* oldID)
{
	bool bRet = false;
	int length = 0;
	if(newID == NULL) return bRet;
	length = strlen(newID);
	if(length <= 0) return bRet;
	if(strcmp(newID, "NULL")==0 || strcmp(newID, "null")==0)
		return bRet;
	else if(length == 1)
	{
		if(strcmp(newID, "+")==0)
		{
			strcpy(oldID, newID);
			return true;
		}
	}
	
	if(length == 16)
	{
		strcpy(oldID, newID);
	}
	else if(length == 36)
	{
		char ID1[9] = {0};
		char ID2[5] = {0};
		char ID3[5] = {0};
		char ID4[5] = {0};
		char ID5[13] = {0};
		if(sscanf(newID, "%8s-%4s-%4s-%4s-%12s", ID1, ID2, ID3, ID4, ID5)!=-1)
		{
			sprintf(oldID, "%s%s", ID4,ID5);
		}
	}
	else if(length < 16)
	{
		int i=0;
		int append = 16-length;
		memset(oldID, 0, 16);
		for(i=0; i<append; i++)
			strcat(oldID, "0");
		strcat(oldID, newID);
	}
	else
	{
		char *p = newID;
		int offset = length - 16;
		p += offset;
		strcpy(oldID, p);
	}
	bRet = true;
	return bRet;
}

void AddChildren(cJSON* target, cJSON* parent)
{
	cJSON* child = NULL;
	if(parent == NULL) return;

	child = parent->child;
	while(child)
	{
		cJSON_AddItemToObject(target, child->string, cJSON_Duplicate(child, 1));
		child = child->next;
	}
}

void AddChildrenEx(cJSON* target, char* filtername, cJSON* parent)
{
	cJSON* child = NULL;
	if(parent == NULL) return;

	child = parent->child;
	while(child)
	{
		if(strcmp(child->string, filtername) == 0)
		{
			cJSON* grandchild = child->child;
			while(grandchild)
			{
				cJSON_AddItemToObject(target, grandchild->string, cJSON_Duplicate(grandchild, 1));
				grandchild = grandchild->next;
			}
		}
		else
		{
			cJSON_AddItemToObject(target, child->string, cJSON_Duplicate(child, 1));
		}
		child = child->next;
	}
}

void AddasChild(cJSON* target, char* name, cJSON* parent)
{
	if(parent == NULL) return;
	cJSON_AddItemToObject(target, name, cJSON_Duplicate(parent, 1));
}

char* Trans2NewFrom( const char *data )
{
	char *buffer = NULL;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* newRoot = NULL;
	cJSON* newContent = NULL;
	int cmdID = 0;
	char handlerName[32] = {0};

	if(!data) return buffer;

	root = cJSON_Parse((char *)data);

	if(!root) return buffer;

	body = cJSON_GetObjectItem( root, WP1_IDENTIFY_FLAG );

	if( !body ) //Check is WISE-PaaS 1.0 or not
	{
		buffer = cJSON_PrintUnformatted(root);
		goto ExitTrans2NewFrom; 
	}

	newRoot = cJSON_CreateObject();
	newContent = cJSON_CreateObject();

	cJSON_AddItemToObject(newRoot, WP2_CONTENT_FLAG, newContent);

	target = cJSON_GetObjectItem(body, WP1_CMDID_FLAG);
	if(target)
	{
		cmdID = target->valueint;
	}

	target = cJSON_GetObjectItem(body, WP1_HANDLERNAME_FLAG);
	if(target)
	{
		strncpy(handlerName, target->valuestring, sizeof(handlerName));
	}

	target = body->child;

	while(target)
	{
		if(strcmp(target->string, WP1_AGENTID_FLAG) == 0)
		{
			char* devID = target->valuestring;
			char newDevID[38] = {0};
			GetNewID(devID, newDevID);
			cJSON_AddStringToObject(newRoot, WP1_AGENTID_FLAG, newDevID);
		}
		else if(strcmp(target->string, WP1_HANDLERNAME_FLAG) == 0)
		{
			cJSON_AddStringToObject(newRoot, WP1_HANDLERNAME_FLAG, target->valuestring);
		}
		else if(strcmp(target->string, WP1_CMDID_FLAG) == 0)
		{
			cJSON_AddNumberToObject(newRoot, WP1_CMDID_FLAG, target->valueint);
		}
		else if(strcmp(target->string, WP1_TS_FLAG) == 0)
		{
			cJSON_AddNumberToObject(newRoot, WP1_TS_FLAG, target->valuedouble);
		}
		else if(strcmp(target->string, WP1_REQID_FLAG) == 0)
		{
			cJSON_AddNumberToObject(newRoot, WP1_REQID_FLAG, target->valuedouble);
		}
		else if(strcmp(target->string, WP1_CATALOGID_FLAG) == 0)
		{
			cJSON_AddNumberToObject(newRoot, WP1_CATALOGID_FLAG, target->valuedouble);
		}
		else
		{
			if(strcmp(handlerName, "general") == 0)
			{
				switch(cmdID)
				{
				case 1:
					if(strcmp(target->string, "devID") == 0)
					{
						char* devID = target->valuestring;
						char newDevID[38] = {0};
						GetNewID(devID, newDevID);
						cJSON_AddStringToObject(newContent, "devID", newDevID);
					}
					else if(strcmp(target->string, "parentID") == 0)
					{
						char* devID = target->valuestring;
						char newDevID[38] = {0};
						GetNewID(devID, newDevID);
						cJSON_AddStringToObject(newContent, "parentID", newDevID);
					}
					else
						cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				case 2055:
					if(strcmp(target->string, "data") == 0)
						AddChildren(newContent, target);
					else
						cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				default:
					cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				}
			}
			else if(strcmp(handlerName, "SUSIControl") == 0)
			{
				switch(cmdID)
				{
				case 527:
					if(strcmp(target->string, "susictrlThr") == 0)
					{
						cJSON* thresholds = cJSON_CreateObject();
						AddChildren(thresholds, target);
						cJSON_AddItemToObject(newContent, "Thresholds", thresholds);
					}
					else
						cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				default:
					cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				}
			}
			else if(strcmp(handlerName, "ProcessMonitor") == 0)
			{
				switch(cmdID)
				{
				case 157:
					if(strcmp(target->string, "swmThr") == 0)
					{
						cJSON* thresholds = cJSON_CreateObject();
						AddChildren(thresholds, target);
						cJSON_AddItemToObject(newContent, "Thresholds", thresholds);
					}
					else
						cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				default:
					cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				}
			}
			else if(strcmp(handlerName, "NetMonitor") == 0)
			{
				switch(cmdID)
				{
				case 157:
					if(strcmp(target->string, "nmThr") == 0)
					{
						cJSON* thresholds = cJSON_CreateObject();
						AddChildren(thresholds, target);
						cJSON_AddItemToObject(newContent, "Thresholds", thresholds);
					}
					else
						cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				default:
					cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				}
			}
			else if(strcmp(handlerName, "HDDMonitor") == 0)
			{
				switch(cmdID)
				{
				case 257:
					if(strcmp(target->string, "hddThr") == 0)
					{
						cJSON* thresholds = cJSON_CreateObject();
						AddChildren(thresholds, target);
						cJSON_AddItemToObject(newContent, "Thresholds", thresholds);
					}
					else
						cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				default:
					cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
					break;
				}
			}
			else
			{
				cJSON_AddItemToObject(newContent, target->string, cJSON_Duplicate(target, 1));
			}
		}
		target = target->next;
	}

	buffer = cJSON_PrintUnformatted(newRoot);
	
ExitTrans2NewFrom:
	if(newRoot)
		cJSON_Delete(newRoot);
	cJSON_Delete(root);

	return buffer;
}

char* Trans2OldFrom( const char *data)
{
	char *buffer = NULL;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* newRoot = NULL;
	cJSON* newbody = NULL;
	int cmdID = 0;
	char handlerName[32] = {0};

	if(!data) return buffer;

	root = cJSON_Parse((char *)data);

	if(!root) return buffer;

	body = cJSON_GetObjectItem( root, WP1_IDENTIFY_FLAG );
	if( body ) //Check is WISE-PaaS 1.0 or not
	{
		buffer = cJSON_PrintUnformatted(root);
		goto ExitTrans2OldFrom; 
	}

	newRoot = cJSON_CreateObject();
	newbody = cJSON_CreateObject();
	cJSON_AddItemToObject(newRoot, WP1_IDENTIFY_FLAG, newbody);

	target = cJSON_GetObjectItem(root, WP1_CMDID_FLAG);
	if(target)
	{
		cmdID = target->valueint;
		//cJSON_AddNumberToObject(newbody, WP1_CMDID_FLAG, cmdID);
	}

	target = cJSON_GetObjectItem(root, WP1_HANDLERNAME_FLAG);
	if(target)
	{
		strncpy(handlerName, target->valuestring, sizeof(handlerName));
		//cJSON_AddStringToObject(newbody, WP1_HANDLERNAME_FLAG, handlerName);
	}

	target = root->child;
	while(target)
	{
		if(strcmp(target->string, WP1_AGENTID_FLAG) == 0)
		{
			char* devID = target->valuestring;
			char oldDevID[17] = {0};
			GetOldID(devID, oldDevID);
			cJSON_AddStringToObject(newbody, WP1_AGENTID_FLAG, oldDevID);
		}
		else if(strcmp(target->string, WP2_CONTENT_FLAG) == 0)
		{
			if(strcmp(handlerName, "general") == 0)
			{
				switch(cmdID)
				{
				case 1:
					{
						cJSON* child = NULL;
						child = target->child;
						while(child)
						{
							if(strcmp(child->string, "devID") == 0)
							{
								char* devID = child->valuestring;
								char oldDevID[17] = {0};
								GetOldID(devID, oldDevID);
								cJSON_AddStringToObject(newbody, "devID", oldDevID);
							}
							else if(strcmp(child->string, "parentID") == 0)
							{
								char* devID = child->valuestring;
								char oldDevID[17] = {0};
								GetOldID(devID, oldDevID);
								cJSON_AddStringToObject(newbody, "parentID", oldDevID);
							}
							else
								cJSON_AddItemToObject(newbody, child->string, cJSON_Duplicate(child, 1));
							child = child->next;
						}
					}
					break;
				case 116: // OS info
					AddasChild(newbody, "osinfo", target);
					break;
				case 124:
					AddasChild(newbody, "handlerlist", target);
					break;
				case 2055:
					AddasChild(newbody, "data", target);
					break;
				default:
					AddChildren(newbody, target);
					break;
				}
			}
			else if(strcmp(handlerName, "SUSIControl") == 0)
			{
				switch(cmdID)
				{
				case 527:
					AddasChild(newbody, "susictrlThr", target);
					break;
				default:
					AddChildren(newbody, target);
					break;
				}
			}
			else if(strcmp(handlerName, "ProcessMonitor") == 0)
			{
				switch(cmdID)
				{
				case 157:
					AddasChild(newbody, "swmThr", target);
					break;
				default:
					AddChildren(newbody, target);
					break;
				}
			}
			else if(strcmp(handlerName, "NetMonitor") == 0)
			{
				switch(cmdID)
				{
				case 157:
					AddasChild(newbody, "nmThr", target);
					break;
				default:
					AddChildren(newbody, target);
					break;
				}
			}
			else if(strcmp(handlerName, "HDDMonitor") == 0)
			{
				switch(cmdID)
				{
				case 257:
					AddasChild(newbody, "hddThr", target);
					break;
				default:
					AddChildren(newbody, target);
					break;
				}
			}
			else
			{
				AddChildren(newbody, target);
			}
		}
		else
		{
			cJSON_AddItemToObject(newbody, target->string, cJSON_Duplicate(target, 1));
		}
		target = target->next;
	}

	buffer = cJSON_PrintUnformatted(newRoot);

ExitTrans2OldFrom:
	if(newRoot)
		cJSON_Delete(newRoot);
	cJSON_Delete(root);

	return buffer;

}

bool _parse_old_topic(const char* topic, char* devid, char* catalog)
{
	bool bRet = false;
	char *start = NULL, *end = NULL;
	if(topic == NULL) return bRet;
	if(devid == NULL) return bRet;
	start = strstr(topic, "/cagent/admin/");
	if(!start)
		start = strstr(topic, "/server/admin/");
	if(start)
	{
		start += strlen("/cagent/admin/");
		end = strstr(start, "/");
		if(end)
		{
			char id[17] = {0};
			strncpy(id, start, end - start < sizeof(id) ? end - start : sizeof(id));
			strcpy(devid, id);
			if(catalog)
				strcpy(catalog, end+1);
			bRet = true;
		}
	}
	return bRet;
}

bool _parse_new_topic(const char* topic, char* devid, char* catalog)
{
	bool bRet = false;
	char *start = NULL, *end = NULL;
	int pos = 3;
	if(topic == NULL) return bRet;
	if(devid == NULL) return bRet;
	start = strstr(topic, "/wisepaas/"); //verify support topic start with "/wisepaas/"
	if(start)
	{
		while(pos >0)
		{
			start = strstr(start, "/")+1;
			if(start == 0)
				return bRet;
			pos--;
		}
		end = strstr(start, "/");
		if(end)
		{
			char id[37] = {0};
			strncpy(id, start, end - start < sizeof(id) ? end - start : sizeof(id));
			strcpy(devid, id);
			if(catalog)
				strcpy(catalog, end+1);
			bRet = true;
		}
	}
	return bRet;
}

bool Trans2NewTopic( const char *oldTopic, const char *productID, char* newTopic )
{
	bool bRet = false;
	char oldID[17] = {0};
	char newID[37] = {0};
	char oldCat[128] = {0};
	if(oldTopic == NULL) return bRet;
	if(newTopic == NULL) return bRet;
	if(!_parse_old_topic(oldTopic, oldID, oldCat))
	{
		strcpy(newTopic, oldTopic);
		return true;
	}

	if(strcmp(oldID, "+") == 0)
	{
		strcpy(newID, oldID);
		bRet = true;
	}
	else
		bRet = GetNewID(oldID, newID);

	if(bRet)
	{
		if(strcmp(oldCat, "agentinfoack")==0)
			sprintf(newTopic, "/wisepaas/device/%s/agentinfoack", newID);
		else if(strcmp(oldCat, "willmessage")==0)
			sprintf(newTopic, "/wisepaas/device/%s/willmessage", newID);
		else if(strcmp(oldCat, "agentactionreq")==0)
			sprintf(newTopic, "/wisepaas/%s/%s/agentactionack", productID, newID);
		else if(strcmp(oldCat, "agentactionack")==0)
			sprintf(newTopic, "/wisepaas/%s/%s/agentactionreq", productID, newID);
		else if(strcmp(oldCat, "agentcallbackreq")==0)
			sprintf(newTopic, "/wisepaas/%s/%s/agentactionreq", productID, newID);
		else if(strcmp(oldCat, "agentcallbackack")==0)
			sprintf(newTopic, "/wisepaas/%s/%s/agentactionack", productID, newID);
		else if(strcmp(oldCat, "deviceinfo")==0)
			sprintf(newTopic, "/wisepaas/device/%s/devinfoack", newID);
		else if(strcmp(oldCat, "notify")==0)
			sprintf(newTopic, "/wisepaas/device/%s/notifyack", newID);
		else if(strcmp(oldCat, "eventnotify")==0)
			sprintf(newTopic, "/wisepaas/%s/%s/eventnotifyack", productID, newID);
		else if(strcmp(oldCat, "agentctrl")==0)
			sprintf(newTopic, "/wisepaas/device/%s/agentctrlreq", newID);
		else
			strcpy(newTopic, oldTopic);
	}
	return bRet;
}

bool Trans2OldTopic( const char *newTopic, char* oldTopic )
{
	bool bRet = false;
	char oldID[17] = {0};
	char newID[37] = {0};
	char newCat[128] = {0};
	if(oldTopic == NULL) return bRet;
	if(newTopic == NULL) return bRet;
	if(!_parse_new_topic(newTopic, newID, newCat))
	{
		strcpy(oldTopic, newTopic);
		return true;
	}

	if(strcmp(newID, "+") == 0)
	{
		strcpy(oldID, newID);
		bRet = true;
	}
	else
		bRet = GetOldID(newID, oldID);

	if(bRet)
	{
		if(strcmp(newCat, "agentinfoack")==0)
			sprintf(oldTopic, "/cagent/admin/%s/agentinfoack", oldID);
		else if(strcmp(newCat, "willmessage")==0)
			sprintf(oldTopic, "/cagent/admin/%s/willmessage", oldID);
		else if(strcmp(newCat, "agentactionreq")==0)
			sprintf(oldTopic, "/cagent/admin/%s/agentcallbackreq", oldID);
		else if(strcmp(newCat, "agentactionack")==0)
			sprintf(oldTopic, "/cagent/admin/%s/agentactionreq", oldID);
		else if(strcmp(newCat, "devinfoack")==0)
			sprintf(oldTopic, "/cagent/admin/%s/deviceinfo", oldID);
		else if(strcmp(newCat, "notifyack")==0)
			sprintf(oldTopic, "/cagent/admin/%s/notify", oldID);
		else if(strcmp(newCat, "eventnotifyack")==0)
			sprintf(oldTopic, "/cagent/admin/%s/eventnotify", oldID);
		else if(strcmp(newCat, "agentctrlreq")==0)
			sprintf(oldTopic, "/server/admin/%s/agentctrl", oldID);
		else
			strcpy(oldTopic, newTopic);
	}
	return bRet;
}

bool GetNewIDFromTopic( const char *topic, char* newID )
{
	bool bRet = false;
	char oldID[17] = {0};
	char strCat[128] = {0};
	if(strstr(topic, "/cagent/admin") == topic)
	{
		if(!_parse_old_topic(topic, oldID, strCat))
			return bRet;
		GetNewID(oldID, newID);
		bRet = true;
	}
	else if(strstr(topic, "/server/admin") == topic)
	{
		if(!_parse_old_topic(topic, oldID, strCat))
			return bRet;
		GetNewID(oldID, newID);
		bRet = true;
	}
	else
	{
		if(!_parse_new_topic(topic, newID, strCat))
			return bRet;
		bRet = true;
	}
	
	return bRet;
}

bool GetOldIDFromTopic( const char *topic, char* oldID )
{
	bool bRet = false;
	char newID[37] = {0};
	char strCat[128] = {0};
	if(strstr(topic, "/cagent/admin") == topic)
	{
		if(!_parse_old_topic(topic, oldID, strCat))
			return bRet;
		bRet = true;
	}
	else if(strstr(topic, "/server/admin") == topic)
	{
		if(!_parse_old_topic(topic, oldID, strCat))
			return bRet;
		bRet = true;
	}
	else
	{
		if(!_parse_new_topic(topic, newID, strCat))
			return bRet;
		GetOldID(newID, oldID);
		bRet = true;
	}
	
	return bRet;
}