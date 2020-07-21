#include "Parser.h"

/* parse  cmdID */
bool ParseReceivedData(void* data, int datalen, int * cmdID)
{
    /*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

    cJSON* root = NULL;
    cJSON* body = NULL;
    cJSON* target = NULL;

    if(!data) return false;
    if(datalen<=0) return false;
    root = cJSON_Parse((char *)data);
    if(!root) return false;

    body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
    if(!body)
    {
        cJSON_Delete(root);
        return false;
    }

    target = cJSON_GetObjectItem(body, AGENTINFO_CMDTYPE);
    if(target)
    {
        *cmdID = target->valueint;
    }
    cJSON_Delete(root);
    return true;
}

int Parser_PackDroidRootErrorRep(char *cpbStr, char ** outputStr)
{
    char * out = NULL;
    int outLen = 0;
    cJSON *pSUSICommDataItem = NULL, *cpbRoot = NULL, *cpbItem = NULL;
    if(cpbStr == NULL || outputStr == NULL) return outLen;

    pSUSICommDataItem = cJSON_CreateObject();
    cpbRoot = cJSON_Parse(cpbStr);
    if(cpbRoot)
    {
        cJSON_AddItemToObject(pSUSICommDataItem, HANDLER_NAME, cpbRoot);
    }

    out = cJSON_PrintUnformatted(pSUSICommDataItem);
    outLen = strlen(out) + 1;
    *outputStr = (char *)(malloc(outLen));
    memset(*outputStr, 0, outLen);
    strcpy(*outputStr, out);
    cJSON_Delete(pSUSICommDataItem);
    printf("%s\n",out);
    free(out);
    return outLen;
}

int Parser_PackSetSensorDataRepEx(char * baseName, char * strRep, int statusCode, char * sessionID, char** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, *sensorInfoListItem = NULL, *eItem = NULL, *subItem = NULL;
	if(outputStr == NULL || sessionID == NULL) 
		return outLen;
	pSUSICommDataItem = cJSON_CreateObject();
    sensorInfoListItem = cJSON_CreateObject();
	cJSON_AddItemToObject(pSUSICommDataItem, DROID_SENSOR_INFO_LIST, sensorInfoListItem);
	eItem = cJSON_CreateArray();
	cJSON_AddItemToObject(sensorInfoListItem, DROID_E_FLAG, eItem);
	{
		/* now only update one item */
		subItem = cJSON_CreateObject();
		cJSON_AddStringToObject(subItem, DROID_N_FLAG, baseName);
		cJSON_AddStringToObject(subItem, DROID_SV_FLAG, strRep);
		cJSON_AddNumberToObject(subItem, DROID_STATUS_CODE_FLAG, statusCode);
		cJSON_AddItemToArray(eItem, subItem);
	}
	cJSON_AddStringToObject(pSUSICommDataItem, DROID_SESSION_ID, sessionID);

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);	
	printf("%s\n",out);	
	free(out);
	return outLen;
}
