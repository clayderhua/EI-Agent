#ifndef _SCHEDULE_DATA_H_
#define _SCHEDULE_DATA_H_

#include "ZSchedule.h"
#include "SUEClientCoreData.h"

typedef enum{
	SAT_UNKNOW = 0,
	SAT_DL = 1,
	SAT_DP = 2,
	SAT_ALL = 15,
	SAT_MAX,
}ScheActType;

typedef enum{
	SST_UNKNOW = 0,
	SST_ENTER = 1,
	SST_LEAVE = 2,
	SST_MAX,
}ScheStatusType;

typedef enum{
    UM_UNKNOW = 0,
    UM_HIGHEST = 1,
    UM_INCREMENT = 2,
    UM_MAX,
}UpgMode;

typedef struct {
	char * pkgType;
    ScheActType actType;
	ZScheType  scheType;
	ZScheHandle zScheHandle;
	char * startTimeStr;
	char * endTimeStr;
	int isDLInfoChecking;
	PDLTaskInfo pObtainedDLInfo;
	int isDLRetChecking;
	int dlRetCode;
    UpgMode upgMode;
	char ** subDevices;
	int subDeviceSize;
	long pkgOwnerId;
}ScheduleInfo;

typedef ScheduleInfo * PScheduleInfo;

#endif
