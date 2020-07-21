#ifndef _SUE_SCHEDULE_H_
#define _SUE_SCHEDULE_H_

typedef int (*ScheOutputMsgCB)(char *msg, int msgLen, void * userData);

typedef void * SUEScheHandle;

#ifdef __cplusplus
extern "C" {
#endif

	#define SCHE_CFG_FILE     "Schedule.cfg"

	SUEScheHandle InitSUESche(char * scheDir);

	int UninitSUESche(SUEScheHandle sueScheHandle);

	int StartSUESche();

	int SetSUESheOutPutMsgCB(SUEScheHandle sueScheHandle, ScheOutputMsgCB outputMsgCB, void * userData);

	int ProcSUEScheCmd(SUEScheHandle sueScheHandle, char * jsonCmd);

	void InterceptorSUEScheDLRet(SUEScheHandle sueScheHandle, char * pkgName, int retCode);

#ifdef __cplusplus
};
#endif

#endif