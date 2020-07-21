#ifndef _ACTION_TRIGGER_H_
#define _ACTION_TRIGGER_H_
#pragma once
#include "stdbool.h"
#include "srp/susiaccess_handler_mgmt.h"
#include "IoTMessageGenerate.h"
#include "cJSON.h"
#include "HandlerThreshold.h"

#ifdef __cplusplus
extern "C" {
#endif

void Action_On_Threshold_Triggered(HANDLE const handler, bool isNormal, char* sensorname, double value, MSG_ATTRIBUTE_T* attr, void *pRev);

void Action_Set_Action_Capability(char* devID, char* name, MSG_CLASSIFY_T *pCapability, MSG_CLASSIFY_T** pAction);

bool Action_Trigger_Command(char* name, char* sensor, thr_action_t* pAction, char* command);

#ifdef __cplusplus
}
#endif

#endif
