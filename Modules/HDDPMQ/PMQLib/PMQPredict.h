#ifndef PMQPREDICT_H
#define PMQPREDICT_H

#include <stdint.h>

#define PredictStatus_t uint32_t

#define PMQ_PREDICT_SUCCESS						((PredictStatus_t)0)
#define PMQ_PREDICT_ERROR						((PredictStatus_t)0xFFFFF0FF)
#define PMQ_HDD_PREDICT_BASE					((PredictStatus_t)0xF1000000)
#define PMQ_PREDICT_SPACE_INSUFFIENT			((PredictStatus_t)(PMQ_HDD_PREDICT_BASE + 1))
#define	PMQ_PREDICT_SPACE_REDUNDANCY			((PredictStatus_t)(PMQ_HDD_PREDICT_BASE + 2))
#define	PMQ_PREDICT_PREDICT_GOOD				((PredictStatus_t)(PMQ_HDD_PREDICT_BASE + 3))
#define	PMQ_PREDICT_PREDICT_FAIL				((PredictStatus_t)(PMQ_HDD_PREDICT_BASE + 4))

#define DAYHOURS			24
#define WEIGHT_VAL			0.8

PredictStatus_t PMQ_SQFPredict(unsigned int* smartList, unsigned int maxProg, int len, int type, double* result);
PredictStatus_t PMQ_HddPredict(unsigned int* smartList, int len, double* result);
double NormalizeValue(double val, double a_min, double a_max, double b_min, double b_max);

#endif // !PMQPREDICT_H

