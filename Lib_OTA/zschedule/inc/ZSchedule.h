#ifndef _Z_SCHEDULE_H_
#define _Z_SCHEDULE_H_

typedef enum{
	ST_UNKNOW = 0,
	ST_ONCE = 1,
	ST_EVERY_MONTH = 2,
	ST_EVERY_WEEK= 3,
	ST_EVERY_DAY = 4,
	ST_MAX,
}ZScheType;

typedef enum{
    SS_UNKNOW = 0,
	SS_NOT_RUN = 1,
	SS_IDLE = 2,
	SS_TASKING = 3,
	SS_MAX,
}ZScheStatus;

typedef struct {
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
}ZScheTime;

typedef ZScheTime * PZScheTime;

typedef struct{
   ZScheType scheType;
	ZScheTime startTime;
	ZScheTime endTime;
}ZScheTimeRange;

typedef ZScheTimeRange * PZScheTimeRange;

typedef void * ZScheHandle;

typedef struct{
	ZScheTimeRange zScheTimeRange;
}ZScheduleTask;

#ifdef __cplusplus
extern "C" {
#endif

	typedef int (*ZScheEnterNotify)(void * userData);

	typedef int (*ZScheLeaveNotify)(void * userData);

	typedef int (*ZScheTask)(int * leaveFalg, void * userData);

	ZScheHandle ZScheCreate(PZScheTimeRange pZScheTR);

	int ZScheRun(ZScheHandle handle, ZScheEnterNotify zScheEnterNotify, ZScheLeaveNotify zScheLeaveNotify, 
		ZScheTask zScheTask, void * userData);

	ZScheStatus ZScheGetStatus(ZScheHandle handle);

	int ZScheModifyTimeRange(ZScheHandle handle, PZScheTimeRange pZScheTR);

	int ZScheCmpTimeRange(PZScheTimeRange pZScheTR1, PZScheTimeRange pZScheTR2);

	int ZScheGetTimeRange(ZScheHandle handle, PZScheTimeRange pResultTR);

	int ZScheStop(ZScheHandle handle);

	int ZScheDestory(ZScheHandle handle);

#ifdef __cplusplus
};
#endif

#endif
