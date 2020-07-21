/****************************************************************************/
/* Copyright( C ) : Advantech Technologies, Inc.								*/
/* Create Date  : 2014/07/07 by Scott68 Chang								*/
/* Modified Date: 2015/02/12 by tang.tao							*/
/* Abstract     : Handler API                                     			*/
/* Reference    : None														*/
/*****************************************************************************/

#include "WISEPlatform.h"
#include "susiaccess_handler_api.h"
#include "HDDHandler.h"
#include "HDDLog.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "HandlerKernel.h"
#include "IoTMessageGenerate.h"
#include "util_path.h"
#include "ReadINI.h"
//#include <vld.h>
#ifdef SQMANAGE
#include "SQRam.h"
#endif

#define HDD_INFO_LIST "hddInfoList"
#define HDD_SMART_INFO_LIST "hddSmartInfoList"
#define MMC_INFO_LIST				"mmcInfoList"
#define HDD_STORAGE_LIST "DiskInfo"

#define HDD_TYPE_HDD "STDDisk"
#define HDD_TYPE_SQFLASH "SQFlash"
#define HDD_TYPE_MMC				"eMMC"

#define HDD_TYPE "hddType"
#define HDD_NAME "hddName"
#define HDD_INDEX "hddIndex"
#define HDD_POWER_ON_TIME "powerOnTime"
#define HDD_TEMP "hddTemp"
#define HDD_MAX_TEMP "hddMaxTemp"
//#define HDD_MAX_PROGRAM "maxProgram"
//#define HDD_AVERAGE_PROGRAM "averageProgram"
#define HDD_ECC_COUNT "eccCount"
#define HDD_POWER_CYCLE "powerCycle"
#define HDD_POWERLOSS_COUNT "unexpectedPowerLossCount"
#define HDD_BAD_BLOCK "laterBadBlock"
#define HDD_CRC_ERROR "crcError"
#define HDD_OPAL_SUPPORT "opalSupport"
#define HDD_OPAL_ENABLE "opalEnable"
#define HDD_OPAL_LOCKED "opalLocked"
//#define HDD_ENDURANCE_CHECK "enduranceCheck"
//#define HDD_MAX_RESERVED_BLOCK "maxReservedBlock"
//#define HDD_CURRENT_RESERVED_BLOCK "currentReservedBlock"
//#define HDD_GOOD_BLOCK_RATE "goodBlockRate"
#define HDD_HEALTH "health"
#define HDD_REMAIN_DAYS "remainDays"
#define HDD_PRODUCT_SERIALNUMBER "productSerialNumber"
#define HDD_FIRMWARE_REVISION "firmwareRevision"

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

#define MMC_VERSION 				"version"
#define MMC_HEALTTH_STATUS			"health"
#define MMC_HEALTTH_PERCENT			"health percent"

#define STORAGE_TOTAL_SPACE "Total Disk Space"
#define STORAGE_FREE_SPACE "Free Disk Space"

#define STORAGE_READ_SPEED "ReadBytePerSec"
#define STORAGE_WRITE_SPEED "WriteBytePerSec"

#define HDD_INI_COTENT "[Platform]\nInterval=10\n#Interval: The time delay between two access round in second."

const int RequestID = cagent_request_hdd_monitoring;
const int ActionID = cagent_action_hdd_monitoring;

//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
typedef struct hdd_context_t {
	hdd_info_t		hddInfo;
	pthread_mutex_t	hddMutex;
}  hdd_context_t;

typedef struct HDD_context_t {
	void* threadHandler;
	bool					bThreadRunning;
	bool					bHasSQFlash;
	struct hdd_context_t	hddCtx;
	DiskItemList* storageList;

	mmc_info_t				MMCInfo;
#ifdef SQMANAGE
	sqram_info_t			sqramList;
#endif
	bool					bFinish;
} HDD_context_t;

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
void *g_LogHandle = NULL;
Handler_info g_PluginInfo;
HANDLER_THREAD_STATUS g_Status = handler_status_no_init;
HDD_context_t g_HandlerContex;
HandlerSendCbf g_sendcbf = NULL;			 // Client Send information (in JSON format) to Cloud Server
HandlerSendCustCbf g_sendcustcbf = NULL;	 // Client Send information (in JSON format) to Cloud Server with custom topic
HandlerAutoReportCbf g_sendreportcbf = NULL; // Client Send report (in JSON format) to Cloud Server with AutoReport topic
HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;
HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
MSG_CLASSIFY_T *g_Capability = NULL; /*the global message structure to describe the sensor data as the handelr capability*/
int g_iRetrieveInterval = 10;		 //10 sec.

//-----------------------------------------------------------------------------
// UTIL Function:
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		printf("DllInitializer\n");
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL)					  // Dynamic load
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
		printf("DllFinalizer\n");
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
static void
Initializer(int argc, char **argv, char **envp)
{
	fprintf(stderr, "DllInitializer\n");
}

__attribute__((destructor))
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void
Finalizer()
{
	fprintf(stderr, "DllFinalizer\n");
	Handler_Uninitialize();
}
#endif

bool UpdateStorageList(DiskItemList *current, DiskItemList *newone)
{
	bool bChanged = false;
	if (newone)
	{
		DiskItemList *item = newone;
		while (item)
		{
			DiskInfoItem *disk = item->disk;
			if (disk)
			{
				char *target = disk->name;
				bool bFound = false;
				DiskItemList *currentitem = current;
				while (currentitem)
				{
					DiskInfoItem *currentdisk = currentitem->disk;
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
					DiskItemList *newdisk = (DiskItemList*)calloc(1, sizeof(DiskItemList));
					DiskItemList *last = current;

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

bool GetStorageInfo(DiskItemList **storages)
{
	DiskItemList *item = NULL;
	bool bChanged = false;
	if (*storages == NULL)
	{
		*storages = util_storage_getdisklist();
		bChanged = true;
	}
	else
	{
		DiskItemList *tmpstorages = util_storage_getdisklist();
		bChanged = UpdateStorageList(*storages, tmpstorages);
		util_storage_freedisklist(tmpstorages);
		tmpstorages = NULL;
	}

	item = *storages;
	while (item)
	{
		if (item->disk)
		{
			DiskInfoItem *disk = item->disk;
			disk->totalspace = util_storage_gettotaldiskspace(disk->name);
			disk->freespace = util_storage_getfreediskspace(disk->name);
			/*if(disk->totalspace>0)
				disk->usage = (1-(double)disk->freespace/(double)disk->totalspace)*100;*/
			HDDLog(g_LogHandle, Debug, "Disk(%s) %I64u/%I64u MB\n", disk->name, disk->freespace, disk->totalspace);
		}
		item = item->next;
	}
#ifdef SQMANAGE
	util_storage_getdiskspeed(*storages);
#endif
	return bChanged;
}
void UpdateHDDInfo(hdd_info_t *hddInfo, MSG_CLASSIFY_T *hddInfoGroup)
{
	if (hddInfo && hddInfo->hddMonInfoList && hddInfoGroup)
	{
		hdd_mon_info_node_t *item = hddInfo->hddMonInfoList->next;
		//construct CJSON of HDD Info and S.M.A.R.T. Info, notice that SQFlash and Std HDD are different
		while (item != NULL)
		{
			char cTemp[128] = {0};
			MSG_CLASSIFY_T *indexGroup = NULL;
			MSG_ATTRIBUTE_T *attr = NULL;
			hdd_mon_info_t *pHDDMonInfo = &item->hddMonInfo;
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

			if (pHDDMonInfo->hdd_type == SQFlash || pHDDMonInfo->hdd_type == SQFNVMe)
			{
				attr = IoT_FindSensorNode(indexGroup, HDD_TYPE);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_TYPE);
				IoT_SetStringValue(attr, HDD_TYPE_SQFLASH, IoT_READONLY);

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
			}
			else if (pHDDMonInfo->hdd_type == StdDisk)
			{
				attr = IoT_FindSensorNode(indexGroup, HDD_TYPE);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_TYPE);
				IoT_SetStringValue(attr, HDD_TYPE_HDD, IoT_READONLY);

				attr = IoT_FindSensorNode(indexGroup, HDD_TEMP);
				if (attr == NULL)
					attr = IoT_AddSensorNode(indexGroup, HDD_TEMP);
				//IoT_SetDoubleValue(attr, pHDDMonInfo->hdd_temp, IoT_READONLY, SENSOR_UNIT_CELSIUS);
				IoT_SetDoubleValueWithMaxMin(attr, pHDDMonInfo->hdd_temp, IoT_READONLY, 60, 0, SENSOR_UNIT_CELSIUS);
			}

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
			item = item->next;
		} // while End
	}
}

void UpdateSMARTInfo(hdd_info_t *hddInfo, MSG_CLASSIFY_T *smartInfoGroup)
{
	if (hddInfo && hddInfo->hddMonInfoList && smartInfoGroup)
	{
		hdd_mon_info_node_t *item = hddInfo->hddMonInfoList->next;
		while (item != NULL)
		{
			hdd_mon_info_t *pHDDMonInfo = &item->hddMonInfo;
			/*if (pHDDMonInfo->hdd_type != StdDisk)
			{
				item = item->next;
				continue;
			}*/
			if (pHDDMonInfo->smartAttriInfoList)
			{
				char cTemp[128] = {0};
				MSG_CLASSIFY_T *indexGroup = NULL;
				MSG_CLASSIFY_T *basetGroup = NULL;
				MSG_ATTRIBUTE_T *attr = NULL;
				smart_attri_info_node_t *curSmartAttri = pHDDMonInfo->smartAttriInfoList->next;
				memset(cTemp, 0, sizeof(cTemp));
				sprintf(cTemp, "Disk%d", pHDDMonInfo->hdd_index);

				indexGroup = IoT_FindGroup(smartInfoGroup, cTemp);
				if (indexGroup == NULL)
					indexGroup = IoT_AddGroup(smartInfoGroup, cTemp);

				basetGroup = IoT_FindGroup(indexGroup, SMART_BASEINFO);
				if (basetGroup == NULL)
					basetGroup = IoT_AddGroup(indexGroup, SMART_BASEINFO);

				attr = IoT_FindSensorNode(basetGroup, HDD_TYPE);
				if (attr == NULL)
					attr = IoT_AddSensorNode(basetGroup, HDD_TYPE);
				if(pHDDMonInfo->hdd_type == SQFlash)
					IoT_SetStringValue(attr, HDD_TYPE_SQFLASH, IoT_READONLY);
				else if (pHDDMonInfo->hdd_type == SQFNVMe)
					IoT_SetStringValue(attr, HDD_TYPE_SQFLASH, IoT_READONLY);
				else
					IoT_SetStringValue(attr, HDD_TYPE_HDD, IoT_READONLY);

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
							int i = curSmartAttri->nvme.datalen-1;
							for (; i >=0 ; i--)
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

void UpdateMMCInfo(mmc_info_t* mmcInfo, MSG_CLASSIFY_T* mmcInfoGroup)
{
	if ( mmcInfo && mmcInfo->mmcMonInfoList && mmcInfoGroup) 
	{ 
		mmc_mon_info_node_t * item = NULL; 
		item = mmcInfo->mmcMonInfoList->next;

		while ( item !=  NULL ) 
		{
			char cTemp[128] = { 0};
			MSG_CLASSIFY_T* indexGroup = NULL;
			MSG_ATTRIBUTE_T* attr = NULL;
			mmc_mon_info_t * pMMCMonInfo = &item->mmcMonInfo;

			memset ( cTemp, 0, sizeof( cTemp  ) ) ;
			sprintf( cTemp, "Disk%d", pMMCMonInfo->mmc_index);

			indexGroup = IoT_FindGroup(mmcInfoGroup, cTemp);
			if(indexGroup == NULL)
				indexGroup = IoT_AddGroup(mmcInfoGroup, cTemp);


			attr = IoT_FindSensorNode(indexGroup, HDD_NAME);
			if(attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_NAME);
			IoT_SetStringValue(attr, pMMCMonInfo->mmc_name, IoT_READONLY);

			attr = IoT_FindSensorNode(indexGroup, HDD_INDEX);
			if(attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_INDEX);
			IoT_SetDoubleValue(attr, pMMCMonInfo->mmc_index, IoT_READONLY, NULL);

			attr = IoT_FindSensorNode(indexGroup, HDD_TYPE);
			if(attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, HDD_TYPE);
			IoT_SetStringValue(attr, HDD_TYPE_MMC, IoT_READONLY);			

			attr = IoT_FindSensorNode(indexGroup, MMC_VERSION);
			if(attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, MMC_VERSION);
			IoT_SetStringValue(attr, pMMCMonInfo->mmc_version, IoT_READONLY);

			attr = IoT_FindSensorNode(indexGroup, MMC_HEALTTH_STATUS);
			if(attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, MMC_HEALTTH_STATUS);
			IoT_SetStringValue(attr, pMMCMonInfo->mmc_health, IoT_READONLY);

			sprintf( cTemp, "%d%%-%d%%", pMMCMonInfo->mmc_health_percent -10, pMMCMonInfo->mmc_health_percent);
			attr = IoT_FindSensorNode(indexGroup, MMC_HEALTTH_PERCENT);
			if(attr == NULL)
				attr = IoT_AddSensorNode(indexGroup, MMC_HEALTTH_PERCENT);
			IoT_SetStringValue(attr, cTemp, IoT_READONLY);

			item = item->next;
		}
	}
}

double B2MB(unsigned long long value)
{
	double result = value / (1024 * 1024);
	return result;
}

void UpdateStorageInfo(DiskItemList *storages, MSG_CLASSIFY_T *storageGroup)
{
	if (storages && storageGroup)
	{
		DiskItemList *item = storages;
		while (item)
		{
			if (item->disk)
			{
				char cTemp[128] = {0};
				MSG_CLASSIFY_T *indexGroup = NULL;
				MSG_ATTRIBUTE_T *attr = NULL;
				DiskInfoItem *disk = item->disk;
				double total = B2MB(disk->totalspace);
				double free = B2MB(disk->freespace);

				memset(cTemp, 0, sizeof(cTemp));
				sprintf(cTemp, "Disk %s", disk->name);

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

void UpdateCapability(HDD_context_t* pHandlerCtx, MSG_CLASSIFY_T *myCapability)
{
	MSG_CLASSIFY_T *hddInfoGroup = NULL;
	MSG_CLASSIFY_T *smartInfoGroup = NULL;
	MSG_CLASSIFY_T* mmcInfoGroup = NULL;
	MSG_CLASSIFY_T *storageGroup = NULL;

	hdd_info_t* hddInfo = NULL;
	DiskItemList* storages = NULL;
	mmc_info_t* mmcInfo = NULL;
	if (pHandlerCtx == NULL || myCapability == NULL)
		return;
	
	hddInfoGroup = IoT_FindGroup(myCapability, HDD_INFO_LIST);
	if (hddInfoGroup == NULL)
		hddInfoGroup = IoT_AddGroupArray(myCapability, HDD_INFO_LIST);

	smartInfoGroup = IoT_FindGroup(myCapability, HDD_SMART_INFO_LIST);
	if (smartInfoGroup == NULL)
		smartInfoGroup = IoT_AddGroupArray(myCapability, HDD_SMART_INFO_LIST);

	mmcInfoGroup = IoT_FindGroup(myCapability, MMC_INFO_LIST);
	if(mmcInfoGroup == NULL)
		mmcInfoGroup = IoT_AddGroupArray(myCapability, MMC_INFO_LIST);
	storageGroup = IoT_FindGroup(myCapability, HDD_STORAGE_LIST);
	if (storageGroup == NULL)
		storageGroup = IoT_AddGroupArray(myCapability, HDD_STORAGE_LIST);
	
	mmcInfo = &pHandlerCtx->MMCInfo;
	if (mmcInfo->mmcMonInfoList )
	{
		UpdateMMCInfo(mmcInfo, hddInfoGroup);
	}

	hddInfo = &pHandlerCtx->hddCtx.hddInfo;
	if (hddInfo && hddInfo->hddMonInfoList)
	{

		UpdateHDDInfo(hddInfo, hddInfoGroup);
		//to construct HDD S.M.A.R.T. info
		UpdateSMARTInfo(hddInfo, smartInfoGroup);
	}

	storages = pHandlerCtx->storageList;
	if (storages)
	{
		UpdateStorageInfo(storages, storageGroup);
	}
	

#ifdef SQMANAGE
	{
		sqram_info_t* ramInfo = &pHandlerCtx->sqramList;
		if (ramInfo)
		{
			sqr_UpdateSQRAM(ramInfo, myCapability);
		}
			
	}
	
#endif
}

/*Create Capability Message Structure to describe sensor data*/
MSG_CLASSIFY_T *CreateCapability(HDD_context_t* pHandlerCtx)
{
	MSG_CLASSIFY_T *myCapability = IoT_CreateRoot(MyTopic);
	UpdateCapability(pHandlerCtx, myCapability);
	return myCapability;
}

void *HDDHandlerThreadStart(void *args)
{
	HDD_context_t *pHandlerCtx = (HDD_context_t *)args;
	hdd_info_t *pHDDInfo = NULL;
	DiskItemList *pStorageList = NULL;
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
		mmc_GetMMCInfo(&pHandlerCtx->MMCInfo);

#ifdef SQMANAGE
		sqr_GetSQRAMInfo(&pHandlerCtx->sqramList);
#endif 

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

		//if (g_HandlerContex.bHasSQFlash)
		{
			bool bStroageChanged = false;
			/*===Retrieve HDD info && create JSON object of infoSpec ===*/
			pthread_mutex_lock(&pHandlerCtx->hddCtx.hddMutex);
			hdd_GetHDDInfo(pHDDInfo);
			bStroageChanged = GetStorageInfo(&pStorageList);
			mmc_GetMMCInfo(&pHandlerCtx->MMCInfo);

#ifdef SQMANAGE
			sqr_GetSQRAMInfo(&pHandlerCtx->sqramList);
#endif 

			pthread_mutex_unlock(&pHandlerCtx->hddCtx.hddMutex);

			HandlerKernel_LockCapability();
			UpdateCapability(pHandlerCtx, g_Capability);
			HandlerKernel_UnlockCapability();
			/*=== Check HDDCount changed and reset the new infoSpec */
			if ((hddCnt != pHDDInfo->hddCount || bStroageChanged) && g_sendcapabilitycbf)
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

void loadINIFile()
{
	char inifile[256] = {0};
	char filename[64] = {0};
	sprintf(filename, "%s.ini", g_PluginInfo.Name);
	util_path_combine(inifile, g_PluginInfo.WorkDir, filename);
	if (!util_is_file_exist(inifile))
	{
		FILE *iniFD = fopen(inifile, "w");
		fwrite(HDD_INI_COTENT, strlen(HDD_INI_COTENT), 1, iniFD);
		fclose(iniFD);
	}
	g_iRetrieveInterval = GetIniKeyInt("Platform", "Interval", inifile);
	if (g_iRetrieveInterval < 1)
		g_iRetrieveInterval = 1;
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  HANDLER_INFO *pluginfo
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize(HANDLER_INFO *pluginfo)
{
	if (pluginfo == NULL)
		return handler_fail;
	// 1. Topic of this handler
	snprintf(pluginfo->Name, sizeof(pluginfo->Name), "%s", MyTopic);
	pluginfo->RequestID = RequestID;
	pluginfo->ActionID = ActionID;
	g_LogHandle = pluginfo->loghandle;
	HDDLog(g_LogHandle, Debug, " %s> Initialize", MyTopic);
	// 2. Copy agent info
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	// 3. Callback function -> Send JSON Data by this callback function

	g_HandlerContex.threadHandler = NULL;
	g_HandlerContex.bThreadRunning = false;
	g_Status = handler_status_no_init;

	/*Create mutex*/
	if (pthread_mutex_init(&g_HandlerContex.hddCtx.hddMutex, NULL) != 0)
	{
		HDDLog(g_LogHandle, Error, " %s> Create HDDInfo mutex failed! ", MyTopic);
		return handler_fail;
	}
	loadINIFile();
	return HandlerKernel_Initialize(pluginfo);
}

int Handler_Uninitialize()
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

	if(g_HandlerContex.MMCInfo.mmcMonInfoList){
		mmc_DestroyMMCInfoList(g_HandlerContex.MMCInfo.mmcMonInfoList);
		g_HandlerContex.MMCInfo.mmcMonInfoList = NULL;
	}
#ifdef SQMANAGE
	if (g_HandlerContex.sqramList.sqramInfoList) {
		sqr_DestroySQRAMInfo(g_HandlerContex.sqramList.sqramInfoList);
		g_HandlerContex.sqramList.count = 0;
		g_HandlerContex.sqramList.sqramInfoList = NULL;
	}
#endif
	//pthread_mutex_unlock(&g_HandlerContex.hddCtx.hddMutex);
	HandlerKernel_Uninitialize();
	pthread_mutex_destroy(&g_HandlerContex.hddCtx.hddMutex);
	return 0;
}

/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  0 : Success
 *          -1 : Failed
 * ***************************************************************************************/
int HANDLER_API Handler_Start(void)
{
	//== Init SQFlash
	pthread_mutex_lock(&g_HandlerContex.hddCtx.hddMutex);
	if (hdd_IsExistSQFlashLib())
	{
		g_HandlerContex.bHasSQFlash = hdd_StartupSQFlashLib();
		if (g_HandlerContex.bHasSQFlash)
		{
			hdd_GetHDDInfo(&g_HandlerContex.hddCtx.hddInfo);
			mmc_GetMMCInfo(&g_HandlerContex.MMCInfo);
		}
	}
#ifdef SQMANAGE
	if (sqr_IsExistSUSIDeviceLib())
	{
		sqr_StartupSUSIDeviceLib();
	}
#endif
	pthread_mutex_unlock(&g_HandlerContex.hddCtx.hddMutex);

	//== Create Thread
	g_HandlerContex.bThreadRunning = true;
	if (pthread_create(&g_HandlerContex.threadHandler, NULL, HDDHandlerThreadStart, &g_HandlerContex) != 0)
	{
		g_HandlerContex.bThreadRunning = false;
		HDDLog(g_LogHandle, Error, " %s> start handler thread failed! ", MyTopic);
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
int HANDLER_API Handler_Stop(void)
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
#ifdef SQMANAGE
	if (sqr_IsExistSUSIDeviceLib())
	{
		sqr_CleanupSUSIDeviceLib();
	}
#endif
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

	if(g_HandlerContex.MMCInfo.mmcMonInfoList){
		mmc_DestroyMMCInfoList(g_HandlerContex.MMCInfo.mmcMonInfoList);
		g_HandlerContex.MMCInfo.mmcMonInfoList = NULL;
	}

	//pthread_mutex_unlock( & g_HandlerContex.hddCtx.hddMutex );
	HandlerKernel_Stop();
	g_Status = handler_status_stop;
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Status 
 *  Input :  None
 *  Output: char * : pOutReply ( JSON )
 *  Return:  int  : Length of the status information in JSON format
 *                       :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status(HANDLER_THREAD_STATUS *pOutStatus) // JSON Format
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
void HANDLER_API Handler_OnStatusChange(HANDLER_INFO *pluginfo)
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
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char ** : pOutReply //caller should free (*pOutRelpy)
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability(char **pOutReply) // JSON Format
{
	char *result = NULL;
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
		mmc_GetMMCInfo(&g_HandlerContex.MMCInfo);
#ifdef SQMANAGE
		sqr_GetSQRAMInfo(&g_HandlerContex.sqramList);
#endif
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
	*pOutReply = (char *)malloc(len + 1);
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
void HANDLER_API Handler_MemoryFree(char *pInData)
{
	if (pInData)
	{
		free(pInData);
		pInData = NULL;
	}
	return;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input :	char * const topic, 
 *				void* const data, 
 *				const size_t datalen
 *  Output: void *pRev1, 
 *				void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv(char *const topic, void *const data, const size_t datalen, void *pRev1, void *pRev2)
{
	int cmdID = 0;
	char sessionID[MAX_SESSION_LEN] = {0};
	//HDDLog(g_LogHandle, Debug, " >Recv Topic [%s] Data %s\n", topic, (char*) data );

	/*Parse Received Command*/
	if (HandlerKernel_ParseRecvCMDWithSessionID((char *)data, &cmdID, sessionID) != handler_success)
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
		HandlerKernel_LiveReportStart(hk_auto_upload_rep, (char *)data);
		break;
	case hk_set_thr_req:
		/*Stop threshold check thread*/
		HandlerKernel_StopThresholdCheck();
		/*setup threshold rule*/
		HandlerKernel_SetThreshold(hk_set_thr_rep, (char *)data);
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
		HandlerKernel_GetSensorData(hk_get_sensors_data_rep, sessionID, (char *)data, NULL);
		break;
	case hk_set_sensors_data_req:
		/*Set Sensor Data with callback function*/
		HandlerKernel_SetSensorData(hk_set_sensors_data_rep, sessionID, (char *)data, NULL);
		break;
	default:
	{
		/* Send command not support reply message*/
		char repMsg[32] = {0};
		int len = 0;
		sprintf(repMsg, "{\"errorRep\":\"Unknown cmd!\"}");
		len = strlen("{\"errorRep\":\"Unknown cmd!\"}");
		if (g_sendcbf)
			g_sendcbf(&g_PluginInfo, hk_error_rep, repMsg, len, NULL, NULL);
	}
	break;
	}
} //Handler_Rec End

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: char * pOutReply
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	/*
	{"susiCommData":{"requestID":1001,"catalogID":4,"commCmd":2053,"handlerName":"general","autoUploadIntervalSec":30,"requestItems":{
		"SUSIControl":{"e":[{"n":"SUSIControl/Hardware Monitor"},{"n":"SUSIControl/GPIO"},{"n":"SUSIControl/Voltate/v1"}]},
		"HDDMonitor":{"e":[{"n":"HDDMonitor/hddInfoList"},{"n":"HDDMonitor/hddSmartInfoList/Disk0-Crucial_CT250MX200SSD1/PowerOnHoursPOH"},{"n":"HDDMonitor/hddSmartInfoList/Disk1-WDC WD10EZEX-08M2NA0/PowerOnHoursPOH"}]},
		"ProcessMonitor":{"e":[{"n":"ProcessMonitor/System Monitor Info"},{"n":"ProcessMonitor/Process Monitor Info/6016-conhost.exe"}]},
		"NetWork":{"e":[{"n":"NetWork/netMonInfoList/VMwareNetworkAdapterVMnet8"},{"n":"NetWork/netMonInfoList/VMwareNetworkAdapterVMnet1/netUsage"}]}
	}}}*/
	//HDDLog(g_LogHandle,Debug, "> %s Start Report", strHandlerName);
	/*create thread to report sensor data*/
	HandlerKernel_AutoReportStart(pInQuery);
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : char *pInQuery, if *pInQuery = NULL, then stop all upload message.
 *  Output: char * pOutReply
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	//HDDLog(g_LogHandle,Debug, "> %s Stop Report", strHandlerName);

	HandlerKernel_AutoReportStop(pInQuery);
}
