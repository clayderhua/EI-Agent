#include "SnGwParser.h"
#include "SnMJSONParser.h"
#include "IoTGWHandler.h"
#include "inc/SensorNetwork_Manager_API.h"
#include "inc/AdvWebClientAPI.h"

// WAPI

typedef struct{
   char URI[MAX_PATH];
   char data[MAX_SET_DATA];
   void* pAsyncSetParam;
}WAPI_SET_PARAMS;

extern int ProcSet_Result( const char *pInJson, const int InDataLen, void *pInParam, void *pRev1 );

static CAGENT_PTHREAD_ENTRY(ThreadSetValue, args)
{
	int i=0;
	int len = 0;
	char buffer[MAX_BUFFER_SIZE]={0};
	WAPI_SET_PARAMS *pWpiSet_Params = args;	
	AsynSetParam *pAsynSetParam = NULL;
	WEBAPI_CODE nStatusCode = WAPI_CODE_FAILED;

	if( pWpiSet_Params == NULL ) goto exit_set;

	pAsynSetParam = pWpiSet_Params->pAsyncSetParam;


	
	//printf("ThreadSetValue data=         %s\n", pWpiSet_Params->data);
	nStatusCode = g_pWAPI_Interface->AdvWAPI_Set(pWpiSet_Params->URI, pWpiSet_Params->data, strlen(pWpiSet_Params->data), buffer, sizeof(buffer));

	memset(buffer,0,sizeof(buffer));

	if( nStatusCode == WAPI_CODE_OK ) {
		snprintf(buffer, sizeof(buffer), "{ \"StatusCode\": 200, \"Result\": {\"sv\":\"Success\"}}");
	} else {
		snprintf(buffer, sizeof(buffer), "{ \"StatusCode\": 500, \"Result\": {\"sv\":\"Fail\"}}");
	}

	ProcSet_Result(buffer, strlen(buffer), pAsynSetParam, NULL);
    
exit_set:

	if( pWpiSet_Params )
		FreeMemory( pWpiSet_Params );

	app_os_thread_exit(0);
	return 0;
}


int ParseReceivedData(void* data, int datalen, int * cmdID, char *sessionId, int nLenSessionId )
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	int ver = 1;
	if(!data) return 0;
	if(datalen<=0) return 0;
	root = cJSON_Parse((char *)data);
	if(!root) { 
		return 0;
	}

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);

	
	if(!body)
	{
		// Try WISE-PaaS/2.0
		body = cJSON_GetObjectItem( root, WISE_2_0_CONTENT_BODY_STRUCT);
		if( !body )
		{	
			cJSON_Delete(root);
			return 0;
		}
		else
			ver = 2;
	}

	if( ver == 1)
		target = cJSON_GetObjectItem(body, AGENTINFO_CMDTYPE);
	else
		target = cJSON_GetObjectItem(root, AGENTINFO_CMDTYPE);


	if(target)
		*cmdID = target->valueint;


	target = cJSON_GetObjectItem( body, AGENTINFO_SESSION_ID ); 

	if(target)
		snprintf(sessionId,nLenSessionId,"%s",target->valuestring );

	PRINTF("cmd=%d sessionId=%s\n", *cmdID, sessionId);
	cJSON_Delete(root);
	return 1;
}

int ParseAgentInfo(void* data, int datalen, int * status, char *parentID, int size)
{
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return 0;
	if(datalen<=0) return 0;
	root = cJSON_Parse((char *)data);
	if(!root) { 
		return 0;
	}

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);

	
	if(!body)
	{
		// Try WISE-PaaS/2.0
		body = cJSON_GetObjectItem( root, WISE_2_0_CONTENT_BODY_STRUCT);
		if( !body )
		{	
			cJSON_Delete(root);
			return 0;
		}
	}

	target = cJSON_GetObjectItem( body, AGENTINFO_STATUS ); 
	if(target)
		*status = target->valueint;

	target = cJSON_GetObjectItem( body, AGENTINFO_PARENT_ID ); 
	if(target)
		strncpy(parentID, target->valuestring, size);

	cJSON_Delete(root);
	return 1;
}


//   topic: /cagent/admin/%s/agentcallbackreq
int GetSenHubUIDfromTopic(const char *topic, char *uid , const int size )
{
	int rc = 0;
	char *start = NULL;
	char *end = NULL;
	char *sp = NULL;
	int len = 0;

	if( topic == NULL || strlen(topic) == 0 || uid == NULL ) return rc;

	sp = strstr(topic,"admin/");
	if( sp )
		start = sp + 6; // after /

	sp = strstr(topic,"/agentcallbackreq");
	if(sp)
	end = sp;

	if( start == NULL || end == NULL ) return rc;

	len = end - start;

	if( size < len ) return rc;

	memcpy( uid, start, len );

	rc = 1;

	return rc;
}

void RePack_GetResultToWISECloud( const char *name, const char *data , char *szResult, int bufsize )
{
	int len = 0;
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	//char szResult[MAX_FUNSET_DATA_SIZE]={0};

	unsigned int nStatusCode = 200;

	cJSON* root = NULL;
	cJSON  *pSubItem = NULL; 
	char* buff;

	if(szResult == NULL)
		return;

	if( data == NULL ) { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return;
	}

	root = cJSON_Parse((char *)data);
	if(!root) { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return;
	}

	pSubItem = cJSON_GetObjectItem( root, SN_STATUS_CODE ); 

	if(!pSubItem)  { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return;
	}

	nStatusCode = pSubItem->valueint;

	// In:    { "StatusCode":200, "Result":{"n":"door","v":26,"min":0, "max":100} }
	// Out: {"n":"SenData/door temp","v":26, "StatusCode": 200}
	
	// tmpbuf => "v": 26,   "sv":"123", "bv":1
	pSubItem = cJSON_GetObjectItem( root, SN_RESUTL ); 
	
	if(!pSubItem)  { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return;
	}
	buff = cJSON_Print(pSubItem);
	GetSenMLValue(buff, tmpbuf, sizeof(tmpbuf));

	if(strlen(tmpbuf) == 0 ) {
		snprintf(tmpbuf,sizeof(tmpbuf),"\"sv\":%s",buff);
	}
	free(buff);
                                                        /* { "n":"%s", %s, "StatusCode":  %d} */
	snprintf(szResult, bufsize, REPLY_SENSOR_FORMAT, name, tmpbuf, nStatusCode );
	return;
}


int RemoveJSONTag(const char *src, char *buf , int bufsize)
{
        int len = 0;
        char *sp = strchr(src,'{' );
        char *end = strrchr(src,'}' );
        if( sp && end ) {
                len = end - sp -1;
				if( len <= bufsize )                
					memcpy( buf, sp+1, len );
				else
					len = -1;
        }
        return len;
}



void RePack_GetWResultToWISECloud( const int code, const char *name, const char *data  /* In:    {"bv":1} */, 
																			 char *szResult /* Out: {"n":"SenData/door temp","v":26, "StatusCode": 200} */ , int bufsize )
{
	int len = 0;
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};

	unsigned int nStatusCode = 200;

	if( code < 0 )
		nStatusCode = code * -1;


	if(szResult == NULL)
		return;

	if( data == NULL ) { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return;
	}

	// remove { }
	RemoveJSONTag(data, tmpbuf, MAX_FUNSET_DATA_SIZE);

                                                        /* { "n":"%s", %s, "StatusCode":  %d} */
	snprintf(szResult, bufsize, REPLY_SENSOR_FORMAT, name, tmpbuf, nStatusCode );
	return;
}



int ProcGetSenHubValue(const char *uid, const char *sessionId, const char *data, char *buffer, const int bufsize)
{
	int len = 0;
	int i =0;
	int ver = 1;	
	char URI[MAX_PATH]={0};
	char handlename[MAX_HANDL_NAME]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	int nStatusCode = 200;


	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pElement = NULL;
	cJSON* pSubItem = NULL;
	cJSON* jsName = NULL;
	int nCount = 0;

	if(!data) return len;

	if( !pSNManagerAPI && !g_pWAPI_Interface )
		return len;

	root = cJSON_Parse((char *)data);
	if(!root)
		goto exit_get;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);

	if(!body) 
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);

	
		if(!body)
		{
			// Try WISE-PaaS/2.0
			body = cJSON_GetObjectItem( root, WISE_2_0_CONTENT_BODY_STRUCT);
			if( !body ) 
				goto exit_get;
			ver = 2;
		}				
	}

	if( ver == 2)
		pSubItem = cJSON_GetObjectItem( root, SEN_HANDLER_NAME ); // handlerName
	else
		pSubItem = cJSON_GetObjectItem( body, SEN_HANDLER_NAME ); 

	if(!pSubItem) goto exit_get;

	snprintf(handlename,sizeof(handlename),"%s",pSubItem->valuestring );

	target = cJSON_GetObjectItem(body, SEN_IDLIST);

	if(!target)	goto exit_get;


	pElement = cJSON_GetObjectItem( target, SENML_E_FLAG ); // "e"
	nCount = cJSON_GetArraySize(pElement);

	for( i = 0; i<nCount ; i ++ ) {
		pSubItem = cJSON_GetArrayItem(pElement, i);
		if( pSubItem == NULL ) continue;

		jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG );
		
		if( jsName == NULL ) continue;

		memset(URI, 0, sizeof(URI) );
		//snprintf(URI,sizeof(URI),"%s/%s/%s", uid,handlename, jsName->valuestring ); // => SenHub_UID/SenHub/SenData/co2

		if( pSNManagerAPI ) {
			snprintf(URI,sizeof(URI),"%s/%s", uid, jsName->valuestring ); // => SenHub_UID/SenHub/SenData/co2
			snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_GetData(URI, CACHE_MODE));
			RePack_GetResultToWISECloud( jsName->valuestring, tmpbuf, sendatas, sizeof(sendatas) ); 
		} else {			
			snprintf(URI,sizeof(URI),"%s/%s/%s", SENHUB_PREFIX, uid, jsName->valuestring  );  // => restapi/WSNManage/SenHub/<DeviceID>/SenData/CO2
			printf("get uri=%s\n", URI);
			nStatusCode = g_pWAPI_Interface->AdvWAPI_Get(URI, tmpbuf, sizeof(tmpbuf));	
			RePack_GetWResultToWISECloud( nStatusCode, jsName->valuestring, tmpbuf, sendatas, sizeof(sendatas) );
		}
		if( i != nCount -1 ) // append ","
			strcat( sendatas, "," );
	}

	/*{"sessionID":"%s", "sensorInfoList":{"e":[%s]} }*/
	snprintf( buffer, bufsize, REPLY_FORMAT, sessionId, sendatas );

exit_get:

	if( root ) cJSON_Delete(root);


	return strlen(buffer);
}

int ProcGetSenHubCapability(const char *uid, const char *data, char *buffer, const int bufsize)
{
	int len = 0;
	int i =0;
	char URI[MAX_PATH]={0};
	//char handlename[MAX_HANDL_NAME]={0};
	char tmpbuf[MAX_BUFFER_SIZE]={0};
	int nStatusCode = 200;

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pSubItem = NULL;

	int nCount = 0;

	if(!data) return len;

	if( !pSNManagerAPI && !g_pWAPI_Interface )
		return len;

	
	// SenHubID/SenHub
	memset(URI, 0, sizeof(URI) );
	memset(tmpbuf, 0, sizeof(tmpbuf));

	if( g_pWAPI_Interface ) // WAPI 
	{
		snprintf(URI,sizeof(URI),"%s/%s", SENHUB_PREFIX, uid ); // => SenHub_UID/SenHub
		if( g_pWAPI_Interface->AdvWAPI_Get(URI, tmpbuf, sizeof(tmpbuf)) != WAPI_CODE_OK ){
			nStatusCode = 400;
			snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
			return strlen(buffer);
		}else{
			//snprintf(buffer,bufsize,"{\"infoSpec\":%s}",tmpbuf );
			snprintf(buffer,bufsize,"%s",tmpbuf );
			return strlen(tmpbuf);		
		}
	}

	snprintf(URI,sizeof(URI),"%s/SenHub", uid ); // => SenHub_UID/SenHub
	snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_GetData(URI, CACHE_MODE));

	//PRINTF("URI=%s Ret=%s\n", URI, tmpbuf);

	//{ \"StatusCode\":  %s, \"Result\": \"%s\"}
	root = cJSON_Parse((char *)tmpbuf);
	if(!root) {
		// "{\"sessionID\":\"%s\",\"StatusCode\":%d}"
		snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
		return  strlen(buffer);;
	}

	pSubItem = cJSON_GetObjectItem( root, SN_STATUS_CODE ); 

	if(!pSubItem) {
		// "{\"sessionID\":\"%s\",\"StatusCode\":%d}"
		snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
		return  strlen(buffer);;
	}
		
	nStatusCode = pSubItem->valueint;

	if( nStatusCode != 200 ) {  // "{\"sessionID\":\"%s\",\"StatusCode\":%d}"
		snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
	}else {
		target = cJSON_GetObjectItem( root, SN_RESUTL );
				
		if( !target ) // "{\"sessionID\":\"%s\",\"StatusCode\":%d}"
			snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
		else {
			char* buff;
			cJSON* infoRoot = cJSON_CreateObject();
			cJSON* infoSpec = cJSON_CreateObject();
			cJSON_AddItemReferenceToObject(infoSpec, "SenHub", target);

			cJSON_AddItemToObject(infoRoot, "infoSpec", infoSpec);
			buff =cJSON_Print(infoRoot);
			snprintf(buffer,bufsize,"%s", buff);
			free(buff);

			cJSON_Delete(infoRoot);	
			infoRoot = NULL;
		}
	}

	cJSON_Delete(root);
	root = NULL;

	//PRINTF("Get SenHub=%s Capability=%s\n", uid, buffer );

	len = strlen(buffer);
	return len;
}


int RePack_SetResultToWISECloud( const char *name, const char *data, int *code, char *szResult, int iLenResult)
{
	int len = 0;
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};

	unsigned int nStatusCode = 202;

	cJSON* root = NULL;
	cJSON  *pSubItem = NULL; 
	char* buff;

	if(szResult == NULL)
	{
		nStatusCode = 400;
		*code = nStatusCode;
		return nStatusCode;
	}

	if( data == NULL ) { 
		nStatusCode = 400;
		*code = nStatusCode;
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return nStatusCode;
	}

	root = cJSON_Parse((char *)data);
	if(!root) { 
		nStatusCode = 400;
		*code = nStatusCode;
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return false;
	}

	// In:    { "StatusCode":200, "Result":{"n":"door","v":26,"min":0, "max":100} }
	// Out: {"n":"SenData/door temp","v":26, "StatusCode": 200}
	
	// tmpbuf => "v": 26,   "sv":"123", "bv":1
	GetSenMLValue(data, tmpbuf, sizeof(tmpbuf));

	pSubItem = cJSON_GetObjectItem( root, SN_STATUS_CODE ); 

	if(!pSubItem) {
		nStatusCode = 400;
		*code = nStatusCode;
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return nStatusCode;
	}

	nStatusCode = pSubItem->valueint;
	*code = pSubItem->valueint;

	// In:    { "StatusCode":200, "Result":{"n":"door","v":26,"min":0, "max":100} }
	// Out: {"n":"SenData/door temp","v":26, "StatusCode": 200}
	
	// tmpbuf => "v": 26,   "sv":"123", "bv":1
	pSubItem = cJSON_GetObjectItem( root, SN_RESUTL ); 
	
	if(!pSubItem)  { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return nStatusCode;
	}
	buff = cJSON_Print(pSubItem);
	GetSenMLValue(buff, tmpbuf, sizeof(tmpbuf));
	if(strlen(tmpbuf) == 0 ) {
		snprintf(tmpbuf,sizeof(tmpbuf),"\"sv\":%s",buff);
	}
	free(buff);

	                                                      /* { "n":"%s", %s, "StatusCode":  %d} */
	snprintf(szResult, iLenResult, REPLY_SENSOR_FORMAT, name, tmpbuf, nStatusCode );

	return nStatusCode;
}

int EnableSenHubAutoReport(const char *uid , int enable )
{
	char URI[MAX_PATH]={0};
	char setvalue[MAX_SET_RFORMAT_SIZE]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	int statuscode = 200;
	int rc = 0;

	AsynSetParam *pAsynSetParam = NULL;

	if( !pSNManagerAPI && !g_pWAPI_Interface )
		return rc;

	if( pSNManagerAPI ) {
		
		pAsynSetParam = AllocateMemory(sizeof(AsynSetParam));
		
		if( pAsynSetParam == NULL ) return rc;

		memset(pAsynSetParam, 0 , sizeof(AsynSetParam));
		pAsynSetParam->index = -1; // It is AP behavior not process, only check not reply to WISE-PaSS

		snprintf(setvalue,sizeof(setvalue),"{\"bv\":%d}",enable);


		snprintf(URI, sizeof(URI), "%s/SenHub/Action/AutoReport", uid );
		snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_SetData(URI, setvalue, pAsynSetParam ));
		RePack_SetResultToWISECloud(URI, tmpbuf, &statuscode, sendatas, sizeof(sendatas));

		if( statuscode != 202 ) { // Error or Success  => pAsynSetParam (x)
				PRINTF("Enable %s   Action/AutoReport Failed", uid);
				FreeMemory( pAsynSetParam ); 
		} else
			rc = 1; // Successful
	} else { // create thread to set and wait result
		//snprintf(URI, sizeof(URI), "%s/%s/SenHub/Action/AutoReport", SENHUB_PREFIX, uid );
		//statuscode = g_pWAPI_Interface->AdvWAPI_Set(URI, setvalue, strlen(setvalue), tmpbuf, sizeof(tmpbuf));
		rc =1;
	}

	return rc;
}



// RMM3.3
// {"susiCommData":{"commCmd":525,"handlerName":"SenHub","sessionID":"XXX","sensorIDList":{"e":[{"n":"SenData/dout","bv":1}]}}}
// 2.0
// {"content":{"sensorIDList":{"e":[{"sv":"abc","n":"SenHub/Action/image-update"}]}},"commCmd":525,"agentID":"","handlerName":"SenHub","sendTS":{"$date":1516946039083}}
int ProcSetSenHubValue(const char *uid, const char *sessionId, const char *cmd, const int index, const char *topic, char *buffer, const int bufsize)
{
	int len = 0;
	int i = 0;
	int ver = 1;
	char URI[MAX_PATH]={0};
	char handlename[MAX_HANDL_NAME]={0};
	//char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char *tmpbuf = NULL;
	char *sendatas = NULL;//[MAX_BUFFER_SIZE]={0};
	char *setvalue = NULL;//[MAX_SET_RFORMAT_SIZE]={0};

	AsynSetParam *pAsynSetParam = NULL;
	WAPI_SET_PARAMS *pWpiSet_Params = NULL;
	int statuscode = 200;
	int nCount = 0;

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pElement = NULL;
	cJSON* pSubItem = NULL;
	cJSON* jsName = NULL;



	if(!cmd) return len;

	if( !pSNManagerAPI && !g_pWAPI_Interface )
		return len;

	tmpbuf   = AllocateMemory(MAX_SET_DATA);
	sendatas = AllocateMemory(MAX_SET_DATA);
	setvalue = AllocateMemory(MAX_SET_DATA);		



	if( tmpbuf == NULL  || sendatas == NULL || setvalue == NULL ) 
	{
		printf("[IoTGWHandler ProcSetSenHubValue] Fail to Allocate Memory\n");
		goto exit_set;
	}

	memset(tmpbuf, 0, MAX_SET_DATA);
	memset(sendatas, 0, MAX_SET_DATA);
	memset(setvalue, 0, MAX_SET_DATA);

	root = cJSON_Parse((char *)cmd);
	if(!root) 
		goto exit_set;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT); // susiCommData
	if(!body) 
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);

	
		if(!body)
		{
			// Try WISE-PaaS/2.0
			body = cJSON_GetObjectItem( root, WISE_2_0_CONTENT_BODY_STRUCT);
			if( !body ) 
				goto exit_set;
			ver = 2;
		}				
	}

	if( ver == 2)
		pSubItem = cJSON_GetObjectItem( root, SEN_HANDLER_NAME ); // handlerName
	else
		pSubItem = cJSON_GetObjectItem( body, SEN_HANDLER_NAME ); // handlerName

	if(!pSubItem) 
		goto exit_set;

	snprintf(handlename,sizeof(handlename),"%s",pSubItem->valuestring ); // handlername -> SenHub


	target = cJSON_GetObjectItem(body, SEN_IDLIST);

	if(!target)
		goto exit_set;


	pElement = cJSON_GetObjectItem( target, SENML_E_FLAG ); // "e"
	nCount = cJSON_GetArraySize(pElement);

	// only support nCount == 1

	for( i = 0; i<nCount ; i ++ ) {
		pSubItem = cJSON_GetArrayItem(pElement, i);
		if( pSubItem == NULL) break;

		jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG ); // SenData/co2
		memset(URI, 0, sizeof(URI) );
		//snprintf(URI,sizeof(URI),"%s/%s/%s", uid,handlename, jsName->valuestring ); // => SenHub_UID/SenHub/SenData/co2
		snprintf(URI,sizeof(URI),"%s/%s", uid, jsName->valuestring ); // => SenHub_UID/SenHub/SenData/co2

		pAsynSetParam = AllocateMemory(sizeof(AsynSetParam));
		if( pAsynSetParam != NULL ) {
			char* buff;
			memset(pAsynSetParam, 0 , sizeof(AsynSetParam));
			pAsynSetParam->index = index;
			snprintf( pAsynSetParam->szTopic, sizeof(pAsynSetParam->szTopic), "%s", topic );
			snprintf( pAsynSetParam->szUID, sizeof(pAsynSetParam->szUID), "%s", uid );
			/* { \"sessionID\":\"%s\", \"sensorInfoList\":{\"e\":[ {\"n\":\"%s\", %%s, \"StatusCode\": %%d } ] } */
			snprintf( pAsynSetParam->szSetFormat, sizeof(pAsynSetParam->szSetFormat), SENHUB_SET_REPLY_FORMAT, sessionId, jsName->valuestring );
			memset( tmpbuf, 0, MAX_SET_DATA );
			buff = cJSON_Print(pSubItem);
			GetSenMLValue(buff, tmpbuf, MAX_SET_DATA);
			free(buff);
			snprintf( setvalue, MAX_SET_DATA,"{%s}", tmpbuf );
			memset( tmpbuf, 0, MAX_SET_DATA );

			if( pSNManagerAPI )
				snprintf(tmpbuf,MAX_SET_DATA,"%s",pSNManagerAPI->SN_Manager_SetData(URI, setvalue, pAsynSetParam ));
			else if ( g_pWAPI_Interface ) {	
				void* threadHandler;
				snprintf(tmpbuf,MAX_SET_DATA,"{ \"StatusCode\": 202, \"Result\": {\"sv\":\"Accepted\"}}");	
				pWpiSet_Params = AllocateMemory(sizeof(WAPI_SET_PARAMS));
				if(pWpiSet_Params ) {
					memset(pWpiSet_Params, 0, sizeof(WAPI_SET_PARAMS));
					snprintf( pWpiSet_Params->URI, sizeof(pWpiSet_Params->URI), "%s/%s", SENHUB_PREFIX, URI); //=> restapi/WSNManage/SenHub/<DeviceID>/SenData/CO2
					
					snprintf( pWpiSet_Params->data, sizeof(pWpiSet_Params->data), "%s", setvalue );
					pWpiSet_Params->pAsyncSetParam = pAsynSetParam;
				}

				if (app_os_thread_create(&threadHandler, ThreadSetValue, pWpiSet_Params) == 0)
				{
					app_os_thread_detach(threadHandler);
				}								
			}

			RePack_SetResultToWISECloud( jsName->valuestring, tmpbuf, &statuscode, sendatas, MAX_SET_DATA );

			if( statuscode != 202 ) // Error or Success  => pAsynSetParam (x)
				FreeMemory( pAsynSetParam );
		}else {
			snprintf( sendatas, MAX_SET_DATA,  REPLY_SENSOR_400ERROR, jsName->valuestring );
		}
		break; // one sessionID -> one Set Param
	}


	/*{"sessionID":"%s", "sensorInfoList":{"e":[%s]} }*/
	snprintf( buffer, bufsize, REPLY_FORMAT, sessionId, sendatas );

	len = strlen(buffer);

exit_set:

	if(root) cJSON_Delete(root);

	if( tmpbuf )   FreeMemory(tmpbuf);
	if( sendatas ) FreeMemory(sendatas);
	if( setvalue ) FreeMemory(setvalue);

	//printf("[IoTGWHandler ProcSetSenHubValue] Leave\n");
	return len;
}

// data: { "StatusCode":  200, "Result": "Success"}
int ProcAsynSetSenHubValueEvent(const char *data, const int datalen, const AsynSetParam *pAsynSetParam, char *buffer, const int bufsize)
{
	int len = 0;
	int i = 0;
	char result[128]={0};
	int statuscode = 200;

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pSubItem = NULL;
	char* buff;
	if(!data) return len;

	root = cJSON_Parse((char *)data);
	if(!root) return len;

	target = cJSON_GetObjectItem(root, SN_STATUS_CODE);  // "StatusCode":  200
	if(!target)
	{
		cJSON_Delete(root);
		return len;
	}

	statuscode = target->valueint;

	target = cJSON_GetObjectItem( root, SN_RESUTL ); // "Result": "Success"

	if(!target)
	{
		cJSON_Delete(root);
		return len;
	}
	buff = cJSON_Print(target);
	GetSenMLValue(buff, result, sizeof(result));

	if(strlen(result) == 0 ) {

		snprintf(result,sizeof(result),"\"sv\":%s",buff);
	}
	free(buff);
	cJSON_Delete(root);

	// { "sessionID":"XXX", "sensorInfoList":{"e":[ {"n":"SenData/dout", %s, "StatusCode": %d } ] } }
	snprintf( buffer, bufsize, pAsynSetParam->szSetFormat, result, statuscode );

	len = strlen(buffer);
	return len;
}



/* =================== Interface Get Set ================================= */

#define DEF_INT_PREFIX "/IoTGW/info"
#define DEF_INT_LEN 12

int ProceGetDefCapability( char *uri, char *buf, const int buflen )
{
	int nStatusCode = 200;
	char *sensor = NULL;

	if( strstr(uri, "IoTGW") ) {
		if(strstr(uri,"type") )
			snprintf( buf, buflen, "{\"sv\":\"WSN\"}" );
		else if ( strstr( uri,"name") )
			snprintf( buf, buflen, "{\"sv\":\"IoTGW\"}" );
		else if ( strstr(uri,"description" ) )
			snprintf( buf, buflen, "{\"sv\":\"This is a Wireless Sensor Network service\"}" );
		else if ( strstr(uri,"status") )
			snprintf( buf, buflen, "{\"sv\":\"Service is Not Available\"}" );
		else {
			snprintf( buf, buflen, "{\"sv\":\"404 Not Found\"}" );
			nStatusCode = 404;
		}
		return nStatusCode;
	}
	snprintf( buf, buflen, "{\"sv\":\"404 Not Found\"}" );
	nStatusCode = 404;
	return nStatusCode;
}


int ProcGetInterfaceValue(const char *sessionId, const char *data, char *buffer, const int bufsize, int enable )
{
	int len = 0;
	int i =0;
	char URI[MAX_PATH]={0};
	char handlename[MAX_HANDL_NAME]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	int nStatusCode = 200;
	int nCount = 0;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pElement = NULL;
	cJSON* pSubItem = NULL;
	cJSON* jsName = NULL;


	if(!data) return len;

	if( !pSNManagerAPI && !g_pWAPI_Interface )
		return len;

	root = cJSON_Parse((char *)data);
	if(!root) {
		return 0;
	}

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return 0;
	}
	pSubItem = cJSON_GetObjectItem( body, SEN_HANDLER_NAME ); 

	if(!pSubItem)
	{
		cJSON_Delete(root);
		return 0;
	}

	snprintf(handlename,sizeof(handlename),"%s",pSubItem->valuestring ); // IoTGW

	target = cJSON_GetObjectItem(body, SEN_IDLIST);
	if(!target)
	{
		cJSON_Delete(root);
		return 0;
	}

	pElement = cJSON_GetObjectItem( target, SENML_E_FLAG ); // "e"
	nCount = cJSON_GetArraySize(pElement);

	for( i = 0; i<nCount ; i ++ ) {
		pSubItem = cJSON_GetArrayItem(pElement, i);
		if( pSubItem == NULL ) continue;

		jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG );
		
		if( jsName == NULL ) continue;

		memset(URI, 0, sizeof(URI) );
		//snprintf(URI,sizeof(URI),"%s/%s", handlename, jsName->valuestring ); // => IoTGW/WSN/0000852CF4B7B0E8/Info/SenHubList
		
		if( pSNManagerAPI ) {
			snprintf(URI,sizeof(URI),"%s", jsName->valuestring ); // => IoTGW/WSN/0000852CF4B7B0E8/Info/SenHubList
			snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_GetData(URI, CACHE_MODE));
			RePack_GetResultToWISECloud( jsName->valuestring, tmpbuf, sendatas, sizeof(sendatas) );
		} else {			
			if( enable == 1 ) {
				snprintf(URI,sizeof(URI),"%s/%s", CONNECTIVITY_PREFIX, jsName->valuestring  );  // => restapi/WSNManage/Connectivity/IoTGW/WSN/0000852CF4B7B0E8/Info/SenHubList
				nStatusCode = g_pWAPI_Interface->AdvWAPI_Get(URI, tmpbuf, sizeof(tmpbuf));	
			} else {
				snprintf(URI,sizeof(URI),"%s", jsName->valuestring  );  // => restapi/WSNManage/Connectivity/IoTGW/WSN/0000852CF4B7B0E8/Info/SenHubList
				nStatusCode = ProceGetDefCapability( URI, tmpbuf, sizeof(tmpbuf));
			}
			RePack_GetWResultToWISECloud( nStatusCode, jsName->valuestring, tmpbuf, sendatas, sizeof(sendatas) );
		}
	
		if( i != nCount -1 ) // append ","
			strcat( sendatas, "," );
	}

	cJSON_Delete(root);

	/*{"sessionID":"%s", "sensorInfoList":{"e":[%s]} }*/
	snprintf( buffer, bufsize, REPLY_FORMAT, sessionId, sendatas );

	return strlen(buffer);
}


int ProcSetInterfaceValue(const char *sessionId, const char *cmd, char *buffer, const int bufsize)
{
	int len = 0;
	int i = 0;
	int ver = 1;
	char URI[MAX_PATH]={0};
	char handlename[MAX_HANDL_NAME]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	char setvalue[MAX_SET_RFORMAT_SIZE]={0};

	AsynSetParam *pAsynSetParam = NULL;
	WAPI_SET_PARAMS *pWpiSet_Params = NULL;

	int statuscode = 200;
	int nCount = 0;

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pElement = NULL;
	cJSON* pSubItem = NULL;
	cJSON* jsName = NULL;

	memset(sendatas, 0, sizeof(sendatas));


	if(!cmd) return len;

	if( !pSNManagerAPI && !g_pWAPI_Interface )
		return len;

	root = cJSON_Parse((char *)cmd);
	if(!root) goto exitSetInterface;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT); // susiCommData

	if(!body)
	{
		// Try WISE-PaaS/2.0
		body = cJSON_GetObjectItem( root, WISE_2_0_CONTENT_BODY_STRUCT);
		if( !body ) 
			goto exitSetInterface;
		ver = 2;
	}				
	

	if( ver == 2)
		pSubItem = cJSON_GetObjectItem( root, SEN_HANDLER_NAME ); // handlerName
	else
		pSubItem = cJSON_GetObjectItem( body, SEN_HANDLER_NAME ); // handlerName

	if(!pSubItem) 
		goto exitSetInterface;

	snprintf(handlename,sizeof(handlename),"%s",pSubItem->valuestring ); // handlername -> IoTGW


	target = cJSON_GetObjectItem(body, SEN_IDLIST);
	if(!target)
		goto exitSetInterface;


	pElement = cJSON_GetObjectItem( target, SENML_E_FLAG ); // "e"
	nCount = cJSON_GetArraySize(pElement);

	// only support nCount == 1

	for( i = 0; i<nCount ; i ++ ) {
		pSubItem = cJSON_GetArrayItem(pElement, i);
		if( pSubItem == NULL) break;

		jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG ); // SenData/co2
		memset(URI, 0, sizeof(URI) );
		//snprintf(URI,sizeof(URI),"%s/%s", handlename, jsName->valuestring ); // => IoTGW/WSN/0000852CF4B7B0E8/Info/Name
		snprintf(URI,sizeof(URI),"%s", jsName->valuestring ); // => IoTGW/WSN/0000852CF4B7B0E8/Info/Name

		pAsynSetParam = AllocateMemory(sizeof(AsynSetParam));
		if( pAsynSetParam != NULL ) {
			char* buff;
			memset(pAsynSetParam, 0 , sizeof(AsynSetParam));
			pAsynSetParam->index = IOTGW_INFTERFACE_INDEX;
			/* { \"sessionID\":\"%s\", \"sensorInfoList\":{\"e\":[ {\"n\":\"%s\", %%s, \"StatusCode\": %%d } ] } */
			snprintf( pAsynSetParam->szSetFormat, sizeof(pAsynSetParam->szSetFormat), SENHUB_SET_REPLY_FORMAT, sessionId, jsName->valuestring );
			memset( tmpbuf, 0, sizeof( tmpbuf ) );
			buff = cJSON_Print(pSubItem);
			GetSenMLValue(buff, tmpbuf, sizeof(tmpbuf));
			free(buff);
			snprintf( setvalue,sizeof(setvalue),"{ %s }", tmpbuf );
			memset( tmpbuf, 0, sizeof( tmpbuf ) );
			
			if( pSNManagerAPI )
				snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_SetData(URI, setvalue, pAsynSetParam ));
			else if ( g_pWAPI_Interface ) {	
				void* threadHandler;
				snprintf(tmpbuf,sizeof(tmpbuf),"{ \"StatusCode\": 202, \"Result\": {\"sv\":\"Accepted\"}}");	

				pWpiSet_Params = AllocateMemory(sizeof(WAPI_SET_PARAMS));
				if(pWpiSet_Params ) {
					memset(pWpiSet_Params, 0, sizeof(WAPI_SET_PARAMS));
					snprintf( pWpiSet_Params->URI, sizeof(pWpiSet_Params->URI), "%s/%s", CONNECTIVITY_PREFIX, URI); //=> /restapi/WSNManage/Connectivity/WSN/XXX
					snprintf( pWpiSet_Params->data, sizeof(pWpiSet_Params->data), "%s", setvalue );
					pWpiSet_Params->pAsyncSetParam = pAsynSetParam;
				}

				if (app_os_thread_create(&threadHandler, ThreadSetValue, pWpiSet_Params) == 0)
				{
					app_os_thread_detach(threadHandler);
				}								
			}
			RePack_SetResultToWISECloud( jsName->valuestring, tmpbuf, &statuscode, sendatas, sizeof(sendatas) );

			if( statuscode != 202 ) // Error or Success  => pAsynSetParam (x)
				FreeMemory( pAsynSetParam );
		}else {
			snprintf( sendatas, sizeof(sendatas),  REPLY_SENSOR_400ERROR, jsName->valuestring );
		}
		break; // one sessionID -> one Set Param
	}



	/*{"sessionID":"%s", "sensorInfoList":{"e":[%s]} }*/
	snprintf( buffer, bufsize, REPLY_FORMAT, sessionId, sendatas );

	len = strlen(buffer);

exitSetInterface:

	if(root)
		cJSON_Delete(root);
	return len;
}

int ProcGetTotalInterface(char *buffer, const int bufsize)
{
	int len = 0;
	char URI[MAX_PATH]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* newroot = NULL;
	char* buff;
	if( !pSNManagerAPI && !g_pWAPI_Interface )
		return len;

	memset(URI, 0, sizeof(URI) );
	memset(buffer, 0, bufsize );

	if( pSNManagerAPI) {
		snprintf(URI,sizeof(URI),"%s", "IoTGW" );
		strncpy(tmpbuf, pSNManagerAPI->SN_Manager_GetData(URI, CACHE_MODE), MAX_FUNSET_DATA_SIZE);
	} else {
		snprintf(URI,sizeof(URI),"%s/IoTGW", CONNECTIVITY_PREFIX );
		if( g_pWAPI_Interface->AdvWAPI_Get(URI, tmpbuf, MAX_FUNSET_DATA_SIZE) != WAPI_CODE_OK )
				memset(tmpbuf, 0, sizeof(tmpbuf));
	}

	root = cJSON_Parse(tmpbuf);
	if(!root)
		return len;

	body = cJSON_GetObjectItem(root, SN_ERROR_REP); // susiCommData

	newroot = cJSON_CreateObject();
	if( pSNManagerAPI )
		cJSON_AddItemToObject(newroot, "IoTGW", cJSON_Duplicate(body, true));
	else
		cJSON_AddItemToObject(newroot, "IoTGW", cJSON_Duplicate(root, true));

	cJSON_Delete(root);
	buff = cJSON_PrintUnformatted(newroot);
	snprintf( buffer, bufsize, "%s", buff);
	len = strlen(buffer);
	free(buff);
	cJSON_Delete(newroot);

	return len;
}

int WProcGetAllSenHubList( char* buffer, const int bufsize)
{
	char URI[MAX_PATH]={0};
	char *tmpbuf = NULL;	
	cJSON* root = NULL;
	cJSON* body = NULL;
	char* buff;
	int len = -1;

	snprintf(URI,sizeof(URI),"%s/AllSenHubList", SENHUB_PREFIX );
	if( g_pWAPI_Interface->AdvWAPI_Get(URI, buffer, bufsize) != WAPI_CODE_OK )
		return len;


	root = cJSON_Parse(buffer);
	if(!root)
	return len;

    	
	// {"n":"AllSenHubList","sv":"xxxxxx,0017000E40000001,xxxxx"}
	body = cJSON_GetObjectItem(root, SENML_SV_FLAG);
	
    tmpbuf = (char*)malloc(bufsize);
    memset(tmpbuf,0, bufsize );
    memset(buffer, 0, bufsize);
	buff = cJSON_PrintUnformatted(body);
	snprintf( tmpbuf, bufsize, "%s", buff);
	free(buff);
	len = strlen(tmpbuf);

	cJSON_Delete(root);
	
	if(len>=2)
		memcpy(buffer, tmpbuf+1, len-2);
	else
	   len = 2;
	   
	free(tmpbuf);   
	return len-2;	
}

int WProcGetSenHubDeviceInfo( const char *deviceID, char* buffer, const int bufsize)
{
	char URI[MAX_PATH]={0};

	snprintf(URI,sizeof(URI),"%s/%s/DevInfo", SENHUB_PREFIX, deviceID );
	if( g_pWAPI_Interface->AdvWAPI_Get(URI, buffer, bufsize) != WAPI_CODE_OK )
		return -1;


	return strlen( buffer );;	
}

int WProcGetSenHubCapability( const char *deviceID, char* buffer, const int bufsize)
{
	char URI[MAX_PATH]={0};

	snprintf(URI,sizeof(URI),"%s/%s", SENHUB_PREFIX, deviceID );
	if( g_pWAPI_Interface->AdvWAPI_Get(URI, buffer, bufsize) != WAPI_CODE_OK )
		return -1;
	
	return strlen( buffer );
}

int WProcGetSenHubLastValue( const char *deviceID, char* buffer, const int bufsize, char *name /*handlername: SenHub, IoTGW */ )
{
	char URI[MAX_PATH]={0};

	snprintf(URI,sizeof(URI),"%s/%s/%s", SENHUB_PREFIX, deviceID, name );
	if( g_pWAPI_Interface->AdvWAPI_Get(URI, buffer, bufsize) != WAPI_CODE_OK ){
		return -1;
	}

	return strlen( buffer );
}
