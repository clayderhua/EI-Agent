#include "ActionMessageGenerate.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define ACT_INFOMATION                   "Information"
#define ACT_E_FLAG                       "e"
#define ACT_N_FLAG                       "n"
#define ACT_BN_FLAG                      "bn"
#define ACT_V_FLAG                       "v"
#define ACT_SV_FLAG                      "sv"
#define ACT_FUNCTION_LIST                "functionList"
#define ACT_FUNCTION_CODE                "functionCode"
#define ACT_NS_DATA                      "nonSensorData"
#define ACT_NO_ACTIONS                   "none"
#define ACT_API_FLAG					 "function"
#define ACT_PARAM_FLAG					 "param"
#define ACT_DESCRIPT_FLAG                "desc"
#define ACT_TYPE_FLAG					 "type"

MSG_CLASSIFY_T* Act_GetInformation(MSG_CLASSIFY_T* pNode)
{
	MSG_CLASSIFY_T *pInfoNode = NULL;
	MSG_ATTRIBUTE_T *pListNode = NULL, *pCodetNode = NULL, *pNonSensor = NULL;
	if(pNode == NULL)
		return pInfoNode;
	if(pNode->type == class_type_root)
		pNode = pNode->sub_list;
	
	pInfoNode = MSG_FindClassify(pNode, ACT_INFOMATION);
	if(!pInfoNode)
		pInfoNode = MSG_AddIoTClassify(pNode, ACT_INFOMATION, NULL, false);

	pListNode = MSG_FindAttribute(pInfoNode, ACT_FUNCTION_LIST, true);
	if(!pListNode)
	{
		pListNode = MSG_AddAttribute(pInfoNode, ACT_FUNCTION_LIST, true);
		MSG_SetStringValue(pListNode, ACT_NO_ACTIONS, NULL);
	}

	pCodetNode = MSG_FindAttribute(pInfoNode, ACT_FUNCTION_CODE, true);
	if(!pCodetNode)
	{
		pCodetNode = MSG_AddAttribute(pInfoNode, ACT_FUNCTION_CODE, true);
		MSG_SetDoubleValue(pCodetNode, 0, NULL, NULL);
	}

	pNonSensor = MSG_FindAttribute(pInfoNode, ACT_NS_DATA, true);
	if(!pNonSensor)
	{
		pNonSensor = MSG_AddAttribute(pInfoNode, ACT_NS_DATA, false);
		MSG_SetBoolValue(pNonSensor, true, NULL);
	}
	return pInfoNode;
}

MSG_CLASSIFY_T* Act_CreateRoot(char* handlerName)
{
	MSG_CLASSIFY_T *pRoot = MSG_CreateRoot();
	MSG_AddJSONClassify(pRoot, handlerName, NULL, false);

	Act_GetInformation(pRoot);
	return pRoot;
}

void Act_AppendAction(MSG_CLASSIFY_T* pNode, char* actName, int actCode)
{
	MSG_ATTRIBUTE_T *pListNode = NULL, *pCodetNode = NULL;
	long curCode = 0;
	if(pNode == NULL)
		return;
	
	pCodetNode = MSG_FindAttribute(pNode, ACT_FUNCTION_CODE, true);
	if(!pCodetNode)
	{
		pCodetNode = MSG_AddAttribute(pNode, ACT_FUNCTION_CODE, true);
		MSG_SetDoubleValue(pCodetNode, actCode, NULL, NULL);
	}
	else
	{
		long code = pCodetNode->v + actCode;
		curCode = pCodetNode->v;
		MSG_SetDoubleValue(pCodetNode, code, NULL, NULL);
	}

	pListNode = MSG_FindAttribute(pNode, ACT_FUNCTION_LIST, true);
	if(!pListNode)
	{
		pListNode = MSG_AddAttribute(pNode, ACT_FUNCTION_LIST, true);
		MSG_SetStringValue(pListNode, actName, NULL);
	}
	else
	{
		if(curCode == 0)
		{
			MSG_SetStringValue(pListNode, actName, NULL);
		}
		else
		{
			char* buff = calloc(1, strlen(pListNode->sv) + strlen(actName) + 2);
			sprintf(buff, "%s,%s", pListNode->sv, actName);
			MSG_SetStringValue(pListNode, buff, NULL);
			free(buff);
		}
	}

}

void Act_SetNonSensorDataFlag(MSG_CLASSIFY_T* pNode, bool bNoSensor)
{
	MSG_CLASSIFY_T *pInfoNode = NULL;
	MSG_ATTRIBUTE_T *pNonSensor = NULL;
	if(pNode == NULL)
		return;
	pInfoNode = Act_GetInformation(pNode);
	pNonSensor = MSG_FindAttribute(pInfoNode, ACT_NS_DATA, true);
	if(!pNonSensor)
		pNonSensor = MSG_AddAttribute(pInfoNode, ACT_NS_DATA, false);
	MSG_SetBoolValue(pNonSensor, bNoSensor, NULL);
}

MSG_CLASSIFY_T* Act_AddAction(MSG_CLASSIFY_T* pNode, char* actName, char* descript, int actCode)
{
	char* funcList = NULL;
	long funcCode = 0;
	if(pNode)
	{
		MSG_CLASSIFY_T *pCurNode = NULL, *pInfoNode = NULL, *pAPINode = NULL;
		MSG_ATTRIBUTE_T *pDescAttr = NULL;

		pInfoNode = Act_GetInformation(pNode);

		pAPINode = MSG_FindClassify(pInfoNode, ACT_API_FLAG);
		if(!pAPINode)
			pAPINode = MSG_AddIoTClassify(pInfoNode, ACT_API_FLAG, NULL, false);
		
		pCurNode = MSG_FindClassify(pInfoNode, actName);
		if(!pCurNode)
		{
			pCurNode = MSG_AddIoTClassify(pAPINode, actName, NULL, false);
			Act_AppendAction(pInfoNode, actName, actCode);
		}

		pDescAttr = MSG_FindJSONAttribute(pCurNode, ACT_DESCRIPT_FLAG);
		if(!pDescAttr)
			pDescAttr = MSG_AddJSONAttribute(pCurNode, ACT_DESCRIPT_FLAG);

		MSG_SetStringValue(pDescAttr, descript, NULL);
		return pCurNode;
	}
	return NULL;
}

MSG_ATTRIBUTE_T * Act_AddParam(MSG_CLASSIFY_T* pNode, char* paramName, char* type)
{
	MSG_ATTRIBUTE_T *attr = NULL;
	if(pNode == NULL)
		return attr;
	
	attr = MSG_FindIoTSensor(pNode, paramName);
    if(attr == NULL)
		attr = MSG_AddIoTSensor(pNode, paramName);

	MSG_AppendIoTSensorAttributeString(attr, ACT_TYPE_FLAG, type);

	return attr;
}

MSG_ATTRIBUTE_T * Act_AddParamDouble(MSG_CLASSIFY_T* pNode, char* paramName)
{
	return Act_AddParam(pNode, paramName, "double");
}

MSG_ATTRIBUTE_T * Act_AddParamFloat(MSG_CLASSIFY_T* pNode, char* paramName)
{
	return Act_AddParam(pNode, paramName, "float");
}

MSG_ATTRIBUTE_T * Act_AddParamBool(MSG_CLASSIFY_T* pNode, char* paramName)
{
	return Act_AddParam(pNode, paramName, "bool");
}

MSG_ATTRIBUTE_T * Act_AddParamLong(MSG_CLASSIFY_T* pNode, char* paramName)
{
	return Act_AddParam(pNode, paramName, "long");
}

MSG_ATTRIBUTE_T * Act_AddParamInt(MSG_CLASSIFY_T* pNode, char* paramName)
{
	return Act_AddParam(pNode, paramName, "int");
}

MSG_ATTRIBUTE_T * Act_AddParamString(MSG_CLASSIFY_T* pNode, char* paramName)
{
	return Act_AddParam(pNode, paramName, "string");
}

MSG_ATTRIBUTE_T * Act_AddParamSerial(MSG_CLASSIFY_T* pNode, char* paramName)
{
	return Act_AddParam(pNode, paramName, "serial");
}
	
MSG_CLASSIFY_T* Act_FindAction(MSG_CLASSIFY_T* pNode, char* actName)
{
	MSG_CLASSIFY_T *pCurNode = NULL;
	if(pNode)
	{
		if(pNode->type == class_type_root)
			pNode = pNode->sub_list;

		pCurNode = MSG_FindClassify(pNode, actName);
	}
	return pCurNode;
}

MSG_ATTRIBUTE_T* Act_FindParam(MSG_CLASSIFY_T* pNode, char* paramName)
{
	return MSG_FindIoTSensor(pNode, paramName);
}

bool Act_DelAction(MSG_CLASSIFY_T* pNode, char* actName)
{
	return MSG_DelClassify(pNode, actName);
}

bool Act_DelParam(MSG_CLASSIFY_T* pNode, char* paramName)
{
	return MSG_DelJSONAttribute(pNode, paramName);
}

void Act_ReleaseAll(MSG_CLASSIFY_T* pRoot)
{
	MSG_ReleaseRoot(pRoot);
}

char *Act_PrintCapability(MSG_CLASSIFY_T* pRoot)
{
	return MSG_PrintUnformatted(pRoot);
}
