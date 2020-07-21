/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/06/02 by Scott Chang									*/
/* Modified Date: 2017/06/02 by Scott Chang									*/
/* Abstract     : Configure File Reader API				   					*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef _CONFIG_READER_H_
#define _CONFIG_READER_H_

#pragma once

#include <stdbool.h>
#include "IPSOParser.h"

typedef struct CONFIG{
	char credentialUrl[256];
	char iotKey[256];
	char strServerIP[256];
	int iPort;
	int SSLMode;
	char strConnID[128];
	char strConnPW[512];
	int iTotalCount;
	char strseed[16];
	char containpath[256];
	char strData[1024];
	char strAccount[128];
	char strPasswd[512];
	char strPrefix[64];
	char strProductTag[64];
	long frequency;
	int launchinterval;
	long reportcountdown;
	int reconnectdelay;

	char strCafile[256];
	char strCertfile[256];
	char strKeyfile[256];
	char strKeypwd[256];
};

typedef struct CAPABILITY{
	MSG_CLASSIFY_T* pCapability;
	struct CAPABILITY* next;
};

#ifdef __cplusplus 
extern "C" { 
#endif 

bool ReadConfig(char* filepath, struct CONFIG* config);
struct CAPABILITY* LoadCapability(char* path);
void FreeCapability(struct CAPABILITY* pCapabilities);

#ifdef __cplusplus 
} 
#endif 

#endif