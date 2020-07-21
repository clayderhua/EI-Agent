#ifndef _DEVICE_MESSAGE_GENERATE_H_
#define _DEVICE_MESSAGE_GENERATE_H_
#include "MsgGenerator.h"
#include <stdbool.h>
#include "srp/susiaccess_def.h"

#ifdef __cplusplus
extern "C" {
#endif

	long long DEV_GetTimeTick();

	bool DEV_CreateAgentInfo(susiaccess_agent_profile_body_t const * pProfile, int status, long long tick, char* strInfo, int length);

	bool DEV_CreateWillMessage(susiaccess_agent_profile_body_t const * pProfile, long long tick, char* strInfo, int length);

	bool DEV_CreateOSInfo(susiaccess_agent_profile_body_t* profile, long long tick, char* strInfo, int length);

	char*  DEV_CreateHandlerList(char* devID, char** handldelist, int count);

	char* DEV_CreateEventNotify(char* subtype, char* message, char * extMsg);

	char* DEV_CreateEventNotifyTs(char* subtype, char* message, char * extMsg, unsigned long long timestamp);

	char* DEV_CreateFullEventNotify(char* devID, int severity, char* handler, char* subtype, char* message, char * extMsg);

	char* DEV_CreateFullEventNotifyTs(char* devID, int severity, char* handler, char* subtype, char* message, char * extMsg, unsigned long long timestamp);

	char* DEV_CreateFullMessage(char* devID, char* handler, int cmdId, char* msg);

	bool DEV_GetAgentInfoTopic(char* devID, char * topic, int length);

	bool DEV_GetWillMessageTopic(char* devID, char * topic, int length);

	bool DEV_GetActionReqTopic(char* devID, char* productTag, char * topic, int length);

	bool DEV_GetEventNotifyTopic(char* devID, char* productTag, char * topic, int length);

#define DEV_GetOSInfoTopic(devID, productTag, topic, length) DEV_GetActionReqTopic(devID, productTag, topic, length)

#define DEV_GetHandlerListTopic(devID, productTag, topic, length) DEV_GetActionReqTopic(devID, productTag, topic, length)

	void DEV_ReleaseBuffer(char* buff);

	char* DEV_PrintPacket(susiaccess_packet_body_t const * pPacket);
	bool DEV_ParseMessage(const void* data, int datalen, susiaccess_packet_body_t * pkt);

#ifdef __cplusplus
}
#endif
#endif