#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include <sys/sem.h>
#include "export.h"
#include "wrapper.h"

typedef struct {
	key_t key;
	int nsems;
	int semflg;
	HANDLE *semObject;
} SEMID;

long ADVPLAT_CALL semget(key_t key, int nsems, int semflg) {
	//IPC_CREAT | 00666
	SEMID *semid = NULL;
	semid = (SEMID *)malloc(sizeof(SEMID));
	semid->semObject = (HANDLE *)calloc(nsems,sizeof(HANDLE));
	semid->key = key;
	semid->nsems = nsems;
	semid->semflg = semflg;
	return (long)semid;
}

int ADVPLAT_CALL semop(int semid, struct sembuf *sops, unsigned nsops) {
	SEMID *id = (SEMID *)semid;
	HANDLE hSemObject = NULL;
	unsigned int i = 0;

	for (i = 0; i < nsops; i++) {
		hSemObject = id->semObject[sops[i].sem_num];
		if (hSemObject != NULL) {
			switch (sops[i].sem_op) {
			case 1:
				ReleaseSemaphore(hSemObject, 1, NULL);
				break;
			case -1:
				WaitForSingleObject(hSemObject, INFINITE);
				break;
			default:
				break;
			}
		}
	}
	return 0;
}

int ADVPLAT_CALL semctl(int semid, int semnum, int cmd, ...) {
	SEMID *id = (SEMID *)semid;
	HANDLE hSemObject = id->semObject[semnum];
	int result = 0;
	va_list ap;
	va_start(ap, cmd);
	
	switch (cmd) {
	case SETVAL:
		if (semnum >= id->nsems) {
			result = -1;
			break;
		}
		if (hSemObject == NULL) {
#ifdef UNICODE
			WCHAR wsz[64];
			swprintf(wsz, 64, L"%X_%X", id->key, semnum);
			LPCWSTR p = wsz;
#else
			CHAR wsz[64];
			snprintf(wsz, 64, "%X_%X", id->key, semnum);
			LPCSTR p = wsz;
#endif

			if (id->semflg & IPC_CREAT) hSemObject = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, p);
			else p = NULL;

			if (hSemObject == NULL) {
				int max = va_arg(ap, int);
				hSemObject = CreateSemaphore(NULL, max, max, p);
				if (hSemObject == NULL) {
					result = -1;
					break;
				}
			}
			id->semObject[semnum] = hSemObject;
		}
		else result = -1;
		break;
	case IPC_RMID:
		if (hSemObject != NULL) {
			CloseHandle(hSemObject);
			id->semObject[semnum] = NULL;
			id->nsems--;
			if (id->nsems == 0) free(id);
		}
		else result = -1;
		break;
	}

	va_end(ap);
	return result;
}