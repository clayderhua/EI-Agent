#ifndef _CAGENT_TOPIC_H_
#define _CAGENT_TOPIC_H_
#include "srp/susiaccess_def.h"

typedef int (*TOPIC_MESSAGE_CB)(char* topic, susiaccess_packet_body_t *pkt, void *pRev1, void* pRev2);

typedef struct saclient_topic_entry
{    
	char name[128];
	TOPIC_MESSAGE_CB callback_func;
	struct saclient_topic_entry *prev;
	struct saclient_topic_entry *next;
} saclient_topic_entry_st;

struct saclient_topic_entry * saclient_topic_first();
struct saclient_topic_entry * saclient_topic_last();
struct saclient_topic_entry * saclient_topic_add(char const * topicname, TOPIC_MESSAGE_CB cbfunc);
void saclient_topic_remove(char* topicname);
struct saclient_topic_entry * saclient_topic_find(char const * topicname);

#endif