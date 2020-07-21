/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2018/09/27 by Fred Chang								    */
/* Modified Date: 2018/09/27 by Fred Chang									*/
/* Abstract     : External Translator for Mosquitto Carrier API definition	*/
/* Reference    : None														*/
/****************************************************************************/

#ifndef _EXTERNEL_TRANSLATOR_H_
#define _EXTERNEL_TRANSLATOR_H_
#include <stdbool.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
	void ET_AssignSolution(char *soln);

	//Before publish
	int ET_getLength(unsigned char *buffer);
	char *ET_PreTopicTranslate(const char* topic, const char *ref, char *buffer, int *len);
	char *ET_PreMessageTranslate(const char* message, const char *ref, char *buffer, int *len);
	int ET_QueuedPublish(void *mosq, time_t *time, void **queue, const char* devid, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);

	//Subscribe
	char *ET_SubscibeTopicTranslate(const char* topic, char *buffer, int *len);

	//After recieve
	char *ET_PostTopicTranslate(const char* topic, const char *ref, char *buffer, int *len);
	char *ET_PostMessageTranslate(const char* message, const char *ref, char *buffer, int *len);
#ifdef __cplusplus
}
#endif
#endif