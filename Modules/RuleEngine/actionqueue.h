#ifndef _ACTIONQUEUE_H_
#define _ACTIONQUEUE_H_
#include <stdbool.h>
#include "HandlerThreshold.h"
#include "IoTMessageGenerate.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ACTION_ON_TRIGGER)(bool isNormal, char* sensorname, double value, MSG_ATTRIBUTE_T* attr, void *pRev);

void* actionqueue_init(const unsigned int slots);

void actionqueue_uninit(void* qAct);

bool actionqueue_setcb(void* qAct, ACTION_ON_TRIGGER func);

bool actionqueue_push(void* qAct, struct thr_item_info_t* item, MSG_ATTRIBUTE_T* attr);

void actionqueue_clear(void* qAct);

#ifdef __cplusplus
}
#endif

#endif