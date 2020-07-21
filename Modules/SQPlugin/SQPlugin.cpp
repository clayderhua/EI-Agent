/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2020 by Scott Chang										*/
/* Modified Date: 2020/5/13 by Scott Chang									*/
/* Abstract     : SQ Management Plugin										*/
/* Reference    : None														*/
/****************************************************************************/
#include "SQPlugin.h"
#include "IoTMessageGenerate.h"
#include "unistd.h"
#include "HandlerKernel.h"
#include "WISEPlatform.h"
#include "susiaccess_handler_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "util_storage.h"
#include "HandlerKernel.h"
#include "IoTMessageGenerate.h"
#include "util_path.h"
#include "ReadINI.h"
#include "sqflashhelper.h"
#include "SQRam.h"
#include "McAfeeHelper.h"
#include "SQPluginLog.h"
#include "WMIHelper.h"
//#include <vld.h>

//-----------------------------------------------------------------------------
// CONST STRING DEFINE
//-----------------------------------------------------------------------------
#define MyTopic	 "SQPlugin"

#define HDD_INFO_LIST "hddInfoList"
#define HDD_SMART_INFO_LIST "hddSmartInfoList"
#define HDD_STORAGE_LIST "DiskInfo"

#define HDD_NAME "hddName"
#define HDD_INDEX "hddIndex"
#define HDD_POWER_ON_TIME "powerOnTime"
#define HDD_TEMP "hddTemp"
#define HDD_MAX_TEMP "hddMaxTemp"
#define HDD_ECC_COUNT "eccCount"
#define HDD_POWER_CYCLE "powerCycle"
#define HDD_POWERLOSS_COUNT "unexpectedPowerLossCount"
#define HDD_BAD_BLOCK "laterBadBlock"
#define HDD_CRC_ERROR "crcError"
#define HDD_OPAL_SUPPORT "opalSupport"
#define HDD_OPAL_ENABLE "opalEnable"
#define HDD_OPAL_LOCKED "opalLocked"
#define HDD_HEALTH "health"
#define HDD_REMAIN_DAYS "remainDays"
#define HDD_PRODUCT_SERIALNUMBER "productSerialNumber"
#define HDD_FIRMWARE_REVISION "firmwareRevision"
#define HDD_NAND_READ "nandRead"
#define HDD_NAND_WRITE "nandWrite"
#define HDD_HOST_READ "hostRead"
#define HDD_HOST_WRITE "hostWrite"

#define SENSOR_UNIT_PERCENT "%"
#define SENSOR_UNIT_HOUR "hour"
#define SENSOR_UNIT_CELSIUS "celsius"

#define SMART_BASEINFO "BaseInfo"
#define SMART_ATTRI_TYPE "type"
#define SMART_ATTRI_NAME "name"
#define SMART_ATTRI_FLAGS "flags"
#define SMART_ATTRI_WORST "worst"
#define SMART_ATTRI_THRESH "thresh"
#define SMART_ATTRI_VALUE "value"
#define SMART_ATTRI_VENDOR "vendorData"

#define STORAGE_TOTAL_SPACE "Total Disk Space"
#define STORAGE_FREE_SPACE "Free Disk Space"
#define STORAGE_SPACE_USAGE "DiskSpaceUsage"
#define STORAGE_DRIVE_LETTER "DiskDriveLetter"

#define STORAGE_READ_SPEED "ReadBytePerSec"
#define STORAGE_WRITE_SPEED "WriteBytePerSec"

#define MCAFEE_VERSION "McAfeeVersion"
#define MCAFEE_UPDATE "McAfeeUpdate"

#define SQ_INFO_LIST "info"
#define SQ_DATA_LIST "data"
#define SQ_ACTION_LIST "action"

#define MCAFEE_DIRECTORY_DEFAULT "..\\SQFlash\\mcafee\\"

#define HDD_INI_COTENT "[Platform]\nInterval=10\n#Interval: The time delay between two access round in second.\nMcAfeeDir=%s\n#McAfeeDir: The path of mcscan32.dll in SQFlash Utility.\n#Download from : https://www2.advantech.com/embedded-boards-design-in-services/embeddedmodule/Software-utility.aspx"
//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
typedef struct hdd_context_t {
	hdd_info_t		hddInfo;
	pthread_mutex_t	hddMutex;
}  hdd_context_t;

typedef struct mcafee_t {
	bool bNewMcAfee;
	int iMcAfeeVer;
	void* threadMCSHandler;
	int iTotalScan;
	int iInfected;
	int iVersionCheckInterval;
	bool bUpdate;
	bool bScan;
}  mcafee_t;

typedef struct selftest_t {
	void* threadSelfTestHandler;
	int iCurHDDIndex;
	bool bThreadSelfTest;
}  selftest_t;

typedef struct SQ_context_t {
	void* threadHandler;
	bool					bThreadRunning;
	bool					bHasSQFlash;
	struct hdd_context_t	hddCtx;
	DiskItemList* storageList;
	sqram_info_t			sqramList;
	bool					bFinish;
	bool bHasMcAfee;
	mcafee_t mcafeectx;
	selftest_t hddtestctx;
} SQ_context_t;

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
Handler_info g_PluginInfo;
HANDLER_THREAD_STATUS g_Status = handler_status_no_init;
SQ_context_t g_HandlerContex;
MSG_CLASSIFY_T* g_Capability = NULL; /*the global message structure to describe the sensor data as the handelr capability*/
int g_iRetrieveInterval = 10;		 //10 sec.
char g_strMcAfeeDir[260] = { 0 };
//-----------------------------------------------------------------------------
// Functions:
//-----------------------------------------------------------------------------
void Handler_Uninitialize();
#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL) // Dynamic load
		{
			// Initialize your stuff or whatever
			// Return FALSE if you don't want your module to be dynamically loaded
		}
		else // Static load
		{
			// Return FALSE if you don't want your module to be statically loaded
			return FALSE;
		}
	}

	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
	{
		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize();
		}
		else // Process is terminating
		{
			// Cleanup
			Handler_Uninitialize();
		}
	}
	return TRUE;
}
#else
__attribute__((constructor))
/**
 * initializer of the shared lib.
 */
static void Initializer(int argc, char** argv, char** envp)
{

}

__attribute__((destructor))
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer()
{
	Handler_Uninitialize();
}
#endif

bool UpdateStorageList(DiskItemList* current, DiskItemList* newone)
{
	bool bChanged = false;
	if (newone)
	{
		DiskItemList* item = newone;
		while (item)
		{
			DiskInfoItem* disk = item->disk;
			if (disk)
			{
				char* target = disk->name;
				bool bFound = false;
				DiskItemList* currentitem = current;
				while (currentitem)
				{
					DiskInfoItem* currentdisk = currentitem->disk;
					if (currentdisk)
					{
						if (strcmp(target, currentdisk->name) == 0)
						{
							bFound = true;
							break;
						}
					}
					currentitem = currentitem->next;
				}

				if (!bFound)
				{
					DiskItemList* newdisk = (DiskItemList*)calloc(1, sizeof(DiskItemList));
					DiskItemList* last = current;

					newdisk->disk = (DiskInfoItem*)calloc(1, sizeof(DiskInfoItem));
					memcpy(newdisk->disk, disk, sizeof(DiskInfoItem));

					while (last)
					{
						if (last->next)
							last = last->next;
						else
							break;
					}
					last->next = newdisk;

					bChanged = true;
				}
			}
			item = item->next;
		}
	}
	return bChanged;
}

bool GetStorageInfo(DiskItemList** storages)
{
	DiskItemList* item = NULL;
	bool bChanged = false;
	if (*storages == NULL)
	{
		*storages = util_storage_getdisklist();
		bChanged = true;
	}
	else
	{
		DiskItemList* tmpstorages = util_storage_getdisklist();
		bChanged = UpdateStorageList(*storages, tmpstorages);
		util_storage_freedisklist(tmpstorages);
		tmpstorages = NULL;
	}

	item = *storages;
	while (item)
	{
		if (item->disk)
		{
			DiskInfoItem* disk = item->disk;
			disk->totalspace = util_storage_gettotaldiskspace(disk->name);
			disk->freespace = util_storage_getfreediskspace(disk->name);
			/*if(disk->totalspace>0)
				disk->usage = (1-(double)disk->freespace/(double)disk->totalspace)*100;*/
			SQLog(Debug, "Disk(%s) %I64u/%I64u MB\n", disk->name, disk->freespace, disk->totalspace);
		}
		item = item->next;
	}
	util_storage_getdiskspeed(*storages);
	return bChanged;
}

double B2MB(unsigned long long value)
{
	double result = value / (1024 * 1024);
	return result;
}

void UpdateStorageInfo(DiskItemList* storages, int diskid, MSG_CLASSIFY_T* diskGroup)
{
	if (storages && diskGroup)
	{
		double disktotal = 0;
		double diskfree = 0;
		double diskusage = 0;
		char cDrivelatter[128] = { 0 };
		unsigned long long writebyte = 0;
		unsigned long long readpersec = 0;
		MSG_ATTRIBUTE_T* attr = NULL;
		DiskItemList* item = storages;
		while (item)
		{
			if (item->disk && item->disk->diskid == diskid)
			{
				DiskInfoItem* disk = item->disk;
				double total = B2MB(disk->totalspace);
				double free = B2MB(disk->freespace);
				disktotal += total;
				diskfree += free;
				if (strlen(cDrivelatter) == 0)
					sprintf(cDrivelatter, "%s", disk->name);
				else
				{
					char cTemp[5] = { 0 };
					sprintf(cTemp, ",%s", disk->name);
					strncat(cDrivelatter, cTemp, sizeof(cDrivelatter));
				}

				readpersec = disk->readpersec;
				writebyte = disk->writepersec;
			}
			item = item->next;
		}

		
		attr = IoT_FindSensorNode(diskGroup, STORAGE_DRIVE_LETTER);
		if (attr == NULL)
			attr = IoT_AddSensorNode(diskGroup, STORAGE_DRIVE_LETTER);
		IoT_SetStringValue(attr, cDrivelatter, IoT_READONLY);

		attr = IoT_FindSensorNode(diskGroup, STORAGE_TOTAL_SPACE);
		if (attr == NULL)
			attr = IoT_AddSensorNode(diskGroup, STORAGE_TOTAL_SPACE);
		IoT_SetDoubleValueWithMaxMin(attr, disktotal, IoT_READONLY, disktotal, disktotal, "Megabyte");

		attr = IoT_FindSensorNode(diskGroup, STORAGE_FREE_SPACE);
		if (attr == NULL)
			attr = IoT_AddSensorNode(diskGroup, STORAGE_FREE_SPACE);
		IoT_SetDoubleValueWithMaxMin(attr, diskfree, IoT_READONLY, disktotal, 0, "Megabyte");

		if (disktotal > 0)
		{
			diskusage = (double)(disktotal - diskfree) * 100 / (double)disktotal;

			attr = IoT_FindSensorNode(diskGroup, STORAGE_SPACE_USAGE);
			if (attr == NULL)
				attr = IoT_AddSensorNode(diskGroup, STORAGE_SPACE_USAGE);
			IoT_SetDoubleValueWithMaxMin(attr, diskusage, IoT_READONLY, 100, 0, "%");
		}

		attr = IoT_FindSensorNode(diskGroup, STORAGE_READ_SPEED);
		if (attr == NULL)
			attr = IoT_AddSensorNode(diskGroup, STORAGE_READ_SPEED);
		IoT_SetDoubleValue(attr, readpersec, IoT_READONLY, "Byte");

		attr = IoT_FindSensorNode(diskGroup, STORAGE_WRITE_SPEED);
		if (attr == NULL)
			attr = IoT_AddSensorNode(diskGroup, STORAGE_WRITE_SPEED);
		IoT_SetDoubleValue(attr, writebyte, IoT_READONLY, "Byte");
	}
}

void UpdateHDDInfo(hdd_info_t* hddInfo, DiskItemList* storages, MSG_CLASSIFY_T* hddInfoGroup)
{
	if (hddInfo && hddInfo->hddMonInfoList && hddInfoGroup)
	{
		hdd_mon_info_node_t* item = hddInfo->hddMonInfoList->next;
		//construct CJSON of HDD Info and S.M.A.R.T. Info, notice that SQFlash and Std HDD are different
		while (item != NULL)
		{
			char cTemp[128] = { 0 };
			MSG_CLASSIFY_T* indexGroup = NULL;
			MSG_ATTRIBUTE_T* attr = NULL;
			hdd_mon_info_t* pHDDMonInfo = &item->hddMonInfo;
			if (pHDDMonInfo->hdd_type != SQFlash && pHDDMonInfo->hdd_type != SQFNVMe)
			{
				/*Not SQFlash*/
				item = item->next;
				continue;
			}

			memset(cTemp, 0, sizeof(cTemp));
			sprintf(cTemp, "Disk%d", pHDDMonInfo->hdd_index);
			indexGroup = IoT_FindGroup(hddInfoGroup, cTemp);
			if (indexGroup == NULL)
				indexGroup = IoT_AddGroup(hddInfoGroup, cTemp);

			attr = IoT_FindSensorNode(indexGroup, HDD_NAME);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_NAME);
			IoT_SetStringValue(attr, pHDDMonInfo->hdd_name, IoT_READONLY);

			attr = IoT_FindSensorNode(indexGroup, HDD_INDEX);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_INDEX);
			IoT_SetDoubleValue(attr, pHDDMonInfo->hdd_index, IoT_READONLY, NULL);

			attr = IoT_FindSensorNode(indexGroup, HDD_POWER_ON_TIME);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_POWER_ON_TIME);
			//IoT_SetDoubleValue(attr, pHDDMonInfo->power_on_time, IoT_READONLY, SENSOR_UNIT_HOUR);
			IoT_SetDoubleValueWithMaxMin(attr, pHDDMonInfo->power_on_time, IoT_READONLY, 4294967295, 0, SENSOR_UNIT_HOUR);

			attr = IoT_FindSensorNode(indexGroup, HDD_HEALTH);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_HEALTH);
			//IoT_SetDoubleValue(attr, pHDDMonInfo->hdd_health_percent, IoT_READONLY, SENSOR_UNIT_PERCENT);
			IoT_SetDoubleValueWithMaxMin(attr, pHDDMonInfo->hdd_health_percent, IoT_READONLY, 100, 0, SENSOR_UNIT_PERCENT);

				attr = IoT_FindSensorNode(indexGroup, HDD_ECC_COUNT);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_ECC_COUNT);
				//IoT_SetDoubleValue(attr, pHDDMonInfo->ecc_count, IoT_READONLY, NULL);
				IoT_SetDoubleValueWithMaxMin(attr, pHDDMonInfo->ecc_count, IoT_READONLY, 65535, 0, NULL);

				attr = IoT_FindSensorNode(indexGroup, HDD_POWERLOSS_COUNT);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_POWERLOSS_COUNT);
				//IoT_SetDoubleValue(attr, pHDDMonInfo->average_program, IoT_READONLY, NULL);
				IoT_SetDoubleValue(attr, pHDDMonInfo->unexpected_power_loss_count, IoT_READONLY, NULL);

				attr = IoT_FindSensorNode(indexGroup, HDD_BAD_BLOCK);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_BAD_BLOCK);
				IoT_SetDoubleValue(attr, pHDDMonInfo->later_bad_block, IoT_READONLY, NULL);

				attr = IoT_FindSensorNode(indexGroup, HDD_OPAL_SUPPORT);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_OPAL_SUPPORT);
				IoT_SetBoolValue(attr, pHDDMonInfo->opal_status & 0x01 ? true : false, IoT_READONLY);

				attr = IoT_FindSensorNode(indexGroup, HDD_OPAL_ENABLE);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_OPAL_ENABLE);
				IoT_SetBoolValue(attr, pHDDMonInfo->opal_status & 0x02 ? true : false, IoT_READONLY);

				attr = IoT_FindSensorNode(indexGroup, HDD_OPAL_LOCKED);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_OPAL_LOCKED);
				IoT_SetBoolValue(attr, pHDDMonInfo->opal_status & 0x04 ? true : false, IoT_READONLY);

				if (pHDDMonInfo->hdd_type != SQFNVMe)
				{
					attr = IoT_FindSensorNode(indexGroup, HDD_CRC_ERROR);
					if (attr == NULL)
						attr = IoT_AddSensorNode(indexGroup, HDD_CRC_ERROR);
					//IoT_SetDoubleValue(attr, pHDDMonInfo->good_block_rate, IoT_READONLY, SENSOR_UNIT_PERCENT);
					IoT_SetDoubleValue(attr, pHDDMonInfo->crc_error, IoT_READONLY, NULL);
				}

				attr = IoT_FindSensorNode(indexGroup, HDD_TEMP);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_TEMP);
				//IoT_SetDoubleValue(attr, pHDDMonInfo->hdd_temp, IoT_READONLY, SENSOR_UNIT_CELSIUS);
				IoT_SetDoubleValue(attr, pHDDMonInfo->hdd_temp, IoT_READONLY, SENSOR_UNIT_CELSIUS);

				attr = IoT_FindSensorNode(indexGroup, HDD_MAX_TEMP);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_MAX_TEMP);
				//IoT_SetDoubleValue(attr, pHDDMonInfo->hdd_temp, IoT_READONLY, SENSOR_UNIT_CELSIUS);
				IoT_SetDoubleValue(attr, pHDDMonInfo->max_temp, IoT_READONLY, SENSOR_UNIT_CELSIUS);

			if (pHDDMonInfo->hdd_type != SQFNVMe)
			{
				attr = IoT_FindSensorNode(indexGroup, HDD_NAND_READ);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_NAND_READ);
				IoT_SetDoubleValue(attr, pHDDMonInfo->nand_read, IoT_READONLY, NULL);

				attr = IoT_FindSensorNode(indexGroup, HDD_NAND_WRITE);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_NAND_WRITE);
				IoT_SetDoubleValue(attr, pHDDMonInfo->nand_write, IoT_READONLY, NULL);
			}
			attr = IoT_FindSensorNode(indexGroup, HDD_HOST_READ);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_HOST_READ);
			IoT_SetDoubleValue(attr, pHDDMonInfo->host_read, IoT_READONLY, NULL);

			attr = IoT_FindSensorNode(indexGroup, HDD_HOST_WRITE);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_HOST_WRITE);
			IoT_SetDoubleValue(attr, pHDDMonInfo->host_write, IoT_READONLY, NULL);

			attr = IoT_FindSensorNode(indexGroup, HDD_POWER_CYCLE);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_POWER_CYCLE);
			//IoT_SetDoubleValue(attr, pHDDMonInfo->max_program, IoT_READONLY, NULL);
			IoT_SetDoubleValue(attr, pHDDMonInfo->power_cycle, IoT_READONLY, NULL);

			if (strlen(pHDDMonInfo->SerialNumber) > 0)
			{
				attr = IoT_FindSensorNode(indexGroup, HDD_PRODUCT_SERIALNUMBER);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_PRODUCT_SERIALNUMBER);
				IoT_SetStringValue(attr, pHDDMonInfo->SerialNumber, IoT_READONLY);
			}

			if (strlen(pHDDMonInfo->FirmwareRevision) > 0)
			{
				attr = IoT_FindSensorNode(indexGroup, HDD_FIRMWARE_REVISION);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_FIRMWARE_REVISION);
				IoT_SetStringValue(attr, pHDDMonInfo->FirmwareRevision, IoT_READONLY);
			}

			attr = IoT_FindSensorNode(indexGroup, HDD_REMAIN_DAYS);
			if (attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_REMAIN_DAYS);
			IoT_SetDoubleValue(attr, pHDDMonInfo->remain_days, IoT_READONLY, "Day(s)");

			UpdateStorageInfo(storages, pHDDMonInfo->hdd_index, indexGroup);
			item = item->next;
		} // while End
	}
}

void UpdateSMARTInfo(hdd_info_t* hddInfo, MSG_CLASSIFY_T* smartInfoGroup)
{
	if (hddInfo && hddInfo->hddMonInfoList && smartInfoGroup)
	{
		hdd_mon_info_node_t* item = hddInfo->hddMonInfoList->next;
		while (item != NULL)
		{
			hdd_mon_info_t* pHDDMonInfo = &item->hddMonInfo;
			if (pHDDMonInfo->hdd_type != SQFlash && pHDDMonInfo->hdd_type != SQFNVMe)
			{
				item = item->next;
				continue;
			}
			if (pHDDMonInfo->smartAttriInfoList)
			{
				char cTemp[128] = { 0 };
				MSG_CLASSIFY_T* indexGroup = NULL;
				MSG_CLASSIFY_T* basetGroup = NULL;
				MSG_ATTRIBUTE_T* attr = NULL;
				smart_attri_info_node_t* curSmartAttri = pHDDMonInfo->smartAttriInfoList->next;
				memset(cTemp, 0, sizeof(cTemp));
				sprintf(cTemp, "Disk%d", pHDDMonInfo->hdd_index);

				indexGroup = IoT_FindGroup(smartInfoGroup, cTemp);
				if (indexGroup == NULL)
					indexGroup = IoT_AddGroup(smartInfoGroup, cTemp);

				basetGroup = IoT_FindGroup(indexGroup, SMART_BASEINFO);
				if (basetGroup == NULL)
					basetGroup = IoT_AddGroup(indexGroup, SMART_BASEINFO);

				attr = IoT_FindSensorNode(basetGroup, HDD_NAME);
				if (attr == NULL)
					attr = IoT_AddSensorNode(basetGroup, HDD_NAME);
				IoT_SetStringValue(attr, pHDDMonInfo->hdd_name, IoT_READONLY);

				attr = IoT_FindSensorNode(basetGroup, HDD_INDEX);
				if (attr == NULL)
					attr = IoT_AddSensorNode(basetGroup, HDD_INDEX);
				IoT_SetDoubleValue(attr, pHDDMonInfo->hdd_index, IoT_READONLY, NULL);

				while (curSmartAttri)
				{
					if (curSmartAttri->bNVMe)
					{
						MSG_CLASSIFY_T* smartGroup = IoT_FindGroup(indexGroup, curSmartAttri->nvme.attriName);
						if (smartGroup == NULL)
							smartGroup = IoT_AddGroup(indexGroup, curSmartAttri->nvme.attriName);

						attr = IoT_FindSensorNode(smartGroup, SMART_ATTRI_TYPE);
						if (attr == NULL)
							attr = IoT_AddSensorNode(smartGroup, SMART_ATTRI_TYPE);
						IoT_SetDoubleValue(attr, curSmartAttri->nvme.attriType, IoT_READONLY, NULL);

						attr = IoT_FindSensorNode(smartGroup, SMART_ATTRI_VALUE);
						if (attr == NULL)
							attr = IoT_AddSensorNode(smartGroup, SMART_ATTRI_VALUE);
						IoT_SetDoubleValue(attr, curSmartAttri->nvme.attriValue, IoT_READONLY, NULL);

						if (curSmartAttri->nvme.datalen > 0)
						{
							char vendorDataStr[33] = { 0 };
							int i = curSmartAttri->nvme.datalen - 1;
							for (; i >= 0; i--)
							{
								char buf[4] = { 0 };
								snprintf(buf, sizeof(buf), "%02X", (unsigned char)curSmartAttri->nvme.attriVendorData[i]);
								strcat(vendorDataStr, buf);
							}

							attr = IoT_FindSensorNode(smartGroup, SMART_ATTRI_VENDOR);
							if (attr == NULL)
								attr = IoT_AddSensorNode(smartGroup, SMART_ATTRI_VENDOR);
							IoT_SetStringValue(attr, vendorDataStr, IoT_READONLY);
						}
					}
					else
					{
						char vendorDataStr[16] = { 0 };
						MSG_CLASSIFY_T* smartGroup = IoT_FindGroup(indexGroup, curSmartAttri->sata.attriName);
						if (smartGroup == NULL)
							smartGroup = IoT_AddGroup(indexGroup, curSmartAttri->sata.attriName);

						attr = IoT_FindSensorNode(smartGroup, SMART_ATTRI_TYPE);
						if (attr == NULL)
							attr = IoT_AddSensorNode(smartGroup, SMART_ATTRI_TYPE);
						IoT_SetDoubleValue(attr, curSmartAttri->sata.attriType, IoT_READONLY, NULL);

						attr = IoT_FindSensorNode(smartGroup, SMART_ATTRI_FLAGS);
						if (attr == NULL)
							attr = IoT_AddSensorNode(smartGroup, SMART_ATTRI_FLAGS);
						IoT_SetDoubleValue(attr, curSmartAttri->sata.attriFlags, IoT_READONLY, NULL);

						attr = IoT_FindSensorNode(smartGroup, SMART_ATTRI_WORST);
						if (attr == NULL)
							attr = IoT_AddSensorNode(smartGroup, SMART_ATTRI_WORST);
						IoT_SetDoubleValue(attr, curSmartAttri->sata.attriWorst, IoT_READONLY, NULL);

						attr = IoT_FindSensorNode(smartGroup, SMART_ATTRI_VALUE);
						if (attr == NULL)
							attr = IoT_AddSensorNode(smartGroup, SMART_ATTRI_VALUE);
						IoT_SetDoubleValue(attr, curSmartAttri->sata.attriValue, IoT_READONLY, NULL);

						snprintf(vendorDataStr, sizeof(vendorDataStr), "%02X%02X%02X%02X%02X%02X",
							(unsigned char)curSmartAttri->sata.attriVendorData[5],
							(unsigned char)curSmartAttri->sata.attriVendorData[4],
							(unsigned char)curSmartAttri->sata.attriVendorData[3],
							(unsigned char)curSmartAttri->sata.attriVendorData[2],
							(unsigned char)curSmartAttri->sata.attriVendorData[1],
							(unsigned char)curSmartAttri->sata.attriVendorData[0]);

						attr = IoT_FindSensorNode(smartGroup, SMART_ATTRI_VENDOR);
						if (attr == NULL)
							attr = IoT_AddSensorNode(smartGroup, SMART_ATTRI_VENDOR);
						IoT_SetStringValue(attr, vendorDataStr, IoT_READONLY);
					}
					curSmartAttri = curSmartAttri->next;
				}
			}
			item = item->next;
		} //while End
	}
}

/*
void UpdateStorageInfo(DiskItemList* storages, hdd_info_t* hddInfo, MSG_CLASSIFY_T* storageGroup)
{
	if (storages && storageGroup)
	{
		DiskItemList* item = storages;
		while (item)
		{
			if (item->disk)
			{
				char cTemp[128] = { 0 };
				MSG_CLASSIFY_T* indexGroup = NULL;
				MSG_ATTRIBUTE_T* attr = NULL;
				DiskInfoItem* disk = item->disk;
				double total = B2MB(disk->totalspace);
				double free = B2MB(disk->freespace);
				bool bHasDisk = false;
				hdd_mon_info_node_t* diskitem = hddInfo->hddMonInfoList->next;
				while (diskitem != NULL)
				{
					if (diskitem->hddMonInfo.hdd_index == disk->diskid)
					{
						if (diskitem->hddMonInfo.hdd_type == SQFlash || diskitem->hddMonInfo.hdd_type == SQFNVMe)
						{
							bHasDisk = true;
							break;
						}
					}
					diskitem = diskitem->next;
				}

				if (!bHasDisk)
				{
					item = item->next;
					continue;
				}

				memset(cTemp, 0, sizeof(cTemp));
				sprintf(cTemp, "Drive %s", disk->name);

				indexGroup = IoT_FindGroup(storageGroup, cTemp);
				if (indexGroup == NULL)
				{
					indexGroup = IoT_AddGroup(storageGroup, cTemp);
				}
				attr = IoT_FindSensorNode(indexGroup, STORAGE_TOTAL_SPACE);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, STORAGE_TOTAL_SPACE);
				IoT_SetDoubleValueWithMaxMin(attr, total, IoT_READONLY, total, total, "Megabyte");

				attr = IoT_FindSensorNode(indexGroup, STORAGE_FREE_SPACE);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, STORAGE_FREE_SPACE);
				IoT_SetDoubleValueWithMaxMin(attr, free, IoT_READONLY, total, 0, "Megabyte");

				attr = IoT_FindSensorNode(indexGroup, HDD_INDEX);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_INDEX);
				IoT_SetDoubleValue(attr, disk->diskid, IoT_READONLY, NULL);

				attr = IoT_FindSensorNode(indexGroup, STORAGE_READ_SPEED);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, STORAGE_READ_SPEED);
				IoT_SetDoubleValue(attr, disk->readpersec, IoT_READONLY, "Byte");

				attr = IoT_FindSensorNode(indexGroup, STORAGE_WRITE_SPEED);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, STORAGE_WRITE_SPEED);
				IoT_SetDoubleValue(attr, disk->writepersec, IoT_READONLY, "Byte");
			}
			item = item->next;
		}
	}
}
*/

bool HasSQFlash(hdd_info_t* hddInfo)
{
	bool bHasDisk = false;
	hdd_mon_info_node_t* diskitem = hddInfo->hddMonInfoList->next;
	while (diskitem != NULL)
	{
		if (diskitem->hddMonInfo.hdd_type == SQFlash || diskitem->hddMonInfo.hdd_type == SQFNVMe)
		{
			bHasDisk = true;
			break;
		}
		diskitem = diskitem->next;
	}
	return bHasDisk;
}

void UpdateAction(SQ_context_t* pHandlerCtx, MSG_CLASSIFY_T* actionGroup)
{
	MSG_ATTRIBUTE_T* attr = NULL;
	attr = IoT_FindSensorNode(actionGroup, "SelfTest");
	if (attr == NULL)
	{
		attr = IoT_AddSensorNode(actionGroup, "SelfTest");
		IoT_SetDoubleValue(attr, 0, IoT_READWRITE, NULL);
	}

	if (pHandlerCtx->bHasMcAfee)
	{
		bool bScan = pHandlerCtx->mcafeectx.bScan;
		bool bUpdate = pHandlerCtx->mcafeectx.bUpdate;
		attr = IoT_FindSensorNode(actionGroup, "VirusScan");
		if (attr == NULL)
		{
			attr = IoT_AddSensorNode(actionGroup, "VirusScan");	
		}
		IoT_SetBoolValue(attr, bScan, IoT_READWRITE);

		attr = IoT_FindSensorNode(actionGroup, "VirusUpdate");
		if (attr == NULL)
		{
			attr = IoT_AddSensorNode(actionGroup, "VirusUpdate");
		}
		IoT_SetBoolValue(attr, bUpdate, IoT_READWRITE);
	}
}

void UpdateBasicInfo(SQ_context_t* pHandlerCtx, MSG_CLASSIFY_T* myCapability)
{
	int version = 0;
	MSG_CLASSIFY_T* infoGroup = NULL;
	MSG_ATTRIBUTE_T* attr = NULL;

	if (myCapability == NULL)
		return;
	infoGroup = IoT_FindGroup(myCapability, SQ_INFO_LIST);
	if (infoGroup == NULL)
		infoGroup = IoT_AddGroup(myCapability, SQ_INFO_LIST);

	if (pHandlerCtx->bHasMcAfee)
	{
		attr = IoT_FindSensorNode(infoGroup, MCAFEE_VERSION);
		if (attr == NULL)
		{
			attr = IoT_AddSensorNode(infoGroup, MCAFEE_VERSION);
			//IoT_SetDoubleValue(attr, pHandlerCtx->mcafeectx.iMcAfeeVer, IoT_READONLY, NULL);
		}
		IoT_SetDoubleValue(attr, pHandlerCtx->mcafeectx.iMcAfeeVer, IoT_READONLY, NULL);

		attr = IoT_FindSensorNode(infoGroup, MCAFEE_UPDATE);
		if (attr == NULL)
		{
			attr = IoT_AddSensorNode(infoGroup, MCAFEE_UPDATE);
			//IoT_SetBoolValue(attr, pHandlerCtx->mcafeectx.bNewMcAfee, IoT_READONLY);
		}
		IoT_SetBoolValue(attr, pHandlerCtx->mcafeectx.bNewMcAfee, IoT_READONLY);
	}
	
}

void UpdateCapability(SQ_context_t* pHandlerCtx, MSG_CLASSIFY_T* myCapability)
{
	MSG_CLASSIFY_T* dataGroup = NULL;
	
	hdd_info_t* hddInfo = NULL;
	DiskItemList* storages = NULL;
	sqram_info_t* ramInfo = NULL;

	if (pHandlerCtx == NULL || myCapability == NULL)
		return;

	UpdateBasicInfo(pHandlerCtx, myCapability);

	dataGroup = IoT_FindGroup(myCapability, SQ_DATA_LIST);
	if (dataGroup == NULL)
		dataGroup = IoT_AddGroup(myCapability, SQ_DATA_LIST);

	hddInfo = &pHandlerCtx->hddCtx.hddInfo;
	if (hddInfo && HasSQFlash(hddInfo))
	{
		MSG_CLASSIFY_T* hddInfoGroup = NULL;
		MSG_CLASSIFY_T* smartInfoGroup = NULL;
		MSG_CLASSIFY_T* actionGroup = NULL;

		hddInfoGroup = IoT_FindGroup(dataGroup, HDD_INFO_LIST);
		if (hddInfoGroup == NULL)
			hddInfoGroup = IoT_AddGroupArray(dataGroup, HDD_INFO_LIST);

		smartInfoGroup = IoT_FindGroup(dataGroup, HDD_SMART_INFO_LIST);
		if (smartInfoGroup == NULL)
			smartInfoGroup = IoT_AddGroupArray(dataGroup, HDD_SMART_INFO_LIST);

		storages = pHandlerCtx->storageList;
		UpdateHDDInfo(hddInfo, storages, hddInfoGroup);
		//to construct HDD S.M.A.R.T. info
		UpdateSMARTInfo(hddInfo, smartInfoGroup);

		/*
		storages = pHandlerCtx->storageList;
		if (storages)
		{
			MSG_CLASSIFY_T* storageGroup = IoT_FindGroup(dataGroup, HDD_STORAGE_LIST);
			if (storageGroup == NULL)
				storageGroup = IoT_AddGroupArray(dataGroup, HDD_STORAGE_LIST);
			UpdateStorageInfo(storages, hddInfo, storageGroup);
		}
		*/

		actionGroup = IoT_FindGroup(myCapability, SQ_ACTION_LIST);
		if (actionGroup == NULL)
			actionGroup = IoT_AddGroup(myCapability, SQ_ACTION_LIST);
		UpdateAction(pHandlerCtx, actionGroup);
	}

	ramInfo = &pHandlerCtx->sqramList;
	if (ramInfo && sqr_HasSQRam(ramInfo))
	{
		sqr_UpdateSQRAM(ramInfo, dataGroup);
	}	
}

/*Create Capability Message Structure to describe sensor data*/
MSG_CLASSIFY_T* CreateCapability(SQ_context_t* pHandlerCtx)
{
	MSG_CLASSIFY_T* myCapability = IoT_CreateRoot(MyTopic);
	UpdateCapability(pHandlerCtx, myCapability);
	return myCapability;
}

void* SQPluginThreadStart(void* args)
{
	SQ_context_t* pHandlerCtx = (SQ_context_t*)args;
	hdd_info_t* pHDDInfo = NULL;
	DiskItemList* pStorageList = NULL;
	int hddCnt = 0;

	if (pHandlerCtx == NULL)
	{
		pthread_exit(0);
		return 0;
	}

	if (!g_Capability)
	{
		pthread_mutex_lock(&pHandlerCtx->hddCtx.hddMutex);
		hdd_GetHDDInfo(&pHandlerCtx->hddCtx.hddInfo);
		GetStorageInfo(&pHandlerCtx->storageList);
		sqr_GetSQRAMInfo(&pHandlerCtx->sqramList);
		if(pHandlerCtx->bHasMcAfee)
			pHandlerCtx->mcafeectx.bNewMcAfee = mc_CheckUpdate();
		pthread_mutex_unlock(&pHandlerCtx->hddCtx.hddMutex);
		HandlerKernel_LockCapability();
		g_Capability = CreateCapability(pHandlerCtx);
		HandlerKernel_UnlockCapability();
		HandlerKernel_SetCapability(g_Capability, false);
	}

	pHDDInfo = &(pHandlerCtx->hddCtx.hddInfo);
	pStorageList = pHandlerCtx->storageList;
	hddCnt = pHDDInfo->hddCount;

	//===Start to AutoRun HDD Info query method
	while (pHandlerCtx->bThreadRunning)
	{
		int i = 0;
		for (i = 0; pHandlerCtx->bThreadRunning && i < g_iRetrieveInterval; i++)
		{
			usleep(1000000);
		}

		if (g_HandlerContex.bHasSQFlash)
		{
			bool bStroageChanged = false;
			/*===Retrieve HDD info && create JSON object of infoSpec ===*/
			pthread_mutex_lock(&pHandlerCtx->hddCtx.hddMutex);
			//if (!g_HandlerContex.hddtestctx.bThreadSelfTest) // HDD is busy for Self test
			hdd_GetHDDInfo(pHDDInfo);
			bStroageChanged = GetStorageInfo(&pStorageList);
			if (pHandlerCtx->bHasMcAfee && !pHandlerCtx->mcafeectx.bScan && !pHandlerCtx->mcafeectx.bUpdate)
			{
				int version = 0;
				if (pHandlerCtx->mcafeectx.iVersionCheckInterval == 0)
				{
					if (mc_GetCurrentVirusVersion(&version))
						pHandlerCtx->mcafeectx.iMcAfeeVer = version;
					pHandlerCtx->mcafeectx.bNewMcAfee = mc_CheckUpdate();
					pHandlerCtx->mcafeectx.iVersionCheckInterval = 10;
				}
				else
				{
					pHandlerCtx->mcafeectx.iVersionCheckInterval--;
				}
			}
				
			sqr_GetSQRAMInfo(&pHandlerCtx->sqramList);

			pthread_mutex_unlock(&pHandlerCtx->hddCtx.hddMutex);

			HandlerKernel_LockCapability();
			UpdateCapability(pHandlerCtx, g_Capability);
			HandlerKernel_UnlockCapability();
			/*=== Check HDDCount changed and reset the new infoSpec */
			if ((hddCnt != pHDDInfo->hddCount || bStroageChanged))
			{
				HandlerKernel_SetCapability(g_Capability, true);
				hddCnt = pHDDInfo->hddCount;
			}
		}
	}
	pHandlerCtx->bFinish = true;
	pthread_exit(0);
	return 0;
}

void* HDDSelfTestThreadStart(void* args)
{
	SQ_context_t* pHandlerCtx = (SQ_context_t*)args;
	/*TODO: Check and Start SelfTest Thread with SQFlash*/
	if (hdd_SelfTest(pHandlerCtx->hddtestctx.iCurHDDIndex))
	{
		char message[256] = { 0 };
		HANDLER_NOTIFY_SEVERITY severity = Severity_Informational;
		unsigned int progress = 0, status = 0;
		while (true)
		{
			progress = 0, status = 0;
			if (!hdd_GetSelfTestStatus(pHandlerCtx->hddtestctx.iCurHDDIndex, &progress, &status))
			{
				severity = Severity_Warning;
				sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d %s\"}", pHandlerCtx->hddtestctx.iCurHDDIndex, "Read Self Test Status Fail!!");
				if (g_PluginInfo.sendeventcbf)
					g_PluginInfo.sendeventcbf(&g_PluginInfo, severity, message, strlen(message), NULL, NULL);
			}

			SQLog(Debug, "HDD %d Self-test status: %02X, Progress: %d %%\n", pHandlerCtx->hddtestctx.iCurHDDIndex, status, 100 - (progress * 10));

			if (status == SELF_TEST_STATUS_IN_PROGRESS)
				Sleep(3000);
			else
				break;
		}

		switch (status)
		{
		case SELF_TEST_STATUS_SUCCESS:
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d %s\"}", pHandlerCtx->hddtestctx.iCurHDDIndex, "Self Test Success!!");
			break;
		case SELF_TEST_STATUS_IN_PROGRESS:
			severity = Severity_Warning;
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d %s\"}", pHandlerCtx->hddtestctx.iCurHDDIndex, "Self Test is in progress, wait for a while and read again");
			break;

		case 1:
			severity = Severity_Warning;
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Self Test error, The self-test routine was aborted by the host\"}", pHandlerCtx->hddtestctx.iCurHDDIndex);
			break;
		case 2:
			severity = Severity_Warning;
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Self Test error, The self-test routine was interrupted by the host with a hardware or software reset\"}", pHandlerCtx->hddtestctx.iCurHDDIndex);
			break;
		case 3:
			severity = Severity_Warning;
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Self Test error, A fatal error or unknown test error occurred while the device was executing its self - test routine and the device was unable to complete the self - test routine.\"}", pHandlerCtx->hddtestctx.iCurHDDIndex);
			break;
		case 4:
			severity = Severity_Warning;
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Self Test error, The previous self-test completed having a test element that failed and the test element that failed is not known.\"}", pHandlerCtx->hddtestctx.iCurHDDIndex);
			break;
		case 5:
			severity = Severity_Warning;
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Self Test error, The previous self - test completed having the electrical element of the test failed.\"}", pHandlerCtx->hddtestctx.iCurHDDIndex);
			break;
		case 6:
			severity = Severity_Warning;
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Self Test error, The previous self - test completed having the servo(and /or seek) test element of the test failed.\"}", pHandlerCtx->hddtestctx.iCurHDDIndex);
			break;
		case 7:
			severity = Severity_Warning;
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Self Test error, The previous self - test completed having the read element of the test failed.\"}", pHandlerCtx->hddtestctx.iCurHDDIndex);
			break;
		case 8:
			severity = Severity_Warning;
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Self Test error, The previous self - test completed having a test element that failed and the device is suspected of having handling damage.\"}", pHandlerCtx->hddtestctx.iCurHDDIndex);
			break;
		default:
			sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Self Test error, error code: %u\"}", pHandlerCtx->hddtestctx.iCurHDDIndex, status);
			break;
		}

		if (g_PluginInfo.sendeventcbf)
			g_PluginInfo.sendeventcbf(&g_PluginInfo, severity, message, strlen(message), NULL, NULL);
	}
	else
	{
		char message[256] = { 0 };
		sprintf(message, "{\"subtype\":\"HDD_SELF_TEST\",\"msg\":\"Disk %d Offline Self Test Fail!\"}", pHandlerCtx->hddtestctx.iCurHDDIndex);
		if (g_PluginInfo.sendeventcbf)
			g_PluginInfo.sendeventcbf(&g_PluginInfo, Severity_Error, message, strlen(message), NULL, NULL);
	}
	pHandlerCtx->hddtestctx.bThreadSelfTest = false;

	pthread_exit(0);
	return 0;
}

void* McAfeeScanThreadStart(void* args)
{
	SQ_context_t* pHandlerCtx = (SQ_context_t*)args;
	int amount = 0;
	char driverletters[32] = { 0 };
	DiskItemList* current = pHandlerCtx->storageList;

	while (current)
	{
		char letter = current->disk->name[0];
		driverletters[amount] = letter;
		amount++;
		current = current->next;
	}

	if (mc_StartScan(driverletters, amount, &pHandlerCtx->mcafeectx.iTotalScan, &pHandlerCtx->mcafeectx.iInfected))
	{
		char message[256] = { 0 };
		HANDLER_NOTIFY_SEVERITY severity = Severity_Informational;
		if (pHandlerCtx->mcafeectx.iInfected > 0)
			severity = Severity_Warning;

		sprintf(message, "{\"subtype\":\"MCAFEE_VIRUS_SCAN\",\"msg\":\"Scan Result: Total scanned %d, Infected %d\"}", pHandlerCtx->mcafeectx.iTotalScan, pHandlerCtx->mcafeectx.iInfected);

		if (g_PluginInfo.sendeventcbf)
			g_PluginInfo.sendeventcbf(&g_PluginInfo, severity, message, strlen(message), NULL, NULL);
	}
	else
	{
		char message[256] = { 0 };
		sprintf(message, "{\"subtype\":\"MCAFEE_VIRUS_SCAN\",\"msg\":\"Cannot start virus scan!\"}");
		if (g_PluginInfo.sendeventcbf)
			g_PluginInfo.sendeventcbf(&g_PluginInfo, Severity_Error, message, strlen(message), NULL, NULL);
	}
	pHandlerCtx->mcafeectx.bScan = false;
	pthread_exit(0);
	return 0;
}

void* McAfeeUpdateThreadStart(void* args)
{
	SQ_context_t* pHandlerCtx = (SQ_context_t*)args;
	int newVersion = 0;
	char driverletters[32] = { 0 };
	
	if(mc_Update(&newVersion))
	{
		char message[256] = { 0 };
		HANDLER_NOTIFY_SEVERITY severity = Severity_Informational;

		sprintf(message, "{\"subtype\":\"MCAFEE_VIRUS_UPDATE\",\"msg\":\"Virus update to Version %d\"}", newVersion);
		pHandlerCtx->mcafeectx.iMcAfeeVer = newVersion;
		pHandlerCtx->mcafeectx.bNewMcAfee = false;
		if (g_PluginInfo.sendeventcbf)
			g_PluginInfo.sendeventcbf(&g_PluginInfo, severity, message, strlen(message), NULL, NULL);
	}
	else
	{
		char message[256] = { 0 };
		sprintf(message, "{\"subtype\":\"MCAFEE_VIRUS_UPDATE\",\"msg\":\"Virus update fail!\"}");
		if (g_PluginInfo.sendeventcbf)
			g_PluginInfo.sendeventcbf(&g_PluginInfo, Severity_Error, message, strlen(message), NULL, NULL);
	}
	pHandlerCtx->mcafeectx.bUpdate = false;
	pthread_exit(0);
	return 0;
}

bool SetSensor(set_data_t* objlist, void* pRev)
{
	set_data_t* current = objlist;
	bool bChanged = false;
	if (current == NULL)
	{
		return false;
	}

	while (current)
	{
		MSG_ATTRIBUTE_T* attr = IoT_FindSensorNodeWithPath(g_Capability, current->sensorname);
		if (attr)
		{
			if (strcmp(current->sensorname, "SQPlugin/action/SelfTest") == 0)
			{
				if (current->newtype == attr_type_numeric)
				{
					if (!g_HandlerContex.hddtestctx.bThreadSelfTest)
					{
						
						g_HandlerContex.hddtestctx.iCurHDDIndex = current->v;
						g_HandlerContex.hddtestctx.bThreadSelfTest = true;
						
						if (pthread_create(&g_HandlerContex.hddtestctx.threadSelfTestHandler, NULL, HDDSelfTestThreadStart, &g_HandlerContex) != 0)
						{
							g_HandlerContex.hddtestctx.bThreadSelfTest = false;
							SQLog(Error, " %s> start handler thread failed! ", MyTopic);
							current->errcode = STATUSCODE_FAIL;
							strcpy(current->errstring, STATUS_FAIL);
						}
						else
						{
							pthread_detach(g_HandlerContex.hddtestctx.threadSelfTestHandler);
							g_HandlerContex.hddtestctx.threadSelfTestHandler = NULL;
							current->errcode = STATUSCODE_SUCCESS;
							strcpy(current->errstring, STATUS_SUCCESS);
						}
					}
					else
					{
						current->errcode = STATUSCODE_SYS_BUSY;
						strcpy(current->errstring, STATUS_SYS_BUSY);
					}
				}
				else
				{
					current->errcode = STATUSCODE_NOT_IMPLEMENT;
					strcpy(current->errstring, STATUS_NOT_IMPLEMENT);
				}
				
			}
			else if (strcmp(current->sensorname, "SQPlugin/action/VirusScan") == 0)
			{
				if (current->newtype == attr_type_boolean)
				{
					if (g_HandlerContex.bHasMcAfee)
					{
						/*TODO: Check and Start VirusScan Thread*/
						if (!g_HandlerContex.mcafeectx.bScan && !g_HandlerContex.mcafeectx.bUpdate)
						{
							g_HandlerContex.mcafeectx.bScan = true;
							if (pthread_create(&g_HandlerContex.mcafeectx.threadMCSHandler, NULL, McAfeeScanThreadStart, &g_HandlerContex) != 0)
							{
								g_HandlerContex.mcafeectx.bScan = false;
								SQLog(Error, " %s> start handler thread failed! ", MyTopic);
								current->errcode = STATUSCODE_FAIL;
								strcpy(current->errstring, STATUS_FAIL);
							}
							else
							{
								pthread_detach(g_HandlerContex.mcafeectx.threadMCSHandler);
								g_HandlerContex.mcafeectx.threadMCSHandler = NULL;
								current->errcode = STATUSCODE_SUCCESS;
								strcpy(current->errstring, STATUS_SUCCESS);
							}
						}
						else
						{
							current->errcode = STATUSCODE_SYS_BUSY;
							strcpy(current->errstring, STATUS_SYS_BUSY);
						}
					}
					else
					{
						current->errcode = STATUSCODE_RESOURCE_LOSE;
						strcpy(current->errstring, STATUS_RESOURCE_LOSE);
					}
				}
				else
				{
					current->errcode = STATUSCODE_FORMAT_ERROR;
					strcpy(current->errstring, STATUS_FORMAT_ERROR);
				}
			}
			else if (strcmp(current->sensorname, "SQPlugin/action/VirusUpdate") == 0)
			{
				if (current->newtype == attr_type_boolean)
				{
					current->errcode = STATUSCODE_SUCCESS;
					strcpy(current->errstring, STATUS_SUCCESS);
					/*TODO: Check and Start VirusScan Thread*/
					if (!g_HandlerContex.mcafeectx.bUpdate && !g_HandlerContex.mcafeectx.bScan)
					{
						g_HandlerContex.mcafeectx.bUpdate = true;
						if (pthread_create(&g_HandlerContex.mcafeectx.threadMCSHandler, NULL, McAfeeUpdateThreadStart, &g_HandlerContex) != 0)
						{
							g_HandlerContex.mcafeectx.bUpdate = false;
							SQLog(Error, " %s> start handler thread failed! ", MyTopic);
							current->errcode = STATUSCODE_FAIL;
							strcpy(current->errstring, STATUS_FAIL);
						}
						else
						{
							pthread_detach(g_HandlerContex.mcafeectx.threadMCSHandler);
							g_HandlerContex.mcafeectx.threadMCSHandler = NULL;
							current->errcode = STATUSCODE_SUCCESS;
							strcpy(current->errstring, STATUS_SUCCESS);
						}
					}
					else
					{
						current->errcode = STATUSCODE_SYS_BUSY;
						strcpy(current->errstring, STATUS_SYS_BUSY);
					}
				}
				else
				{
					current->errcode = STATUSCODE_FORMAT_ERROR;
					strcpy(current->errstring, STATUS_FORMAT_ERROR);
				}
			}
			else
			{
				current->errcode = STATUSCODE_NOT_IMPLEMENT;
				strcpy(current->errstring, STATUS_NOT_IMPLEMENT);
			}
		}
		else
		{
			current->errcode = STATUSCODE_NOT_FOUND;
			strcpy(current->errstring, STATUS_NOT_FOUND);
		}
		current = current->next;
	}
	return true;
}

void loadINIFile()
{
	char inifile[256] = { 0 };
	char filename[64] = { 0 };
	char path[MAX_PATH] = { 0 };
	sprintf(filename, "%s.ini", g_PluginInfo.Name);
	util_path_combine(inifile, g_PluginInfo.WorkDir, filename);
	util_path_combine(path, g_PluginInfo.WorkDir, MCAFEE_DIRECTORY_DEFAULT);
	if (!util_is_file_exist(inifile))
	{
		FILE* iniFD = fopen(inifile, "w");
		char tmp[512] = { 0 };
		sprintf(tmp, HDD_INI_COTENT, path);
		fwrite(tmp, strlen(tmp), 1, iniFD);
		fclose(iniFD);
	}
	g_iRetrieveInterval = GetIniKeyInt("Platform", "Interval", inifile);
	if (g_iRetrieveInterval < 1)
		g_iRetrieveInterval = 1;

	GetIniKeyStringDef("Platform", "McAfeeDir", inifile, g_strMcAfeeDir, sizeof(g_strMcAfeeDir), path);
}
//--------------------------------------------------------------------------------------------------------------
//--------------------------------------Handler Functions-------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	if (pluginfo == NULL)
		return handler_fail;
	// 1. Topic of this handler
	snprintf(pluginfo->Name, sizeof(pluginfo->Name), "%s", MyTopic);
	g_SQLogHandle = pluginfo->loghandle;
	SQLog(Debug, " %s> Initialize", MyTopic);
	// 2. Copy agent info
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	// 3. Callback function -> Send JSON Data by this callback function
	memset(&g_HandlerContex, 0, sizeof(SQ_context_t));
	g_HandlerContex.threadHandler = NULL;
	g_HandlerContex.bThreadRunning = false;
	g_HandlerContex.hddtestctx.threadSelfTestHandler = NULL;
	g_HandlerContex.hddtestctx.bThreadSelfTest = false;
	g_HandlerContex.mcafeectx.threadMCSHandler = NULL;
	g_HandlerContex.mcafeectx.bScan = false;
	g_HandlerContex.mcafeectx.bUpdate = false;
	g_Status = handler_status_no_init;

	/*Create mutex*/
	if (pthread_mutex_init(&g_HandlerContex.hddCtx.hddMutex, NULL) != 0)
	{
		SQLog(Error, " %s> Create HDDInfo mutex failed! ", MyTopic);
		return handler_fail;
	}
	loadINIFile();
	return HandlerKernel_Initialize(pluginfo);
}

/* **************************************************************************************
 *  Function Name: Handler_Uninitialize
 *  Description: Release the objects or variables used in this handler
 *  Input :  None
 *  Output: None
 *  Return:  void
 * ***************************************************************************************/
void Handler_Uninitialize()
{
	if (g_HandlerContex.bThreadRunning == true)
	{
		g_HandlerContex.bThreadRunning = false;
		pthread_join(g_HandlerContex.threadHandler, NULL);
		g_HandlerContex.threadHandler = NULL;
	}

	/*Release Capability Message Structure*/
	if (g_Capability)
	{
		IoT_ReleaseAll(g_Capability);
		g_Capability = NULL;
	}

	//pthread_mutex_lock(&g_HandlerContex.hddCtx.hddMutex);
	if (g_HandlerContex.bHasSQFlash)
	{
		hdd_CleanupSQFlashLib();
		g_HandlerContex.bHasSQFlash = false;
	}

	if (g_HandlerContex.storageList)
	{
		util_storage_freedisklist(g_HandlerContex.storageList);
		g_HandlerContex.storageList = NULL;
	}

	if (g_HandlerContex.hddCtx.hddInfo.hddMonInfoList)
	{
		hdd_DestroyHDDInfoList(g_HandlerContex.hddCtx.hddInfo.hddMonInfoList);
		g_HandlerContex.hddCtx.hddInfo.hddMonInfoList = NULL;
	}

	if (g_HandlerContex.sqramList.sqramInfoList) {
		sqr_DestroySQRAMInfo(g_HandlerContex.sqramList.sqramInfoList);
		g_HandlerContex.sqramList.count = 0;
		g_HandlerContex.sqramList.sqramInfoList = NULL;
	}

	if (g_HandlerContex.bHasMcAfee)
	{
		mc_Uninitialize();
		g_HandlerContex.bHasMcAfee = false;
	}

	HandlerKernel_Uninitialize();
	pthread_mutex_destroy(&g_HandlerContex.hddCtx.hddMutex);
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
 *  Input :  None
 *  Output: char * : pOutStatus       // cagent handler status
 *  Return:  handler_success  : Success Init Handler
 *			 handler_fail : Fail Init Handler
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	int iRet = handler_fail;
	if (!pOutStatus)
		return iRet;
	switch (g_Status)
	{
	default:
	case handler_status_no_init:
	case handler_status_init:
	case handler_status_stop:
		*pOutStatus = g_Status;
		break;
	case handler_status_start:
	case handler_status_busy:
		*pOutStatus = g_Status;
		break;
	}
	iRet = handler_success;
	return iRet;
}


/* **************************************************************************************
 *  Function Name: Handler_OnStatusChange
 *  Description: Agent can notify handler the status is changed.
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  None
 * ***************************************************************************************/
void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	//printf(" %s> Update Status", MyTopic);
	if (pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf(g_PluginInfo.Name, sizeof(g_PluginInfo.Name), "%s", MyTopic);
		g_PluginInfo.RequestID = 0;
		g_PluginInfo.ActionID = 0;
	}
}


/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	//== Init SQFlash
	int McAfeeVer = 0;
	pthread_mutex_lock(&g_HandlerContex.hddCtx.hddMutex);
	if (hdd_IsExistSQFlashLib())
	{
		g_HandlerContex.bHasSQFlash = hdd_StartupSQFlashLib();
		if (g_HandlerContex.bHasSQFlash)
		{
			hdd_GetHDDInfo(&g_HandlerContex.hddCtx.hddInfo);
		}
	}
	if (sqr_IsExistSUSIDeviceLib())
	{
		sqr_StartupSUSIDeviceLib();
	}
	pthread_mutex_unlock(&g_HandlerContex.hddCtx.hddMutex);

	if (mc_Initialize(g_strMcAfeeDir, &McAfeeVer))
	{
		g_HandlerContex.bHasMcAfee = true;
		g_HandlerContex.mcafeectx.iMcAfeeVer = McAfeeVer;
	}

	//== Create Thread
	g_HandlerContex.bThreadRunning = true;
	if (pthread_create(&g_HandlerContex.threadHandler, NULL, SQPluginThreadStart, &g_HandlerContex) != 0)
	{
		g_HandlerContex.bThreadRunning = false;
		SQLog(Error, " %s> start handler thread failed! ", MyTopic);
		return handler_fail;
	}

	HandlerKernel_Start();

	g_Status = handler_status_start;

	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Stop
 *  Description: Stop the handler
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	if (g_HandlerContex.bThreadRunning == true)
	{
		g_HandlerContex.bThreadRunning = false;
		//pthread_cancel(g_HandlerContex.threadHandler);
		pthread_join(g_HandlerContex.threadHandler, NULL);
		g_HandlerContex.threadHandler = NULL;
	}

	//pthread_mutex_lock( & g_HandlerContex.hddCtx.hddMutex );
	if (g_HandlerContex.bHasSQFlash)
	{
		hdd_CleanupSQFlashLib();
		g_HandlerContex.bHasSQFlash = false;
	}

	if (sqr_IsExistSUSIDeviceLib())
	{
		sqr_CleanupSUSIDeviceLib();
	}

	if (g_HandlerContex.storageList)
	{
		util_storage_freedisklist(g_HandlerContex.storageList);
		g_HandlerContex.storageList = NULL;
	}

	if (g_HandlerContex.hddCtx.hddInfo.hddMonInfoList)
	{
		hdd_DestroyHDDInfoList(g_HandlerContex.hddCtx.hddInfo.hddMonInfoList);
		g_HandlerContex.hddCtx.hddInfo.hddMonInfoList = NULL;
	}

	if (g_HandlerContex.bHasMcAfee)
	{
		mc_Uninitialize();
		g_HandlerContex.bHasMcAfee = false;
	}

	//pthread_mutex_unlock( & g_HandlerContex.hddCtx.hddMutex );
	HandlerKernel_Stop();
	g_Status = handler_status_stop;
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input : char * const topic, 
 *			void* const data, 
 *			const size_t datalen
 *  Output: void *pRev1, 
 *			void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int cmdID = 0;
	char sessionID[MAX_SESSION_LEN] = { 0 };
	//HDDLog(g_LogHandle, Debug, " >Recv Topic [%s] Data %s\n", topic, (char*) data );

	/*Parse Received Command*/
	if (HandlerKernel_ParseRecvCMDWithSessionID((char*)data, &cmdID, sessionID) != handler_success)
		return;
	switch (cmdID)
	{
	case hk_get_capability_req:
		if (!g_Capability)
		{
			HandlerKernel_LockCapability();
			g_Capability = CreateCapability(&g_HandlerContex);
			HandlerKernel_UnlockCapability();
		}
		HandlerKernel_SetCapability(g_Capability, true);
		break;
	case hk_auto_upload_req:
		/*start live report*/
		HandlerKernel_LiveReportStart(hk_auto_upload_rep, (char*)data);
		break;
	case hk_set_thr_req:
		/*Stop threshold check thread*/
		HandlerKernel_StopThresholdCheck();
		/*setup threshold rule*/
		HandlerKernel_SetThreshold(hk_set_thr_rep, (char*)data);
		/*register the threshold check callback function to handle trigger event*/
		//HandlerKernel_SetThresholdTrigger(on_threshold_triggered);
		/*Restart threshold check thread*/
		HandlerKernel_StartThresholdCheck();
		break;
	case hk_del_thr_req:
		/*Stop threshold check thread*/
		HandlerKernel_StopThresholdCheck();
		/*clear threshold check callback function*/
		HandlerKernel_SetThresholdTrigger(NULL);
		/*Delete all threshold rules*/
		HandlerKernel_DeleteAllThreshold(hk_del_thr_rep);
		break;
	case hk_get_sensors_data_req:
		/*Get Sensor Data with callback function*/
		HandlerKernel_GetSensorData(hk_get_sensors_data_rep, sessionID, (char*)data, NULL);
		break;
	case hk_set_sensors_data_req:
		/*Set Sensor Data with callback function*/
		HandlerKernel_SetSensorData(hk_set_sensors_data_rep, sessionID, (char*)data, SetSensor);
		break;
	default:
	{
		/* Send command not support reply message*/
		char repMsg[32] = { 0 };
		int len = 0;
		sprintf(repMsg, "{\"errorRep\":\"Unknown cmd!\"}");
		len = strlen("{\"errorRep\":\"Unknown cmd!\"}");
		if (g_PluginInfo.sendcbf)
			g_PluginInfo.sendcbf(&g_PluginInfo, hk_error_rep, repMsg, len, NULL, NULL);
	}
	break;
	}
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	HandlerKernel_AutoReportStart(pInQuery);
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : None
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	HandlerKernel_AutoReportStop(pInQuery);
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char ** : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{
	char* result = NULL;
	int len = 0;

	//HDDLog(g_LogHandle, Debug, "> %s Get Capability", MyTopic);

	if (!pOutReply)
		return len;

	/*Create Capability Message Structure to describe sensor data*/
	if (!g_Capability)
	{
		pthread_mutex_lock(&g_HandlerContex.hddCtx.hddMutex);
		hdd_GetHDDInfo(&g_HandlerContex.hddCtx.hddInfo);
		GetStorageInfo(&g_HandlerContex.storageList);
		sqr_GetSQRAMInfo(&g_HandlerContex.sqramList);
		pthread_mutex_unlock(&g_HandlerContex.hddCtx.hddMutex);

		HandlerKernel_LockCapability();
		g_Capability = CreateCapability(&g_HandlerContex);
		HandlerKernel_UnlockCapability();
		HandlerKernel_SetCapability(g_Capability, false);
	}
	/*generate capability JSON string*/
	result = IoT_PrintCapability(g_Capability);

	/*create buffer to store the string*/
	len = strlen(result);
	*pOutReply = (char*)malloc(len + 1);
	memset(*pOutReply, 0, len + 1);
	strcpy(*pOutReply, result);
	free(result);
	return len;
}

/* **************************************************************************************
 *  Function Name: Handler_MemoryFree
 *  Description: free the mamory allocated for Handler_Get_Capability
 *  Input : char *pInData.
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_MemoryFree(char* pInData)
{
	if (pInData)
	{
		free(pInData);
		pInData = NULL;
	}
	return;
}