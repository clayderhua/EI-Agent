#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "ZSchedule.h"
#include "InternalData.h"

#ifdef WIN32
#include "wrapper.h"
#endif

static int TimeSECompare(PZScheTimeRange pZScheTR, time_t * pSTimeS, time_t * pETimeS);
static int TimeRangeCheck(PZScheTimeRange pZScheTR, time_t * pSTimeS, time_t * pETimeS);
static time_t ScheGetNowTime(ZScheType zST);
static int InitScheTimeS(ZScheType zST, PZScheTime pZSheT, time_t * resultTS);
//static int InitScheSETimeS(PZScheContext pZSContext);

static int TimeSECompare(PZScheTimeRange pZScheTR, time_t * pSTimeS, time_t * pETimeS)
{
	int iRet = 0;
	if(pZScheTR == NULL) return iRet;
	{
		time_t tmpStartS = 0, tmpEndS = 0;
		if(InitScheTimeS(pZScheTR->scheType, &pZScheTR->startTime, &tmpStartS) == 0)
		{
			if(InitScheTimeS(pZScheTR->scheType, &pZScheTR->endTime, &tmpEndS) == 0)
			{
				if(tmpEndS > tmpStartS)
				{
					if(pSTimeS) *pSTimeS = tmpStartS;
					if(pETimeS) *pETimeS = tmpEndS;
					iRet = 1;
				}
			}
		}
	}
	return iRet;
}

static int TimeRangeCheck(PZScheTimeRange pZScheTR, time_t * pSTimeS, time_t * pETimeS)
{
	int iRet = 0;
	if(pZScheTR == NULL)
		return iRet;
	if(pZScheTR->startTime.hour > DEF_HOUR_MAX || pZScheTR->endTime.hour > DEF_HOUR_MAX)
		return iRet;
	if(pZScheTR->startTime.minute > DEF_MINUTE_MAX || pZScheTR->endTime.minute > DEF_MINUTE_MAX)
		return iRet;
	if(pZScheTR->startTime.second > DEF_SECOND_MAX || pZScheTR->endTime.second >DEF_SECOND_MAX)
		return iRet;
	switch (pZScheTR->scheType)
	{
	case ST_EVERY_DAY:
		{
			iRet = 1;
			break;
		}
	case ST_EVERY_WEEK:
		{
			if(pZScheTR->startTime.day >= DEF_WDAY_MIN && pZScheTR->startTime.day <= DEF_WDAY_MAX &&
				pZScheTR->endTime.day >= DEF_WDAY_MIN && pZScheTR->endTime.day <= DEF_WDAY_MAX)
			{
				iRet = 1;
			}
			break;
		}
	case ST_EVERY_MONTH:
		{
			if(pZScheTR->startTime.day >= DEF_MDAY_MIN && pZScheTR->startTime.day <= DEF_MDAY_MAX &&
				pZScheTR->endTime.day >= DEF_MDAY_MIN && pZScheTR->endTime.day <= DEF_MDAY_MAX)
			{
				iRet = 1;
			}
			break;
		}
	case ST_ONCE:
		{

			if(pZScheTR->startTime.day >= DEF_MDAY_MIN && pZScheTR->startTime.day <= DEF_MDAY_MAX &&
				pZScheTR->endTime.day >= DEF_MDAY_MIN && pZScheTR->endTime.day <= DEF_MDAY_MAX &&
				pZScheTR->startTime.month >= DEF_MONTH_MIN && pZScheTR->startTime.month <= DEF_MONTH_MAX &&
				pZScheTR->endTime.month >= DEF_MONTH_MIN && pZScheTR->endTime.month <= DEF_MONTH_MAX &&
				pZScheTR->startTime.year >= DEF_YEAR_MIN && pZScheTR->startTime.year <= DEF_YEAR_MAX &&
				pZScheTR->endTime.year >= DEF_YEAR_MIN && pZScheTR->endTime.year <= DEF_YEAR_MAX)
			{
				iRet = 1;
			}
			break;
		}
	default:
		printf("TimeRangeCheck: Error invalid scheType\n");
		break;
	}
	if(iRet)
	{
		iRet = TimeSECompare(pZScheTR, pSTimeS, pETimeS);
	}
	return iRet;
}

static time_t ScheGetNowTime(ZScheType zST)
{
	time_t tRet = 0;
	time_t tNow = time(NULL);
	if(zST == ST_ONCE)
	{
		tRet = tNow;
	}
	else
	{
		struct tm tmLNow;
		if(localtime_r(&tNow, &tmLNow))
		{
         if(zST == ST_EVERY_MONTH)
			{
				tRet += (tmLNow.tm_mday-1)*3600*24;
			}
			else if(zST == ST_EVERY_WEEK)
			{
				tRet += tmLNow.tm_wday*3600*24;
			}
			tRet += tmLNow.tm_hour*3600 + tmLNow.tm_min*60 + tmLNow.tm_sec;
		}
	}
	return tRet;
}

static int InitScheTimeS(ZScheType zST, PZScheTime pZSheT, time_t * resultTS)
{
	int iRet = -1;
	if(pZSheT != NULL && resultTS != NULL)
	{
		if(zST== ST_ONCE)
		{
			time_t tmpT = 0;
			struct tm tmpTm;
			memset(&tmpTm, 0, sizeof(struct tm));
			tmpTm.tm_year = pZSheT->year-1900;
			tmpTm.tm_mon = pZSheT->month - 1;
			tmpTm.tm_mday = pZSheT->day;
			tmpTm.tm_hour = pZSheT->hour;
			tmpTm.tm_min = pZSheT->minute;
			tmpTm.tm_sec = pZSheT->second;
			tmpTm.tm_isdst = -1;
			tmpT = mktime(&tmpTm);
			if(tmpT != -1)
			{
				*resultTS = tmpT;
				iRet = 0;
			}
		}
		else 
		{
			*resultTS = 0;
			if(zST == ST_EVERY_MONTH || zST == ST_EVERY_WEEK)
			{
				*resultTS += (pZSheT->day - 1)*3600*24;
			}
			*resultTS += pZSheT->hour*3600 + pZSheT->minute*60 + pZSheT->second;
			iRet = 0;
		}
	}
	return iRet;
}

static void * ScheTaskThreadStart(void *args)
{
	PZScheContext pZSContext = (PZScheContext)args;
	if(pZSContext)
	{
		pZSContext->taskRet = pZSContext->zScheTask(&pZSContext->leaveFlag, pZSContext->userData);
	}
	return 0;
}

static void * ScheThreadStart(void *args)
{
#define  DEF_PER_SLEEP_MS    500

	PZScheContext pZSContext = (PZScheContext)args;
	if(pZSContext)
	{
		time_t nowTS = 0;
		pZSContext->scheStatus = SS_IDLE;
		int ret = 0;

		while(pZSContext->isScheThreadRunning)
		{
			nowTS = ScheGetNowTime(pZSContext->scheTR.scheType);
			do {
				if(nowTS < 0)
					break;

				if(nowTS < pZSContext->startTimeS || nowTS > pZSContext->endTimeS) // schedule is not start or reach the end time
				{
					if(pZSContext->scheStatus == SS_TASKING)
					{
						pZSContext->leaveFlag = 1;
						if(pZSContext->zScheLeaveNotify) {
							pZSContext->zScheLeaveNotify(pZSContext->userData); // SUEZScheLeaveNotify
						}
						pZSContext->scheStatus = SS_IDLE;
					}
				}
				else // schedule is start...
				{
					if(pZSContext->scheStatus == SS_IDLE)
					{
						pZSContext->leaveFlag = 0;
						if(pZSContext->zScheEnterNotify) {
							ret = pZSContext->zScheEnterNotify(pZSContext->userData);
						}
						if (ret != 0) { // if enter notify fail, retry in next round
							break;
						}
						pZSContext->scheStatus = SS_TASKING;
						pZSContext->taskRet = 1;
					}
					if(pZSContext->taskRet > 0 && pZSContext->zScheTask) //if taskRet greater than 0 then repeat execute task.
					{
						pZSContext->taskRet = 0;
                        pthread_create(&pZSContext->scheTaskThreadT, NULL, ScheTaskThreadStart, pZSContext);
					}
				}
			} while (0);

			usleep(1000*DEF_PER_SLEEP_MS);
		}
		if(pZSContext->scheStatus == SS_TASKING)
		{
			if(pZSContext->zScheLeaveNotify) {
				pZSContext->zScheLeaveNotify(pZSContext->userData);
			}
			pZSContext->scheStatus = SS_IDLE;
		}
		pZSContext->isScheThreadRunning = 0;
	}
	return 0;
}

//---------------------------interface implement S--------------------------
ZScheHandle ZScheCreate(PZScheTimeRange pZScheTR)
{
	PZScheContext pZScheContext = NULL;
	if(pZScheTR != NULL)
	{
		time_t tmpSTimeS = 0, tmpETimeS = 0;
		if(TimeRangeCheck(pZScheTR, &tmpSTimeS, &tmpETimeS))
		{
			pZScheContext = (PZScheContext)malloc(sizeof(ZScheContext));
			memset(pZScheContext, 0, sizeof(ZScheContext));
			pZScheContext->scheStatus = SS_NOT_RUN;
			pthread_mutex_init(&pZScheContext->dataMutex, NULL);
            memcpy(&pZScheContext->scheTR, pZScheTR, sizeof(ZScheTimeRange));
			pZScheContext->startTimeS = tmpSTimeS;
			pZScheContext->endTimeS = tmpETimeS;
		}
	}
	return pZScheContext;
}

int ZScheRun(ZScheHandle handle, ZScheEnterNotify zScheEnterNotify, ZScheLeaveNotify zScheLeaveNotify, 
				 ZScheTask zScheTask, void * userData)
{
	int iRet = -1;
	if(handle != NULL)
	{
		PZScheContext pZSContext = (PZScheContext)handle;
		if(!pZSContext->isScheThreadRunning)
		{
			pZSContext->zScheEnterNotify = zScheEnterNotify;
			pZSContext->zScheLeaveNotify = zScheLeaveNotify;
			pZSContext->zScheTask = zScheTask;
			pZSContext->userData = userData;
			pZSContext->isScheThreadRunning = 1;
			if(pthread_create(&pZSContext->scheThreadT, NULL, ScheThreadStart, pZSContext) != 0)
			{
				pZSContext->isScheThreadRunning = 0;
				pZSContext->zScheEnterNotify = NULL;
				pZSContext->zScheLeaveNotify = NULL;
				pZSContext->zScheTask = NULL;
				pZSContext->userData = NULL;
			}
			else iRet = 0;
		}
	}
	return iRet;
}

ZScheStatus ZScheGetStatus(ZScheHandle handle)
{
	ZScheStatus zSS = SS_UNKNOW;
	if(handle != NULL)
	{
		PZScheContext pZSContext = (PZScheContext)handle;
		pthread_mutex_lock(&pZSContext->dataMutex);
		zSS = pZSContext->scheStatus;
		pthread_mutex_unlock(&pZSContext->dataMutex);
	}
   return zSS;
}

int ZScheModifyTimeRange(ZScheHandle handle, PZScheTimeRange pZScheTR)
{
	int iRet = -1;
	if(handle != NULL && pZScheTR != NULL)
	{
		PZScheContext pZSContext = (PZScheContext)handle;
		time_t tmpSTimeS = 0, tmpETimeS = 0;
		if(TimeRangeCheck(pZScheTR, &tmpSTimeS, &tmpETimeS))
		{
			pthread_mutex_lock(&pZSContext->dataMutex);
			memcpy(&pZSContext->scheTR, pZScheTR, sizeof(ZScheTimeRange));
			pZSContext->startTimeS = tmpSTimeS;
			pZSContext->endTimeS = tmpETimeS;
			pthread_mutex_unlock(&pZSContext->dataMutex);
			iRet = 0;
		}
	}
	return iRet;
}

int ZScheCmpTimeRange(PZScheTimeRange pZScheTR1, PZScheTimeRange pZScheTR2)
{
	int iRet = -1;
	if(pZScheTR1 != NULL && pZScheTR2 != NULL)
	{
		if(pZScheTR1->scheType == pZScheTR2->scheType)
		{
			switch(pZScheTR2->scheType)
			{
			case ST_ONCE:
				{
					if(pZScheTR1->startTime.year != pZScheTR2->startTime.year ||
						pZScheTR1->startTime.month != pZScheTR2->startTime.month ||
						pZScheTR1->endTime.year != pZScheTR2->endTime.year ||
						pZScheTR1->endTime.month != pZScheTR2->endTime.month )
						break;
				}
			case ST_EVERY_MONTH:
			case ST_EVERY_WEEK:
				{
					if(pZScheTR1->startTime.day != pZScheTR2->startTime.day ||
						pZScheTR1->endTime.day != pZScheTR2->endTime.day )
						break;
				}
			case ST_EVERY_DAY:
				{
					if(pZScheTR1->startTime.hour != pZScheTR2->startTime.hour ||
						pZScheTR1->endTime.hour != pZScheTR2->endTime.hour ||
						pZScheTR1->startTime.minute != pZScheTR2->startTime.minute ||
						pZScheTR1->endTime.minute != pZScheTR2->endTime.minute ||
						pZScheTR1->startTime.second != pZScheTR2->startTime.second ||
						pZScheTR1->endTime.second != pZScheTR2->endTime.second )
						break;
					iRet = 0;
					break;
				}
			default:
				printf("ZScheCmpTimeRange: Error invalid scheType\n");
				break;
			}
		}
	}
	return iRet;
}

int ZScheGetTimeRange(ZScheHandle handle, PZScheTimeRange pResultTR)
{
	int iRet = -1;
	if(handle != NULL && pResultTR != NULL)
	{
		PZScheContext pZSContext = (PZScheContext)handle;
		pthread_mutex_lock(&pZSContext->dataMutex);
		memcpy(pResultTR, &pZSContext->scheTR, sizeof(ZScheTimeRange));
		pthread_mutex_unlock(&pZSContext->dataMutex);
	}
	return iRet;
}

int ZScheStop(ZScheHandle handle)
{
	int iRet = -1;
	if(handle != NULL)
	{
		PZScheContext pZSContext = (PZScheContext)handle;
		if(pZSContext->isScheThreadRunning)
		{
			pZSContext->leaveFlag = 1;
			if (pZSContext->scheTaskThreadT) {
				pthread_join(pZSContext->scheTaskThreadT, NULL);
			}
			pZSContext->isScheThreadRunning = 0;
			pthread_join(pZSContext->scheThreadT, NULL);
		}
		pZSContext->scheThreadT = 0;
		pZSContext->scheTaskThreadT = 0;
		iRet = 0;
	}
	return iRet;
}

int ZScheDestory(ZScheHandle handle)
{
	int iRet = 0;
	if(handle != NULL)
	{
		PZScheContext pZSContext = (PZScheContext)handle;
		ZScheStop(pZSContext);
		pthread_mutex_destroy(&pZSContext->dataMutex);
		free(pZSContext);
		pZSContext = NULL;
	}
	return iRet;
}
//---------------------------interface implement E--------------------------
