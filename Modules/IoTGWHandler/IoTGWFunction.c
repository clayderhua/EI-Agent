/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/09 by Eric Liang															     */
/* Modified Date: 2015/03/09 by Eric Liang															 */
/* Abstract       :  IoTGW Function                 													             */
/* Reference    : None																									 */
/****************************************************************************/

#include <stdlib.h>
#include <platform.h>

#include "IoTGWFunction.h"
#include "inc/IoTGW_Def.h"

#include "BasicFun_Tool.h"

#include <time.h>
#include <sys/time.h>

#define DEF_SENHUB_AGENTINFO_JSON					"{\"parentID\":\"%s\",\"hostname\":\"%s\",\"sn\":\"%s\",\"mac\":\"%s\",\"version\":\"%s\",\"type\":\"%s\",\"product\":\"%s\",\"manufacture\":\"%s\",\"account\":\"%s\",\"passwd\":\"%s\",\"status\":%d,\"tag\":\"%s\",\"rootId\":\"%s\"}"

long long GetTimeTick()
{
	long long tick = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tick = (long long)tv.tv_sec*1000 + (long long)tv.tv_usec/1000;
	return tick;
}

int GetAgentInfoData(char *outData, int buflen, void *pInput, char* parentID, char* rootID )
{
	Handler_info *pSenHubHandler = (Handler_info*)pInput;
	long long ts = GetTimeTick();
	char newID[DEF_DEVID_LENGTH] = {0};
	GetNewID(pSenHubHandler->agentInfo->devId, newID);
	/*"{\"parentID\":\"%s\",\"hostname\":\"%s\",\"sn\":\"%s\",\"mac\":\"%s\",\"version\":\"%s\",\"type\":\"%s\",\"product\":\"%s\",\"manufacture\":\"%s\",\"account\":\"%s\",\"passwd\":\"%s\",\"status\":%d,\"tag\":\"%s\"}"*/
	snprintf(outData, buflen, DEF_SENHUB_AGENTINFO_JSON, parentID,
																					pSenHubHandler->agentInfo->hostname,
																					pSenHubHandler->agentInfo->sn,
																					pSenHubHandler->agentInfo->mac,
																					pSenHubHandler->agentInfo->version,
																					pSenHubHandler->agentInfo->type,
																					pSenHubHandler->agentInfo->product,
																					pSenHubHandler->agentInfo->manufacture,
																					"", //binding account 
																					"", //binding password
																					pSenHubHandler->agentInfo->status,
																					pSenHubHandler->agentInfo->productId,
																					rootID
																					);
	return strlen(outData);

}

int PackSenHubPlugInfo(Handler_info *pSenHubHandler, Handler_info *pGwHandlerInfo, const char *mac, const char *hostname, const char *product, const int status , const char *handleName, const char* parentID)
{
	char newID[DEF_DEVID_LENGTH]={0};

	if( pSenHubHandler == NULL || pGwHandlerInfo == NULL ) return -1;

	if( pSenHubHandler->agentInfo == NULL ) return -1;


	GetNewID(mac, newID);

	memset( pSenHubHandler->agentInfo,0,sizeof(cagent_agent_info_body_t));

	memcpy( pSenHubHandler->agentInfo, pGwHandlerInfo->agentInfo, sizeof(cagent_agent_info_body_t));

	snprintf(pSenHubHandler->Name,MAX_TOPIC_LEN, "%s", handleName); // "general"

	snprintf( pSenHubHandler->agentInfo->hostname, DEF_HOSTNAME_LENGTH, "%s", hostname );
	snprintf( pSenHubHandler->agentInfo->devId, DEF_DEVID_LENGTH, "%s", newID ); // -> NewID
	snprintf( pSenHubHandler->agentInfo->sn, DEF_SN_LENGTH, "%s", mac );         // Origin ID
	snprintf( pSenHubHandler->agentInfo->mac, DEF_MAC_LENGTH, "%s", mac );       
	snprintf( pSenHubHandler->agentInfo->type, DEF_MAX_STRING_LENGTH, "SenHub");
	snprintf( pSenHubHandler->agentInfo->product, DEF_MAX_STRING_LENGTH, "%s",product);
	pSenHubHandler->agentInfo->status = status;

	// FOR ParentID
	snprintf( pSenHubHandler->WorkDir, DEF_MAX_STRING_LENGTH, "%s", parentID);
	//if( pSenHubHandler->agentInfo->parentid )
	//	printf("not found parent id\n");

	return 1;
}


int GetSenHubAgentInfobyUID(const void *pAgentInfoArray, const int Max, const char * uid)
{
	// Critical Section
	int index = -1;
	int i = 0;
	cagent_agent_info_body_t *pAgentInfo = (cagent_agent_info_body_t*)pAgentInfoArray;

	if( pAgentInfoArray == NULL ) return index;

	for( i= 0;  i < Max ; i ++ ) {
		if( pAgentInfo->status == 1 ) {
			if( !memcmp(pAgentInfo->devId, uid, strlen(uid)) )	{			
				index =i;
			    break;
			}
			pAgentInfo++;
		} else
			pAgentInfo++;
	}

	if( index == -1 ) {
		PRINTF("Can't Find =%s AgentInfo in Table\r\n", uid);
	}
	return index;
}

Handler_info* GetSenHubAgentInfobyIDtoHandle( const void *pAgentInfoArray, const int Max, const char * uid)
{
	// Critical Section
	int index = -1;
	int i = 0;
	cagent_agent_info_body_t *pAgentInfo = (cagent_agent_info_body_t*)pAgentInfoArray;

	if( pAgentInfoArray == NULL ) return index;

	for( i= 0;  i < Max ; i ++ ) {
		if( pAgentInfo->status == 1 ) {
			if( !memcmp(pAgentInfo->devId, uid, strlen(uid)) )	{			
				index =i;
			    break;
			}
			pAgentInfo++;
		} else
			pAgentInfo++;
	}

	if( index == -1 ) {
		PRINTF("Can't Find =%s AgentInfo in Table\r\n", uid);
		pAgentInfo = NULL;
	}
	return pAgentInfo;
}

int GetUsableIndex(void *pAgentInfoArray, int Max )
{
	// Critical Section
	int index = -1;
	int i = 0;
	cagent_agent_info_body_t *pAgentInfo = (cagent_agent_info_body_t*)pAgentInfoArray;

	if( pAgentInfoArray == NULL ) return index;

	for( i= 0;  i < Max ; i ++ ) {
		if( pAgentInfo->status == 0 ) {
			pAgentInfo->status = 1;
			index = i;
			break;
		} else
			pAgentInfo++;
	}

	if( index == -1 ) {
		PRINTF("Can't get an empty AgentInfo Table\r\n");
	}

	return index;
}




