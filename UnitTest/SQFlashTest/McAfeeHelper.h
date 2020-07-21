#pragma once
#ifndef _MCAFEE_HELPER_H_
#define _MCAFEE_HELPER_H_
#include "stdbool.h"

bool mc_Initialize(char* filepath, int* version);
void mc_Uninitialize();
bool mc_GetCurrentVirusVersion(int* version);
bool mc_StartScan(char* driverletters, int amount, int* total, int* infected);
bool mc_CheckUpdate();
bool mc_Update(int* newVersion);

#endif //_MCAFEE_HELPER_H_
