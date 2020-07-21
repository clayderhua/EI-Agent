#ifndef _PACKAGE_LOG_PARSER_H_
#define _PACKAGE_LOG_PARSER_H_

#include "SUEClientCore.h"

#define LOG_NAME "OTAPackageLog.log"
#define SECTION_PROPERTY "[property]"
#define SECTION_STATUS "[status]"
#define SECTION_END "[end]"
#define SECTION_EXIT "[exit]"
#define SECTION_NULL_NUM 0
#define SECTION_PROPERTY_NUM 1
#define SECTION_STATUS_NUM 2
#define SECTION_END_NUM 3
#define KEY_SW_NAME "name"
#define KEY_SW_VERSION "version"
#define KEY_SW_RESULT "result"
#define KEY_SW_MESSAGE "message"
#define EQUAL_FLAG     "="
#define SEMI_FLAG     ";"
#define VALUE_SW_RESULT_SUCCESS "success"
#define VALUE_SW_RESULT_FAILED "failed"
#define READ_LEN  2049
#define STATUS_LEN  2049
#define MESSAGE_LEN  256
#define SW_NAME_LEN  256
#define SW_VERSION_LEN  32
#define SW_RESULT_LEN  10
#define ERROR_OPEN_LOG_FAILED  -1
#define ERROR_ILLEGAL_LOG  -2

typedef struct section{
	//int  index;  //section name
	char name[SW_NAME_LEN];		//
	char version[SW_VERSION_LEN];
	char result[SW_RESULT_LEN];
	char message[MESSAGE_LEN];
    struct section * next;
}Section_Type;
typedef struct section section;

typedef struct section_head{
    struct section * next;
}Section_Head;

int SUEPkgDeployCheckCB(const void * const checkHandle, NotifyDPCheckMsgCB notifyDpMsgCB, int * isQuit, void * userData);

#endif
