/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/08/18 by Fred Chang									*/
/* Modified Date: 2016/03/03 by Scott Chang									*/
/* Abstract     :  															*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __sys_shm_h__
#define __sys_shm_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "export.h"
#include <sys/ipc.h>
	WISEPLATFORM_API long shmget(key_t key, size_t size, int shmflg);
	WISEPLATFORM_API void* shmat(long shmid, const void *shmaddr, int shmflg);
	WISEPLATFORM_API int shmdt(const void *shmaddr);

#ifdef __cplusplus
}
#endif

#endif //__sys_shm_h__