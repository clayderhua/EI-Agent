#include "topic.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "WISEPlatform.h"  //for strtok_r wrapping

static struct saclient_topic_entry *g_subscribe_topics = NULL;
struct saclient_topic_entry * saclient_topic_first()
{
	struct saclient_topic_entry *target = g_subscribe_topics;
	return target;
}

struct saclient_topic_entry * saclient_topic_last()
{
	struct saclient_topic_entry *topic = g_subscribe_topics;
	struct saclient_topic_entry *target = NULL;
	while(topic != NULL)
	{
		target = topic;
		topic = topic->next;
	}
	return target;
}

struct saclient_topic_entry * saclient_topic_add(char const * topicname, TOPIC_MESSAGE_CB cbfunc)
{
	struct saclient_topic_entry *topic = NULL;
	
	topic = (struct saclient_topic_entry *)malloc(sizeof(*topic));
	
	if (topic == NULL)
		return NULL;

	strncpy(topic->name, topicname, strlen(topicname)+1);
	topic->callback_func = cbfunc;
	topic->next = NULL;	
	topic->prev = NULL;	

	if(g_subscribe_topics == NULL)
	{
		g_subscribe_topics = topic;
	} else {
		struct saclient_topic_entry *lasttopic = saclient_topic_last();
		lasttopic->next = topic;
		topic->prev = lasttopic;
	}
	return topic;
}

void saclient_topic_remove(char* topicname)
{
	struct saclient_topic_entry *topic = g_subscribe_topics;
	struct saclient_topic_entry *target = NULL;
	while(topic != NULL)
	{
		if(strcmp(topic->name, topicname) == 0)
		{
			if(g_subscribe_topics == topic)
				g_subscribe_topics = topic->next;
			if(topic->prev != NULL)
				topic->prev->next = topic->next;
			if(topic->next != NULL)
				topic->next->prev = topic->prev;
			target = topic;
			break;
		}
		topic = topic->next;
	}
	if(target!=NULL)
	{
		free(target);
		target = NULL;
	}
}

struct saclient_topic_entry * saclient_topic_find(char const * topicname)
{
	struct saclient_topic_entry *topic = g_subscribe_topics;
	struct saclient_topic_entry *target = NULL;

	while(topic != NULL)
	{
		if(strchr(topic->name, '+')>0)
		{
			if(strchr(topicname, '+')>0)
			{
				if(strcmp(topic->name, topicname) == 0)
				{
					target = topic;
					break;
				}
			}
			else
			{
				char *delim = "+";
				char *p = NULL;
				char *token = NULL;
				char tName[128] = {0};
				bool match = true;
				char* ss = (char *)topicname;
				strncpy(tName, topic->name, sizeof(tName));
				p = strtok_r(tName, delim, &token);
				while(p)
				{
					ss = strstr(ss, p);
					if(ss > 0)
						ss += strlen(p);
					else
					{
						match = false;
						break;
					}
					p=strtok_r(NULL,delim, &token);
				}
				if(match)
					target = topic;
			}
		}
		else
		{
			if(strcmp(topic->name, topicname) == 0)
			{
				target = topic;
				break;
			}
		}
		topic = topic->next;
	}
	return target;
}