#include <stdio.h>
#include <stdlib.h>
#include "AdvCC.h"
#include "jsontype.h"
#include "jsoncollection.h"


JSONCollect *gc = NULL;

static void ReleaseNodeMem(JSONode *node) {
	if (node->alloc != 0) free(node->s);
	node->alloc = 0;
	free(node);
}

JSONode *ClearItem(JSONode *node) {
	if (node == NULL) return NULL;
	/*if(node->alloc != 0) {
		node->s[0] = 0;
	}*/
	node->type = JSON_TYPE_INVAILD;
	node->len = 0;
	node->array = NULL;
	node->next = NULL;
	node->key = NULL;
	node->value = NULL;
	node->root = NULL;
	node->prev = NULL;
	node->manager = NULL;
	return node;
}


void GC_Init() {
	if(gc == NULL) gc = NewCollection();
}

void GC_Release() {
	if(gc != NULL) ReleaseCollection(&gc);
}

JSONode *GC_NewItem() {
	JSONode *node = NULL;
	if (gc != NULL) node = GetNewItem(gc);
	return node;
}

void GC_DestroyItem(JSONode *node) {
	if(node == NULL) return;
	if (node->collection == NULL) {
		ReleaseNodeMem(node);
	}
	else {
		DestroyItem((JSONCollect*)node->collection, node);
	}
	return;
}

JSONCollect *NewCollection() {
	JSONCollect *c = (JSONCollect *)malloc(sizeof(JSONCollect));
	c->queue = NULL;
	Queue_Init((QueueHandler *)&c->queue);
	c->tail = NULL;
	c->count = 0;

	c->spare = (JSONCollect *)malloc(sizeof(JSONCollect));
	c->spare->queue = NULL;
	Queue_Init((QueueHandler *)&c->spare->queue);
	c->spare->tail = NULL;
	c->spare->count = 0;
	return c;
}


JSONode *GetNewItem(JSONCollect* collect) {
	JSONTag tag;
	JSONode *node = NULL;
	QueueHandler qh = (QueueHandler)collect->queue;
	JSONCollect *cspare = collect->spare;
	QueueHandler cspare_qh = (QueueHandler)cspare->queue;
	if (cspare != NULL) {
		if (Queue_GetAmount(cspare_qh) == 0) {
			node = (JSONode *)malloc(sizeof(JSONode));
			node->s = NULL;
			node->alloc = 0;
			ClearItem(node);
			node->collection = collect;
			tag.n = node;
			node->manager = qh;
			Queue_AddWithKey(qh, &tag, sizeof(JSONTag), (unsigned long)node);
		}
		else {
			Queue_Get(cspare_qh, &tag);
			node = tag.n;
			ClearItem(node);
			node->collection = collect;
			node->manager = qh;
			Queue_AddWithKey(qh, &tag, sizeof(JSONTag), (unsigned long)node);
		}
	}
	else {
		node = (JSONode *)malloc(sizeof(JSONode));
		node->s = NULL;
		node->alloc = 0;
		ClearItem(node);
		node->collection = collect;
		tag.n = node;
		node->manager = qh;
		Queue_AddWithKey(qh, &tag, sizeof(JSONTag), (unsigned long)node);
	}

	return node;
}

void DestroyItem(JSONCollect* collect, JSONode *node) {
	if(node == NULL || collect == NULL) return;
	JSONTag tag;
	QueueHandler qh = (QueueHandler)collect->queue;
	JSONCollect *cspare = collect->spare;
	QueueHandler cspare_qh = (QueueHandler)cspare->queue;
	if (Queue_GetWithKey(qh, &tag, (unsigned long)node) == CON_RESULT_NORMAL) {
		tag.n->collection = cspare;
		tag.n->manager = cspare_qh;
		Queue_AddWithKey(cspare_qh, &tag, sizeof(JSONTag), (unsigned long)tag.n);
	}
	else {
		ReleaseNodeMem(node);
	}
}

void ReleaseCollection(JSONCollect** pcollect) {
	JSONCollect* collect = *pcollect;
	JSONTag tag;
	QueueHandler qh = (QueueHandler)collect->queue;
	JSONCollect *cspare = collect->spare;
	QueueHandler cspare_qh = (QueueHandler)cspare->queue;

	if (cspare != NULL) {
		while (Queue_Get(cspare_qh, &tag) == CON_RESULT_NORMAL) {
			ReleaseNodeMem(tag.n);
		}
		Queue_Destroy(&cspare_qh);
		free(cspare);
	}

	while (Queue_Get(qh, &tag) == CON_RESULT_NORMAL) {
		ReleaseNodeMem(tag.n);
	}
	Queue_Destroy(&qh);
	free(collect);
	*pcollect = NULL;
}

void LinkItem(JSONCollect* collect, JSONode *node) {
	JSONTag tag;
	QueueHandler qh = (QueueHandler)collect->queue;
	if (node == NULL || collect == NULL) return;
	node->collection = collect;
	node->manager = qh;
	tag.n = node;
	Queue_AddWithKey(qh, &tag, sizeof(JSONTag), (unsigned long)tag.n);
	return;
}

void UnlinkItem(JSONCollect* collect, JSONode *node) {
	JSONTag tag;
	QueueHandler qh = (QueueHandler)collect->queue;
	if(node == NULL || collect == NULL) return;
	if (Queue_GetWithKey(qh, &tag, (unsigned long)node) == CON_RESULT_NORMAL) {
		node->collection = NULL;
		node->manager = qh;
	}
	return;
}
