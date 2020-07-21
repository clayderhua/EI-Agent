#ifndef __CONCURRENT_CONTAINER_H__
#define __CONCURRENT_CONTAINER_H__

#if defined(WIN32)
#pragma once
#ifdef ADVCC_EXPORTS
#define ADVCC_CALL __stdcall
#define ADVCC_EXPORT __declspec(dllexport)
#else
#define ADVCC_CALL __stdcall
#define ADVCC_EXPORT
#endif
#define __func__ __FUNCTION__
#else
#define ADVCC_CALL
#define ADVCC_EXPORT
#endif

enum {
    CON_RESULT_NORMAL = 0,
    CON_RESULT_EMPTY,
    CON_RESULT_NO_HANDLER,
    CON_RESULT_ERROR_SOURCE,
    CON_RESULT_ERROR_DESTINATION,
};

typedef struct SeqInfo SeqInfo;
typedef struct SeqElement SeqElement;
typedef struct SeqHandle SeqHandle;
typedef SeqHandle* SeqHandler;

#ifdef __cplusplus
extern "C" {
#endif
ADVCC_EXPORT void ADVCC_CALL Seq_Init(SeqHandler *handler);

ADVCC_EXPORT void ADVCC_CALL Seq_InitBlocking(SeqHandler *handler, int maxItems);

ADVCC_EXPORT void ADVCC_CALL Seq_Destroy(SeqHandler *handler);

ADVCC_EXPORT void ADVCC_CALL Seq_ReQueueToHead(SeqHandler handler, SeqElement *seq);
ADVCC_EXPORT void ADVCC_CALL Seq_ReQueueToTail(SeqHandler handler, SeqElement *seq);

ADVCC_EXPORT int ADVCC_CALL Seq_PeekSeqElement(SeqHandler handler, SeqElement *seq, void *data);
ADVCC_EXPORT int ADVCC_CALL Seq_DelSeqElement(SeqHandler handler, SeqElement *seq);
ADVCC_EXPORT int ADVCC_CALL Seq_DelSeqElementBlocking(SeqHandler handler, SeqElement *seq);


ADVCC_EXPORT int ADVCC_CALL Seq_GetAmount(SeqHandler handler);
ADVCC_EXPORT int ADVCC_CALL Seq_GetAmountWithKey(SeqHandler handler, unsigned long key);

ADVCC_EXPORT int ADVCC_CALL Seq_PeekWithKey(SeqHandler handler, void *data, unsigned long key);
ADVCC_EXPORT int ADVCC_CALL Seq_PeekHead(SeqHandler handler, void *data);
ADVCC_EXPORT int ADVCC_CALL Seq_PeekTail(SeqHandler handler, void *data);

ADVCC_EXPORT int ADVCC_CALL Seq_PushHead(SeqHandler handler, void *data, int dataSize);
ADVCC_EXPORT int ADVCC_CALL Seq_PopHead(SeqHandler handler, void *data);
ADVCC_EXPORT int ADVCC_CALL Seq_PushTail(SeqHandler handler, void *data, int dataSize);
ADVCC_EXPORT int ADVCC_CALL Seq_PopTail(SeqHandler handler, void *data);

ADVCC_EXPORT int ADVCC_CALL Seq_PushHeadWithKey(SeqHandler handler, void *data, int dataSize, unsigned long key);
ADVCC_EXPORT int ADVCC_CALL Seq_PushTailWithKey(SeqHandler handler, void *data, int dataSize, unsigned long key);
ADVCC_EXPORT int ADVCC_CALL Seq_PopWithKey(SeqHandler handler, void *data, unsigned long key);

ADVCC_EXPORT int ADVCC_CALL Seq_PushHeadBlocking(SeqHandler handler, void *data, int dataSize);
ADVCC_EXPORT int ADVCC_CALL Seq_PopHeadBlocking(SeqHandler handler, void *data);
ADVCC_EXPORT int ADVCC_CALL Seq_PushTailBlocking(SeqHandler handler, void *data, int dataSize);
ADVCC_EXPORT int ADVCC_CALL Seq_PopTailBlocking(SeqHandler handler, void *data);

ADVCC_EXPORT int ADVCC_CALL Seq_PushHeadWithKeyBlocking(SeqHandler handler, void *data, int dataSize, unsigned long key);
ADVCC_EXPORT int ADVCC_CALL Seq_PushTailWithKeyBlocking(SeqHandler handler, void *data, int dataSize, unsigned long key);
ADVCC_EXPORT int ADVCC_CALL Seq_PopWithKeyBlocking(SeqHandler handler, void *data, unsigned long key);

ADVCC_EXPORT void ADVCC_CALL Seq_Flush(SeqHandler handler);
ADVCC_EXPORT int ADVCC_CALL Seq_WaitForEmptyBlocking(SeqHandler handler);
ADVCC_EXPORT void ADVCC_CALL Seq_PrintAll(SeqHandler handler);

#ifdef ANDROID
#define Seq_Init(handler) do {\
} while (0)
#define Seq_PushHead(handler, data, dataSize) ({int p = 0; p;})
#define Seq_GetAmount(handler) ({int p = 0; p;})
#define Seq_PopTail(handler, data) ({int p = 0; p;})
#define Seq_PushTail(handler, data, dataSize) ({int p = 0; p;})
#endif

#ifdef __cplusplus
}
#endif

/************************************************************************/
//Queue
typedef SeqHandle* QueueHandler;

#define Queue_Init(handler)                         Seq_Init(handler)
#define Queue_Destroy(handler)                      Seq_Destroy(handler)


#define Queue_GetAmount(handler)						Seq_GetAmount(handler)

#define Queue_Redo(handler,data,dataSize)				Seq_PushTail(handler,data,dataSize)
#define Queue_Add(handler,data,dataSize)				Seq_PushHead(handler,data,dataSize)
#define Queue_Get(handler,data)							Seq_PopTail(handler,data)
#define Queue_Peek(handler,data)						Seq_PeekTail(handler,data)

#define Queue_AddWithKey(handler,data,dataSize,key)     Seq_PushHeadWithKey(handler,data,dataSize,key)
#define Queue_GetWithKey(handler,data,key)              Seq_PopWithKey(handler,data,key)
#define Queue_PeekWithKey(handler,data,key)				Seq_PeekWithKey(handler,data,key)

#define Queue_Flush(handler)							Seq_Flush(handler)
/************************************************************************/


/************************************************************************/
//Stack
typedef SeqHandle* StackHandler;

#define Stack_Init(handler)                         Seq_Init(handler);
#define Stack_Destroy(handler)                      Seq_Destroy(handler);

#define Stack_GetAmount(handler)                    Seq_GetAmount(handler)

#define Stack_Push(handler,data,dataSize)           Seq_PushHead(handler,data,dataSize)
#define Stack_Pop(handler,data)                     Seq_PopHead(handler,data)
#define Stack_Flush(handler)                        Seq_Flush(handler)
/************************************************************************/

#if defined  __linux__
/************************************************************************/
//BlockQueue
typedef SeqHandle* BlockQHandler;

#define BlockQ_Init(handler, maxItems)               Seq_InitBlocking(handler, maxItems)
#define BlockQ_Destroy(handler)                      Seq_Destroy(handler)

#define BlockQ_PeekHead(handler)                     Seq_PeekHead(handler)
#define BlockQ_PeekTail(handler)                     Seq_PeekTail(handler)
#define BlockQ_ReQueue(handler, seq)                 Seq_ReQueueToHead(handler, seq)


#define BlockQ_GetAmount(handler)                    Seq_GetAmount(handler)

#define BlockQ_FindEqual(handler, data)              Seq_FindEqual(handler, data)
#define BlockQ_PeekSeqElement(handler, seq, data)    Seq_PeekSeqElement(handler, seq, data)
#define BlockQ_DelSeqElementBlocking(handler, seq)   Seq_DelSeqElementBlocking(handler, seq)


#define BlockQ_Add(handler,data,dataSize)            Seq_PushHeadBlocking(handler,data,dataSize)
#define BlockQ_Get(handler,data)                     Seq_PopTailBlocking(handler,data)
#define BlockQ_Flush(handler)                        Seq_Flush(handler)
#define BlockQ_WaitForEmpty(handler)                 Seq_WaitForEmptyBlocking(handler)
/************************************************************************/
#endif

#endif //__CONCURRENT_CONTAINER_H__
