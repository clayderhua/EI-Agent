/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2016 by Zach Chih															     */
/* Modified Date: 2016/11/29 by Zach Chih															 */
/* Abstract     : DataSync                                   													*/
/* Reference    : None																									 */
/****************************************************************************/
#ifndef DATA_SYNC_H
#define DATA_SYNC_H

#include "srp/susiaccess_handler_mgmt.h"
#include <stdbool.h>
#include <stdint.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <Windows.h>
#ifndef DATASYNC_API
#define DATASYNC_API WINAPI
#endif
#else
#define DATASYNC_API
#endif

#define LOG_TAG "DS"
#include "Log.h"

#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

#ifdef __cplusplus
extern "C" {
#endif

typedef int (DATASYNC_API *PUBLISHCB) (char const * topic, int qos, int retain, susiaccess_packet_body_t const * pkt);

bool DATASYNC_API DataSync_Initialize(char * pWorkdir,Handler_List_t *pLoaderList,void* pLogHandle);
void DATASYNC_API DataSync_Uninitialize();
void DATASYNC_API DataSync_SetFuncCB(PUBLISHCB g_publishCB);
void DATASYNC_API DataSync_Insert_Cap(void* const handle,char *cap,char *captopic, int result);
void DATASYNC_API DataSync_Insert_Data(void* const handle,char *rep,char *reptopic, int cmd);
void DATASYNC_API DataSync_Set_LostTime(int64_t losttime);
void DATASYNC_API DataSync_Set_ReConTime(int64_t recontime);


#ifdef __cplusplus
}
#endif


#endif
