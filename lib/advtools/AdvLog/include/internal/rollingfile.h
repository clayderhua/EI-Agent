/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/07/07 by Fred Chang									*/
/* Modified Date: 2015/07/07 by Fred Chang									*/
/* Abstract     : Advantech Logging Library    						        */
/* Reference    : None														*/
/****************************************************************************/
#ifndef __ROLLING_FILE_H__
#define __ROLLING_FILE_H__

void RollFile_Open(const char *path);
FILE *RollFile_Check();
void RollFile_Flush(int length);
void RollFile_RefreshConfigure();
void RollFile_Close();


#endif //__ROLLING_FILE_H__


