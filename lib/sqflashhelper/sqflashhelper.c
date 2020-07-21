#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "WISEPlatform.h"
#include "sqflashhelper.h"
#include "util_string.h"
#include "util_path.h"

#define MAX_HDD_COUNT 32

bool GetHDDHealth(hdd_mon_info_t *hddMonInfo);
bool GetAttriNameFromType(hdd_type_t hdd_type, smart_attri_type_t attriType, char *attriName);

smart_attri_info_node_t *FindSmartAttriInfoNode(smart_attri_info_list attriInfoList, char *attriName);
smart_attri_info_node_t *FindSmartAttriInfoNodeWithType(smart_attri_info_list attriInfoList, smart_attri_type_t attriType);
smart_attri_info_list CreateSmartAttriInfoList();
int InsertSmartAttriInfoNode(smart_attri_info_list attriInfoList, smart_attri_info_t *attriInfo);
int InsertSmartAttriInfoNodeEx(smart_attri_info_list attriInfoList, smart_attri_info_t *attriInfo);
int UpdateSmartAttriInfoNode(smart_attri_info_list attriInfoList, smart_attri_info_t *attriInfo);
int DeleteSmartAttriInfoNode(smart_attri_info_list attriInfoList, char *attriName);
int DeleteSmartAttriInfoNodeWithID(smart_attri_info_list attriInfoList, smart_attri_type_t attriType);
int DeleteAllSmartAttriInfoNode(smart_attri_info_list attriInfoList);
void DestroySmartAttriInfoList(smart_attri_info_list attriInfoList);
bool IsSmartAttriInfoListEmpty(smart_attri_info_list attriInfoList);

int InsertNVMeAttriInfoNode(smart_attri_info_list attriInfoList, nvme_attri_info_t *attriInfo);
int UpdateNVMeAttriInfoNode(smart_attri_info_list attriInfoList, nvme_attri_info_t *attriInfo);
bool GetSQNVMeAttriNameFromType(nvme_smart_attri_type_t attriType, char *attriName);

hdd_mon_info_node_t *FindHDDInfoNode(hdd_mon_info_list hddInfoList, char *hddName);
hdd_mon_info_node_t *FindHDDInfoNodeWithID(hdd_mon_info_list hddInfoList, int hddIndex);
hdd_mon_info_list hdd_CreateHDDInfoList();
int InsertHDDInfoNode(hdd_mon_info_list hddInfoList, hdd_mon_info_t *hddMonInfo);
int InsertHDDInfoNodeEx(hdd_mon_info_list hddInfoList, hdd_mon_info_t *hddMonInfo);
int DeleteHDDInfoNode(hdd_mon_info_list hddInfoList, char *hddName);
int DeleteHDDInfoNodeWithID(hdd_mon_info_list hddInfoList, int hddIndex);
int DeleteAllHDDInfoNode(hdd_mon_info_list hddInfoList);
void hdd_DestroyHDDInfoList(hdd_mon_info_list hddInfoList);
bool IsHddInfoListEmpty(hdd_mon_info_list hddInfoList);
#ifdef WIN32
//-----------------------------------Crystal Disk Info------------------------------------------
#include "SimpleSmart.h"
SSMART_T m_Ata = NULL;
int g_iDevID = 0;
bool hdd_Iinit()
{
	if (m_Ata != NULL)
		return true;

	m_Ata = SSMART_Init();
	return true;
}

void hdd_Uniinit()
{
	if (m_Ata == NULL)
		return;

	SSMART_Uninit(m_Ata);
	m_Ata = NULL;
}

int hdd_GetCount()
{
	int count = 0;
	if (m_Ata == NULL)
		return count;
	count = SSMART_GetDeviceCount(m_Ata);
	return count;
}

bool hdd_GetModelName(char *pBuff)
{
	bool bRet = false;
	if (m_Ata == NULL)
		return bRet;
	bRet = SSMART_GetDeviceModelName(m_Ata, g_iDevID, pBuff);

	return bRet;
}
bool hdd_UpdateSMARTInfo()
{
	bool bRet = false;
	if (m_Ata == NULL)
		return bRet;
	bRet = SSMART_UpdateSmartInfo(m_Ata, g_iDevID);
	return bRet;
}

bool hdd_GetSMARTAttribute(char *pBuff)
{
	bool bRet = false;
	if (m_Ata == NULL)
		return bRet;
	bRet = SSMART_GetSmartInfo(m_Ata, g_iDevID, pBuff);
	return bRet;
}

bool hdd_GetFirmwareRevision(char *pBuff)
{
	bool bRet = false;
	if (m_Ata == NULL)
		return bRet;
	bRet = SSMART_GetFirmwareRevision(m_Ata, g_iDevID, pBuff);
	return bRet;
}

bool hdd_GetSerialNumber(char *pBuff)
{
	bool bRet = false;
	if (m_Ata == NULL)
		return bRet;
	bRet = SSMART_GetSerialNumber(m_Ata, g_iDevID, pBuff);
	return bRet;
}
//---------------------------------SQFlash lib data define--------------------------------------
#define DEF_SQFLASH_LIB_NAME "SQFlash.dll"
typedef int (*PSQFlash_ReadVendorSMARTAttributes)(int index, PVSMARTAttr_t Attr, int AttrSize);
typedef int (*PSQFlash_GetSerialNumber)(int index, char *SerialNumberStr);
typedef int (*PSQFlash_NVMe_StdSMART)(int index, PNVMeStdSMART_t Attr);
typedef int (*PSQFlash_NVMe_VendorSMART)(int index, PNVMeVenSMART_t Attr);
typedef int (*PSQFlash_NVMe_StdSMARTRaw)(int index, uint8_t stdSmartRaw[512]);
typedef int (*PSQFlash_NVMe_VendorSMARTRaw)(int index, uint8_t vendorSmartRaw[512]);
typedef int (*PSQFlash_GetFirmwareRevision)(int index, char *FirmwareRevisionStr);
typedef int (*PSQFlash_ReadParsedSMARTAttribute)(int index, PSATASMART_t sataSmart);
typedef int (*PSQFlash_GetInformation)(int index, uint32_t infoType, uint32_t *result);
typedef int (*PSQFlash_ExecuteSMARTSelfTest)(int index, unsigned char Subcommand);
typedef int (*PSQFlash_ReadSMARTSelfTestStatus)(int index, uint8_t* progress, uint8_t* result);

void *hSQFlashDll = NULL;
PSQFlash_ReadVendorSMARTAttributes pSQFlash_ReadVendorSMARTAttributes = NULL;
PSQFlash_GetSerialNumber pSQFlash_GetSerialNumber = NULL;
PSQFlash_NVMe_StdSMART pSQFlash_NVMe_StdSMART = NULL;
PSQFlash_NVMe_VendorSMART pSQFlash_NVMe_VendorSMART = NULL;
PSQFlash_NVMe_StdSMARTRaw pSQFlash_NVMe_StdSMARTRaw = NULL;
PSQFlash_NVMe_VendorSMARTRaw pSQFlash_NVMe_VendorSMARTRaw = NULL;
PSQFlash_GetFirmwareRevision pSQFlash_GetFirmwareRevision = NULL;
PSQFlash_ReadParsedSMARTAttribute pSQFlash_ReadParsedSMARTAttribute = NULL;
PSQFlash_GetInformation pSQFlash_GetInformation = NULL;
PSQFlash_ExecuteSMARTSelfTest pSQFlash_ExecuteSMARTSelfTest = NULL;
PSQFlash_ReadSMARTSelfTestStatus pSQFlash_ReadSMARTSelfTestStatus = NULL;

bool hdd_IsExistSQFlashLib()
{
	bool bRet = false;
	void *hSQFlash = NULL;
	bRet = hdd_Iinit();
	if (!bRet)
		return bRet;

	hSQFlash = LoadLibrary(DEF_SQFLASH_LIB_NAME);
	if (hSQFlash != NULL)
	{
		bRet = true;
		FreeLibrary(hSQFlash);
		hSQFlash = NULL;
	}
	return bRet;
}

void GetSQFlashFunction(void *hSQFlashDll)
{
	if (hSQFlashDll != NULL)
	{
		pSQFlash_ReadVendorSMARTAttributes = (PSQFlash_ReadVendorSMARTAttributes)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_ReadVendorSMARTAttributes");
		pSQFlash_GetSerialNumber = (PSQFlash_GetSerialNumber)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_GetSerialNumber");
		pSQFlash_NVMe_StdSMART = (PSQFlash_NVMe_StdSMART)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_NVMe_StdSMART");
		pSQFlash_NVMe_VendorSMART = (PSQFlash_NVMe_VendorSMART)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_NVMe_VendorSMART");
		pSQFlash_NVMe_StdSMARTRaw = (PSQFlash_NVMe_StdSMARTRaw)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_NVMe_StdSMARTRaw");
		pSQFlash_NVMe_VendorSMARTRaw = (PSQFlash_NVMe_VendorSMARTRaw)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_NVMe_VendorSMARTRaw");
		pSQFlash_GetFirmwareRevision = (PSQFlash_GetFirmwareRevision)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_GetFirmwareRevision");
		pSQFlash_ReadParsedSMARTAttribute = (PSQFlash_ReadParsedSMARTAttribute)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_ReadParsedSMARTAttribute");
		pSQFlash_GetInformation = (PSQFlash_GetInformation)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_GetInformation");
		pSQFlash_ExecuteSMARTSelfTest = (PSQFlash_ExecuteSMARTSelfTest)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_ExecuteSMARTSelfTest");
		pSQFlash_ReadSMARTSelfTestStatus = (PSQFlash_ReadSMARTSelfTestStatus)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_ReadSMARTSelfTestStatus");
	}
}

bool hdd_StartupSQFlashLib()
{
	bool bRet = false;
	bRet = hdd_Iinit();
	if (!bRet)
		return bRet;
	hSQFlashDll = LoadLibrary(DEF_SQFLASH_LIB_NAME);
	if (hSQFlashDll != NULL)
	{
		GetSQFlashFunction(hSQFlashDll);
		bRet = true;
	}
	return bRet;
}

bool hdd_CleanupSQFlashLib()
{
	bool bRet = true;

	if (hSQFlashDll != NULL)
	{
		FreeLibrary(hSQFlashDll);
		hSQFlashDll = NULL;
		pSQFlash_ReadVendorSMARTAttributes = NULL;
		pSQFlash_NVMe_StdSMART = NULL;
		pSQFlash_NVMe_VendorSMART = NULL;
		pSQFlash_NVMe_StdSMARTRaw = NULL;
		pSQFlash_NVMe_VendorSMARTRaw = NULL;
		pSQFlash_GetSerialNumber = NULL;
		pSQFlash_GetFirmwareRevision = NULL;
		pSQFlash_ReadParsedSMARTAttribute = NULL;
		pSQFlash_GetInformation = NULL;
		pSQFlash_ExecuteSMARTSelfTest = NULL;
		pSQFlash_ReadSMARTSelfTestStatus = NULL;
	}

	hdd_Uniinit();

	return bRet;
}

bool SelectDevice(int deveNum)
{
	bool bRet = true;
	g_iDevID = deveNum;
	return bRet;
}

bool GetDeviceModelName(char *pModelNameBuf, unsigned int len)
{
	bool bRet = false;
	bRet = hdd_GetModelName(pModelNameBuf);
	return bRet;
}

bool CheckAccessCode(char *pAccessCode)
{
	bool bRet = true;
	return bRet;
}

bool GetSmartAttribute(PExtSMARTAttr_t pASAT)
{
	bool bRet = false;
	if (pSQFlash_ReadParsedSMARTAttribute)
	{
		SATASMART_t venSMART = {0};
		bRet = pSQFlash_ReadParsedSMARTAttribute(g_iDevID, &venSMART) == SQF_SUCCESS;
		if (bRet)
		{
			pASAT->UncorrectableECCCnt = venSMART.UncorrectableECCCnt;
			pASAT->PowerOnHours = venSMART.PowerOnHours;
			pASAT->PowerCycleCnt = venSMART.PowerCycleCnt;
			pASAT->DeviceCapacity = venSMART.DeviceCapacity;
			pASAT->UserCapacity = venSMART.UserCapacity;
			pASAT->TotalAvailSpareBlk = venSMART.TotalAvailSpareBlk;
			pASAT->RemainingSpareBlk = venSMART.RemainingSpareBlk;

			pASAT->TotalEraseCnt = venSMART.TotalEraseCnt;
			pASAT->PHYErrCnt = venSMART.PHYErrCnt;
			pASAT->LaterBadBlkCnt = venSMART.LaterBadBlkCnt;
			pASAT->EarlyBadBlkCnt = venSMART.EarlyBadBlkCnt;
			pASAT->AvgEraseCnt = venSMART.AvgEraseCnt;

			pASAT->MaxEraseCnt = venSMART.MaxEraseCnt;
			pASAT->UnexpectedPwrLostCnt = venSMART.UnexpectedPwrLostCnt;
			pASAT->VolStabilizerTriggerCnt = venSMART.VolStabilizerTriggerCnt;
			pASAT->GuaranteedFlush = venSMART.GuaranteedFlush;
			pASAT->DriveStatus = venSMART.DriveStatus;

			pASAT->UnexpectedPwrLostCnt2 = venSMART.UnexpectedPwrLostCnt2;
			pASAT->MaxTemperature = venSMART.MaxTemperature;
			pASAT->MinTemperature = venSMART.MinTemperature;
			pASAT->CurTemperature = venSMART.CurTemperature;
			pASAT->SSDLifeUsed = venSMART.SSDLifeUsed;

			pASAT->CRCErrCnt = venSMART.CRCErrCnt;
			pASAT->SSDLifeLeft = venSMART.SSDLifeLeft;

			pASAT->TotalNANDRead = venSMART.TotalNANDRead;
			pASAT->TotalNANDWritten = venSMART.TotalNANDWritten;
			pASAT->HostWrite = venSMART.HostWrite;
			pASAT->HostRead = venSMART.HostRead;
		}
	}
	else if (pSQFlash_ReadVendorSMARTAttributes) /*Old SQFlash Structure*/
	{
		VSMARTAttr_t venSMART = {0};
		bRet = pSQFlash_ReadVendorSMARTAttributes(g_iDevID, &venSMART, sizeof(VSMARTAttr_t)) == SQF_SUCCESS;
		if (bRet)
		{
			pASAT->MaxEraseCnt = venSMART.MaxProgram;
			pASAT->AvgEraseCnt = venSMART.AverageProgram;
			pASAT->SSDLifeLeft = venSMART.EnduranceCheck;
			pASAT->PowerOnHours = venSMART.PowerOnTime;
			pASAT->UncorrectableECCCnt = venSMART.EccCount;
			pASAT->MaxReservedBlock = venSMART.MaxReservedBlock;
			pASAT->CurrentReservedBlock = venSMART.CurrentReservedBlock;
			pASAT->GoodBlockRate = venSMART.GoodBlockRate;
		}
	}
	return bRet;
}

bool GetNVMeSmartAttribute(PExtNVMeSMART_t pASAT)
{
	bool bRet = false;

	if (pASAT == NULL)
		return bRet;

	if (pSQFlash_NVMe_StdSMART)
	{
		NVMeStdSMART_t stdSMART = {0};
		bRet = pSQFlash_NVMe_StdSMART(g_iDevID, &stdSMART) == SQF_SUCCESS;
		if (bRet)
		{
			pASAT->CriticalWarning = stdSMART.CriticalWarning;													 // 0x01
			pASAT->CompositeTemperature = stdSMART.CompositeTemperature;										 // 0x02
			pASAT->AvailableSpare = stdSMART.AvailableSpare;													 // 0x03
			pASAT->AvailableSpareThreshold = stdSMART.AvailableSpareThreshold;									 // 0x04
			pASAT->PercentageUsed = stdSMART.PercentageUsed;													 // 0x05
			memcpy(pASAT->DataUnitsRead, stdSMART.DataUnitsRead, sizeof(stdSMART.DataUnitsRead));				 // 0x07
			memcpy(pASAT->DataUnitsWritten, stdSMART.DataUnitsWritten, sizeof(stdSMART.DataUnitsWritten));		 // 0x08
			memcpy(pASAT->HostReadCommands, stdSMART.HostReadCommands, sizeof(stdSMART.HostReadCommands));		 // 0x09
			memcpy(pASAT->HostWriteCommands, stdSMART.HostWriteCommands, sizeof(stdSMART.HostWriteCommands));	 // 0x0A
			memcpy(pASAT->ControllerBusyTime, stdSMART.ControllerBusyTime, sizeof(stdSMART.ControllerBusyTime)); // 0x0B
			memcpy(pASAT->PowerCycles, stdSMART.PowerCycles, sizeof(stdSMART.PowerCycles));						 // 0x0C
			memcpy(pASAT->PowerOnHours, stdSMART.PowerOnHours, sizeof(stdSMART.PowerOnHours));					 // 0x0D
			memcpy(pASAT->UnsafeShutdowns, stdSMART.UnsafeShutdowns, sizeof(stdSMART.UnsafeShutdowns));			 // 0x0E
			memcpy(pASAT->MediaErrors, stdSMART.MediaErrors, sizeof(stdSMART.MediaErrors));						 // 0x0F
			memcpy(pASAT->ErrorLogNumber, stdSMART.ErrorLogNumber, sizeof(stdSMART.ErrorLogNumber));			 // 0x10
			pASAT->WarningCompositeTemperatureTime = stdSMART.WarningCompositeTemperatureTime;					 // 0x11
			pASAT->CriticalCompositeTemperatureTime = stdSMART.CriticalCompositeTemperatureTime;				 // 0x12
			pASAT->TemperatureSensor1 = stdSMART.TemperatureSensor1;											 // 0x13
			pASAT->TemperatureSensor2 = stdSMART.TemperatureSensor2;											 // 0x14
			pASAT->TemperatureSensor3 = stdSMART.TemperatureSensor3;											 // 0x15
			pASAT->TemperatureSensor4 = stdSMART.TemperatureSensor4;											 // 0x16
			pASAT->TemperatureSensor5 = stdSMART.TemperatureSensor5;											 // 0x17
			pASAT->TemperatureSensor6 = stdSMART.TemperatureSensor6;											 // 0x18
			pASAT->TemperatureSensor7 = stdSMART.TemperatureSensor7;											 // 0x19
			pASAT->TemperatureSensor8 = stdSMART.TemperatureSensor8;											 // 0x1A
		}
		else
			return bRet;
	}

	if (pSQFlash_NVMe_VendorSMART)
	{
		NVMeVenSMART_t venSMART = {0};
		bRet = pSQFlash_NVMe_VendorSMART(g_iDevID, &venSMART) == SQF_SUCCESS;
		if (bRet)
		{
			memcpy(pASAT->FlashReadSector, venSMART.FlashReadSector, sizeof(venSMART.FlashReadSector));	   // 0x1C
			memcpy(pASAT->FlashWriteSector, venSMART.FlashWriteSector, sizeof(venSMART.FlashWriteSector)); // 0x1D
			memcpy(pASAT->UNCError, venSMART.UNCError, sizeof(venSMART.UNCError));						   // 0x1E
			pASAT->PyhError = venSMART.PyhError;														   // 0x1F
			pASAT->EarlyBadBlock = venSMART.EarlyBadBlock;												   // 0x20
			pASAT->LaterBadBlock = venSMART.LaterBadBlock;												   // 0x21
			pASAT->MaxEraseCount = venSMART.MaxEraseCount;												   // 0x22
			pASAT->AvgEraseCount = venSMART.AvgEraseCount;												   // 0x23
			memcpy(pASAT->CurPercentSpares, venSMART.CurPercentSpares, sizeof(venSMART.CurPercentSpares)); // 0x24
			pASAT->CurTemperature = venSMART.CurTemperature;											   // 0x25
			pASAT->LowestTemperature = venSMART.LowestTemperature;										   // 0x26, K
			pASAT->HighestTemperature = venSMART.HighestTemperature;									   // 0x27, K
			pASAT->ChipInternalTemperature = venSMART.ChipInternalTemperature;							   // 0x28, K
			pASAT->SpareBlocks = venSMART.SpareBlocks;													   // 0x29
		}
		else
			return bRet;
	}
	//if (pSQFlash_NVMe_StdSMARTRaw)
	//{
	//	bRet = pSQFlash_NVMe_StdSMARTRaw(g_iDevID, pASAT->smartAttriStd) == SQF_SUCCESS;
	//	if (!bRet)
	//		return bRet;
	//}
	//if (pSQFlash_NVMe_VendorSMARTRaw)
	//{
	//	bRet = pSQFlash_NVMe_VendorSMARTRaw(g_iDevID, pASAT->smartAttriVen) == SQF_SUCCESS;
	//	if (!bRet)
	//		return bRet;
	//}
	return bRet;
}

bool GetSerialNumber(char *SerialNumberStr)
{
	bool bRet = false;
	if (pSQFlash_GetSerialNumber)
	{
		bRet = pSQFlash_GetSerialNumber(g_iDevID, SerialNumberStr) == SQF_SUCCESS;
	}
	return bRet;
}

bool GetFirmwareRevision(char *FirmwareRevisionStr)
{
	bool bRet = false;
	if (pSQFlash_GetFirmwareRevision)
	{
		bRet = pSQFlash_GetFirmwareRevision(g_iDevID, FirmwareRevisionStr) == SQF_SUCCESS;
	}
	return bRet;
}

bool GetSQFEndurance(unsigned int *endurance)
{
	bool bRet = false;
	if (pSQFlash_GetInformation)
	{
		bRet = pSQFlash_GetInformation(g_iDevID, SQF_INFO_TYPE_ENDURANCE, endurance) == SQF_SUCCESS;
	}
	return bRet;
}

bool GetSQFOPALSTATUS(unsigned int *status)
{
	bool bRet = false;
	if (pSQFlash_GetInformation)
	{
		bRet = pSQFlash_GetInformation(g_iDevID, SQF_INFO_TYPE_OPALSTATUS, status) == SQF_SUCCESS;
	}
	return bRet;
}

bool SQFSelfTest(int devID)
{
	bool bRet = false;
	unsigned char Subcommand = SMART_SHORT_SELF_TEST_OFFLINE_MODE;
	if(pSQFlash_ExecuteSMARTSelfTest)
		bRet = pSQFlash_ExecuteSMARTSelfTest(devID, Subcommand) == SQF_SUCCESS;
	return bRet;
}

bool GetSQFSelfTestStatus(int devID, unsigned int *progress, unsigned int *status)
{
	bool bRet = false;
	if(pSQFlash_ReadSMARTSelfTestStatus)
		bRet = pSQFlash_ReadSMARTSelfTestStatus(devID, progress, status) == SQF_SUCCESS;
	return bRet;
}

bool GetSmartAttributeStd(char *pBuf, unsigned int len)
{
	bool bRet = hdd_GetSMARTAttribute(pBuf);
	return bRet;
}

bool GetHDDCount(int *pHDDCount)
{
	bool bRet = false;
	int hddCount = hdd_GetCount();
	bRet = true;
	*pHDDCount = hddCount;
	return bRet;
}

bool HDDSetAttriInfo(hdd_mon_info_t *hddMonInfo, char *attriBuf, int attriBufLen)
{
	bool bRet = false;
	if (hddMonInfo == NULL || attriBuf == NULL || attriBufLen <= 0)
		return bRet;
	if (hddMonInfo->hdd_type == SQFNVMe)
	{
		/*TODO: Need to verify */
		unsigned long long value = 0;
		PExtNVMeSMART_t ataSmartAttrTable = (PExtNVMeSMART_t)attriBuf;
		hddMonInfo->power_cycle = ((unsigned char)ataSmartAttrTable->PowerCycles[3] * 256 * 256 * 256) + ((unsigned char)ataSmartAttrTable->PowerCycles[2] * 256 * 256) +
								  ((unsigned char)ataSmartAttrTable->PowerCycles[1] * 256) + (unsigned char)ataSmartAttrTable->PowerCycles[0];
		hddMonInfo->unexpected_power_loss_count = ((unsigned char)ataSmartAttrTable->UnsafeShutdowns[3] * 256 * 256 * 256) + ((unsigned char)ataSmartAttrTable->UnsafeShutdowns[2] * 256 * 256) +
												  ((unsigned char)ataSmartAttrTable->UnsafeShutdowns[1] * 256) + (unsigned char)ataSmartAttrTable->UnsafeShutdowns[0];
		hddMonInfo->later_bad_block = ataSmartAttrTable->LaterBadBlock;
		hddMonInfo->power_on_time = ((unsigned char)ataSmartAttrTable->PowerOnHours[3] * 256 * 256 * 256) + ((unsigned char)ataSmartAttrTable->PowerOnHours[2] * 256 * 256) +
									((unsigned char)ataSmartAttrTable->PowerOnHours[1] * 256) + (unsigned char)ataSmartAttrTable->PowerOnHours[0];
		hddMonInfo->ecc_count = ((unsigned char)ataSmartAttrTable->UNCError[3] * 256 * 256 * 256) + ((unsigned char)ataSmartAttrTable->UNCError[2] * 256 * 256) +
								((unsigned char)ataSmartAttrTable->UNCError[1] * 256) + (unsigned char)ataSmartAttrTable->UNCError[0];
		hddMonInfo->crc_error = 0; /*NVMe not support.*/
		hddMonInfo->hdd_temp = ataSmartAttrTable->CompositeTemperature - 273.15;
		hddMonInfo->max_temp = ataSmartAttrTable->HighestTemperature - 273.15;
		//hddMonInfo->max_program = ataSmartAttrTable->MaxEraseCount;
		hddMonInfo->average_program = ataSmartAttrTable->AvgEraseCount;

		if (hddMonInfo->smartAttriInfoList == NULL)
		{
			hddMonInfo->smartAttriInfoList = CreateSmartAttriInfoList();
		}

		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, CriticalWarning, ataSmartAttrTable->CriticalWarning, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, CompositeTemperature, ataSmartAttrTable->CompositeTemperature, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, AvailableSpare, ataSmartAttrTable->AvailableSpare, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, AvailableSpareThreshold, ataSmartAttrTable->AvailableSpareThreshold, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, PercentageUsed, ataSmartAttrTable->PercentageUsed, 0, NULL);
		value = ((unsigned long long)ataSmartAttrTable->DataUnitsRead[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->DataUnitsRead[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->DataUnitsRead[1] * 256) + (unsigned long long)ataSmartAttrTable->DataUnitsRead[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, DataUnitsRead, value, 16, ataSmartAttrTable->DataUnitsRead);
		value = ((unsigned long long)ataSmartAttrTable->DataUnitsWritten[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->DataUnitsWritten[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->DataUnitsWritten[1] * 256) + (unsigned long long)ataSmartAttrTable->DataUnitsWritten[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, DataUnitsWritten, value, 16, ataSmartAttrTable->DataUnitsWritten);
		value = ((unsigned long long)ataSmartAttrTable->HostReadCommands[7] * 256 * 256 * 256 * 256 * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->HostReadCommands[6] * 256 * 256 * 256 * 256 * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->HostReadCommands[5] * 256 * 256 * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->HostReadCommands[4] * 256 * 256 * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->HostReadCommands[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->HostReadCommands[2] * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->HostReadCommands[1] * 256) + (unsigned long long)ataSmartAttrTable->HostReadCommands[0];
		hddMonInfo->host_read = value;
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, HostReadCommands, (long)value, 16, ataSmartAttrTable->HostReadCommands);
		value = ((unsigned long long)ataSmartAttrTable->HostWriteCommands[7] * 256 * 256 * 256 * 256 * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->HostWriteCommands[6] * 256 * 256 * 256 * 256 * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->HostWriteCommands[5] * 256 * 256 * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->HostWriteCommands[4] * 256 * 256 * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->HostWriteCommands[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->HostWriteCommands[2] * 256 * 256) + 
			((unsigned long long)ataSmartAttrTable->HostWriteCommands[1] * 256) + (unsigned long long)ataSmartAttrTable->HostWriteCommands[0];
		hddMonInfo->host_write = value;
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, HostWriteCommands, (long)value, 16, ataSmartAttrTable->HostWriteCommands);
		value = ((unsigned long long)ataSmartAttrTable->ControllerBusyTime[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->ControllerBusyTime[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->ControllerBusyTime[1] * 256) + (unsigned long long)ataSmartAttrTable->ControllerBusyTime[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, ControllerBusyTime, value, 16, ataSmartAttrTable->ControllerBusyTime);
		value = ((unsigned long long)ataSmartAttrTable->PowerCycles[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->PowerCycles[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->PowerCycles[1] * 256) + (unsigned long long)ataSmartAttrTable->PowerCycles[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, PowerCycles, value, 16, ataSmartAttrTable->PowerCycles);
		value = ((unsigned long long)ataSmartAttrTable->PowerOnHours[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->PowerOnHours[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->PowerOnHours[1] * 256) + (unsigned long long)ataSmartAttrTable->PowerOnHours[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, PowerOnHours, value, 16, ataSmartAttrTable->PowerOnHours);
		value = ((unsigned long long)ataSmartAttrTable->UnsafeShutdowns[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->UnsafeShutdowns[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->UnsafeShutdowns[1] * 256) + (unsigned long long)ataSmartAttrTable->UnsafeShutdowns[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, UnsafeShutdowns, value, 16, ataSmartAttrTable->UnsafeShutdowns);
		value = ((unsigned long long)ataSmartAttrTable->MediaErrors[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->MediaErrors[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->MediaErrors[1] * 256) + (unsigned long long)ataSmartAttrTable->MediaErrors[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, MediaErrors, value, 16, ataSmartAttrTable->MediaErrors);
		value = ((unsigned long long)ataSmartAttrTable->ErrorLogNumber[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->ErrorLogNumber[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->ErrorLogNumber[1] * 256) + (unsigned long long)ataSmartAttrTable->ErrorLogNumber[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, ErrorLogNumber, value, 16, ataSmartAttrTable->ErrorLogNumber);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, WarningCompositeTemperatureTime, ataSmartAttrTable->WarningCompositeTemperatureTime, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, CriticalCompositeTemperatureTime, ataSmartAttrTable->CriticalCompositeTemperatureTime, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, TemperatureSensor1, ataSmartAttrTable->TemperatureSensor1, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, TemperatureSensor2, ataSmartAttrTable->TemperatureSensor2, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, TemperatureSensor3, ataSmartAttrTable->TemperatureSensor3, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, TemperatureSensor4, ataSmartAttrTable->TemperatureSensor4, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, TemperatureSensor5, ataSmartAttrTable->TemperatureSensor5, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, TemperatureSensor6, ataSmartAttrTable->TemperatureSensor6, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, TemperatureSensor7, ataSmartAttrTable->TemperatureSensor7, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, TemperatureSensor8, ataSmartAttrTable->TemperatureSensor8, 0, NULL);
		value = ((unsigned long long)ataSmartAttrTable->FlashReadSector[7] * 256 * 256 * 256 * 256 * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->FlashReadSector[6] * 256 * 256 * 256 * 256 * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->FlashReadSector[5] * 256 * 256 * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->FlashReadSector[4] * 256 * 256 * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->FlashReadSector[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->FlashReadSector[2] * 256 * 256) + 
			((unsigned long long)ataSmartAttrTable->FlashReadSector[1] * 256) + (unsigned long long)ataSmartAttrTable->FlashReadSector[0];
		//hddMonInfo->nand_read = value;
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, FlashReadSector, (long)value, 8, ataSmartAttrTable->FlashReadSector);
		value = ((unsigned long long)ataSmartAttrTable->FlashWriteSector[7] * 256 * 256 * 256 * 256 * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->FlashWriteSector[6] * 256 * 256 * 256 * 256 * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->FlashWriteSector[5] * 256 * 256 * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->FlashWriteSector[4] * 256 * 256 * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->FlashWriteSector[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->FlashWriteSector[2] * 256 * 256) +
			((unsigned long long)ataSmartAttrTable->FlashWriteSector[1] * 256) + (unsigned long long)ataSmartAttrTable->FlashWriteSector[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, FlashWriteSector, (long)value, 8, ataSmartAttrTable->FlashWriteSector);
		value = ((unsigned long long)ataSmartAttrTable->UNCError[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->UNCError[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->UNCError[1] * 256) + (unsigned long long)ataSmartAttrTable->UNCError[0];
		//hddMonInfo->nand_write = value;
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, UNCError, value, 8, ataSmartAttrTable->UNCError);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, PyhError, ataSmartAttrTable->PyhError, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, EarlyBadBlock, ataSmartAttrTable->EarlyBadBlock, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, LaterBadBlock, ataSmartAttrTable->LaterBadBlock, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, MaxEraseCount, ataSmartAttrTable->MaxEraseCount, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, AvgEraseCount, ataSmartAttrTable->AvgEraseCount, 0, NULL);
		value = ((unsigned long long)ataSmartAttrTable->CurPercentSpares[3] * 256 * 256 * 256) + ((unsigned long long)ataSmartAttrTable->CurPercentSpares[2] * 256 * 256) + ((unsigned long long)ataSmartAttrTable->CurPercentSpares[1] * 256) + (unsigned long long)ataSmartAttrTable->CurPercentSpares[0];
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, CurPercentSpares, value, 8, ataSmartAttrTable->CurPercentSpares);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, CurTemperature, ataSmartAttrTable->CurTemperature, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, LowestTemperature, ataSmartAttrTable->LowestTemperature, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, HighestTemperature, ataSmartAttrTable->HighestTemperature, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, ChipInternalTemperature, ataSmartAttrTable->ChipInternalTemperature, 0, NULL);
		SetNVMeSmartAttribute(hddMonInfo->smartAttriInfoList, SpareBlocks, ataSmartAttrTable->SpareBlocks, 0, NULL);
	}
	else if (hddMonInfo->hdd_type == SQFlash)
	{
		PExtSMARTAttr_t ataSmartAttrTable = (PExtSMARTAttr_t)attriBuf;
		//hddMonInfo->max_program = ataSmartAttrTable->MaxEraseCnt;
		hddMonInfo->average_program = ataSmartAttrTable->AvgEraseCnt;
		//hddMonInfo->endurance_check = ataSmartAttrTable->EnduranceCheck;
		hddMonInfo->power_on_time = ataSmartAttrTable->PowerOnHours;
		hddMonInfo->ecc_count = ataSmartAttrTable->UncorrectableECCCnt;
		//hddMonInfo->max_reserved_block = ataSmartAttrTable->MaxReservedBlock;
		//hddMonInfo->current_reserved_block = ataSmartAttrTable->CurrentReservedBlock;
		//hddMonInfo->good_block_rate = ataSmartAttrTable->GoodBlockRate;
		//strcpy(hddMonInfo->SerialNumber, ataSmartAttrTable->SerialNumber);
		hddMonInfo->power_cycle = ataSmartAttrTable->PowerCycleCnt;
		hddMonInfo->unexpected_power_loss_count = ataSmartAttrTable->UnexpectedPwrLostCnt;
		hddMonInfo->later_bad_block = ataSmartAttrTable->LaterBadBlkCnt;
		hddMonInfo->crc_error = ataSmartAttrTable->CRCErrCnt;
		hddMonInfo->hdd_temp = ataSmartAttrTable->CurTemperature;
		hddMonInfo->max_temp = ataSmartAttrTable->MaxTemperature;
		hddMonInfo->nand_read = ataSmartAttrTable->TotalNANDRead;
		hddMonInfo->nand_write = ataSmartAttrTable->TotalNANDWritten;
		hddMonInfo->host_read = ataSmartAttrTable->HostRead;
		hddMonInfo->host_write = ataSmartAttrTable->HostWrite;

		if (ataSmartAttrTable->smartAttriBuf)
		{
			int offset = 0;
			smart_attri_info_t smartAttriInfo;
			smart_attri_info_node_t *findNode = NULL;
			int bufLen = sizeof(ataSmartAttrTable->smartAttriBuf);
			char *attrbuf = ataSmartAttrTable->smartAttriBuf;
			bool bHasTemperature = false;
			for (offset = 2; offset < bufLen; offset += 12)
			{
				if (offset + 12 >= bufLen)
				{
					break;
				}
				else
				{
					memset(&smartAttriInfo, 0, sizeof(smart_attri_info_t));
					smartAttriInfo.attriType = (unsigned char)attrbuf[offset];
					// Attribute type 0x00, 0xfe, 0xff are invalid
					if (smartAttriInfo.attriType > SmartAttriTypeUnknown && smartAttriInfo.attriType <= FreeFallProtection)
					{
						smartAttriInfo.attriFlags = ((unsigned char)attrbuf[offset + 1] << 8) + (unsigned char)attrbuf[offset + 2];
						smartAttriInfo.attriValue = (unsigned char)attrbuf[offset + 3];
						smartAttriInfo.attriWorst = (unsigned char)attrbuf[offset + 4];
						memcpy(smartAttriInfo.attriVendorData, &attrbuf[offset + 5], 6);
						GetAttriNameFromType(hddMonInfo->hdd_type, smartAttriInfo.attriType, smartAttriInfo.attriName);

						if (hddMonInfo->smartAttriInfoList == NULL)
						{
							hddMonInfo->smartAttriInfoList = CreateSmartAttriInfoList();
						}
						findNode = FindSmartAttriInfoNodeWithType(hddMonInfo->smartAttriInfoList, smartAttriInfo.attriType);
						if (findNode)
						{
							UpdateSmartAttriInfoNode(hddMonInfo->smartAttriInfoList, &smartAttriInfo);
						}
						else
						{
							InsertSmartAttriInfoNode(hddMonInfo->smartAttriInfoList, &smartAttriInfo);
						}
					}
					else
						break;

					if (smartAttriInfo.attriType == FreeFallProtection)
						break;
				}
			}
		}
	}
	else if (hddMonInfo->hdd_type == StdDisk)
	{
		int offset = 0;
		bool bHasTemperature = false;
		smart_attri_info_t smartAttriInfo;
		smart_attri_info_node_t *findNode = NULL;
		for (offset = 2; offset < attriBufLen; offset += 12)
		{
			if (offset + 12 >= attriBufLen)
			{
				break;
			}
			else
			{
				memset(&smartAttriInfo, 0, sizeof(smart_attri_info_t));
				smartAttriInfo.attriType = (unsigned char)attriBuf[offset];
				// Attribute type 0x00, 0xfe, 0xff are invalid
				if (smartAttriInfo.attriType > SmartAttriTypeUnknown && smartAttriInfo.attriType <= FreeFallProtection)
				{
					smartAttriInfo.attriFlags = ((unsigned char)attriBuf[offset + 1] << 8) + (unsigned char)attriBuf[offset + 2];
					smartAttriInfo.attriValue = (unsigned char)attriBuf[offset + 3];
					smartAttriInfo.attriWorst = (unsigned char)attriBuf[offset + 4];
					memcpy(smartAttriInfo.attriVendorData, &attriBuf[offset + 5], 6);
					GetAttriNameFromType(hddMonInfo->hdd_type, smartAttriInfo.attriType, smartAttriInfo.attriName);
					if (smartAttriInfo.attriType == Temperature)
					{
						bHasTemperature = true;
						hddMonInfo->hdd_temp = ((unsigned char)smartAttriInfo.attriVendorData[1] * 256) + (unsigned char)smartAttriInfo.attriVendorData[0];
					}
					else if (smartAttriInfo.attriType == AirflowTemperatureWDC)
					{
						if (!bHasTemperature)
							hddMonInfo->hdd_temp = smartAttriInfo.attriValue;
					}
					else if (smartAttriInfo.attriType == PowerOnHoursPOH)
					{
						hddMonInfo->power_on_time = ((unsigned char)smartAttriInfo.attriVendorData[3] * 256 * 256 * 256) + ((unsigned char)smartAttriInfo.attriVendorData[2] * 256 * 256) +
													((unsigned char)smartAttriInfo.attriVendorData[1] * 256) + (unsigned char)smartAttriInfo.attriVendorData[0];
					}
					else if (smartAttriInfo.attriType == ReportedUncorrectableErrors)
					{
						hddMonInfo->ecc_count = ((unsigned char)smartAttriInfo.attriVendorData[1] * 256) + (unsigned char)smartAttriInfo.attriVendorData[0];
					}
					else if (smartAttriInfo.attriType == PowerCycleCount)
					{
						hddMonInfo->power_cycle = ((unsigned char)smartAttriInfo.attriVendorData[1] * 256) + (unsigned char)smartAttriInfo.attriVendorData[0];
					}

					if (hddMonInfo->smartAttriInfoList == NULL)
					{
						hddMonInfo->smartAttriInfoList = CreateSmartAttriInfoList();
					}
					findNode = FindSmartAttriInfoNodeWithType(hddMonInfo->smartAttriInfoList, smartAttriInfo.attriType);
					if (findNode)
					{
						UpdateSmartAttriInfoNode(hddMonInfo->smartAttriInfoList, &smartAttriInfo);
					}
					else
					{
						InsertSmartAttriInfoNode(hddMonInfo->smartAttriInfoList, &smartAttriInfo);
					}
				}
				else
					break;

				if (smartAttriInfo.attriType == FreeFallProtection)
					break;
			}
		}
	}
	bRet = true;
	return bRet;
}

bool RefreshHddInfoList(hdd_mon_info_list hddMonInfoList, int hddcount)
{
	bool bRet = false;
	if (hddMonInfoList == NULL)
		return bRet;
	{
		int i = 0;
		char tmpDiskNameWcs[128] = {0};
		hdd_mon_info_t hddMonInfo;
		DeleteAllHDDInfoNode(hddMonInfoList);
		while (i < hddcount)
		{
			if (SelectDevice(i))
			{
				memset(tmpDiskNameWcs, 0, sizeof(tmpDiskNameWcs));
				if (GetDeviceModelName(tmpDiskNameWcs, sizeof(tmpDiskNameWcs)) == true && strlen(tmpDiskNameWcs) > 0)
				{
					memset(&hddMonInfo, 0, sizeof(hdd_mon_info_t));
					hddMonInfo.hdd_index = i;
					hddMonInfo.hdd_type = HddTypeUnknown;
					hddMonInfo.smartAttriInfoList = NULL;
					hddMonInfo.max_program = INVALID_DEVMON_VALUE;
					InsertHDDInfoNode(hddMonInfoList, &hddMonInfo);
				}
			}
			i++;
		}
	}
	return bRet;
}

bool hdd_GetHDDInfo(hdd_info_t *pHDDInfo)
{
	bool bRet = false;
	char tmpDiskNameWcs[128] = {0};
	char tmpDiskNameBs[128] = {0};
	int curHddCnt = 0;
	int curHddIndex = 0;
	hdd_mon_info_node_t *curHddMonInfoNode = NULL;
	hdd_mon_info_t *curHddMonInfo = NULL;
	if (!pHDDInfo)
		return bRet;
	GetHDDCount(&curHddCnt);
	if ((curHddCnt == 0) || (curHddCnt != pHDDInfo->hddCount))
	{
		if (pHDDInfo->hddMonInfoList == NULL)
		{
			pHDDInfo->hddMonInfoList = hdd_CreateHDDInfoList();
		}
		RefreshHddInfoList(pHDDInfo->hddMonInfoList, curHddCnt);
		pHDDInfo->hddCount = curHddCnt;
	}

	curHddMonInfoNode = pHDDInfo->hddMonInfoList->next;
	while (curHddMonInfoNode)
	{
		curHddMonInfo = (hdd_mon_info_t *)&curHddMonInfoNode->hddMonInfo;
		curHddIndex = curHddMonInfo->hdd_index;
		if (SelectDevice(curHddIndex))
		{
			if (strlen(curHddMonInfo->hdd_name) <= 0)
			{
				memset(tmpDiskNameWcs, 0, sizeof(tmpDiskNameWcs));
				if (GetDeviceModelName(tmpDiskNameWcs, sizeof(tmpDiskNameWcs)) == true || strlen(tmpDiskNameWcs) > 0)
				{
					wcstombs(tmpDiskNameBs, (wchar_t *)tmpDiskNameWcs, sizeof(tmpDiskNameBs));
					sprintf(curHddMonInfo->hdd_name, "%s", tmpDiskNameBs);
				}
				TrimStr(curHddMonInfo->hdd_name);
			}

			CheckAccessCode("");
			curHddMonInfo->power_cycle = INVALID_DEVMON_VALUE;
			curHddMonInfo->unexpected_power_loss_count = INVALID_DEVMON_VALUE;
			curHddMonInfo->later_bad_block = INVALID_DEVMON_VALUE;
			curHddMonInfo->power_on_time = INVALID_DEVMON_VALUE;
			curHddMonInfo->ecc_count = INVALID_DEVMON_VALUE;
			curHddMonInfo->crc_error = INVALID_DEVMON_VALUE;
			curHddMonInfo->hdd_temp = INVALID_DEVMON_VALUE;
			curHddMonInfo->max_temp = INVALID_DEVMON_VALUE;

			if (strstr(curHddMonInfo->hdd_name, "SQF-C"))
			{
				ExtNVMeSMART_t nvmeSmartAttrTable = {0};
				unsigned int endurance = 0;
				curHddMonInfo->hdd_type = SQFNVMe;
				if (GetNVMeSmartAttribute(&nvmeSmartAttrTable))
				{
					/*TODO*/
					HDDSetAttriInfo(curHddMonInfo, (char *)&nvmeSmartAttrTable, sizeof(ExtNVMeSMART_t));
				}
				if (curHddMonInfo->max_program == 0 || curHddMonInfo->max_program == INVALID_DEVMON_VALUE)
				{
					if (GetSQFEndurance(&endurance))
						curHddMonInfo->max_program = endurance;
				}
				if (strlen(curHddMonInfo->SerialNumber) <= 0)
					GetSerialNumber(curHddMonInfo->SerialNumber);

				if (strlen(curHddMonInfo->FirmwareRevision) <= 0)
					GetFirmwareRevision(curHddMonInfo->FirmwareRevision);

				if (curHddMonInfo->opal_status == INVALID_DEVMON_VALUE)
					GetSQFOPALSTATUS(&curHddMonInfo->opal_status);
			}
			else if (strstr(curHddMonInfo->hdd_name, "SQF"))
			{
				ExtSMARTAttr_t extSmartAttrTable = {0};
				unsigned int endurance = 0;
				curHddMonInfo->hdd_type = SQFlash;
				if (GetSmartAttribute(&extSmartAttrTable))
				{
					hdd_UpdateSMARTInfo();
					GetSmartAttributeStd(extSmartAttrTable.smartAttriBuf, sizeof(extSmartAttrTable.smartAttriBuf));
					HDDSetAttriInfo(curHddMonInfo, (char *)&extSmartAttrTable, sizeof(ExtSMARTAttr_t));
				}
				if (curHddMonInfo->max_program == 0 || curHddMonInfo->max_program == INVALID_DEVMON_VALUE)
				{
					if (GetSQFEndurance(&endurance))
						curHddMonInfo->max_program = endurance;
				}
				if (strlen(curHddMonInfo->SerialNumber) <= 0)
					GetSerialNumber(curHddMonInfo->SerialNumber);
				if (strlen(curHddMonInfo->FirmwareRevision) <= 0)
					GetFirmwareRevision(curHddMonInfo->FirmwareRevision);
				if (curHddMonInfo->opal_status == INVALID_DEVMON_VALUE)
					GetSQFOPALSTATUS(&curHddMonInfo->opal_status);
			}
			else
			{
				char smartAttriBuf[512] = {0};
				hdd_UpdateSMARTInfo();
				curHddMonInfo->hdd_type = StdDisk;
				if (GetSmartAttributeStd(smartAttriBuf, sizeof(smartAttriBuf)))
				{
					HDDSetAttriInfo(curHddMonInfo, smartAttriBuf, sizeof(smartAttriBuf));
				}
				if (strlen(curHddMonInfo->SerialNumber) <= 0)
				{
					char tmpBufferWcs[64] = {0};
					char tmpBufferBs[64] = {0};
					if (hdd_GetSerialNumber(tmpBufferWcs) == true || strlen(tmpBufferWcs) > 0)
					{
						wcstombs(tmpBufferBs, (wchar_t *)tmpBufferWcs, sizeof(tmpBufferBs));
						strncpy(curHddMonInfo->SerialNumber, tmpBufferBs, sizeof(curHddMonInfo->SerialNumber));
						TrimStr(curHddMonInfo->SerialNumber);
					}
				}
				if (strlen(curHddMonInfo->FirmwareRevision) <= 0)
				{
					char tmpBufferWcs[32] = {0};
					char tmpBufferBs[32] = {0};
					if (hdd_GetFirmwareRevision(tmpBufferWcs) == true || strlen(tmpBufferWcs) > 0)
					{
						wcstombs(tmpBufferBs, (wchar_t *)tmpBufferWcs, sizeof(tmpBufferBs));
						strncpy(curHddMonInfo->FirmwareRevision, tmpBufferBs, sizeof(curHddMonInfo->FirmwareRevision));
						TrimStr(curHddMonInfo->FirmwareRevision);
					}
				}
			}
			GetHDDHealth(curHddMonInfo);
		}
		curHddMonInfoNode = curHddMonInfoNode->next;
	}
	bRet = true;
	return bRet;
}

#else
//#include "common.h"

#define SMART_TOOL_NAME "smartctl"
#define SDA_DEV_TYPE "sda"
#define SDB_DEV_TYPE "sdb"
#define HDA_DEV_TYPE "hda"

static char SmartctlPath[MAX_PATH] = {0};
bool hdd_IsExistSQFlashLib()
{
	bool bRet = false;
	if (strlen(SmartctlPath) == 0)
	{
		char moudlePath[MAX_PATH] = {0};
		memset(moudlePath, 0, sizeof(moudlePath));
		util_module_path_get(moudlePath);
		snprintf(SmartctlPath, sizeof(SmartctlPath), "%s/%s", moudlePath, SMART_TOOL_NAME);
	}
	if (access(SmartctlPath, F_OK) == F_OK)
	{
		bRet = true;
	}
	return bRet;
}

bool hdd_StartupSQFlashLib()
{
	bool bRet = false;
	bRet = true;
	return bRet;
}

bool hdd_CleanupSQFlashLib()
{
	bool bRet = false;
	bRet = true;
	return bRet;
}

static int HddIndex = 0;

int GetHddMonInfoWithSmartctl(hdd_mon_info_list hddMonInfoList, char *devType)
{
	int hddCnt = 0;
	if (hddMonInfoList == NULL || devType == NULL)
		return hddCnt;
	{
		char cmdStr[256] = {0};
		FILE *pF = NULL;
		snprintf(cmdStr, sizeof(cmdStr), "%s -a /dev/%s", SmartctlPath, devType);
		pF = popen(cmdStr, "r");
		if (pF)
		{
			char tmp[1024] = {0};
			smart_attri_info_t curSmartAttriInfo;
			hdd_mon_info_t *pCurHddMonInfo = NULL;
			bool bHasTemperature = false;
			bool bHasECCCount = false;
			bool bNewGoodBlockRate = false;
			unsigned int initblocks = 0;
			//#pragma region while fgets
			while (fgets(tmp, sizeof(tmp), pF) != NULL)
			{
				//#pragma region Device Mode
				if (strstr(tmp, "Device Model:"))
				{
					if (pCurHddMonInfo)
					{
						hddCnt++;
						GetHDDHealth(pCurHddMonInfo);
						InsertHDDInfoNodeEx(hddMonInfoList, pCurHddMonInfo);
						DestroySmartAttriInfoList(pCurHddMonInfo->smartAttriInfoList); // add by tang.tao @2015-8-13 11:19:25
						free(pCurHddMonInfo);
						pCurHddMonInfo = NULL;
					}
					pCurHddMonInfo = (hdd_mon_info_t *)malloc(sizeof(hdd_mon_info_t));
					memset((char *)pCurHddMonInfo, 0, sizeof(hdd_mon_info_t));
					{
						int i = 0;
						char *infoUnit[4] = {NULL};
						char *token = NULL;
						infoUnit[i] = strtok_r(tmp, ":", &token);
						i++;
						while ((infoUnit[i] = strtok_r(NULL, ":\n", &token)) != NULL)
						{
							i++;
						}
						if (i >= 2)
						{
							TrimStr(infoUnit[1]);
							pCurHddMonInfo->hdd_index = HddIndex;
							HddIndex++;
							//sprintf(pCurHddMonInfo->hdd_name, "Disk%d-%s", pCurHddMonInfo->hdd_index, infoUnit[1]);
							sprintf(pCurHddMonInfo->hdd_name, "%s", infoUnit[1]);
						}
					}

					if (strstr(pCurHddMonInfo->hdd_name, "SQF"))
					{
						pCurHddMonInfo->hdd_type = SQFlash;
						if (pCurHddMonInfo->hdd_name[4] == 'S' || pCurHddMonInfo->hdd_name[4] == 's')
						{
							switch (pCurHddMonInfo->hdd_name[7])
							{
							case 'M':
							case 'V':
								pCurHddMonInfo->max_program = 3000;
								break;
							case 'U':
								pCurHddMonInfo->max_program = 30000;
								break;
							case 'S':
								pCurHddMonInfo->max_program = 100000;
								break;
							//case 'E':
							//	if (brand == '-')   // Toshiba
							//		pCurHddMonInfo->max_program = 7000;
							//	else if (brand == 'A')  // Intel
							//		pCurHddMonInfo->max_program = 5000;
							//	else
							//		pCurHddMonInfo->max_program = 0;
							//	break;
							default:
								pCurHddMonInfo->max_program = 0;
								break;
							}
						}
						else if (pCurHddMonInfo->hdd_name[4] == 'C' || pCurHddMonInfo->hdd_name[4] == 'c')
						{
							//pCurHddMonInfo->hdd_type = SQFNVMe; /*not support*/
							pCurHddMonInfo->hdd_type = HddTypeUnknown;
							switch (pCurHddMonInfo->hdd_name[7])
							{
							case 'S':
							case 's':
								pCurHddMonInfo->max_program = 100000;
								break;
							case 'U':
							case 'u':
								pCurHddMonInfo->max_program = 30000;
								break;
							case 'M':
							case 'm':
							case 'V':
							case 'v':
								pCurHddMonInfo->max_program = 3000;
								break;
							case 'E':
							case 'e':
								pCurHddMonInfo->max_program = 7000;
								break;
							default:
								pCurHddMonInfo->max_program = 0;
								break;
							}
						}
					}
					else
					{
						pCurHddMonInfo->hdd_type = StdDisk;
					}
				}
				//#pragma endregion Device Mode
				else if (strstr(tmp, "ATTR"))
				{
					int i = 0;
					char *attrUnit[9] = {NULL};
					char *token = NULL;
					memset((char *)&curSmartAttriInfo, 0, sizeof(smart_attri_info_t));
					attrUnit[i] = strtok_r(tmp, ",", &token);
					i++;
					while ((attrUnit[i] = strtok_r(NULL, ",\n", &token)) != NULL)
					{
						i++;
					}
					if (i >= 5)
					{
						smart_attri_info_node_t *findNode = NULL;
						char tmpAttriName[64] = {0};

						unsigned int tmpAttriType = 0;
						TrimStr(attrUnit[1]);
						tmpAttriType = atoi(attrUnit[1]);

						GetAttriNameFromType(pCurHddMonInfo->hdd_type, tmpAttriType, tmpAttriName);
						if (strlen(tmpAttriName) > 0)
						{
							strcpy(curSmartAttriInfo.attriName, tmpAttriName);							
						}
						
						curSmartAttriInfo.attriType = tmpAttriType;
						
						TrimStr(attrUnit[2]);
						curSmartAttriInfo.attriValue = atoi(attrUnit[2]);
						
						TrimStr(attrUnit[3]);
						curSmartAttriInfo.attriThresh = atoi(attrUnit[3]);
						
						TrimStr(attrUnit[6]);
						{ //add VendorData process
							unsigned int tmpVendorData = atoi(attrUnit[6]);
							memcpy(curSmartAttriInfo.attriVendorData, (char *)&tmpVendorData, sizeof(char)*6);
							
						}
						curSmartAttriInfo.attriFlags = atoi(attrUnit[5]);
						
						curSmartAttriInfo.attriWorst = atoi(attrUnit[4]);
						
						if (pCurHddMonInfo->hdd_type == SQFlash) //SQFlash
						{
							switch (tmpAttriType)
							{
							case ReadErrorRate:
							{
								if (!bHasECCCount)
								{
									int tmpData = 0;
									TrimStr(attrUnit[6]);
									tmpData = atoi(attrUnit[6]);
									pCurHddMonInfo->ecc_count = (tmpData >> 16);
									
									bHasECCCount = true;
								}
								break;
							}
							case PowerOnHoursPOH:
							{
								int tmpData = 0;
								TrimStr(attrUnit[6]);
								tmpData = atoi(attrUnit[6]);
								pCurHddMonInfo->power_on_time = tmpData;
								
								break;
							}
							case PowerCycleCount:
							{

								pCurHddMonInfo->power_cycle = ((unsigned char)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
								
								break;
							}
							case SQInitialSpareBlocksAvailable:
							{
								bNewGoodBlockRate = true;
								initblocks = ((unsigned char)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
								break;
							}
							case SQSpareBlocksRemaining:
							{
								if (bNewGoodBlockRate && initblocks > 0)
								{
									//unsigned int remainblock = ((unsigned char)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
									//pCurHddMonInfo->good_block_rate = remainblock * 100 / initblocks;
									//no used
								}
								break;
							}
							case SQBadBlockCount:
							{
								pCurHddMonInfo->later_bad_block = ((unsigned char)curSmartAttriInfo.attriVendorData[5] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[4];
								
								break;
							}
							case SQEraseCount:
							{
								int tmpData = 0;
								TrimStr(attrUnit[6]);
								tmpData = atoi(attrUnit[6]);
								pCurHddMonInfo->average_program = (tmpData >> 16);
								
								break;
							}
							case SQUnexpectedPowerLossCount:
							{
								pCurHddMonInfo->unexpected_power_loss_count = ((unsigned char)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
								
								break;
							}
							case Temperature:
							{
								pCurHddMonInfo->hdd_temp = ((unsigned char)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
								pCurHddMonInfo->max_temp = ((unsigned char)curSmartAttriInfo.attriVendorData[5] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[4];
								
								break;
							}
							case SQCRCError:
							{
								int tmpData = 0;
								TrimStr(attrUnit[6]);
								tmpData = atoi(attrUnit[6]);
								pCurHddMonInfo->crc_error = tmpData;
								
								break;
							}
							case DriveTemperature: //SQ Flash: SSD Life Remaining
							{
								pCurHddMonInfo->hdd_health_percent = (unsigned char)curSmartAttriInfo.attriVendorData[0];
								
								break;
							}
							case DataAddressMarkerrors: //SQ Flash: PercentageOfSparesRemainin
							case SQEnduranceRemainLife: //SQFlash Old
							{
								//int tmpData = 0;
								//TrimStr(attrUnit[6]);
								//tmpData = atoi(attrUnit[6]);
								//pCurHddMonInfo->endurance_check = tmpData;
								//no used
								break;
							}
							case SQPowerOnTime: //SQFlash Old
							{
								int tmpData = 0;
								TrimStr(attrUnit[6]);
								tmpData = atoi(attrUnit[6]);
								pCurHddMonInfo->power_on_time = tmpData / (60 * 60);
								
								break;
							}
							case SQTotalNANDRead: //SQ Flash: Old SQECCLog
							{
								unsigned long long tmpData = 0;
								TrimStr(attrUnit[6]);
								tmpData = ((unsigned long long)curSmartAttriInfo.attriVendorData[5] * 256*256*256*256*256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[4]*256*256*256*256) +
											((unsigned long long)curSmartAttriInfo.attriVendorData[3] * 256*256*256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[2]*256*256) +
											((unsigned long long)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned long long)curSmartAttriInfo.attriVendorData[0];
								pCurHddMonInfo->nand_read = tmpData;
								
								break;
							}
							case SQTotalNANDWritten: //SQFlash old
							{
								unsigned long long tmpData = 0;
								TrimStr(attrUnit[6]);
								tmpData = ((unsigned long long)curSmartAttriInfo.attriVendorData[5] * 256*256*256*256*256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[4]*256*256*256*256) +
											((unsigned long long)curSmartAttriInfo.attriVendorData[3] * 256*256*256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[2]*256*256) +
											((unsigned long long)curSmartAttriInfo.attriVendorData[1] * 256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[0]);
								pCurHddMonInfo->nand_write = tmpData;
								
								break;
							}
							case TotalLBAsWritten: //SQFlash: TotalHostWrite
							{
								unsigned long long tmpData = 0;
								TrimStr(attrUnit[6]);
								tmpData = ((unsigned long long)curSmartAttriInfo.attriVendorData[5] * 256*256*256*256*256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[4]*256*256*256*256) +
											((unsigned long long)curSmartAttriInfo.attriVendorData[3] * 256*256*256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[2]*256*256) +
											((unsigned long long)curSmartAttriInfo.attriVendorData[1] * 256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[0]);
								pCurHddMonInfo->host_write = tmpData;
								
								break;
							}
							case TotalLBAsRead: //SQFlash: TotalHostRead
							{
								unsigned long long tmpData = 0;
								TrimStr(attrUnit[6]);
								tmpData = ((unsigned long long)curSmartAttriInfo.attriVendorData[5] * 256*256*256*256*256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[4]*256*256*256*256) +
											((unsigned long long)curSmartAttriInfo.attriVendorData[3] * 256*256*256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[2]*256*256) +
											((unsigned long long)curSmartAttriInfo.attriVendorData[1] * 256) + ((unsigned long long)curSmartAttriInfo.attriVendorData[0]);
								pCurHddMonInfo->host_read = tmpData;
								
								break;
							}
							default:
								break;
							}
						}
						else //General disk
						{
							if (tmpAttriType > SmartAttriTypeUnknown && tmpAttriType <= FreeFallProtection)
							{
								if (curSmartAttriInfo.attriType == PowerCycleCount)
								{
									pCurHddMonInfo->power_cycle = ((unsigned char)curSmartAttriInfo.attriVendorData[3] * (256 * 256 * 256)) + ((unsigned char)curSmartAttriInfo.attriVendorData[2] * (256 * 256)) +
																  ((unsigned char)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
																									  
								}
								else if (curSmartAttriInfo.attriType == Temperature)
								{
									bHasTemperature = true;
									pCurHddMonInfo->hdd_temp = ((unsigned char)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
									
								}
								else if (curSmartAttriInfo.attriType == AirflowTemperatureWDC)
								{
									if (!bHasTemperature)
										pCurHddMonInfo->hdd_temp = curSmartAttriInfo.attriValue;
										
								}
								else if (curSmartAttriInfo.attriType == PowerOnHoursPOH)
								{
									pCurHddMonInfo->power_on_time = ((unsigned char)curSmartAttriInfo.attriVendorData[3] * (256 * 256 * 256)) + ((unsigned char)curSmartAttriInfo.attriVendorData[2] * (256 * 256)) +
																	((unsigned char)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
																	
								}
								else if (curSmartAttriInfo.attriType == ReportedUncorrectableErrors)
								{
									pCurHddMonInfo->ecc_count = ((unsigned char)curSmartAttriInfo.attriVendorData[1] * 256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
									
								}
							}
						}

						if (pCurHddMonInfo->smartAttriInfoList == NULL)
						{
							pCurHddMonInfo->smartAttriInfoList = CreateSmartAttriInfoList();
						}
						findNode = FindSmartAttriInfoNodeWithType(pCurHddMonInfo->smartAttriInfoList, curSmartAttriInfo.attriType);
						if (findNode)
						{
							UpdateSmartAttriInfoNode(pCurHddMonInfo->smartAttriInfoList, &curSmartAttriInfo);
						}
						else
						{
							InsertSmartAttriInfoNodeEx(pCurHddMonInfo->smartAttriInfoList, &curSmartAttriInfo);
						}
					}
				}
				memset(tmp, 0, sizeof(tmp));
			}
//#pragma endregion while fgets
			if (pCurHddMonInfo)
			{
				hddCnt++;
				GetHDDHealth(pCurHddMonInfo);
				InsertHDDInfoNodeEx(hddMonInfoList, pCurHddMonInfo);
				DestroySmartAttriInfoList(pCurHddMonInfo->smartAttriInfoList); // add by tang.tao @2015-8-13 11:19:25
				free(pCurHddMonInfo);
				pCurHddMonInfo = NULL;
			}
		}
		pclose(pF);
	}
	return hddCnt;
}

bool hdd_GetHDDInfo(hdd_info_t *pHDDInfo)
{
	bool bRet = false;
	if (pHDDInfo == NULL)
		return bRet;

	{
		int tmpHddCnt = 0;
		pHDDInfo->hddCount = 0;
		if (pHDDInfo->hddMonInfoList == NULL)
		{
			pHDDInfo->hddMonInfoList = hdd_CreateHDDInfoList();
		}
		else
		{
			DeleteAllHDDInfoNode(pHDDInfo->hddMonInfoList);
		}
		HddIndex = 0;
		tmpHddCnt = GetHddMonInfoWithSmartctl(pHDDInfo->hddMonInfoList, SDA_DEV_TYPE);
		pHDDInfo->hddCount += tmpHddCnt;
		tmpHddCnt = GetHddMonInfoWithSmartctl(pHDDInfo->hddMonInfoList, SDB_DEV_TYPE);
		pHDDInfo->hddCount += tmpHddCnt;
		tmpHddCnt = GetHddMonInfoWithSmartctl(pHDDInfo->hddMonInfoList, HDA_DEV_TYPE);
		pHDDInfo->hddCount += tmpHddCnt;
		bRet = true;
	}
	return bRet;
}
#endif

//---------------------device monitor hdd smart attribute list function ------------------------
smart_attri_info_node_t *FindSmartAttriInfoNode(smart_attri_info_list attriInfoList, char *attriName)
{
	smart_attri_info_node_t *findNode = NULL, *head = NULL;
	if (attriInfoList == NULL || attriName == NULL)
		return findNode;
	head = attriInfoList;
	findNode = head->next;
	while (findNode)
	{
		if (findNode->bNVMe)
		{
			if (!strcmp(findNode->nvme.attriName, attriName))
				break;
		}
		else
		{
			if (!strcmp(findNode->sata.attriName, attriName))
				break;
		}
		findNode = findNode->next;
	}

	return findNode;
}

smart_attri_info_node_t *FindSmartAttriInfoNodeWithType(smart_attri_info_list attriInfoList, smart_attri_type_t attriType)
{
	smart_attri_info_node_t *findNode = NULL, *head = NULL;
	if (attriInfoList == NULL || attriType == SmartAttriTypeUnknown)
		return findNode;
	head = attriInfoList;
	findNode = head->next;
	while (findNode)
	{
		if (findNode->bNVMe)
		{
			if (findNode->nvme.attriType == attriType)
				break;
		}
		else
		{
			if (findNode->sata.attriType == attriType)
				break;
		}
		findNode = findNode->next;
	}
	return findNode;
}

smart_attri_info_list CreateSmartAttriInfoList()
{
	smart_attri_info_node_t *head = NULL;
	head = (smart_attri_info_node_t *)malloc(sizeof(smart_attri_info_node_t));
	if (head)
	{
		head->next = NULL;
		memset(head->sata.attriName, 0, sizeof(head->sata.attriName));
		head->sata.attriFlags = 0;
		head->sata.attriType = SmartAttriTypeUnknown;
		head->sata.attriValue = 0;
		head->sata.attriWorst = 0;
		memset(head->sata.attriVendorData, 0, sizeof(head->sata.attriVendorData));
	}
	return head;
}

int InsertSmartAttriInfoNode(smart_attri_info_list attriInfoList, smart_attri_info_t *attriInfo)
{
	int iRet = -1;
	smart_attri_info_node_t *newNode = NULL, *findNode = NULL, *head = NULL;
	if (attriInfoList == NULL || attriInfo == NULL)
		return iRet;
	head = attriInfoList;
	findNode = FindSmartAttriInfoNode(head, attriInfo->attriName);
	if (findNode == NULL)
	{
		newNode = (smart_attri_info_node_t *)malloc(sizeof(smart_attri_info_node_t));
		memset(newNode, 0, sizeof(smart_attri_info_node_t));
		strcpy(newNode->sata.attriName, attriInfo->attriName);
		newNode->sata.attriFlags = attriInfo->attriFlags;
		newNode->sata.attriType = attriInfo->attriType;
		newNode->sata.attriThresh = attriInfo->attriThresh;
		newNode->sata.attriValue = attriInfo->attriValue;
		newNode->sata.attriWorst = attriInfo->attriWorst;
		memcpy(newNode->sata.attriVendorData, attriInfo->attriVendorData, sizeof(attriInfo->attriVendorData));
		newNode->next = head->next;
		head->next = newNode;
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

int InsertSmartAttriInfoNodeEx(smart_attri_info_list attriInfoList, smart_attri_info_t *attriInfo)
{
	int iRet = -1;
	smart_attri_info_node_t *newNode = NULL, *head = NULL;
	if (attriInfoList == NULL || attriInfo == NULL)
		return iRet;
	head = attriInfoList;
	newNode = (smart_attri_info_node_t *)malloc(sizeof(smart_attri_info_node_t));
	memset(newNode, 0, sizeof(smart_attri_info_node_t));
	strcpy(newNode->sata.attriName, attriInfo->attriName);
	newNode->sata.attriFlags = attriInfo->attriFlags;
	newNode->sata.attriType = attriInfo->attriType;
	newNode->sata.attriThresh = attriInfo->attriThresh;
	newNode->sata.attriValue = attriInfo->attriValue;
	newNode->sata.attriWorst = attriInfo->attriWorst;
	memcpy(newNode->sata.attriVendorData, attriInfo->attriVendorData, sizeof(attriInfo->attriVendorData));
	newNode->next = head->next;
	head->next = newNode;
	iRet = 0;
	return iRet;
}

int UpdateSmartAttriInfoNode(smart_attri_info_list attriInfoList, smart_attri_info_t *attriInfo)
{
	int iRet = -1;
	smart_attri_info_node_t *findNode = NULL, *head = NULL;
	if (attriInfoList == NULL || attriInfo == NULL)
		return iRet;
	head = attriInfoList;
	findNode = FindSmartAttriInfoNodeWithType(head, attriInfo->attriType);
	if (findNode)
	{
		memset(findNode->sata.attriName, 0, sizeof(findNode->sata.attriName));
		strcpy(findNode->sata.attriName, attriInfo->attriName);
		findNode->sata.attriFlags = attriInfo->attriFlags;
		findNode->sata.attriThresh = attriInfo->attriThresh;
		findNode->sata.attriValue = attriInfo->attriValue;
		findNode->sata.attriWorst = attriInfo->attriWorst;
		memset(findNode->sata.attriVendorData, 0, sizeof(findNode->sata.attriVendorData));
		memcpy(findNode->sata.attriVendorData, attriInfo->attriVendorData, sizeof(attriInfo->attriVendorData));
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

int InsertNVMeAttriInfoNode(smart_attri_info_list attriInfoList, nvme_attri_info_t *attriInfo)
{
	int iRet = -1;
	smart_attri_info_node_t *newNode = NULL, *findNode = NULL, *head = NULL;
	if (attriInfoList == NULL || attriInfo == NULL)
		return iRet;
	head = attriInfoList;
	findNode = FindSmartAttriInfoNode(head, attriInfo->attriName);
	if (findNode == NULL)
	{
		newNode = (smart_attri_info_node_t *)malloc(sizeof(smart_attri_info_node_t));
		memset(newNode, 0, sizeof(smart_attri_info_node_t));
		strcpy(newNode->nvme.attriName, attriInfo->attriName);
		newNode->nvme.attriType = attriInfo->attriType;
		newNode->nvme.attriValue = attriInfo->attriValue;
		newNode->nvme.datalen = attriInfo->datalen;
		memcpy(newNode->nvme.attriVendorData, attriInfo->attriVendorData, sizeof(attriInfo->attriVendorData));
		newNode->bNVMe = true;
		newNode->next = head->next;
		head->next = newNode;
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

int UpdateNVMeAttriInfoNode(smart_attri_info_list attriInfoList, nvme_attri_info_t *attriInfo)
{
	int iRet = -1;
	smart_attri_info_node_t *findNode = NULL, *head = NULL;
	if (attriInfoList == NULL || attriInfo == NULL)
		return iRet;
	head = attriInfoList;
	findNode = FindSmartAttriInfoNodeWithType(head, attriInfo->attriType);
	if (findNode)
	{
		memset(findNode->nvme.attriName, 0, sizeof(findNode->nvme.attriName));
		strcpy(findNode->nvme.attriName, attriInfo->attriName);
		findNode->nvme.attriType = attriInfo->attriType;
		findNode->nvme.attriValue = attriInfo->attriValue;
		findNode->nvme.datalen = attriInfo->datalen;
		memcpy(findNode->nvme.attriVendorData, attriInfo->attriVendorData, sizeof(attriInfo->attriVendorData));
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

int SetNVMeSmartAttribute(smart_attri_info_list attriInfoList, nvme_smart_attri_type_t attriType, long value, int len, char *rowdata)
{
	int iRet = -1;
	smart_attri_info_node_t *findNode = NULL;
	nvme_attri_info_t smartAttriInfo = {0};
	smartAttriInfo.attriType = attriType;
	smartAttriInfo.attriValue = value;
	smartAttriInfo.datalen = len;
	memcpy(smartAttriInfo.attriVendorData, rowdata, len * sizeof(char));
	GetSQNVMeAttriNameFromType(attriType, smartAttriInfo.attriName);
	findNode = FindSmartAttriInfoNodeWithType(attriInfoList, attriType);
	if (findNode)
	{
		iRet = UpdateNVMeAttriInfoNode(attriInfoList, &smartAttriInfo);
	}
	else
	{
		iRet = InsertNVMeAttriInfoNode(attriInfoList, &smartAttriInfo);
	}
	return iRet;
}

int DeleteSmartAttriInfoNode(smart_attri_info_list attriInfoList, char *attriName)
{
	int iRet = -1;
	smart_attri_info_node_t *delNode = NULL, *head = NULL;
	smart_attri_info_node_t *p = NULL;
	if (attriInfoList == NULL || attriName == NULL)
		return iRet;
	head = attriInfoList;
	p = head;
	delNode = head->next;
	while (delNode)
	{
		if (delNode->bNVMe)
		{
			if (!strcmp(delNode->nvme.attriName, attriName))
			{
				p->next = delNode->next;
				free(delNode);
				delNode = NULL;
				iRet = 0;
				break;
			}
		}
		else
		{
			if (!strcmp(delNode->sata.attriName, attriName))
			{
				p->next = delNode->next;
				free(delNode);
				delNode = NULL;
				iRet = 0;
				break;
			}
		}

		p = delNode;
		delNode = delNode->next;
	}
	if (iRet == -1)
		iRet = 1;
	return iRet;
}

int DeleteSmartAttriInfoNodeWithID(smart_attri_info_list attriInfoList, smart_attri_type_t attriType)
{
	int iRet = -1;
	smart_attri_info_node_t *delNode = NULL, *head = NULL;
	smart_attri_info_node_t *p = NULL;
	if (attriInfoList == NULL || attriType == SmartAttriTypeUnknown)
		return iRet;
	head = attriInfoList;
	p = head;
	delNode = head->next;
	while (delNode)
	{
		if (delNode->bNVMe)
		{
			if (delNode->nvme.attriType == attriType)
			{
				p->next = delNode->next;
				free(delNode);
				delNode = NULL;
				iRet = 0;
				break;
			}
		}
		else
		{
			if (delNode->sata.attriType == attriType)
			{
				p->next = delNode->next;
				free(delNode);
				delNode = NULL;
				iRet = 0;
				break;
			}
		}
		p = delNode;
		delNode = delNode->next;
	}
	if (iRet == -1)
		iRet = 1;
	return iRet;
}

int DeleteAllSmartAttriInfoNode(smart_attri_info_list attriInfoList)
{
	int iRet = -1;
	smart_attri_info_node_t *delNode = NULL, *head = NULL;
	if (attriInfoList == NULL)
		return iRet;
	head = attriInfoList;
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

void DestroySmartAttriInfoList(smart_attri_info_list attriInfoList)
{
	if (NULL == attriInfoList)
		return;
	DeleteAllSmartAttriInfoNode(attriInfoList);
	free(attriInfoList);
	attriInfoList = NULL;
}

bool IsSmartAttriInfoListEmpty(smart_attri_info_list attriInfoList)
{
	bool bRet = true;
	smart_attri_info_node_t *curNode = NULL, *head = NULL;
	if (attriInfoList == NULL)
		return bRet;
	head = attriInfoList;
	curNode = head->next;
	if (curNode != NULL)
		bRet = false;
	return bRet;
}
//----------------------------------------------------------------------------------------------

//---------------------device monitor hdd info list fuction ------------------------------------
hdd_mon_info_node_t *FindHDDInfoNode(hdd_mon_info_list hddInfoList, char *hddName)
{
	hdd_mon_info_node_t *findNode = NULL, *head = NULL;
	if (hddInfoList == NULL || hddName == NULL)
		return findNode;
	head = hddInfoList;
	findNode = head->next;
	while (findNode)
	{
		if (!strcmp(findNode->hddMonInfo.hdd_name, hddName))
			break;
		findNode = findNode->next;
	}

	return findNode;
}

hdd_mon_info_node_t *FindHDDInfoNodeWithID(hdd_mon_info_list hddInfoList, int hddIndex)
{
	hdd_mon_info_node_t *findNode = NULL, *head = NULL;
	if (hddInfoList == NULL)
		return findNode;
	head = hddInfoList;
	findNode = head->next;
	while (findNode)
	{
		if (findNode->hddMonInfo.hdd_index == hddIndex)
			break;
		findNode = findNode->next;
	}
	return findNode;
}

hdd_mon_info_list hdd_CreateHDDInfoList()
{
	hdd_mon_info_node_t *head = NULL;
	head = (hdd_mon_info_node_t *)malloc(sizeof(hdd_mon_info_node_t));
	memset(head, 0, sizeof(hdd_mon_info_node_t));
	if (head)
	{
		head->next = NULL;
		memset(head->hddMonInfo.hdd_name, 0, sizeof(head->hddMonInfo.hdd_name));
		head->hddMonInfo.hdd_index = INVALID_DEVMON_VALUE;
		head->hddMonInfo.hdd_type = SmartAttriTypeUnknown;
		head->hddMonInfo.power_cycle = INVALID_DEVMON_VALUE;
		head->hddMonInfo.unexpected_power_loss_count = INVALID_DEVMON_VALUE;
		head->hddMonInfo.ecc_count = INVALID_DEVMON_VALUE;
		head->hddMonInfo.later_bad_block = INVALID_DEVMON_VALUE;
		head->hddMonInfo.crc_error = INVALID_DEVMON_VALUE;
		head->hddMonInfo.power_on_time = INVALID_DEVMON_VALUE;
		head->hddMonInfo.hdd_health_percent = INVALID_DEVMON_VALUE;
		head->hddMonInfo.hdd_temp = INVALID_DEVMON_VALUE;
		head->hddMonInfo.max_temp = INVALID_DEVMON_VALUE;
		head->hddMonInfo.remain_days = DEFAULT_REMAIN_DAYS;
		head->hddMonInfo.opal_status = INVALID_DEVMON_VALUE;
		head->hddMonInfo.max_program = INVALID_DEVMON_VALUE;
		head->hddMonInfo.average_program = INVALID_DEVMON_VALUE;
		memset(head->hddMonInfo.SerialNumber, 0, sizeof(head->hddMonInfo.SerialNumber));
		head->hddMonInfo.smartAttriInfoList = NULL;
	}
	return head;
}

int InsertHDDInfoNode(hdd_mon_info_list hddInfoList, hdd_mon_info_t *hddMonInfo)
{
	int iRet = -1;
	hdd_mon_info_node_t *newNode = NULL, *findNode = NULL, *head = NULL;
	if (hddInfoList == NULL || hddMonInfo == NULL)
		return iRet;
	head = hddInfoList;
	findNode = FindHDDInfoNodeWithID(head, hddMonInfo->hdd_index);
	if (findNode == NULL)
	{
		newNode = (hdd_mon_info_node_t *)malloc(sizeof(hdd_mon_info_node_t));
		memset(newNode, 0, sizeof(hdd_mon_info_node_t));
		strcpy(newNode->hddMonInfo.hdd_name, hddMonInfo->hdd_name);
		newNode->hddMonInfo.hdd_index = hddMonInfo->hdd_index;
		newNode->hddMonInfo.hdd_type = hddMonInfo->hdd_type;
		newNode->hddMonInfo.power_cycle = hddMonInfo->power_cycle;
		newNode->hddMonInfo.unexpected_power_loss_count = hddMonInfo->unexpected_power_loss_count;
		newNode->hddMonInfo.ecc_count = hddMonInfo->ecc_count;
		newNode->hddMonInfo.later_bad_block = hddMonInfo->later_bad_block;
		newNode->hddMonInfo.crc_error = hddMonInfo->crc_error;
		newNode->hddMonInfo.hdd_health_percent = hddMonInfo->hdd_health_percent;
		newNode->hddMonInfo.hdd_temp = hddMonInfo->hdd_temp;
		newNode->hddMonInfo.max_temp = hddMonInfo->max_temp;
		newNode->hddMonInfo.power_on_time = hddMonInfo->power_on_time;
		newNode->hddMonInfo.remain_days = hddMonInfo->remain_days;
		newNode->hddMonInfo.opal_status = hddMonInfo->opal_status;
		newNode->hddMonInfo.max_program = hddMonInfo->max_program;
		newNode->hddMonInfo.average_program = hddMonInfo->average_program;
		strcpy(newNode->hddMonInfo.SerialNumber, hddMonInfo->SerialNumber);
		if (hddMonInfo->smartAttriInfoList)
		{
			smart_attri_info_node_t *curAtrriInfoNode = NULL;
			newNode->hddMonInfo.smartAttriInfoList = CreateSmartAttriInfoList();
			curAtrriInfoNode = hddMonInfo->smartAttriInfoList->next;
			while (curAtrriInfoNode)
			{
				if (curAtrriInfoNode->bNVMe)
				{
					InsertNVMeAttriInfoNode(newNode->hddMonInfo.smartAttriInfoList, &curAtrriInfoNode->nvme);
				}
				else
				{
					InsertSmartAttriInfoNode(newNode->hddMonInfo.smartAttriInfoList, &curAtrriInfoNode->sata);
				}

				curAtrriInfoNode = curAtrriInfoNode->next;
			}
		}
		else
		{
			newNode->hddMonInfo.smartAttriInfoList = NULL;
		}
		newNode->next = head->next;
		head->next = newNode;
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

int InsertHDDInfoNodeEx(hdd_mon_info_list hddInfoList, hdd_mon_info_t *hddMonInfo)
{
	int iRet = -1;
	hdd_mon_info_node_t *newNode = NULL, *head = NULL;
	if (hddInfoList == NULL || hddMonInfo == NULL)
		return iRet;
	head = hddInfoList;
	newNode = (hdd_mon_info_node_t *)malloc(sizeof(hdd_mon_info_node_t));
	memset(newNode, 0, sizeof(hdd_mon_info_node_t));
	strcpy(newNode->hddMonInfo.hdd_name, hddMonInfo->hdd_name);
	newNode->hddMonInfo.hdd_index = hddMonInfo->hdd_index;
	newNode->hddMonInfo.hdd_type = hddMonInfo->hdd_type;
	newNode->hddMonInfo.power_cycle = hddMonInfo->power_cycle;
	newNode->hddMonInfo.unexpected_power_loss_count = hddMonInfo->unexpected_power_loss_count;
	newNode->hddMonInfo.ecc_count = hddMonInfo->ecc_count;
	newNode->hddMonInfo.later_bad_block = hddMonInfo->later_bad_block;
	newNode->hddMonInfo.crc_error = hddMonInfo->crc_error;
	newNode->hddMonInfo.hdd_health_percent = hddMonInfo->hdd_health_percent;
	newNode->hddMonInfo.hdd_temp = hddMonInfo->hdd_temp;
	newNode->hddMonInfo.max_temp = hddMonInfo->max_temp;
	newNode->hddMonInfo.power_on_time = hddMonInfo->power_on_time;
	newNode->hddMonInfo.remain_days = hddMonInfo->remain_days;
	newNode->hddMonInfo.opal_status = hddMonInfo->opal_status;
	strcpy(newNode->hddMonInfo.SerialNumber, hddMonInfo->SerialNumber);
	if (hddMonInfo->smartAttriInfoList)
	{
		smart_attri_info_node_t *curAtrriInfoNode = NULL;
		newNode->hddMonInfo.smartAttriInfoList = CreateSmartAttriInfoList();
		curAtrriInfoNode = hddMonInfo->smartAttriInfoList->next;
		while (curAtrriInfoNode)
		{
			if (curAtrriInfoNode->bNVMe)
			{
				InsertNVMeAttriInfoNode(newNode->hddMonInfo.smartAttriInfoList, &curAtrriInfoNode->nvme);
			}
			else
			{
				InsertSmartAttriInfoNode(newNode->hddMonInfo.smartAttriInfoList, &curAtrriInfoNode->sata);
			}
			curAtrriInfoNode = curAtrriInfoNode->next;
		}
	}
	else
	{
		newNode->hddMonInfo.smartAttriInfoList = NULL;
	}
	newNode->next = head->next;
	head->next = newNode;
	iRet = 0;
	return iRet;
}

int DeleteHDDInfoNode(hdd_mon_info_list hddInfoList, char *hddName)
{
	int iRet = -1;
	hdd_mon_info_node_t *delNode = NULL, *head = NULL;
	hdd_mon_info_node_t *p = NULL;
	if (hddInfoList == NULL || hddName == NULL)
		return iRet;
	head = hddInfoList;
	p = head;
	delNode = head->next;
	while (delNode)
	{
		if (!strcmp(delNode->hddMonInfo.hdd_name, hddName))
		{
			p->next = delNode->next;
			if (delNode->hddMonInfo.smartAttriInfoList)
			{
				DestroySmartAttriInfoList(delNode->hddMonInfo.smartAttriInfoList);
				delNode->hddMonInfo.smartAttriInfoList = NULL;
			}
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

int DeleteHDDInfoNodeWithID(hdd_mon_info_list hddInfoList, int hddIndex)
{
	int iRet = -1;
	hdd_mon_info_node_t *delNode = NULL, *head = NULL;
	hdd_mon_info_node_t *p = NULL;
	if (hddInfoList == NULL)
		return iRet;
	head = hddInfoList;
	p = head;
	delNode = head->next;
	while (delNode)
	{
		if (delNode->hddMonInfo.hdd_index == hddIndex)
		{
			p->next = delNode->next;
			if (delNode->hddMonInfo.smartAttriInfoList)
			{
				DestroySmartAttriInfoList(delNode->hddMonInfo.smartAttriInfoList);
				delNode->hddMonInfo.smartAttriInfoList = NULL;
			}
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

int DeleteAllHDDInfoNode(hdd_mon_info_list hddInfoList)
{
	int iRet = -1;
	hdd_mon_info_node_t *delNode = NULL, *head = NULL;
	if (hddInfoList == NULL)
		return iRet;
	head = hddInfoList;
	delNode = head->next;
	while (delNode)
	{
		head->next = delNode->next;
		if (delNode->hddMonInfo.smartAttriInfoList)
		{
			DestroySmartAttriInfoList(delNode->hddMonInfo.smartAttriInfoList);
			delNode->hddMonInfo.smartAttriInfoList = NULL;
		}
		free(delNode);
		delNode = head->next;
	}
	iRet = 0;
	return iRet;
}

void hdd_DestroyHDDInfoList(hdd_mon_info_list hddInfoList)
{
	if (NULL == hddInfoList)
		return;
	DeleteAllHDDInfoNode(hddInfoList);
	free(hddInfoList);
	hddInfoList = NULL;
}

bool IsHddInfoListEmpty(hdd_mon_info_list hddInfoList)
{
	bool bRet = true;
	hdd_mon_info_node_t *curNode = NULL, *head = NULL;
	if (hddInfoList == NULL)
		return bRet;
	head = hddInfoList;
	curNode = head->next;
	if (curNode != NULL)
		bRet = false;
	return bRet;
}
//----------------------------------------------------------------------------------------------

bool GetHDDHealth(hdd_mon_info_t *hddMonInfo)
{
	bool bRet = false;
	if (hddMonInfo == NULL)
		return bRet;
	hddMonInfo->remain_days = DEFAULT_REMAIN_DAYS;
	if (hddMonInfo->hdd_type == SQFNVMe)
	{
		//unsigned int endurance = 0;
		hddMonInfo->hdd_health_percent = 100;
		//if (GetSQFEndurance(&endurance))
		if(hddMonInfo->max_program>0)
		{
			int health = 100 - hddMonInfo->average_program * 100 / hddMonInfo->max_program;
			hddMonInfo->hdd_health_percent = health < 0 ? 0 : health;
		}
		else
		{
			if (hddMonInfo->hdd_name[7] == 'S' || hddMonInfo->hdd_name[7] == 's')
			{
				hddMonInfo->max_program = 100000;
			}
			else if (hddMonInfo->hdd_name[7] == 'U' || hddMonInfo->hdd_name[7] == 'u')
			{
				hddMonInfo->max_program = 30000;
			}
			else if (hddMonInfo->hdd_name[7] == 'M' || hddMonInfo->hdd_name[7] == 'm')
			{
				hddMonInfo->max_program = 3000;
			}
			else if (hddMonInfo->hdd_name[7] == 'V' || hddMonInfo->hdd_name[7] == 'v')
			{
				hddMonInfo->max_program = 3000;
			}
			else if (hddMonInfo->hdd_name[7] == 'E' || hddMonInfo->hdd_name[7] == 'e')
			{
				hddMonInfo->max_program = 7000;
			}

			if (hddMonInfo->max_program > 0)
			{
				int health = 100 - hddMonInfo->average_program * 100 / hddMonInfo->max_program;
				hddMonInfo->hdd_health_percent = health < 0 ? 0 : health;
			}
			else
			{
				hddMonInfo->hdd_health_percent = 100;
			}
		}

		if (hddMonInfo->max_program >= 0 && hddMonInfo->max_program != INVALID_DEVMON_VALUE && hddMonInfo->average_program > 0 )
		{
			double usedDay = 0, estDay = 0, remDay = 0;
			usedDay = (double)hddMonInfo->power_on_time / 24.0;
			estDay = ((double)hddMonInfo->max_program / (double)hddMonInfo->average_program) * (double)usedDay * 0.8;
			remDay = estDay - usedDay;
			hddMonInfo->remain_days = remDay < 0 ? 0 : remDay;
		}
		else if (hddMonInfo->hdd_health_percent >= 0 && hddMonInfo->hdd_health_percent < 100)
		{
			double usedDay = 0, estDay = 0, remDay = 0;
			usedDay = (double)hddMonInfo->power_on_time / 24.0;
			estDay = (usedDay / ((100 - (double)hddMonInfo->hdd_health_percent) / 100)) * 0.8;
			remDay = estDay - usedDay;
			hddMonInfo->remain_days = remDay < 0 ? 0 : remDay;
		}
		else
			hddMonInfo->remain_days = DEFAULT_REMAIN_DAYS;
	}
	else if (hddMonInfo->hdd_type == SQFlash)
	{
		bool bHasLifeRemain = false;
		if (hddMonInfo->smartAttriInfoList)
		{
			smart_attri_info_node_t *curAttriInfoNode = hddMonInfo->smartAttriInfoList->next;
			smart_attri_info_t *curAttriInfo = NULL;
			//double maxProgram = 0;
			//double avgProgram = 0;
			//unsigned int health = 0;
			hddMonInfo->hdd_health_percent = 100;

			while (curAttriInfoNode)
			{
				curAttriInfo = (smart_attri_info_t *)&curAttriInfoNode->sata;
				switch (curAttriInfo->attriType)
				{
				case DriveTemperature: //SQ Flash: SSD Life Remaining
				{
					hddMonInfo->hdd_health_percent = (curAttriInfo->attriVendorData[1] << 8) + curAttriInfo->attriVendorData[0];
					bHasLifeRemain = true;
				}
				default:
					break;
				}
				curAttriInfoNode = curAttriInfoNode->next;
			}
		}
		
		if (hddMonInfo->max_program <= 0)
		{
			if (hddMonInfo->hdd_name[7] == 'S' || hddMonInfo->hdd_name[7] == 's')
			{
				hddMonInfo->max_program = 100000;
			}
			else if (hddMonInfo->hdd_name[7] == 'U' || hddMonInfo->hdd_name[7] == 'u')
			{
				hddMonInfo->max_program = 30000;
			}
			else if (hddMonInfo->hdd_name[7] == 'M' || hddMonInfo->hdd_name[7] == 'm')
			{
				hddMonInfo->max_program = 3000;
			}
			else if (hddMonInfo->hdd_name[7] == 'V' || hddMonInfo->hdd_name[7] == 'v')
			{
				hddMonInfo->max_program = 3000;
			}
			else if (hddMonInfo->hdd_name[7] == 'E' || hddMonInfo->hdd_name[7] == 'e')
			{
				hddMonInfo->max_program = 7000;
			}
		}

		if (!bHasLifeRemain)
		{
			if (hddMonInfo->max_program != 0)
			{
				int health = 100 - hddMonInfo->average_program * 100 / hddMonInfo->max_program;
				hddMonInfo->hdd_health_percent = health < 0 ? 0 : health;
			}
			else
			{
				hddMonInfo->hdd_health_percent = 100;
			}
		}
		if (hddMonInfo->max_program >= 0 && hddMonInfo->max_program != INVALID_DEVMON_VALUE && hddMonInfo->average_program > 0)
		{
			double usedDay = 0, estDay = 0, remDay = 0;
			usedDay = (double)hddMonInfo->power_on_time / 24.0;
			estDay = ((double)hddMonInfo->max_program / (double)hddMonInfo->average_program) * (double)usedDay * 0.8;
			remDay = estDay - usedDay;
			hddMonInfo->remain_days = remDay < 0 ? 0 : remDay;
		}
		else if (hddMonInfo->hdd_health_percent >= 0 && hddMonInfo->hdd_health_percent < 100)
		{
			double usedDay = 0, estDay = 0, remDay = 0;
			usedDay = (double)hddMonInfo->power_on_time / 24.0;
			estDay = (usedDay / ((100 - (double)hddMonInfo->hdd_health_percent) / 100)) * 0.8;
			remDay = estDay - usedDay;
			hddMonInfo->remain_days = remDay < 0 ? 0 : remDay;
		}
		else
			hddMonInfo->remain_days = DEFAULT_REMAIN_DAYS;
	}
	else if (hddMonInfo->hdd_type == StdDisk)
	{
		if (hddMonInfo->smartAttriInfoList)
		{
			/*
		  Ref: Acronis Drive Monitor http://kb.acronis.com/content/9264
		  05	Reallocated Sectors Count	     2	70
		  10	Spin Retry Count	             2	50
		  184	End-to-End Error	             1	50
		  196	Reallocation Event Count	     1	40
		  197	Current Pending Sectors Count	 1	40
		  198	Offline uncorrectable Sectors Count	2	70
		  201	Soft Read Error Rate	         1	20
		  */
			int tmpHealth = INVALID_DEVMON_VALUE;
			smart_attri_info_node_t *curAttriInfoNode = hddMonInfo->smartAttriInfoList->next;
			smart_attri_info_t *curAttriInfo = NULL;
			while (curAttriInfoNode)
			{
				curAttriInfo = (smart_attri_info_t *)&curAttriInfoNode->sata;
				switch (curAttriInfo->attriType)
				{
				case ReallocatedSectorsCount:
				{
					if (tmpHealth == INVALID_DEVMON_VALUE)
						tmpHealth = 100;
					{
						unsigned int tmpData = (curAttriInfo->attriVendorData[1] << 8) + curAttriInfo->attriVendorData[0];
						int tmpValue = tmpData * 2 < 70 ? tmpData * 2 : 70;
						tmpHealth = (tmpHealth * (100 - tmpValue)) / 100;
					}
					break;
				}
				case SpinRetryCount:
				{
					if (tmpHealth == INVALID_DEVMON_VALUE)
						tmpHealth = 100;
					{
						unsigned int tmpData = (curAttriInfo->attriVendorData[1] << 8) + curAttriInfo->attriVendorData[0];
						int tmpValue = tmpData * 2 < 50 ? tmpData * 2 : 50;
						tmpHealth = (tmpHealth * (100 - tmpValue)) / 100;
					}
					break;
				}
				case EndtoEnderror:
				{
					if (tmpHealth == INVALID_DEVMON_VALUE)
						tmpHealth = 100;
					{
						unsigned int tmpData = (curAttriInfo->attriVendorData[1] << 8) + curAttriInfo->attriVendorData[0];
						int tmpValue = tmpData < 50 ? tmpData : 50;
						tmpHealth = (tmpHealth * (100 - tmpValue)) / 100;
					}
					break;
				}
				case ReallocationEventCount:
				{
					if (tmpHealth == INVALID_DEVMON_VALUE)
						tmpHealth = 100;
					{
						unsigned int tmpData = (curAttriInfo->attriVendorData[1] << 8) + curAttriInfo->attriVendorData[0];
						int tmpValue = tmpData < 40 ? tmpData : 40;
						tmpHealth = (tmpHealth * (100 - tmpValue)) / 100;
					}
					break;
				}
				case CurrentPendingSectorCount:
				{
					if (tmpHealth == INVALID_DEVMON_VALUE)
						tmpHealth = 100;
					{
						unsigned int tmpData = (curAttriInfo->attriVendorData[1] << 8) + curAttriInfo->attriVendorData[0];
						int tmpValue = tmpData < 40 ? tmpData : 40;
						tmpHealth = (tmpHealth * (100 - tmpValue)) / 100;
					}
					break;
				}
				case UncorrectableSectorCount:
				{
					if (tmpHealth == INVALID_DEVMON_VALUE)
						tmpHealth = 100;
					{
						unsigned int tmpData = (curAttriInfo->attriVendorData[1] << 8) + curAttriInfo->attriVendorData[0];
						int tmpValue = tmpData * 2 < 70 ? tmpData * 2 : 70;
						tmpHealth = (tmpHealth * (100 - tmpValue)) / 100;
					}
					break;
				}
				case OffTrackSoftReadErrorRate:
				{
					if (tmpHealth == INVALID_DEVMON_VALUE)
						tmpHealth = 100;
					{
						unsigned int tmpData = (curAttriInfo->attriVendorData[1] << 8) + curAttriInfo->attriVendorData[0];
						int tmpValue = tmpData < 20 ? tmpData : 20;
						tmpHealth = (tmpHealth * (100 - tmpValue)) / 100;
					}
					break;
				}
				default:
				{
					break;
				}
				}
				curAttriInfoNode = curAttriInfoNode->next;
			}
			hddMonInfo->hdd_health_percent = tmpHealth == INVALID_DEVMON_VALUE ? 100 : tmpHealth;

			if (hddMonInfo->hdd_health_percent >= 0 && hddMonInfo->hdd_health_percent < 100)
			{
				double usedDay = 0, estDay = 0, remDay = 0;
				usedDay = (double)hddMonInfo->power_on_time / 24.0;
				estDay = (usedDay / ((100 - (double)hddMonInfo->hdd_health_percent) / 100)) * 0.8;
				remDay = estDay - usedDay;
				hddMonInfo->remain_days = remDay < 0 ? 0 : remDay;
			}
			else
				hddMonInfo->remain_days = DEFAULT_REMAIN_DAYS;
		}
	}

	bRet = true;
	return bRet;
}

bool GetAttriNameFromType(hdd_type_t hdd_type, smart_attri_type_t attriType, char *attriName)
{
	bool bRet = false;
	if (NULL == attriName)
		return bRet;
	if (hdd_type == SQFNVMe)
	{
		return GetSQNVMeAttriNameFromType(attriType, attriName);
	}
	switch (attriType)
	{
	case ReadErrorRate:
	{
		strcpy(attriName, "ReadErrorRate");
		break;
	}
	case ThroughputPerformance:
	{
		strcpy(attriName, "ThroughputPerformance");
		break;
	}
	case SpinUpTime:
	{
		strcpy(attriName, "SpinUpTime");
		break;
	}
	case StartStopCount:
	{
		strcpy(attriName, "StartStopCount");
		break;
	}
	case ReallocatedSectorsCount:
	{
		strcpy(attriName, "ReallocatedSectorsCount");
		break;
	}
	case ReadChannelMargin:
	{
		strcpy(attriName, "ReadChannelMargin");
		break;
	}
	case SeekErrorRate:
	{
		strcpy(attriName, "SeekErrorRate");
		break;
	}
	case SeekTimePerformance:
	{
		strcpy(attriName, "SeekTimePerformance");
		break;
	}
	case PowerOnHoursPOH:
	{
		strcpy(attriName, "PowerOnHoursPOH");
		break;
	}
	case SpinRetryCount:
	{
		strcpy(attriName, "SpinRetryCount");
		break;
	}
	case CalibrationRetryCount:
	{
		strcpy(attriName, "CalibrationRetryCount");
		break;
	}
	case PowerCycleCount:
	{
		strcpy(attriName, "PowerCycleCount");
		break;
	}
	case SoftReadErrorRate:
	{
		strcpy(attriName, "SoftReadErrorRate");
		break;
	}
	case SQDeviceCapacity:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "DeviceCapacity");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQUserCapacity:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "UserCapacity");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQInitialSpareBlocksAvailable:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "InitialSpareBlocksAvailable");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQSpareBlocksRemaining:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "SpareBlocksRemaining");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQTotalEraseCount:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "TotalEraseCount");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQSATAPHYErrorCount:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "SATAPHYErrorCount");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQBadBlockCount:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "BadBlockCount");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQEraseCount:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "EraseCount");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQUnexpectedPowerLossCount:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "UnexpectedPowerLossCount");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQPowerFailureProtectionStatus:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "PowerFailureProtectionStatus");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SATADownshiftErrorCount:
	{
		strcpy(attriName, "SATADownshiftErrorCount");
		break;
	}
	case EndtoEnderror:
	{
		strcpy(attriName, "EndtoEnderror");
		break;
	}
	case HeadStability:
	{
		strcpy(attriName, "HeadStability");
		break;
	}
	case InducedOpVibrationDetection:
	{
		strcpy(attriName, "InducedOpVibrationDetection");
		break;
	}
	case ReportedUncorrectableErrors:
	{
		strcpy(attriName, "ReportedUncorrectableErrors");
		break;
	}
	case CommandTimeout:
	{
		strcpy(attriName, "CommandTimeout");
		break;
	}
	case HighFlyWrites:
	{
		strcpy(attriName, "HighFlyWrites");
		break;
	}
	case AirflowTemperatureWDC:
	{
		strcpy(attriName, "AirflowTemperatureWDC");
		break;
	}
		// 	case TemperatureDifferencefrom100:
		// 		{
		// 			strcpy(attriName, "TemperatureDifferencefrom100");
		// 			break;
		// 		}
	case GSenseErrorRate:
	{
		strcpy(attriName, "GSenseErrorRate");
		break;
	}
	case PoweroffRetractCount:
	{

		if (hdd_type == SQFlash)
			strcpy(attriName, "UnexpectedPowerLossCount");
		else
			strcpy(attriName, "PoweroffRetractCount");
		break;
	}
	case LoadCycleCount:
	{
		strcpy(attriName, "LoadCycleCount");
		break;
	}
	case Temperature:
	{
		strcpy(attriName, "Temperature");
		break;
	}
	case HardwareECCRecovered:
	{
		strcpy(attriName, "HardwareECCRecovered");
		break;
	}
	case ReallocationEventCount:
	{
		strcpy(attriName, "ReallocationEventCount");
		break;
	}
	case CurrentPendingSectorCount:
	{
		strcpy(attriName, "CurrentPendingSectorCount");
		break;
	}
	case UncorrectableSectorCount:
	{
		strcpy(attriName, "UncorrectableSectorCount");
		break;
	}
	case UltraDMACRCErrorCount:
	{
		strcpy(attriName, "UltraDMACRCErrorCount");
		break;
	}
	case MultiZoneErrorRate:
	{
		strcpy(attriName, "MultiZoneErrorRate");
		break;
	}
		// 	case WriteErrorRateFujitsu:
		// 		{
		// 			strcpy(attriName, "WriteErrorRateFujitsu");
		// 			break;
		// 		}
	case OffTrackSoftReadErrorRate:
	{
		strcpy(attriName, "OffTrackSoftReadErrorRate");
		break;
	}
	case DataAddressMarkerrors:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "PercentageOfSparesRemaining");
		else
			strcpy(attriName, "DataAddressMarkerrors");
		break;
	}
	case RunOutCancel:
	{
		strcpy(attriName, "RunOutCancel");
		break;
	}
	case SoftECCCorrection:
	{
		strcpy(attriName, "SoftECCCorrection");
		break;
	}
	case ThermalAsperityRateTAR:
	{
		strcpy(attriName, "ThermalAsperityRateTAR");
		break;
	}
	case FlyingHeight:
	{
		strcpy(attriName, "FlyingHeight");
		break;
	}
	case SpinHighCurrent:
	{
		strcpy(attriName, "SpinHighCurrent");
		break;
	}
	case SpinBuzz:
	{
		strcpy(attriName, "SpinBuzz");
		break;
	}
	case OfflineSeekPerformance:
	{
		strcpy(attriName, "OfflineSeekPerformance");
		break;
	}
	case VibrationDuringWrite:
	{
		strcpy(attriName, "VibrationDuringWrite");
		break;
	}
	case ShockDuringWrite:
	{
		strcpy(attriName, "ShockDuringWrite");
		break;
	}
	case SQCRCError:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "CRCError");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case DiskShift:
	{
		strcpy(attriName, "DiskShift");
		break;
	}
	case GSenseErrorRateAlt:
	{
		strcpy(attriName, "GSenseErrorRateAlt");
		break;
	}
	case LoadedHours:
	{
		strcpy(attriName, "LoadedHours");
		break;
	}
	case LoadUnloadRetryCount:
	{
		strcpy(attriName, "LoadUnloadRetryCount");
		break;
	}
	case LoadInTime:
	{
		strcpy(attriName, "LoadInTime");
		break;
	}
	case TorqueAmplificationCount:
	{
		strcpy(attriName, "TorqueAmplificationCount");
		break;
	}
	case PowerOffRetractCycle:
	{
		strcpy(attriName, "PowerOffRetractCycle");
		break;
	}
	case GMRHeadAmplitude:
	{
		strcpy(attriName, "GMRHeadAmplitude");
		break;
	}
	case DriveTemperature:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "SSDLifeRemaining");
		else
			strcpy(attriName, "DriveTemperature");
		break;
	}
	case SQEnduranceRemainLife:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "SQEnduranceRemainLife");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQPowerOnTime:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "SQPowerOnTime");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQTotalNANDRead: //Old SQECCLog:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "TotalNANDRead");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case SQTotalNANDWritten: //Old SQGoodBlockRate:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "TotalNANDWritten");
		else
			sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	case HeadFlyingHours:
	{
		strcpy(attriName, "HeadFlyingHours");
		break;
	}
		// 	case TransferErrorRateFujitsu:
		// 		{
		// 			strcpy(attriName, "TransferErrorRateFujitsu");
		// 			break;
		// 		}
	case TotalLBAsWritten:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "TotalHostWrite");
		else
			strcpy(attriName, "TotalLBAsWritten");
		break;
	}
	case TotalLBAsRead:
	{
		if (hdd_type == SQFlash)
			strcpy(attriName, "TotalHostRead");
		else
			strcpy(attriName, "TotalLBAsRead");
		break;
	}
	case ReadErrorRetryRate:
	{
		strcpy(attriName, "ReadErrorRetryRate");
		break;
	}
	case FreeFallProtection:
	{
		strcpy(attriName, "FreeFallProtection");
		break;
	}
	default:
	{
		sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	}
	bRet = true;
	return bRet;
}
bool GetSQNVMeAttriNameFromType(nvme_smart_attri_type_t attriType, char *attriName)
{
	bool bRet = false;
	if (NULL == attriName)
		return bRet;
	switch (attriType)
	{
		// NVMe Standard SMART
	case CriticalWarning:
	{
		strcpy(attriName, "CriticalWarning");
		break;
	}
	case CompositeTemperature:
	{
		strcpy(attriName, "CompositeTemperature");
		break;
	}
	case AvailableSpare:
	{
		strcpy(attriName, "AvailableSpare");
		break;
	}
	case AvailableSpareThreshold:
	{
		strcpy(attriName, "AvailableSpareThreshold");
		break;
	}
	case PercentageUsed:
	{
		strcpy(attriName, "PercentageUsed");
		break;
	}
	case DataUnitsRead:
	{
		strcpy(attriName, "DataUnitsRead");
		break;
	}
	case DataUnitsWritten:
	{
		strcpy(attriName, "DataUnitsWritten");
		break;
	}
	case HostReadCommands:
	{
		strcpy(attriName, "HostReadCommands");
		break;
	}
	case HostWriteCommands:
	{
		strcpy(attriName, "HostWriteCommands");
		break;
	}
	case ControllerBusyTime:
	{
		strcpy(attriName, "ControllerBusyTime");
		break;
	}
	case PowerCycles:
	{
		strcpy(attriName, "PowerCycles");
		break;
	}
	case PowerOnHours:
	{
		strcpy(attriName, "PowerOnHours");
		break;
	}
	case UnsafeShutdowns:
	{
		strcpy(attriName, "UnsafeShutdowns");
		break;
	}
	case MediaErrors:
	{
		strcpy(attriName, "MediaErrors");
		break;
	}
	case ErrorLogNumber:
	{
		strcpy(attriName, "ErrorLogNumber");
		break;
	}
	case WarningCompositeTemperatureTime:
	{
		strcpy(attriName, "WarningCompositeTemperatureTime");
		break;
	}
	case CriticalCompositeTemperatureTime:
	{
		strcpy(attriName, "CriticalCompositeTemperatureTime");
		break;
	}
	case TemperatureSensor1:
	{
		strcpy(attriName, "TemperatureSensor1");
		break;
	}
	case TemperatureSensor2:
	{
		strcpy(attriName, "TemperatureSensor2");
		break;
	}
	case TemperatureSensor3:
	{
		strcpy(attriName, "TemperatureSensor3");
		break;
	}
	case TemperatureSensor4:
	{
		strcpy(attriName, "TemperatureSensor4");
		break;
	}
	case TemperatureSensor5:
	{
		strcpy(attriName, "TemperatureSensor5");
		break;
	}
	case TemperatureSensor6:
	{
		strcpy(attriName, "TemperatureSensor6");
		break;
	}
	case TemperatureSensor7:
	{
		strcpy(attriName, "TemperatureSensor7");
		break;
	}
	case TemperatureSensor8:
	{
		strcpy(attriName, "TemperatureSensor8");
		break;
	}
	// NVMe Vendor SMART
	case FlashReadSector:
	{
		strcpy(attriName, "FlashReadSector");
		break;
	}
	case FlashWriteSector:
	{
		strcpy(attriName, "FlashWriteSector");
		break;
	}
	case UNCError:
	{
		strcpy(attriName, "UNCError");
		break;
	}
	case PyhError:
	{
		strcpy(attriName, "PyhError");
		break;
	}
	case EarlyBadBlock:
	{
		strcpy(attriName, "EarlyBadBlock");
		break;
	}
	case LaterBadBlock:
	{
		strcpy(attriName, "LaterBadBlock");
		break;
	}
	case MaxEraseCount:
	{
		strcpy(attriName, "MaxEraseCount");
		break;
	}
	case AvgEraseCount:
	{
		strcpy(attriName, "AvgEraseCount");
		break;
	}
	case CurPercentSpares:
	{
		strcpy(attriName, "CurPercentSpares");
		break;
	}
	case CurTemperature:
	{
		strcpy(attriName, "CurTemperature");
		break;
	}
	case LowestTemperature:
	{
		strcpy(attriName, "LowestTemperature");
		break;
	}
	case HighestTemperature:
	{
		strcpy(attriName, "HighestTemperature");
		break;
	}
	case ChipInternalTemperature:
	{
		strcpy(attriName, "ChipInternalTemperature");
		break;
	}
	case SpareBlocks:
	{
		strcpy(attriName, "SpareBlocks");
		break;
	}
	default:
	{
		sprintf(attriName, "Unknown_%d", attriType);
		break;
	}
	}
	bRet = true;
	return bRet;
}

bool hdd_SelfTest(int devID)
{
	return SQFSelfTest(devID);
}

bool hdd_GetSelfTestStatus(int devID, unsigned int* progress, unsigned int* status)
{
	return GetSQFSelfTestStatus(devID, progress, status);
}