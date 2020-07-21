/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2018/09/27 by Fred Chang								    */
/* Modified Date: 2018/09/27 by Fred Chang									*/
/* Abstract     : External Translator for Azure								*/
/* Reference    : None														*/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "WISEPlatform.h"
#include "mosquitto.h"
#include "ExternalTranslator.h"
#include "CompressionTunnel.h"

enum {
	RMM = 0,
	AZURE_PAAS = 1,
};

static int solution = RMM;
static int second = 0;
static char method[16] = "none";

void ET_AssignSolution(char *soln) {
	char *start = NULL;
	char *end = NULL;
	char temp[16] = { 0 };
	if (soln == NULL) return;
	if (strncmp(soln, "Azure-PaaS=", strlen("Azure-PaaS=")) == 0) {
		solution = AZURE_PAAS;
	}
	start = strchr(soln,',');
	if (start != NULL) {
		start++;
		end = strstr(start,":s,");
		if (end != NULL) {
			memcpy(temp, start, end - start);
			second = atoi(temp);
			start = strchr(end, ',');
			if (start == NULL) return;
			start++;
		}
		end = strstr(start, ":m,");
		if (end != NULL) {
			memcpy(temp, start, end - start);
			snprintf(method,sizeof(method),"%s",temp);
		}
	}

}

//*************************Before publishing*************************************
static void ToAzureTopic(char *orig, char *dest) {
	int len = strlen(orig);
	int i = 0;
	for (i = 0; i < len; i++) {
		if (orig[i] == '/') dest[i] = '[';
		else dest[i] = orig[i];
	}
}

#define AZURE_IOT_HUB_PREFIX "devices/%s/messages/events/["
char *ET_PreTopicTranslate(const char* topic, const char *ref, char *buffer, int *len) {
	if (buffer == NULL && len == NULL) return topic;
	switch (solution) {
		case AZURE_PAAS:
		{
			snprintf(buffer, *len, AZURE_IOT_HUB_PREFIX, ref);
			ToAzureTopic(topic, buffer + strlen(buffer));
			*len = strlen(buffer);
			buffer[*len] = ']';
			return buffer;
		}
		default:
			return topic;
	}
}

int ET_getLength(unsigned char *buffer) {
	int len = 0;
	if (buffer[0] == 16 && buffer[1] == 99) {
		len = buffer[5] - 64;
		len <<= 4;
		len += buffer[6] - 64;
		len <<= 4;
		len += buffer[7] - 64;
		len <<= 4;
		len += buffer[8] - 64;
		len <<= 4;
		len += buffer[9] - 160;
	}
	return len;
}

char *ET_PreMessageTranslate(const char* message, const char *ref, char *buffer, int *len) {
	if (buffer == NULL && len == NULL) return message;
	switch (solution) {
		case AZURE_PAAS:
		{
			return message;
			/*unsigned int maxDst = 0;
			unsigned char *temp = NULL;
			int srcLen = strlen(message);
			maxDst = GetMaxCompressedLen(srcLen);
			temp = (unsigned char *)malloc(maxDst+20);
			temp[0] = 16;
			temp[1] = 99;
			sprintf(temp + 2, "zip");
			maxDst = CompressData(message, srcLen, temp+10, maxDst);
			temp[5] = 64 + ((maxDst >> 16) & 0xF);
			temp[6] = 64 + ((maxDst >> 12) & 0xF);
			temp[7] = 64 + ((maxDst >> 8) & 0xF);
			temp[8] = 64 + ((maxDst >> 4) & 0xF);
			temp[9] = 160 + (maxDst & 0xF);
			maxDst += 10;
			temp[maxDst] = 0;
			maxDst += 1;
			if (maxDst <= *len) {
				*len = maxDst;
				memcpy(message, temp, maxDst);
			}
			free(temp);
			return message;*/
		}
		default:
			return message;
	}
}



int ET_QueuedPublish(void *mosq, time_t *time, void **queue, const char* devid, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain) {
	char *result = NULL;
	int resultLen = 0;
	char compression_topic[256] = { 0 };
	char replacedtopic[256] = { 0 };
	int realTopicLen = 0;
	char *realTopic = NULL;
	switch (solution) {
	case AZURE_PAAS:
		{
			if (strncmp(method, "none", 4) == 0 && second == 0) {
				return mosquitto_publish(mosq, mid, topic, payloadlen, payload, qos, retain);
			} else {
				result = CT_QueueToPayload(queue, time, topic, payload, payloadlen, &resultLen, second, method);
				if (result) {
					// bulk out
					sprintf(compression_topic, "/wisepaas/RMM/%s/compression-tunnel", devid);
					realTopicLen = sizeof(replacedtopic);
					realTopic = ET_PreTopicTranslate(compression_topic, devid, replacedtopic, &realTopicLen);
					return mosquitto_publish(mosq, mid, realTopic, resultLen, result, qos, retain);
				}
				else {
					*mid = -1;
				}
				return MOSQ_ERR_SUCCESS;
			}
		}
	default:
		return mosquitto_publish(mosq, mid, topic, payloadlen, payload, qos, retain);
	}
}

//*************************************************************************************
#define AZURE_IOT_HUB_SUB_PREFIX "devices/%s/messages/devicebound/#"
char *ET_SubscibeTopicTranslate(const char* topic, char *buffer, int *len) {
	char *str = NULL;
	char *delim = "/";
	char *saveptr = NULL;
	char *pch = NULL;
	if (*topic != '/') return topic;
	if (buffer == NULL && len == NULL) return topic;
	switch (solution) {
		case AZURE_PAAS:
		{
			str = strdup(topic);
			pch = strtok_r(str, delim, &saveptr);
			while (pch != NULL)
			{
				//printf("%s\n", pch);
				pch = strtok_r(NULL, delim, &saveptr);
				if (pch == NULL) break;
				if (isdigit(*pch)) {
					snprintf(buffer, len, AZURE_IOT_HUB_SUB_PREFIX, pch);
					free(str);
					return buffer;
				}
			}
			free(str);
			return NULL;
		}
		default:
			return topic;
	}
}

//******************************After recieving****************************************
char *ET_PostTopicTranslate(const char* topic, const char *ref, char *buffer, int *len) {
	if (buffer == NULL && len == NULL) return topic;
	switch (solution) {
		case AZURE_PAAS:
		{
			if (NULL != strstr(topic, "messages/devicebound/%24.to=")) {
				char *s = strstr(ref, "\"topic\":\"");
				if (s != NULL) {
					char *e = NULL;
					s += 9;
					e = strchr(s, '\"');
					memcpy(buffer, s, (int)(e - s));
					*len = strlen(buffer);
					return buffer;
				}
			}
		}
		default:
			return topic;
	}
}
char *ET_PostMessageTranslate(const char* message, const char *ref, char *buffer, int *len) {
	if (buffer == NULL && len == NULL) return message;
	switch (solution) {
		case AZURE_PAAS:
		{
			if (NULL != strstr(ref, "messages/devicebound/%24.to=")) {
				char *s = strstr(message, "\"body\":{");
				if (s != NULL) {
					char *e = NULL;
					s += 7;
					e = strstr(s, "\"topic\":\"") - 3;
					*len = (int)(e - s) + 2;
					return s;
				}
			}
		}
		default:
			return message;
	}
}