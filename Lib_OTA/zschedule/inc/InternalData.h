#ifndef _INTERNAL_DATA_
#define _INTERNAL_DATA_

#include <time.h>
#include "ZSchedule.h"

#define DEF_HOUR_MAX     23
#define DEF_MINUTE_MAX   59
#define DEF_SECOND_MAX   59
#define DEF_MONTH_MIN    1
#define DEF_MONTH_MAX    12
#define DEF_MDAY_MIN     1
#define DEF_MDAY_MAX     31
#define DEF_WDAY_MIN     1
#define DEF_WDAY_MAX     7
#define DEF_YEAR_MIN     1970
#define DEF_YEAR_MAX     (2016+1000)

typedef struct {
	ZScheTimeRange scheTR;
   time_t startTimeS;
	time_t endTimeS;
	ZScheStatus scheStatus;

	pthread_mutex_t dataMutex;

	pthread_t scheThreadT;
	int isScheThreadRunning;
	void * userData;
	ZScheEnterNotify zScheEnterNotify;
	ZScheLeaveNotify zScheLeaveNotify; 
	ZScheTask zScheTask;
	int leaveFlag;
	pthread_t scheTaskThreadT;
	int taskRet;
}ZScheContext;

typedef ZScheContext * PZScheContext;

#endif
