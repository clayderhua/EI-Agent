#include "actionqueue.h"
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include "basequeue.h"

struct triggerdata {
	bool isNormal;
	char sensorname[256];
	double value;
	MSG_ATTRIBUTE_T* attr;
	void *pRev;
};

struct trigger_ctx{
   void*				threadHandler;
   bool					isThreadRunning;
   struct triggerdata*	triggerqueue;
   ACTION_ON_TRIGGER	on_triggered_cb;
};

bool action_create(struct triggerdata *const trigger, bool isNormal, char* sensorname, double value, MSG_ATTRIBUTE_T* attr, void *pRev)
{
	if(!trigger)
		return false;

	trigger->isNormal = isNormal;
	strcpy(trigger->sensorname, sensorname);
	trigger->value = value;
	trigger->attr = attr;
	trigger->pRev = pRev;

	return true;
}

void action_free(struct triggerdata *const trigger)
{
	if(!trigger)
		return;

	free(trigger);
}

void* threat_action_queue(void* args)
{
	struct trigger_ctx *precvContex = NULL;
	unsigned long interval = 100*1000; //microsecond.

	if(args == NULL)
		goto TRIGGER_EXIT;
	precvContex = (struct trigger_ctx *)args;
	while(precvContex->isThreadRunning)
	{
		struct triggerdata *trigger = NULL;
		usleep(interval);
		trigger = (struct triggerdata *)queue_get((struct queue *const)precvContex->triggerqueue);
		if(!trigger)
			continue;
		if(precvContex->on_triggered_cb!= NULL)
		{
			precvContex->on_triggered_cb(trigger->isNormal, trigger->sensorname, trigger->value, trigger->attr, trigger->pRev);
		}
		action_free(trigger);
		trigger = NULL;
	}
TRIGGER_EXIT:
	pthread_exit(0);
	return 0;
}

void* actionqueue_init(const unsigned int slots)
{
	struct trigger_ctx* triggerthreadctx =  malloc(sizeof( struct trigger_ctx));
	memset(triggerthreadctx, 0, sizeof(struct trigger_ctx));

	triggerthreadctx->triggerqueue = malloc(sizeof( struct queue));

	if(triggerthreadctx->triggerqueue)
	{
		if(queue_init((struct queue *const)triggerthreadctx->triggerqueue, slots, sizeof(struct triggerdata)))
		{
			triggerthreadctx->isThreadRunning = true;
			if (pthread_create(&triggerthreadctx->threadHandler, NULL, threat_action_queue, triggerthreadctx) != 0)
			{
				triggerthreadctx->isThreadRunning = false;
				queue_uninit((struct queue *const)triggerthreadctx->triggerqueue, action_free);
				free(triggerthreadctx->triggerqueue);
				triggerthreadctx->triggerqueue = NULL;
			}
			else
			{
				//pthread_detach(triggerthreadctx->threadHandler);
				//triggerthreadctx->threadHandler = 0;
				return triggerthreadctx;
			}
		}
		/*Failed to create trigger queue*/
		free(triggerthreadctx->triggerqueue);
		triggerthreadctx->triggerqueue = NULL;			
	}
	free(triggerthreadctx);
	triggerthreadctx = NULL;		
	return triggerthreadctx;
}

void actionqueue_uninit(void* qtrigger)
{
	struct trigger_ctx* triggerthreadctx = NULL;
	if(qtrigger == NULL)
		return;
	triggerthreadctx = qtrigger;
	triggerthreadctx->on_triggered_cb = NULL;
	
	if(triggerthreadctx->isThreadRunning == true)
	{
		triggerthreadctx->isThreadRunning = false;
		pthread_join(triggerthreadctx->threadHandler, NULL);
		triggerthreadctx->threadHandler = 0;
	}

	if(triggerthreadctx->triggerqueue)
	{
		queue_uninit((struct queue *const)triggerthreadctx->triggerqueue, action_free);
		free(triggerthreadctx->triggerqueue);
		triggerthreadctx->triggerqueue = NULL;
	}
	free(triggerthreadctx);
}

bool actionqueue_setcb(void* qtrigger, ACTION_ON_TRIGGER func)
{
	struct trigger_ctx* triggerthreadctx = NULL;
	if(qtrigger == NULL)
		return false;
	triggerthreadctx = qtrigger;
	triggerthreadctx->on_triggered_cb = func;
	return true;
}

bool actionqueue_push(void* qtrigger, struct thr_item_info_t* item, MSG_ATTRIBUTE_T* attr)
{
	struct trigger_ctx* triggerthreadctx = NULL;
	if(qtrigger == NULL)
		return false;
	triggerthreadctx = qtrigger;

	if(item == NULL) return false;
	if(triggerthreadctx->triggerqueue)
	{
		struct triggerdata *newtrigger = malloc(sizeof(struct triggerdata));
		action_create(newtrigger, item->isNormal, item->pathname, item->checkRetValue, attr, item);
		if(!queue_put((struct queue *const)triggerthreadctx->triggerqueue, newtrigger))
		{
			action_free(newtrigger);
			newtrigger = NULL;
		}
		else
			return true;
	}
	return false;
}

void actionqueue_clear(void* qtrigger)
{
	struct trigger_ctx* triggerthreadctx = NULL;
	if(qtrigger == NULL)
		return;
	triggerthreadctx = qtrigger;

	if(triggerthreadctx->triggerqueue)
	{
		queue_clear((struct queue *const)triggerthreadctx->triggerqueue, action_free);
	}
}