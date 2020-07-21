#ifndef _GENMESSAGE_H_
#define _GENMESSAGE_H_

#include "susiaccess_handler_api.h"
#include "MsgGenerator.h"
#include "DiskPMQInfo.h"

typedef enum
{
	GenSuccess = 0,
	GenFail,
	GenParmInsufficient,
} GenStatus_t;

GenStatus_t PMQMsgSetParam(MSG_CLASSIFY_T *msg, pParamInfo info);
GenStatus_t PMQMsgSetAction(MSG_CLASSIFY_T *msg, pActionInfo info);
GenStatus_t PMQMsgSetEvent(MSG_CLASSIFY_T *msg, pEventInfo info);
GenStatus_t PMQMsgSetPMQInfo(MSG_CLASSIFY_T *msg, pDiskPMQ diskpmq);
GenStatus_t PMQMsgSetInfo(MSG_CLASSIFY_T *msg, pInfoConfig info);
GenStatus_t UpdatePMQMsg(MSG_CLASSIFY_T *msg, pDiskPMQ diskpmq, pHandlerConfig pmqConfig);
GenStatus_t GenEventStrbyByteCode(char *msgStr, char *idStr, unsigned char byteCode);
char *CreateEventMsg(pDiskPMQ diskpmq);
MSG_CLASSIFY_T* CreatePMQMsg();

#endif

