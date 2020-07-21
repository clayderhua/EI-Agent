// PMQPredictLib.cpp : Defines the exported functions for the DLL application.
//
#include "PMQPredict.h"
#include "../DiskPMQInfo.h"
#include <stdio.h>
#include <math.h>
#include <float.h>

PredictStatus_t PMQ_SQFPredict(unsigned int* smartList, unsigned int maxProg, int len, int type, double* result)
{
	double usedDay;
	double estDay;
	double remingDay;
	double returnVal;

	if (smartList == NULL)
		return PMQ_PREDICT_ERROR;

	if (len != 2)
		return PMQ_PREDICT_ERROR;

	if (smartList[1] == 0 || smartList[1] < 0)
		return PMQ_PREDICT_ERROR;

	if (maxProg == 0)
		return PMQ_PREDICT_ERROR;

	usedDay = (double)smartList[0] / (double)DAYHOURS;
	estDay = ((double)maxProg / (double)smartList[1]) * (double)usedDay * (double)WEIGHT_VAL;
	remingDay = estDay - usedDay;

	if (remingDay < 0)
		remingDay = 0;

	if (remingDay > 2000)
		remingDay = 2000;

	if (remingDay <= 30)
		*result = (NormalizeValue(30 - remingDay, 0, 30, 67, 100) / 100);
	else if (remingDay <= 60)
		*result = (NormalizeValue(60 - remingDay, 31, 60, 55, 66) / 100);
	else if (remingDay > 60)
		*result = (NormalizeValue(2000 - remingDay, 61, 2000, 1, 54) / 100); 

	return PMQ_PREDICT_SUCCESS;
}

PredictStatus_t PMQ_HddPredict(unsigned int* smartList, int len, double* result)
{
	const double x_max = -log(DBL_EPSILON);
	
	double x = 0, p_cal = 1.0;
	double th = 0.385;																// threshold to classify the prediction
	double coef[] = { -1.667, 0.009144, 0.00001238, 1.417, -0.00009659, 0.7102 };	// coefficient for logistic regression
	int i;
	
	for (i = 0; i < len; i++) 
		x += coef[i] * smartList[i];

	if (x > x_max)
	{
		*result = p_cal;
		return PMQ_PREDICT_PREDICT_FAIL;				// predict fail
	}
	else 
	{
		p_cal = exp(x) / (1 + exp(x));
		*result = p_cal;

		if (p_cal > th)
			return PMQ_PREDICT_PREDICT_FAIL;			// predict fail
		else
			return PMQ_PREDICT_PREDICT_GOOD;			// predict good
	}

}

double NormalizeValue(double val, double a_min, double a_max, double b_min, double b_max)
{
	int result = 0;

	if (a_max <= a_min)
		return result;

	if (b_max <= b_min)
		return result;

	result  = (int)(((val - a_min) * (b_max - b_min)) / (a_max - a_min) + b_min) * 100;
	return result / 100;
}