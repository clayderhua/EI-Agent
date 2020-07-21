#ifndef __unistd_h__
#define __unistd_h__
#ifdef __cplusplus
extern "C" {
#endif
#include <windows.h>

#define sleep(u) Sleep((u)*1000)
#define usleep(u) Sleep((u)/1000)

#define getcwd _getcwd

#define F_OK 0x00
#define access _access


int getppid();

#ifdef __cplusplus
}
#endif
#endif //__unistd_h__