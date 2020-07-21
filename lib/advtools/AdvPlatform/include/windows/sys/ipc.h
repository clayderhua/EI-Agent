/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/08/18 by Fred Chang									*/
/* Modified Date: 2015/08/18 by Fred Chang									*/
/* Abstract     :  					*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __sys_ipc_h__
#define __sys_ipc_h__

#ifdef __cplusplus
extern "C" {
#endif
typedef	long	key_t;	/* XXX should be in types.h */

/* common mode bits */
#define	IPC_R		00400	/* read permission */
#define	IPC_W		00200	/* write/alter permission */

/* SVID required constants (same values as system 5) */
#define	IPC_CREAT	01000	/* create entry if key does not exist */
#define	IPC_EXCL	02000	/* fail if key exists */
#define	IPC_NOWAIT	04000	/* error if request must wait */

#define	IPC_PRIVATE	(key_t)0 /* private key */

#define	IPC_RMID	0	/* remove identifier */
#define	IPC_SET		1	/* set options */
#define	IPC_STAT	2	/* get options */

#if 0
struct ipc_perm {
	key_t          __key; /* Key supplied to semget(2) */
	uid_t          uid;   /* Effective UID of owner */
	gid_t          gid;   /* Effective GID of owner */
	uid_t          cuid;  /* Effective UID of creator */
	gid_t          cgid;  /* Effective GID of creator */
	unsigned short mode;  /* Permissions */
	unsigned short __seq; /* Sequence number */
};
#endif

#ifdef __cplusplus
}
#endif
#endif //__sys_ipc_h__




