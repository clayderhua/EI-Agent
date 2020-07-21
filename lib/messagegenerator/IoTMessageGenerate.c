#include "IoTMessageGenerate.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cJSON.h"

long long IoT_GetTimeTick()
{
	return MSG_GetTimeTick();
}

#pragma region Add_Resource
MSG_CLASSIFY_T* IoT_CreateRoot(char* handlerName)
{
	MSG_CLASSIFY_T *pRoot = MSG_CreateRoot();
	MSG_AddJSONClassify(pRoot, handlerName, NULL, false);
	return pRoot;
}

MSG_CLASSIFY_T* IoT_AddGroup(MSG_CLASSIFY_T* pNode, char* groupName)
{
	MSG_CLASSIFY_T *pCurNode = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;

		pCurNode = MSG_FindClassify(pNode, groupName);
		if(!pCurNode)
			pCurNode = MSG_AddIoTClassify(pNode, groupName, NULL, false);
	}
	return pCurNode;
}

MSG_CLASSIFY_T* IoT_AddGroupArray(MSG_CLASSIFY_T* pNode, char* groupName)
{
	MSG_CLASSIFY_T *pCurNode = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;

		pCurNode = MSG_FindClassify(pNode, groupName);
		if(!pCurNode)
			pCurNode = MSG_AddIoTClassify(pNode, groupName, NULL, true);
	}
	return pCurNode;
}

MSG_ATTRIBUTE_T* IoT_AddGroupAttribute(MSG_CLASSIFY_T* pNode, char* attrName)
{
	MSG_ATTRIBUTE_T *attr = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;

		attr = MSG_FindJSONAttribute(pNode, attrName);
		if(!attr)
			attr = MSG_AddJSONAttribute(pNode, attrName);
	}
	return attr;
}

MSG_ATTRIBUTE_T* IoT_AddSensorNode(MSG_CLASSIFY_T* pNode, char* senName)
{
	MSG_ATTRIBUTE_T *attr = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;

		attr = MSG_FindIoTSensor(pNode, senName);
		if(!attr)
			attr = MSG_AddIoTSensor(pNode, senName);
	}
	return attr;
}
	
MSG_ATTRIBUTE_T* IoT_AddOPTSNode(MSG_CLASSIFY_T* pNode)
{
	MSG_CLASSIFY_T *pCurNode = NULL;
	MSG_ATTRIBUTE_T *attr = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;
		pCurNode = MSG_FindClassify(pNode, "opTS");
		if(!pCurNode) {
			pCurNode = MSG_AddJSONClassify(pNode, "opTS", NULL, false);
		}

		attr = MSG_FindJSONAttribute(pCurNode, "$date");
		if(!attr) {
			attr = MSG_AddJSONAttribute(pCurNode, "$date");
		}
	}
	return attr;
}

#define IoT_READ_STRING "r"
#define IoT_WRITE_STRING "w"
#define IoT_READWRITE_STRING "rw"

char* IoT_GetReadWriteString(IoT_READWRITE_MODE readwritemode)
{
	switch (readwritemode)
	{
	default:
	case IoT_READONLY:
		return IoT_READ_STRING;
		break;
	case IoT_WRITEONLY:
		return IoT_WRITE_STRING;
		break;
	case IoT_READWRITE:
		return IoT_READWRITE_STRING;
		break;
	case IoT_NODEFINE:
		return NULL;
		break;
	}
	return NULL;
}

IoT_READWRITE_MODE IoT_GetReadWriteMode(char* readwritemode)
{
	IoT_READWRITE_MODE mode = IoT_NODEFINE;
	if(readwritemode == NULL)
		return mode;

	if(strcmp(readwritemode, "r") == 0)
	{
		mode = IoT_READONLY;
	}
	else if(strcmp(readwritemode, "w") == 0)
	{
		mode = IoT_WRITEONLY;
	}
	else if(strcmp(readwritemode, "rw") == 0)
	{
		mode = IoT_READWRITE;
	}
	return mode;
}

bool IoT_SetDoubleValue(MSG_ATTRIBUTE_T* attr, double value, IoT_READWRITE_MODE readwritemode, char *unit)
{
	bool bRet = MSG_SetDoubleValue(attr, value, IoT_GetReadWriteString(readwritemode), unit);
	return bRet;
}

bool IoT_SetDoubleValueWithMaxMin(MSG_ATTRIBUTE_T* attr, double value, IoT_READWRITE_MODE readwritemode, double max, double min, char *unit)
{
	bool bRet = MSG_SetDoubleValueWithMaxMin(attr, value, IoT_GetReadWriteString(readwritemode), max, min, unit);
	return bRet;
}

void IoT_SetMaxArrayValueSize(int v)
{
	MSG_SetMaxArrayValueSize(v);
}

bool IoT_AppendDoubleValueFull(MSG_ATTRIBUTE_T* attr,
							   double value,
							   int time,
							   IoT_READWRITE_MODE readwritemode,
							   char *unit,
							   bool hasMaxMin,
							   double max,
							   double min)
{
	bool bRet = MSG_AppendDoubleValueFull(attr, value, time, IoT_GetReadWriteString(readwritemode), unit, hasMaxMin, max, min);
	return bRet;
}

bool IoT_AppendDoubleValue(MSG_ATTRIBUTE_T* attr, double value, int time)
{
	bool bRet = MSG_AppendDoubleValue(attr, value, time);
	return bRet;
}

bool IoT_ResetDoubleValues(MSG_ATTRIBUTE_T* attr) {
	return MSG_ResetDoubleValues(attr);
}

bool IoT_AppendBoolValueFull(MSG_ATTRIBUTE_T* attr,
							 bool value,
							 int time,
							 IoT_READWRITE_MODE readwritemode)
{
	return MSG_AppendBoolValueFull(attr, value, time, IoT_GetReadWriteString(readwritemode));
}

bool IoT_AppendBoolValue(MSG_ATTRIBUTE_T* attr, bool value, int time)
{
	return MSG_AppendBoolValue(attr, value, time);
}

bool IoT_ResetBoolValues(MSG_ATTRIBUTE_T* attr) {
	return MSG_ResetBoolValues(attr);
}

bool IoT_AppendStringValueFull(MSG_ATTRIBUTE_T* attr,
							 char* value,
							 int time,
							 IoT_READWRITE_MODE readwritemode)
{
	return MSG_AppendStringValueFull(attr, value, time, IoT_GetReadWriteString(readwritemode));
}

bool IoT_AppendStringValue(MSG_ATTRIBUTE_T* attr, char* value, int time)
{
	return MSG_AppendStringValue(attr, value, time);
}

bool IoT_ResetStringValues(MSG_ATTRIBUTE_T* attr) {
	return MSG_ResetStringValues(attr);
}

bool IoT_SetFloatValue(MSG_ATTRIBUTE_T* attr, float value, IoT_READWRITE_MODE readwritemode, char *unit)
{
	bool bRet = MSG_SetFloatValue(attr, value, IoT_GetReadWriteString(readwritemode), unit);
	return bRet;
}

bool IoT_SetFloatValueWithMaxMin(MSG_ATTRIBUTE_T* attr, float value, IoT_READWRITE_MODE readwritemode, float max, float min, char *unit)
{
	bool bRet = MSG_SetFloatValueWithMaxMin(attr, value, IoT_GetReadWriteString(readwritemode), max, min, unit);
	return bRet;
}

bool IoT_SetBoolValue(MSG_ATTRIBUTE_T* attr, bool bvalue, IoT_READWRITE_MODE readwritemode)
{
	bool bRet = MSG_SetBoolValue(attr, bvalue, IoT_GetReadWriteString(readwritemode));
	return bRet;
}

bool IoT_SetStringValue(MSG_ATTRIBUTE_T* attr, char *svalue, IoT_READWRITE_MODE readwritemode)
{
	bool bRet = MSG_SetStringValue(attr, svalue, IoT_GetReadWriteString(readwritemode));
	return bRet;
}

bool IoT_SetTimestampValue(MSG_ATTRIBUTE_T* attr,  unsigned int value, IoT_READWRITE_MODE readwritemode)
{
	bool bRet = MSG_SetTimestampValue(attr, value, IoT_GetReadWriteString(readwritemode));
	return bRet;
}

bool IoT_SetDateValue(MSG_ATTRIBUTE_T* attr, char *svalue, IoT_READWRITE_MODE readwritemode)
{
	bool bRet = MSG_SetDateValue(attr, svalue, IoT_GetReadWriteString(readwritemode));
	return bRet;
}

bool IoT_SetNULLValue(MSG_ATTRIBUTE_T* attr, IoT_READWRITE_MODE readwritemode)
{
	bool bRet = MSG_SetNULLValue(attr, IoT_GetReadWriteString(readwritemode));
	return bRet;
}

void IoT_AppendGroup(MSG_CLASSIFY_T* pNode, MSG_CLASSIFY_T* pGroup)
{
	MSG_CLASSIFY_T *pCurNode = NULL;
	if(pGroup)
	{
		if(pGroup->type == class_type_root)
		{
			pCurNode = pGroup->sub_list;
		}
		else
			pCurNode = pGroup;

		if(pNode)
		{
			if(pNode->sub_list == NULL)
				pNode->sub_list = MSG_Clone(pCurNode, true);
			else
			{
				MSG_CLASSIFY_T *pTarget = pNode->sub_list;
				while(pTarget)
				{
					if(pTarget->next == NULL)
					{
						pTarget->next = MSG_Clone(pCurNode, true);
						break;
					}
					pTarget = pTarget->next;
				}
			}
		}
			
	}
}
#pragma endregion Add_Resource

#pragma region Release_Resource
bool IoT_DelGroup(MSG_CLASSIFY_T* pParentNode, char* groupName)
{
	return MSG_DelClassify(pParentNode, groupName);
}

bool IoT_DelGroupAttribute(MSG_CLASSIFY_T* pNode, char* attrName)
{
	return MSG_DelJSONAttribute(pNode, attrName);
}

bool IoT_DelSensorNode(MSG_CLASSIFY_T* pNode, char* senName)
{
	return MSG_DelIoTSensor(pNode, senName);
}

void IoT_ReleaseAll(MSG_CLASSIFY_T* pNode)
{
	MSG_ReleaseRoot(pNode);
}
#pragma endregion Release_Resource

#pragma region Find_Resource
MSG_CLASSIFY_T* IoT_FindGroup(MSG_CLASSIFY_T* pParentNode, char* groupName)
{
	MSG_CLASSIFY_T* pTargetNode = NULL;
	if(pParentNode)
	{
		if(pParentNode->type == class_type_root)
			pParentNode = pParentNode->sub_list;
		pTargetNode = MSG_FindClassify(pParentNode, groupName);
	}
	return pTargetNode;
}

MSG_ATTRIBUTE_T* IoT_FindGroupAttribute(MSG_CLASSIFY_T* pNode, char* attrName)
{
	MSG_ATTRIBUTE_T* pTargetAttr = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;
		pTargetAttr = MSG_FindJSONAttribute(pNode, attrName);
	}
	return pTargetAttr;
}

MSG_ATTRIBUTE_T* IoT_FindSensorNode(MSG_CLASSIFY_T* pNode, char* senName)
{
	MSG_ATTRIBUTE_T* pTargetAttr = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;
		pTargetAttr = MSG_FindIoTSensor(pNode, senName);
	}
	return pTargetAttr;
}

MSG_ATTRIBUTE_T* IoT_FindSensorNodeWithPath(MSG_CLASSIFY_T *msg,char *path)
{
	MSG_ATTRIBUTE_T* attr = NULL;
	//char *delim = "/";
	//char *str=strstr(path, delim);

	if(path)
	{
		attr = MSG_FindAttributeWithPath(msg, path, true);
	}
	return attr;
}

bool IoT_IsSensorExist(MSG_CLASSIFY_T *msg,char *path)
{
	MSG_ATTRIBUTE_T* attr = IoT_FindSensorNodeWithPath(msg, path);
	return attr!=NULL?true:false;
}

bool IoT_HasWritableSensorNode(MSG_CLASSIFY_T* pRoot)
{
	MSG_CLASSIFY_T* curClass = pRoot;

	while(curClass)
	{
		if(curClass->attr_list)
		{
			struct msg_attr *curAttr = curClass->attr_list;
			while(curAttr)
			{
				if(strcmp(curAttr->readwritemode, "w") == 0)
				{
					return true;
				}
				else if(strcmp(curAttr->readwritemode, "rw") == 0)
				{
					return true;
				}
				curAttr = curAttr->next;	
			}
		}
		if(IoT_HasWritableSensorNode(curClass->sub_list))
			return true;
		curClass = curClass->next;
	}
	return false;
}

#pragma endregion Find_Resource

#pragma region Generate_JSON 
char *IoT_PrintCapability(MSG_CLASSIFY_T* pRoot)
{
	return MSG_PrintUnformatted(pRoot);
	/*int size = 14;
	char* filter[] ={"n", "bn", "v","sv","bv","id","StatusCode","sessionID","$date","asm", "nonSensorData", "max", "min", "u"};
	return MSG_JSONPrintWithFiltered(pRoot, filter, size, false, false);*/
}

char *IoT_PrintFullCapability(MSG_CLASSIFY_T* pRoot, char *agentID)
{
	cJSON* root = NULL;
	cJSON* target = NULL;
	cJSON* susCmd = NULL;
	cJSON* datetime = NULL;
	char* data = NULL;
	long long tick = 0;

	if(pRoot == NULL || agentID == NULL)
		return NULL;
	data = MSG_PrintUnformatted(pRoot);
	target = cJSON_Parse(data);
	free(data);
	if(!target) return NULL;
	
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "content", target);
	cJSON_AddNumberToObject(root, "commCmd", 2052);
	cJSON_AddStringToObject(root, "agentID", agentID);
	cJSON_AddStringToObject(root, "handlerName", "general");
	
	tick = IoT_GetTimeTick();

	datetime = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "sendTS", datetime);
	cJSON_AddNumberToObject(datetime, "$date", tick);

	data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return data;
}

char *IoT_PrintData(MSG_CLASSIFY_T* pRoot)
{
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	
	return MSG_PrintWithFiltered(pRoot,filter,size);
}

char *IoT_PrintLiveData(MSG_CLASSIFY_T* pRoot)
{
	char* filter[] ={"n", "bn", "v","sv","bv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);

	return MSG_JSONPrintWithFiltered(pRoot, filter, size, false, false);
}

char *IoT_PrintDataEx(MSG_CLASSIFY_T* pRoot, bool bsimulate)
{
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	
	return MSG_JSONPrintWithFiltered(pRoot, filter, size, false, bsimulate);
}

char *IoT_PrintFlatData(MSG_CLASSIFY_T* pRoot, bool bsimulate)
{
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	
	return MSG_PrintFlatWithFiltered(pRoot,filter,size, bsimulate);
}

char *IoT_PrintFullData(MSG_CLASSIFY_T* pRoot, char *agentID)
{
	cJSON* root = NULL;
	cJSON* target = NULL;
	cJSON* susCmd = NULL;
	cJSON* datetime = NULL;
	char* data = NULL;
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	long long tick = 0;

	if(pRoot == NULL || agentID == NULL)
		return NULL;
	data = MSG_PrintWithFiltered(pRoot,filter,size);
	if(data)
	{
		target = cJSON_Parse(data);
		free(data);
	}
	if(!target) return NULL;

	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "content", target);
	cJSON_AddNumberToObject(root, "commCmd", 2055);
	cJSON_AddStringToObject(root, "agentID", agentID);
	cJSON_AddStringToObject(root, "handlerName", "general");
	
	tick = IoT_GetTimeTick();
	
	datetime = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "sendTS", datetime);
	cJSON_AddNumberToObject(datetime, "$date", tick);

	data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return data;
}

char *IoT_PrintFullDataEx(MSG_CLASSIFY_T* pRoot, char *agentID, bool bsimulate)
{
	cJSON* root = NULL;
	cJSON* target = NULL;
	cJSON* susCmd = NULL;
	cJSON* datetime = NULL;
	char* data = NULL;
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	long long tick = 0;

	if(pRoot == NULL || agentID == NULL)
		return NULL;
	data = MSG_JSONPrintWithFiltered(pRoot, filter, size, false, bsimulate);
	if(data)
	{
		target = cJSON_Parse(data);
		free(data);
	}
	if(!target) return NULL;

	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "content", target);
	cJSON_AddNumberToObject(root, "commCmd", 2055);
	cJSON_AddStringToObject(root, "agentID", agentID);
	cJSON_AddStringToObject(root, "handlerName", "general");
	
	tick = IoT_GetTimeTick();
	
	datetime = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "sendTS", datetime);
	cJSON_AddNumberToObject(datetime, "$date", tick);

	data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return data;
}

char *IoT_PrintFullFlatData(MSG_CLASSIFY_T* pRoot, char *agentID, bool bsimulate)
{
	cJSON* root = NULL;
	cJSON* target = NULL;
	cJSON* susCmd = NULL;
	cJSON* datetime = NULL;
	char* data = NULL;
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	long long tick = 0;

	if(pRoot == NULL || agentID == NULL)
		return NULL;
	data = MSG_PrintFlatWithFiltered(pRoot,filter,size, bsimulate);
	if(data)
	{
		target = cJSON_Parse(data);
		free(data);
	}
	if(!target) return NULL;

	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "content", target);
	cJSON_AddNumberToObject(root, "commCmd", 2055);
	cJSON_AddStringToObject(root, "agentID", agentID);
	cJSON_AddStringToObject(root, "handlerName", "general");
	
	tick = IoT_GetTimeTick();

	datetime = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "sendTS", datetime);
	cJSON_AddNumberToObject(datetime, "$date", tick);

	data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return data;
}

char *IoT_PrintSelectedData(MSG_CLASSIFY_T* pRoot, char* reqItems)
{
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	
	return MSG_PrintSelectedWithFiltered(pRoot,filter,size, reqItems);
}

char *IoT_PrintSelectedLiveData(MSG_CLASSIFY_T* pRoot, char* reqItems)
{
	char* filter[] ={"n", "bn", "v","sv","bv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);

	return MSG_JSONPrintSelectedWithFiltered(pRoot, filter, size, reqItems, false, false);
}


char *IoT_PrintSelectedDataEx(MSG_CLASSIFY_T* pRoot, char* reqItems, bool bsimulate)
{
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	
	return MSG_JSONPrintSelectedWithFiltered(pRoot,filter,size, reqItems, false, bsimulate);
}

char *IoT_PrintSelectedFlatData(MSG_CLASSIFY_T* pRoot, char* reqItems, bool bsimulate)
{
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	
	return MSG_PrintSelectedFlatWithFiltered(pRoot,filter,size, reqItems, bsimulate);
}

char *IoT_PrintFullSelectedData(MSG_CLASSIFY_T* pRoot, char* reqItems, char *agentID)
{
	cJSON* root = NULL;
	cJSON* target = NULL;
	cJSON* susCmd = NULL;
	cJSON* datetime = NULL;
	char* data = NULL;
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	long long tick = 0;

	if(pRoot == NULL || agentID == NULL)
		return NULL;
	data = MSG_PrintSelectedWithFiltered(pRoot,filter,size, reqItems);
	if(data)
	{
		target = cJSON_Parse(data);
		free(data);
	}
	if(!target) return NULL;
	
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "content", target);
	cJSON_AddNumberToObject(root, "commCmd", 2055);
	cJSON_AddStringToObject(root, "agentID", agentID);
	cJSON_AddStringToObject(root, "handlerName", "general");
	
	tick = IoT_GetTimeTick();
	
	datetime = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "sendTS", datetime);
	cJSON_AddNumberToObject(datetime, "$date", tick);

	data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return data;
}

char *IoT_PrintFullSelectedDataEx(MSG_CLASSIFY_T* pRoot, char* reqItems, char *agentID, bool bsimulate)
{
	cJSON* root = NULL;
	cJSON* target = NULL;
	cJSON* susCmd = NULL;
	cJSON* datetime = NULL;
	char* data = NULL;
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	long long tick = 0;

	if(pRoot == NULL || agentID == NULL)
		return NULL;
	data = MSG_JSONPrintSelectedWithFiltered(pRoot,filter,size, reqItems, false, bsimulate);
	if(data)
	{
		target = cJSON_Parse(data);
		free(data);
	}
	if(!target) return NULL;

	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "content", target);
	cJSON_AddNumberToObject(root, "commCmd", 2055);
	cJSON_AddStringToObject(root, "agentID", agentID);
	cJSON_AddStringToObject(root, "handlerName", "general");
	
	tick = IoT_GetTimeTick();
	
	datetime = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "sendTS", datetime);
	cJSON_AddNumberToObject(datetime, "$date", tick);

	data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return data;
}

char *IoT_PrintFullSelectedFlatData(MSG_CLASSIFY_T* pRoot, char* reqItems, char *agentID, bool bsimulate)
{
	cJSON* root = NULL;
	cJSON* target = NULL;
	cJSON* susCmd = NULL;
	cJSON* datetime = NULL;
	char* data = NULL;
	char* filter[] ={"n", "bn", "v","sv","bv","av","asv","abv","id","StatusCode","sessionID", "$date"};
	int size = sizeof(filter) / sizeof(char*);
	long long tick = 0;

	if(pRoot == NULL || agentID == NULL)
		return NULL;
	data = MSG_PrintSelectedFlatWithFiltered(pRoot,filter,size, reqItems, bsimulate);
	if(data)
	{
		target = cJSON_Parse(data);
		free(data);
	}
	if(!target) return NULL;

	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "content", target);
	cJSON_AddNumberToObject(root, "commCmd", 2055);
	cJSON_AddStringToObject(root, "agentID", agentID);
	cJSON_AddStringToObject(root, "handlerName", "general");
	
	tick = IoT_GetTimeTick();
	
	datetime = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "sendTS", datetime);
	cJSON_AddNumberToObject(datetime, "$date", tick);

	data = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return data;
}


void IoT_SetDataChangeCallback(MSG_CLASSIFY_T* pRoot, AttributeChangedCbf on_datachanged, void* pRev1)
{
	MSG_SetDataChangeCallback(pRoot, on_datachanged, pRev1);
}

#pragma endregion Generate_JSON 