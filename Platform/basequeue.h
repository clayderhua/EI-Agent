#ifndef _BASEQUEUE_H_
#define _BASEQUEUE_H_
#include <pthread.h>
#include <stdbool.h>
#include "export.h"

struct queue {
    pthread_mutex_t		lock;
    pthread_cond_t		wait_room;
    pthread_cond_t		wait_data;
    unsigned int		size;
    unsigned int		head;
    unsigned int		tail;
    void				**obj_queue;
};

#ifdef __cplusplus
extern "C" {
#endif

WISEPLATFORM_API bool queue_init(struct queue *const q, const unsigned int slots, const unsigned int datasize);

WISEPLATFORM_API void queue_uninit(struct queue *const q, void(*freefn)(void*));

WISEPLATFORM_API void* queue_get(struct queue *const q);

WISEPLATFORM_API bool queue_put(struct queue *const q, void *const data);

WISEPLATFORM_API bool queue_clear(struct queue *const q, void(*freefn)(void*));

#ifdef __cplusplus
}
#endif

#endif