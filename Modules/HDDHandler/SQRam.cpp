#include "SQRam.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "WISEPlatform.h"
#include "SusiDeviceAPI.h"
#include "SPD.h"
#include "util_string.h"
#include "WMIHelper.h"

#define SQR_RAM_LIST "SQRAMList"
#define SQR_INDEX "Index"
#define SQR_SERIAL_NUMBER "SerialNumber"
#define SQR_PRODUCT_NAME "Type"
#define SQR_PRODUCT_TYPE "DDR"
#define SQR_TEMPERATURE "Temperature"
#define SQR_CLOCK_RATE "Speed"
#define SQR_CLOCK_DEFINED "DefinedSpeed"
#define SQR_CAPACITY "Capacity"
#define SQR_VOLTAGE "Voltage"
#define SQR_RANKS "Ranks"
#define SQR_BANK "Bank"
#define SQR_MANUFACTURE_DATE "Date"
#define SQR_WRITE_PROTECT "SPDLock"
#define SQR_MANUFACTURER "Manufacturer"
#define SQR_IC_VENDOR "ICVendor"
#define SENSOR_UNIT_CELSIUS "Celsius"
#define SENSOR_UNIT_GIGABYTE "GB"
#define SENSOR_UNIT_MEGAHERTZ "MHz"
#define SENSOR_UNIT_VOLTAGE "V"
#define INVALID_DEVMON_VALUE  (-999)
#define DEF_SUSIDEV_LIB_NAME "SusiDevice.dll"

typedef int (__stdcall *PSusiDeviceGetValue)(SusiId_t Id, uint32_t* pValue);
PSusiDeviceGetValue pSusiDeviceGetValue = NULL;
void* g_hSUSIDevice = NULL;

bool sqr_IsExistSUSIDeviceLib()
{
	bool bRet = false;
	void* hSUSIDevice = NULL;

	hSUSIDevice = LoadLibrary(DEF_SUSIDEV_LIB_NAME);
	if (hSUSIDevice != NULL)
	{
		bRet = true;
		FreeLibrary((HMODULE)hSUSIDevice);
		hSUSIDevice = NULL;
	}
	return bRet;
}

void GetSUSIDeviceFunction(void* hSUSIDevice)
{
	if (hSUSIDevice != NULL)
	{
		pSusiDeviceGetValue = (PSusiDeviceGetValue)GetProcAddress((HMODULE)hSUSIDevice, "SusiDeviceGetValue");
	}
}

bool sqr_StartupSUSIDeviceLib()
{
	bool bRet = false;
	g_hSUSIDevice = LoadLibrary(DEF_SUSIDEV_LIB_NAME);
	if (g_hSUSIDevice != NULL)
	{
		GetSUSIDeviceFunction(g_hSUSIDevice);
		bRet = true;
	}
	return bRet;
}

bool sqr_CleanupSUSIDeviceLib()
{
	bool bRet = true;

	if (g_hSUSIDevice != NULL)
	{
		FreeLibrary((HMODULE)g_hSUSIDevice);
		g_hSUSIDevice = NULL;
		pSusiDeviceGetValue = NULL;
	}

	return bRet;
}

bool GetRAMCount(int* pRAMCount)
{
	bool bRet = false;
	unsigned int ramCount = 0;
	if (pRAMCount == NULL)
		return bRet;
	if(pSusiDeviceGetValue)
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_QTY, &ramCount) == SUSI_STATUS_SUCCESS;
	if (!bRet)
		return bRet;
	*pRAMCount = ramCount;
	return bRet;
}

bool GetRAMType(int index, int* pRAMType, char* cType)
{
	bool bRet = false;
	unsigned int ramtype = 0;
	if (cType == NULL || pRAMType == NULL)
		return bRet;
	/*TODO: Check why SPD_ID_DRAM_TYPE(index) not worked*/
	if (pSusiDeviceGetValue)
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_TYPE(index), &ramtype) == SUSI_STATUS_SUCCESS;
	
	if (!bRet)
		return bRet;
	/*
	 * {0x0B,"DDR3 SDRAM"},
	 * {0x0C,"DDR4 SDRAM"}
	 */
	switch (ramtype)
	{
	case 11:
		strcpy(cType, "DDR3 SDRAM");
		bRet = true;
		break;
	case 12:
		strcpy(cType, "DDR4 SDRAM");
		bRet = true;
		break;
	default:
		strcpy(cType, "Not support");
		bRet = false;
		break;
	}
	*pRAMType = ramtype;
	return bRet;
}

bool GetRAMModel(int index, int type, char* cModel)
{
	bool bRet = false;
	unsigned int model = 0;
	if (cModel == NULL || type == 0)
		return bRet;
	if (pSusiDeviceGetValue)
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_MODULETYPE(index), &model) == SUSI_STATUS_SUCCESS;

	if (!bRet)
		return bRet;
	/*
	 DRAM_TYPE=DDR3
		0x00:"Undefined"
		0x01:"RDIMM"
		0x02:"UDIMM"
		0x03:"SO-DIMM"
		0x04:"Micro-DIMM"
		0x05:"Mini-RDIMM"
		0x06:"Mini-UDIMM"
		0x07:"Mini-CDIMM"
		0x08:"72b-SO-UDIMM"
		0x09:"72b-SO-RDIMM"
		0x0A:"72b-SO-CDIMM"
		0x0B:"LRDIMM"
		
	DRAM_TYPE=DDR4
		0x00:"Undefined"
		0x01:"RDIMM"
		0x02:"UDIMM"
		0x03:"SO-DIMM"
		0x04:"LRDIMM"
		0x05:"Mini-RDIMM"
		0x06:"Mini-UDIMM"
		0x07:"Reserved"
		0x08:"72b-SO-RDIMM"
		0x09:"72b-SO-UDIMM"
		0x0A:"Reserved"
		0x0B:"Reserved"
		0x0C:"16b-SO-DIMM"
		0x0D:"32b-SO-DIMM"
		0x0E:"Reserved"
		0x0F:"Reserved"
	 */
	if (type == 11) //DDR3
	{
		switch (model)
		{
		case 1:
			strcpy(cModel, "RDIMM");
			bRet = true;
			break;
		case 2:
			strcpy(cModel, "UDIMM");
			bRet = true;
			break;
		case 3:
			strcpy(cModel, "SO-DIMM");
			bRet = true;
			break;
		case 4:
			strcpy(cModel, "Micro-DIMM");
			bRet = true;
			break;
		case 5:
			strcpy(cModel, "Mini-RDIMM");
			bRet = true;
			break;
		case 6:
			strcpy(cModel, "Mini-UDIMM");
			bRet = true;
			break;
		case 7:
			strcpy(cModel, "Mini-CDIMM");
			bRet = true;
			break;
		case 8:
			strcpy(cModel, "72b-SO-UDIMM");
			bRet = true;
			break;
		case 9:
			strcpy(cModel, "72b-SO-RDIMM");
			bRet = true;
			break;
		case 10:
			strcpy(cModel, "72b-SO-CDIMM");
			bRet = true;
			break;
		case 11:
			strcpy(cModel, "LRDIMM");
			bRet = true;
			break;
		default:
			strcpy(cModel, "Not support");
			bRet = false;
			break;
		}
	}
	else if (type == 12)
	{
		switch (model)
		{
		case 1:
			strcpy(cModel, "RDIMM");
			bRet = true;
			break;
		case 2:
			strcpy(cModel, "UDIMM");
			bRet = true;
			break;
		case 3:
			strcpy(cModel, "SO-DIMM");
			bRet = true;
			break;
		case 4:
			strcpy(cModel, "LRDIMM");
			bRet = true;
			break;
		case 5:
			strcpy(cModel, "Mini-RDIMM");
			bRet = true;
			break;
		case 6:
			strcpy(cModel, "Mini-UDIMM");
			bRet = true;
			break;
		case 8:
			strcpy(cModel, "72b-SO-RDIMM");
			bRet = true;
			break;
		case 9:
			strcpy(cModel, "72b-SO-UDIMM");
			bRet = true;
			break;
		case 12:
			strcpy(cModel, "16b-SO-DIMM");
			bRet = true;
			break;
		case 13:
			strcpy(cModel, "32b-SO-DIMM");
			bRet = true;
			break;
		default:
			strcpy(cModel, "Not support");
			bRet = false;
			break;
		}
	}
	else
	{
		strcpy(cModel, "Not support");
		bRet = false;
	}
	
	return bRet;
}

bool GetRAMSN(int index, char* cSerialNum)
{
	bool bRet = false;
	char sn[5][4] = { 0 };
	int i = 0;
	if (cSerialNum == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_PARTNUMBER1(index), (uint32_t*)&sn[0]) == SUSI_STATUS_SUCCESS;
		bRet |= pSusiDeviceGetValue(SPD_ID_DRAM_PARTNUMBER2(index), (uint32_t*)&sn[1]) == SUSI_STATUS_SUCCESS;
		bRet |= pSusiDeviceGetValue(SPD_ID_DRAM_PARTNUMBER3(index), (uint32_t*)&sn[2]) == SUSI_STATUS_SUCCESS;
		bRet |= pSusiDeviceGetValue(SPD_ID_DRAM_PARTNUMBER4(index), (uint32_t*)&sn[3]) == SUSI_STATUS_SUCCESS;
		bRet |= pSusiDeviceGetValue(SPD_ID_DRAM_PARTNUMBER5(index), (uint32_t*)&sn[4]) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;

	sprintf(cSerialNum, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
		sn[0][3],
		sn[0][2],
		sn[0][1],
		sn[0][0],
		sn[1][3],
		sn[1][2],
		sn[1][1],
		sn[1][0],
		sn[2][3],
		sn[2][2],
		sn[2][1],
		sn[2][0],
		sn[3][3],
		sn[3][2],
		sn[3][1],
		sn[3][0],
		sn[4][3],
		sn[4][2],
		sn[4][1],
		sn[4][0]);

	return bRet;
}

bool GetRAMTemperature(int index, double* temperature)
{
	bool bRet = false;
	unsigned int iTemper = 0;
	int i = 0;
	if (temperature == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_TEMPERATURE(index), & iTemper) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;

	*temperature = (double)iTemper / 100 - 273.15;

	return bRet;
}

bool GetRAMWriteProtect(int index, bool *bEnable)
{
	bool bRet = false;
	unsigned int status = 0;
	int i = 0;
	if (bEnable == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_WRITEPROTECTION(index), &status) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;

	*bEnable = status == 0 ? false : true;

	return bRet;
}

bool GetRAMCapacity(int index, unsigned int* size)
{ 
	//Unit: GB
	bool bRet = false;
	unsigned int status = 0;
	int i = 0;
	if (size == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_SIZE(index), &status) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;

	*size = status;

	return bRet;
}

bool GetRAMSpeed(int index, unsigned int* speed)
{
	bool bRet = false;
	unsigned int status = 0;
	int i = 0;
	if (speed == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_SPEED(index), &status) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;

	*speed = status;

	return bRet;
}

bool GetRAMRanks(int index, unsigned int* rank)
{
	bool bRet = false;
	unsigned int status = 0;
	int i = 0;
	if (rank == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_RANK(index), &status) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;

	*rank = status;

	return bRet;
}

bool GetRAMVoltage(int index, double* voltage)
{
	bool bRet = false;
	unsigned int status = 0;
	int i = 0;
	if (voltage == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_VOLTAGE(index), &status) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;

	*voltage = (double)status / 1000;

	return bRet;
}

bool GetRAMBank(int index, unsigned int* bank)
{
	bool bRet = false;
	unsigned int status = 0;
	int i = 0;
	if (bank == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_BANK(index), &status) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;

	*bank = status;

	return bRet;
}

bool GetRAMWeekYear(int index, char* week, char* year)
{
	bool bRet = false;
	unsigned int status = 0;
	int i = 0;
	if (week == NULL || year == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_WEEKYEAR(index), &status) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;

	sprintf(week, "%02x", status >> 8 & 0xFF);
	sprintf(year, "%02x", status & 0xFF);
	return bRet;
}

bool GetRAMManufacturer(int index, char* manufacturer)
{
	bool bRet = false;
	unsigned int status = 0;
	int i = 0;
	if (manufacturer == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_MANUFACTURE(index), &status) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;
	if (status == 0xC88A)
	{
		strcpy(manufacturer, "Advantech");
	}
	else
	{
		strcpy(manufacturer, "Not Support");
	}
	return bRet;
}

bool GetRAMICVendor(int index, char* icvendor)
{
	bool bRet = false;
	unsigned int status = 0;
	int i = 0;
	if (icvendor == NULL)
		return bRet;

	if (pSusiDeviceGetValue)
	{
		bRet = pSusiDeviceGetValue(SPD_ID_DRAM_DRAMIC(index), &status) == SUSI_STATUS_SUCCESS;
	}
	if (!bRet)
		return bRet;
	switch (status)
	{
	case 0xCE80:
		strcpy(icvendor, "Samsung");
		break;
	case 0xAD80:
		strcpy(icvendor, "HYNIX");
		break;
	case 0x2C80:
		strcpy(icvendor, "Micron");
		break;
	default:
		strcpy(icvendor, "Not Support");
		break;
	}
	return bRet;
}

int InsertRAMInfoNode(sqram_info_list ramInfoList, sqram_node_t* ramInfo)
{
	int iRet = -1;
	sqram_node_t* newNode = NULL, * head = NULL;
	if (ramInfoList == NULL || ramInfo == NULL)
		return iRet;
	head = ramInfoList;
	newNode = (sqram_node_t*)malloc(sizeof(sqram_node_t));
	memcpy(newNode, ramInfo, sizeof(sqram_node_t));
	newNode->next = head->next;
	head->next = newNode;
	iRet = 0;
	return iRet;
}

int DeleteRAMInfoNodeWithID(sqram_info_list ramInfoList, int index)
{
	int iRet = -1;
	sqram_node_t* delNode = NULL, * head = NULL;
	sqram_node_t* p = NULL;
	if (ramInfoList == NULL)
		return iRet;
	head = ramInfoList;
	p = head;
	delNode = head->next;
	while (delNode)
	{
		if (delNode->Index == index)
		{
			p->next = delNode->next;
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		p = delNode;
		delNode = delNode->next;
	}
	if (iRet == -1)
		iRet = 1;
	return iRet;
}

int DeleteAllRAMInfoNode(sqram_info_list ramInfoList)
{
	int iRet = -1;
	sqram_node_t* delNode = NULL, * head = NULL;
	if (ramInfoList == NULL)
		return iRet;
	head = ramInfoList;
	delNode = head->next;
	while (delNode)
	{
		head->next = delNode->next;
		free(delNode);
		delNode = head->next;
	}
	iRet = 0;
	return iRet;
}

bool RefreshRAMInfoList(sqram_info_list ramInfoList, int count)
{
	bool bRet = false;
	if (ramInfoList == NULL)
		return bRet;
	{
		int index = 0;
		sqram_node_t ramInfo;
		ram_node_list tmplist = wmi_GetMemorySpeed();//WMI get RAM Speed
		DeleteAllRAMInfoNode(ramInfoList);
		while (index < count)
		{
			int type = 0;
			unsigned int temp = 0;
			char week[3] = { 0 };
			char year[3] = { 0 };
			double dtmp = 0;
			char ramtype[16] = { 0 };
			char tmpDiskNameWcs[32] = { 0 };
			bool bWriteProtect = false;

			if(GetRAMType(index, &type, ramtype)) 
			{
				ramInfo.Index = index;
				ramInfo.ramType = type;
				strcpy(ramInfo.DDRType, ramtype);
			}
			else
			{
				continue;
			}

			if (GetRAMModel(index, type, tmpDiskNameWcs) && strlen(tmpDiskNameWcs) > 0)
			{
				strncpy(ramInfo.ModelType, tmpDiskNameWcs, sizeof(ramInfo.ModelType));
				TrimStr(ramInfo.ModelType);
			}
			else
			{
				continue;
			}
			
			memset(tmpDiskNameWcs, 0, sizeof(tmpDiskNameWcs));
			if (GetRAMSN(index, tmpDiskNameWcs) && strlen(tmpDiskNameWcs) > 0)
			{
				strncpy(ramInfo.SerialNumber, tmpDiskNameWcs, sizeof(ramInfo.SerialNumber));
			}
			else
			{
				continue;
			}

			if (GetRAMWriteProtect(index, &bWriteProtect))
			{
				ramInfo.SPDLock = bWriteProtect;
			}
			else
			{
				continue;
			}

			if (GetRAMSpeed(index, &temp))
			{
				ramInfo.ClockRate = temp;
			}

			ramInfo.Speed = wmi_FindRAMSpeed(tmplist, ramInfo.Index);

			if (GetRAMCapacity(index, &temp))
			{
				ramInfo.Capacity = temp;
			}

			if (GetRAMVoltage(index, &dtmp))
			{
				ramInfo.Voltage = dtmp;
			}

			if (GetRAMRanks(index, &temp))
			{
				ramInfo.Ranks = temp;
			}
			
			if (GetRAMBank(index, &temp))
			{
				ramInfo.Bank = temp;
			}

			if (GetRAMWeekYear(index, week, year))
			{
				strcpy(ramInfo.Week, week);
				strcpy(ramInfo.Year, year);
			}

			memset(ramtype, 0, sizeof(ramtype));
			if (GetRAMManufacturer(index, ramtype))
			{
				strcpy(ramInfo.Manufacturer, ramtype);
			}

			memset(ramtype, 0, sizeof(ramtype));
			if (GetRAMICVendor(index, ramtype))
			{
				strcpy(ramInfo.ICVendor, ramtype);
			}

			InsertRAMInfoNode(ramInfoList, &ramInfo);

			index++;
		}
		wmi_ReleaseRAMList(tmplist);
	}
	return bRet;
}

void sqr_UpdateSQRAM(sqram_info_t* ramInfo, MSG_CLASSIFY_T* myCapability)
{
	MSG_CLASSIFY_T* sqramGroup = NULL;
	if (myCapability == NULL)
		return;
	sqramGroup = IoT_FindGroup(myCapability, SQR_RAM_LIST);
	if (sqramGroup == NULL)
		sqramGroup = IoT_AddGroupArray(myCapability, SQR_RAM_LIST);

	if (ramInfo && ramInfo->sqramInfoList && sqramGroup)
	{
		sqram_node_t* item = ramInfo->sqramInfoList->next;
		while (item != NULL)
		{
			char cTemp[128] = { 0 };
			MSG_CLASSIFY_T* indexGroup = NULL;
			MSG_ATTRIBUTE_T* attr = NULL;
			if (strstr(item->SerialNumber, "SQR-") == 0) //Filter SQRAM
			{
				item = item->next;
				continue;
			}
			memset(cTemp, 0, sizeof(cTemp));
			sprintf(cTemp, "SQRAM%d", item->Index);
			indexGroup = IoT_FindGroup(sqramGroup, cTemp);
			if (indexGroup == NULL)
				indexGroup = IoT_AddGroup(sqramGroup, cTemp);

			attr = IoT_FindSensorNode(indexGroup, SQR_INDEX);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_INDEX);
			IoT_SetDoubleValue(attr, item->Index, IoT_READONLY, NULL);

			if (strlen(item->ModelType) > 0)
			{
				attr = IoT_FindSensorNode(indexGroup, SQR_PRODUCT_NAME);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, SQR_PRODUCT_NAME);
				IoT_SetStringValue(attr, item->ModelType, IoT_READONLY);
			}

			if (strlen(item->DDRType) > 0)
			{
				attr = IoT_FindSensorNode(indexGroup, SQR_PRODUCT_TYPE);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, SQR_PRODUCT_TYPE);
				IoT_SetStringValue(attr, item->DDRType, IoT_READONLY);
			}

			if (strlen(item->SerialNumber) > 0)
			{
				attr = IoT_FindSensorNode(indexGroup, SQR_SERIAL_NUMBER);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, SQR_SERIAL_NUMBER);
				IoT_SetStringValue(attr, item->SerialNumber, IoT_READONLY);
			}

			attr = IoT_FindSensorNode(indexGroup, SQR_TEMPERATURE);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_TEMPERATURE);
			IoT_SetDoubleValue(attr, item->Temperature, IoT_READONLY, SENSOR_UNIT_CELSIUS);

			attr = IoT_FindSensorNode(indexGroup, SQR_CLOCK_DEFINED);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_CLOCK_DEFINED);
			IoT_SetDoubleValue(attr, item->ClockRate, IoT_READONLY, SENSOR_UNIT_MEGAHERTZ);

			attr = IoT_FindSensorNode(indexGroup, SQR_CLOCK_RATE);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_CLOCK_RATE);
			IoT_SetDoubleValue(attr, item->Speed, IoT_READONLY, SENSOR_UNIT_MEGAHERTZ);

			attr = IoT_FindSensorNode(indexGroup, SQR_CAPACITY);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_CAPACITY);
			IoT_SetDoubleValue(attr, item->Capacity, IoT_READONLY, SENSOR_UNIT_GIGABYTE);

			attr = IoT_FindSensorNode(indexGroup, SQR_VOLTAGE);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_VOLTAGE);
			IoT_SetDoubleValue(attr, item->Voltage, IoT_READONLY, SENSOR_UNIT_VOLTAGE);

			attr = IoT_FindSensorNode(indexGroup, SQR_RANKS);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_RANKS);
			IoT_SetDoubleValue(attr, item->Ranks, IoT_READONLY, NULL);

			attr = IoT_FindSensorNode(indexGroup, SQR_BANK);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_BANK);
			IoT_SetDoubleValue(attr, item->Bank, IoT_READONLY, NULL);

			{
				char weekyear[8] = { 0 };
				sprintf(weekyear, "%s-%s", item->Week, item->Year);
				attr = IoT_FindSensorNode(indexGroup, SQR_MANUFACTURE_DATE);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, SQR_MANUFACTURE_DATE);
				IoT_SetStringValue(attr, weekyear, IoT_READONLY);
			}

			attr = IoT_FindSensorNode(indexGroup, SQR_MANUFACTURER);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_MANUFACTURER);
			IoT_SetStringValue(attr, item->Manufacturer, IoT_READONLY);

			attr = IoT_FindSensorNode(indexGroup, SQR_IC_VENDOR);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_IC_VENDOR);
			IoT_SetStringValue(attr, item->ICVendor, IoT_READONLY);
			
			attr = IoT_FindSensorNode(indexGroup, SQR_WRITE_PROTECT);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, SQR_WRITE_PROTECT);
			IoT_SetBoolValue(attr, item->SPDLock, IoT_READONLY);

			item = item->next;
		} // while End
	}
}

sqram_info_list sqr_CreateSQRAMInfoList()
{
	sqram_node_t* head = NULL;
	head = (sqram_node_t*)malloc(sizeof(sqram_node_t));
	memset(head, 0, sizeof(sqram_node_t));
	if (head)
	{
		head->next = NULL;
		head->Index = INVALID_DEVMON_VALUE;
		head->Temperature = INVALID_DEVMON_VALUE;
		head->Speed = INVALID_DEVMON_VALUE;
		head->SPDLock = false;
		head->ClockRate = INVALID_DEVMON_VALUE;
		strcpy(head->DDRType, "Not support");
	}
	return head;
}

void sqr_DestroySQRAMInfo(sqram_info_list ramInfoList)
{
	if (NULL == ramInfoList)
		return;
	DeleteAllRAMInfoNode(ramInfoList);
	free(ramInfoList);
	ramInfoList = NULL;
}

bool sqr_GetSQRAMInfo(sqram_info_t* pRamInfo)
{
	bool bRet = false;
	int curRAMCnt = 0;
	sqram_node_t* curRAMInfoNode = NULL;
	if (!pRamInfo)
		return bRet;

	GetRAMCount(&curRAMCnt);
	if ((curRAMCnt == 0) || (curRAMCnt != pRamInfo->count))
	{
		if (pRamInfo->sqramInfoList == NULL)
		{
			pRamInfo->sqramInfoList = sqr_CreateSQRAMInfoList();
		}
		if (curRAMCnt == 0)
		{
			bRet = true;
			return bRet;
		}
		RefreshRAMInfoList(pRamInfo->sqramInfoList, curRAMCnt);
		pRamInfo->count = curRAMCnt;
	}

	curRAMInfoNode = pRamInfo->sqramInfoList->next;
	while (curRAMInfoNode)
	{
		int curRAMIndex = curRAMInfoNode->Index;
		int curRAMType = curRAMInfoNode->ramType;
		double temp = 0;
		if (GetRAMTemperature(curRAMIndex, &temp))
		{
			curRAMInfoNode->Temperature = temp;
		}
		
		curRAMInfoNode = curRAMInfoNode->next;
	}
	bRet = true;
	return bRet;
}