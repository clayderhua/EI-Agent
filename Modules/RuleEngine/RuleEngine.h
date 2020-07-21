#ifndef _RUEL_ENGINE_H_
#define _RUEL_ENGINE_H_
#pragma once
#include "srp/susiaccess_handler_mgmt.h"

//Define Function Flag:
#define NoneCode				0x00
#define ThresholdCode			0x01
#define ActionCode				0x02
#define EventCode				0x04
#define EIServiceCode			0x08
#define WAPICode				0x10

#define NoneStr					"none"
#define ThresholdStr			"rulecheck"
#define EventStr				"eventnotify"
#define ActionStr				"actiontrigger"
#define EIServiceStr			"eiservice"
#define WAPIStr					"wapi"

#ifdef __cplusplus
extern "C" {
#endif

void HANDLER_API Handler_RecvCapability(HANDLE const handler, void const * const requestData, unsigned int const requestLen);

void HANDLER_API Handler_RecvData(HANDLE const handler, void const * const requestData, unsigned int const requestLen);

#ifdef __cplusplus
}
#endif

#endif
