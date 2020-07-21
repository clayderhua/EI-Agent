#ifndef _THRESHOLD_CHECK_H_
#define _THRESHOLD_CHECK_H_

#include <stdbool.h>
#include "HandlerThreshold.h"
#ifdef __cplusplus
extern "C" {
#endif

void ThresholdCheck_Initialize(void (*on_triggered)(bool isnormal,
													char* sensorname, double value, MSG_ATTRIBUTE_T* attr,
													void *pRev));
void ThresholdCheck_Uninitialize();

bool ThresholdCheck_SetThreshold(thr_item_list pThresholdList, char *pInQuery, char** result, char** warnmsg);
void ThresholdCheck_WhenDelThrCheckNormal(thr_item_list thrItemList, char** checkMsg, unsigned int bufLen);
bool ThresholdCheck_DeleteAllThrNode(thr_item_list thrList);
bool ThresholdCheck_UpdateValue(thr_item_list pThresholdList, char *pInQuery, char* devID);
bool ThresholdCheck_CheckThr(thr_item_list curThrItemList, char ** checkRetMsg, unsigned int bufLen, bool * isNormal);
bool ThresholdCheck_QueueingAction(thr_item_list curThrItemList);

#ifdef __cplusplus
}
#endif
#endif //_THRESHOLD_CHECK_H_