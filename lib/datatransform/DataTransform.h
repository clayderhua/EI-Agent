/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/11/20 by Scott Chang								    */
/* Modified Date: 2017/11/20 by Scott Chang									*/
/* Abstract     : WISE-PaaS data transform									*/
/* Reference    : None														*/
/****************************************************************************/
#pragma once
#ifndef _DATA_TRANSFORM_H_
#define _DATA_TRANSFORM_H_

#include "stdbool.h"
#include "unistd.h"
#include "cJSON.h"
//-----------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//EI-PaaS and WISE-PaaS transfer
bool GetNewID(char* oldID, char* newID);
bool GetOldID(char* newID, char* oldID);
char* Trans2NewFrom( const char *data );
char* Trans2OldFrom( const char *data);
bool Trans2NewTopic( const char *oldTopic, const char *tag, char* newTopic );
bool Trans2OldTopic( const char *newTopic, char* oldTopic );
bool GetNewIDFromTopic( const char *topic, char* newID );
bool GetOldIDFromTopic( const char *topic, char* oldID );

#ifdef __cplusplus
}
#endif

#endif //_DATA_TRANSFORM_H_