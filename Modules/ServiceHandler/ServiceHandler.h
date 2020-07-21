#ifndef _SERVICE_HANDLER_H_
#define _SERVICE_HANDLER_H_

#include "susiaccess_handler_ex_api.h"
#include <string>
#include <stdbool.h>
#include <Log.h>


#define HANDLER_NAME "SERVICE"

#if defined(WIN32) //windows
#define SERVIVE_SDK_LIB_NAME "EIServiceSDK.dll"
#elif defined(__linux)//linux
#define SERVIVE_SDK_LIB_NAME "libEIServiceSDK.so"
#endif

#define TEMP_BUF        1024
#define MAX_SERVICE   64
#define SERVICE_NAME 64
#define MAX_BUF       1024000

#define DEF_SERVICE_NAME                 "Service"
#define DEF_HANDLER_NAME               "SUSIControl"
#define AGENTINFO_BODY_STRUCT	"susiCommData"
#define AGENTINFO_INFOSPEC            "infoSpec"
#define AGENTINFO_TIMESTAMP		"sendTS"
#define AGENTINFO_DATA                     "data"
#define AGENTINFO_TS                           "opTS"
#define AGENT_EVNET                             "eventnotify"
#define AGENTINFO_HANDLERNAME	"handlerName"
#define AGENTINFO_CATALOG		"catalogID"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_AGENTID		"agentID"
#define AGENTINFO_CMDTYPE				"commCmd"
#define AGENTINFO_SESSION_ID         "sessionID"
#define SN_ERROR_REP                           "result"

#define SN_STATUS_CODE                     "StatusCode"
#define SN_RESUTL                                 "Result"

#define SENML_E_FLAG								"e"
#define SENML_N_FLAG								"n"

#define GET_SET_REPLY_FORMAT "{ \"sessionID\":\"%s\", \"sensorInfoList\":%s}"

struct PluginInfo 
{
	HANDLER_INFO plugin;

	int sendMsg(int commcmd, std::string payload)
	{
		return plugin.sendcbf(&plugin, commcmd, payload.c_str(), payload.length(), NULL, NULL);
	}

	int sendCapability(std::string payload)
	{
		return plugin.sendcapabilitycbf(&plugin, payload.c_str(), payload.length(), NULL, NULL);
	}

	int sendReportData(std::string payload)
	{
		return plugin.sendreportcbf(&plugin, payload.c_str(), payload.length(), NULL, NULL);
	}

	int sendEventNotify(HANDLER_NOTIFY_SEVERITY severity, std::string payload)
	{
		return plugin.sendeventcbf(&plugin, severity, payload.c_str(), payload.length(), NULL, NULL);
	}

	int sendCustMsg(int commcmd, std::string topic, std::string payload)
	{
		return plugin.sendcustcbf(&plugin, commcmd, topic.c_str(), payload.c_str(), payload.length(), NULL, NULL);
	}

	int recvCustMsg(std::string topic, HandlerCustMessageRecvCbf func)
	{
		return plugin.subscribecustcbf(topic.c_str(), func);
	}
};

class ServiceHandler
{
public:
	ServiceHandler(HANDLER_INFO* pHandler);
	~ServiceHandler(void);

	void ServiceStart();
	void ServiceStop();
	void ServiceRecv(std::string payload);
private:
	PluginInfo* g_pHandler;

	bool ReplaceDevID(std::string inBuf, std::string outBuf);
};

#endif //_SERVICE_HANDLER_H_
