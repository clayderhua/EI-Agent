/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2016/05/25 by Fred Chang									*/
/* Modified Date: 2016/05/25 by Fred Chang									*/
/* Abstract     :  					*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __sys_sem_h__
#define __sys_sem_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "export.h"
#include <sys/ipc.h>

	/* Flags for `semop'.  */
	#define SEM_UNDO	0x1000		/* undo the operation on exit */

	/* Commands for `semctl'.  */
	#define GETPID		11		/* get sempid */
	#define GETVAL		12		/* get semval */
	#define GETALL		13		/* get all semval's */
	#define GETNCNT		14		/* get semncnt */
	#define GETZCNT		15		/* get semzcnt */
	#define SETVAL		16		/* set semval */
	#define SETALL		17		/* set all semval's */
#if 0
	struct semid_ds {
		struct ipc_perm sem_perm;  /* Ownership and permissions */
		time_t          sem_otime; /* Last semop time */
		time_t          sem_ctime; /* Last change time */
		unsigned long   sem_nsems; /* No. of semaphores in set */
	};
#endif
	/*
	 * semop's sops parameter structure
	 */
	struct sembuf {
		unsigned short	sem_num;	/* semaphore # */
		short		sem_op;		/* semaphore operation */
		short		sem_flg;	/* operation flags */
	};
#if 0
	union semun {
		int              val;    /* Value for SETVAL */
		//struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
		unsigned short  *array;  /* Array for GETALL, SETALL */
		//struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
	};
#endif
	ADVPLAT_EXPORT long ADVPLAT_CALL semget(key_t key, int nsems, int semflg);
	ADVPLAT_EXPORT int ADVPLAT_CALL semop(int semid, struct sembuf *sops, unsigned nsops);
	ADVPLAT_EXPORT int ADVPLAT_CALL semctl(int semid, int semnum, int cmd, ...);
#ifdef __cplusplus
}
#endif

#endif //__sys_sem_h__