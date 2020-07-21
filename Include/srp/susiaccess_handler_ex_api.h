#ifndef _CAGENT_HANDLER_EX_H_
#define _CAGENT_HANDLER_EX_H_

#include "susiaccess_handler_api.h"

typedef AGENT_SEND_STATUS (*HandlerConnectServerCbf)(char const * ip, int port, char const * mqttauth, tls_type tlstype, char const * psk);
typedef AGENT_SEND_STATUS (*HandlerDisconnectCbf)();
typedef AGENT_SEND_STATUS (*HandlerRenameCbf)(char const * name);
typedef void (*HandlerAddVirtualHandlerCbf)(char *,char *);
typedef AGENT_SEND_STATUS  (*HandlerUnsubscribeCustCbf) ( char const * const topic); 

typedef struct HANDLER_INFO_EX
{	
	char Name[MAX_TOPIC_LEN];						// The handler name
	char ServerIP[DEF_MAX_STRING_LENGTH];
	int ServerPort;
	char WorkDir[DEF_MAX_STRING_LENGTH];							
	int RequestID;
	int ActionID;
	MSG_FORMAT MSgTransform;						//Transform message to other format, 0: old format, 1:current ei-paas format
	void* loghandle;								// log file handler

	//GetLibFNP  GetLibAPI;							// Get the Function Point of comman library api
	cagent_agent_info_body_t * agentInfo;			// Info of the Agent
	HandlerSendCbf  sendcbf;						// Client Send information (in JSON format) to Cloud Server	
	HandlerSendCustCbf  sendcustcbf;			    // Client Send information (in JSON format) to Cloud Server with custom topic
	HandlerSubscribeCustCbf subscribecustcbf;		// Client subscribe the custom topic to receive message from Cloud Server 
	HandlerSendCapabilityCbf sendcapabilitycbf;			// Client Send Spec.Info (in JSON format) to Cloud Server with SpecInfo topic	
	HandlerAutoReportCbf sendreportcbf;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
	HandlerSendEventCbf sendeventcbf;				// Client Send Event Notify (in JSON format) to Cloud Server with EventNotify topic	
	
	HandlerConnectServerCbf connectservercbf;				// connect to specific server callback function
	HandlerDisconnectCbf disconnectcbf;						// disconnect callback function
	HandlerRenameCbf renamecbf;								// rename callback function
	HandlerAddVirtualHandlerCbf addvirtualhandlercbf;
	HandlerAutoReportCbf internelreportcbf;	

	char serverAuth[DEF_USER_PASS_LENGTH];
	tls_type TLSType;
	char PSK[DEF_USER_PASS_LENGTH];

	HandlerUnsubscribeCustCbf unsubscribecustcbf;		// Client unsubscribe the custom topic to receive message from Cloud Server 
}HANDLER_INFO_EX, Handler_info_ex;

#endif /* _CAGENT_HANDLER_EX_H_ */