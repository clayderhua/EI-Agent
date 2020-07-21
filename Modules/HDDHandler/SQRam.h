#pragma once
#ifndef _SQRAM_H_
#define _SQRAM_H_
#include <stdbool.h>
#include "IoTMessageGenerate.h"

typedef struct sqram_node_t {
	int Index;
	double	Temperature;
	bool SPDLock;
	int ClockRate;
	int Speed;
	char SerialNumber[21];
	char ModelType[16];
	char DDRType[16];
	int ramType;
	unsigned int Capacity;
	unsigned int Ranks;
	unsigned int Bank;
	double Voltage;
	char Week[3];
	char Year[3];
	char Manufacturer[16];
	char ICVendor[16];
	struct sqram_node_t* next;
}sqram_node_t;

typedef sqram_node_t* sqram_info_list;

typedef struct {
	int count;
	sqram_info_list sqramInfoList;
}sqram_info_t;

void sqr_UpdateSQRAM(sqram_info_t* ramInfo, MSG_CLASSIFY_T* myCapability);

sqram_info_list sqr_CreateSQRAMInfoList();

void sqr_DestroySQRAMInfo(sqram_info_list ramInfoList);

bool sqr_GetSQRAMInfo(sqram_info_t* pRamInfo);

bool sqr_IsExistSUSIDeviceLib();

bool sqr_StartupSUSIDeviceLib();

bool sqr_CleanupSUSIDeviceLib();

#endif