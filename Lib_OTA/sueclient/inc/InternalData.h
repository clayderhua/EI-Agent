#ifndef _SUE_INTERNAL_DATA_H_
#define _SUE_INTERNAL_DATA_H_

#include <pthread.h>

#include "QueueHelper.h"
#include "SUESchedule.h"
#include "SUEClient.h"

typedef struct {
	char * interfacePath;// /OTAService
	char * connection;
	char * ip;
	int port;
	char * accept;
	char * secWebsocketKey;//websocket
	int secWebSocketVersion;
	char * upgrade;
	char * secWebSocketAccept;
}WSParams, *PWSParams;

typedef struct SUECContext{
	int isInited;
	int isStarted;
	char * devID;
	SUECOutputMsgCB outputMsgCB;
	void * opMsgCBUserData;
	SUECTaskStatusCB taskStatusCB;
	void * tsCBUserData;
	OutputActType opActType;
	pthread_mutex_t dataMutex;

	pthread_mutex_t opMsgQueueMutex;
	PQHQueue opMsgQueue;

	int isOutputMsgThreadRunning;
	pthread_t outputMsgThreadT;

	char * dlReqPkgName;
	int dlReqRet;
	PDLTaskInfo repDLTaskInfo;

   SUEScheHandle sueScheHandle;

	char * osName;
	char * arch;
	char * tags;

	char * wsUrl;
	WSParams wsParams;

	pthread_mutex_t SubDevMutex;
	int isMonitorSubDevThreadRunning;
	pthread_t MonitorSubDevThreadT;
}SUECContext, *PSUECContext;

#define SUEC_CONTEXT_INITIALIZE   {0, 0, NULL, NULL, \
	NULL, NULL, NULL, OPAT_CONTINUE, \
	0, 0, NULL, 0, \
	0, NULL, 0, NULL, \
	NULL, NULL, NULL, NULL, {NULL, 0}, 0, 0, 0}
#endif 
