#include "SnMJSONParser.h"
#include "BasicFun_Tool.h"

// Parse single JSON data only
SenML_V_Type GetSenMLValue( const char *pData /*{"n":"door temp", "u":"Cel","v":0}*/, char *pOutValue /* "v"*/, int nSize )
{
	SenML_V_Type  vt = SenML_Unknow_Type;

	cJSON* root = NULL;
	cJSON * vItem = NULL;
	int i = 0;
	root = cJSON_Parse((char *)pData);
	if(!root) return vt;

	for ( i = SenML_Type_V; i < SenML_Type_MAX; i++ ) {
		vItem = cJSON_GetObjectItem( root, SenML_Types[i] );
		if( vItem != NULL ) {
			vt = i;	
			if( vt == SenML_Type_SV )
				snprintf(pOutValue,nSize, "\"%s\":\"%s\"",SenML_Types[i],vItem->valuestring	);
			else if( vt == SenML_Type_V )
				snprintf(pOutValue,nSize, "\"%s\":%d",SenML_Types[i],vItem->valueint	);
			else if ( vt == SenML_Type_BV )
				snprintf(pOutValue,nSize, "\"%s\":%d",SenML_Types[i],vItem->valueint	);
			break;
		} // End of If
	} // End of For
	cJSON_Delete(root);
	return vt;
}

// Not Found:0, Read:1  Write:2 RW:3
ASM GetSenMLVarableRW( const cJSON *pItem )
{
	ASM status = UNKNOW;
	cJSON *vItem = NULL;

	if( !pItem ) return status;

	vItem = cJSON_GetObjectItem( pItem, SENML_ASM_FLAG );

	if( vItem != NULL ) {
		if( !strcasecmp(vItem->valuestring, SENML_R ) ) // Read Only
			status = READ_ONLY; 
		else if( !strcasecmp(vItem->valuestring, SENML_RW ) ) // Read Write
			status = RW;
		else if( !strcasecmp(vItem->valuestring, SENML_W ) )  // Write Only
			status = WRITE_ONLY;
	}

	return status;
}

SenML_V_Type GetSenMLValueType( const cJSON *pItem )
{
	SenML_V_Type  vt = SenML_Unknow_Type;
	cJSON *vItem = NULL;
	int i = 0;

	if( !pItem ) return vt;

	for ( i = SenML_Type_V; i < SenML_Type_MAX; i++ ) {
		vItem = cJSON_GetObjectItem( pItem, SenML_Types[i] );
		if( vItem != NULL ) {
			vt = i;	
			break;
		} // End of If
	} // End of For
	return vt;
}


char *GetSenMLDataValue( const char *pData /*{"n":"door temp", "u":"Cel","v":0}*/,
												  const char*key     /*SENML_N_FLAG, SENML_V_FLAG, SENML_SV_FLAG, SENML_BV_FLAG...*/  )
{
	cJSON* root = NULL;
	cJSON * pSubItem = NULL;
	char Name[128]={0};
	memset(Name,0,sizeof(Name));

	if(!pData) 
		return NULL;

	root = cJSON_Parse((char *)pData);
	if(!root) 
		return NULL;

	pSubItem = cJSON_GetObjectItem(root, key );

	if( pSubItem != NULL )
		strcpy(Name, pSubItem->valuestring);

	cJSON_Delete(root);
	return "Name";
}


// SenData/SenHub => pOutBuf: {"e": [{"n":"door temp", "u":"Cel","v":0,"min":0,"max":100,"asm":"r","type":"d","rt":"ucum.Cel","st":"ipso","exten":""}], "bn":"SenData"}
// SenData/Info => pOutBuf: {"e": [{"n":"Health","v":100,"asm":"r"},{"n":"Name","sv":"Room","asm":"rw"}], "bn":"Info"}
int GetJSONbySpecifCategory( char *data , char *pCategory1, char *pCategory2, char *pOutBuf, int BufSize )
{
	cJSON* root = NULL;
	cJSON* categ1 = NULL;
	cJSON* categ2 = NULL;
	char* buff;
	if(!data ||  pOutBuf == NULL ) return 0;

	root = cJSON_Parse((char *)data);
	if(!root) return 0;
	 
	if( pCategory1 == NULL ) {
		buff = cJSON_Print( root );
		snprintf(pOutBuf, BufSize, "%s", buff );
		free(buff);
		cJSON_Delete(root);
		return strlen( pOutBuf );
	}

	categ1 = cJSON_GetObjectItem( root, pCategory1 ); // "SenHub"
  
	if( categ1 == NULL ) {
		cJSON_Delete(root);
		return 0;
	}

	if( pCategory2 == NULL ) {
		buff = cJSON_Print( categ1 );
		snprintf(pOutBuf, BufSize, "%s", buff );
		free(buff);
		cJSON_Delete(root);
		return strlen( pOutBuf );
	}

	categ2 = cJSON_GetObjectItem( categ1, pCategory2 ); // "SenData"

	if( categ2 == NULL ) {
		cJSON_Delete(root);
		return 0;
	}
	buff = cJSON_Print( categ2 );
	snprintf(pOutBuf, BufSize, "%s",  buff);
	free(buff);
	cJSON_Delete(root);
	return strlen( pOutBuf );
}

/*{"e":	[{"n":	"door temp","v":	0},{"n":"co2","v":1000}],"bn":	"SenData"} */
int GetSenML_ElementCount( const char * data )
{
	int count = 0;
	cJSON* root = NULL;
	cJSON* eItem = NULL;


	if(!data) return count;

	root = cJSON_Parse((char *)data);
	if(!root) return count;

	eItem = cJSON_GetObjectItem( root, SENML_E_FLAG ); // "e"

	if(eItem)
		count = cJSON_GetArraySize(eItem);

	cJSON_Delete(root);
	return count;
}

int GetElementSenMLNameValue( const char *data /*{"e":	[{"n":	"door temp","v":	0},{"n":"co2","v":1000}],"bn":	"SenData"} */,  
															   const int i,
															   char *pOutItem, int nItemSize,
															   char *pOutName /* door temp*/, int nNameSize, 
															   char *pOutValue /*"v": 0*/, int nValueSize)
{
	cJSON* root = NULL;
	cJSON* eItem = NULL;
	cJSON* subItem = NULL;
	cJSON* jsName =NULL;
	char* buff;
	int rc = -1;

	if(!data || !pOutName || !pOutValue )
		return rc;

	root = cJSON_Parse((char *)data);

	if(!root)
		return rc;

	eItem = cJSON_GetObjectItem( root, SENML_E_FLAG ); // "e"

	subItem = cJSON_GetArrayItem(eItem, i);

	if( !subItem )
		goto ExitSenMLEleNV;


	 jsName = cJSON_GetObjectItem( subItem, SENML_N_FLAG );

	 if( !jsName )
		 goto ExitSenMLEleNV;

	 buff = cJSON_Print(subItem);
	 snprintf( pOutItem, nItemSize, "%s",buff);
	 free(buff);
	 snprintf(pOutName, nNameSize,"%s",jsName->valuestring);

	 GetSenMLValue( data, pOutValue, nValueSize );

	 rc = 0;
ExitSenMLEleNV:

	 cJSON_Delete(root);

	 return rc;
}

int CheckDataIsWritable( const cJSON *jsonItem )
{
	int writable = 0;  // 0: read only, 1:writable

	return writable;
}

int GetElementString( const char *data, int i, char *pOutBuf, int nSize )
{
	cJSON* root = NULL;
	cJSON* eItem = NULL;
	cJSON* subItem = NULL;
	char* buff;
	if(!data || !pOutBuf) return 0;

	root = cJSON_Parse((char *)data);

	if(!root) return 0;

	eItem = cJSON_GetObjectItem( root, SENML_E_FLAG ); // "e"

	if( !eItem ) goto ExitGetEleString;

	subItem = cJSON_GetArrayItem(eItem, i);

	if( !subItem )	goto ExitGetEleString;
	buff = cJSON_Print( subItem );
	snprintf(pOutBuf, nSize, "%s",  buff);
	free(buff);
ExitGetEleString:

	cJSON_Delete(root);

	return strlen( pOutBuf );

}

char* RemoveDuplicateNode(const char* Data)
{
	char* newData = NULL;
	cJSON* root = cJSON_Parse((char *)Data);	
	if(root)
	{
		cJSON* target = cJSON_GetObjectItem(root, "agentID");
		if(target)
		{
			cJSON_DeleteItemFromObject(root, "agentID");
		}
		target = cJSON_GetObjectItem(root, "sendTS");
		if(target)
		{
			cJSON_DeleteItemFromObject(root, "sendTS");
		}
		newData = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
	}
	return newData;
}

void ReplaceDevIDList(cJSON* item, char* name)
{
	cJSON* value = cJSON_GetObjectItem(item, name);
	if(value)
	{
		char lists[10240] = {0}; 
		char buffer[10240] = {0};
		char* p;
		char* delim=",";
		char* token = NULL;
		strcpy(lists, value->valuestring);
		p = strtok_r(lists, delim, &token);
		while(p)
		{
			char newID[37] = {0};
			GetNewID(p, newID);
			strcat(buffer, newID);
			p = strtok_r(NULL, delim, &token);
			if(p)
				strcat(buffer, ",");
		}
		cJSON_DeleteItemFromObject(item, name);
		cJSON_AddStringToObject(item, name, buffer);
	}
}

char* ReplaceSenHubList(const char* Data)
{
	char* newData = NULL;
	cJSON* root = NULL;
	if(Data == NULL) return newData;
	//PRINTF("Data(%d) = %s\r\n", strlen(Data), Data);
	root = cJSON_Parse(Data);
	if(root)
	{
		cJSON* body = cJSON_GetObjectItem(root, "IoTGW");
		if(body)
		{
			cJSON* interFace = body->child;
			while(interFace)
			{
				cJSON* mac = interFace->child;
				while(mac)
				{
					cJSON* info = cJSON_GetObjectItem(mac, "Info");
					if(info)
					{
						cJSON* items = cJSON_GetObjectItem(info, "e");
						if(items)
						{
							int i=0;
							int size = cJSON_GetArraySize(items);
							for(i=0;i<size;i++)
							{
								cJSON* item = cJSON_GetArrayItem(items, i);
								if(item)
								{
									cJSON* name = cJSON_GetObjectItem(item, "n");
									if(name)
									{
										if(strcmp(name->valuestring,"SenHubList")==0)
										{
											ReplaceDevIDList(item, "sv");
										}
										else if(strcmp(name->valuestring,"Neighbor")==0)
										{
											ReplaceDevIDList(item, "sv");
										}
									}
								}
							}
						}
					}
					mac = mac->next;
				}
				interFace = interFace->next;
			}
		}
		newData = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
	}
	//PRINTF("NewData(%d) = %s\r\n", strlen(newData), newData);
	return newData;
}


