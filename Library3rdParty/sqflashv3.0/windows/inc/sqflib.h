#ifndef _SQFLIB_H_
#define _SQFLIB_H_

/* Error code */
#define SQF_SUCCESS							0

// (error code < 0) Fail/Error
#define SQF_ERROR							-1
#define SQF_ACCESS_FAILED					-2
#define SQF_CMD_FAILED						-3

#define SQF_SIZE_TOO_SMALL					-5
#define SQF_INVALID_PARAMETER				-6
#define SQF_NOT_FOUND_RESOURCE				-7

#define SQF_NOT_FOUND_DISK					-10
#define SQF_INVALID_SQFLASH					-11
#define SQF_INVALID_APKEY					-12
#define SQF_INVALID_ACCESS_CODE				-13
#define SQF_INVALID_PASSWORD				-14
#define SQF_INVALID_ACTION					-15

#define SQF_UNSUPPORTED_FEATURE				-20

// (error code > 0) Success, but has specify status
#define SQF_ACCESS_CODE_EXIST				10
#define SQF_ACCESS_CODE_NOT_EXIST			11
#define SQF_ACCESS_CODE_NOT_MATCH			12

/* Standard APIs */
int SQFlash_GetModelName(int index, char ModelNameStr[41]);
int SQFlash_GetSerialNumber(int index, char SerialNumberStr[21]);
int SQFlash_GetFirmwareRevision(int index, char FirmwareRevisionStr[9]);
int SQFlash_GetIdentifyDevice(int index, unsigned char IdentifyDevice[512]);
int SQFlash_GetDiskType(int index, char DiskType[4]);

#define SQF_INFO_TYPE_BASE			0x00020000
#define SQF_INFO_TYPE_INTERFACE		(SQF_INFO_TYPE_BASE + 0)
#define SQF_INFO_TYPE_ENDURANCE		(SQF_INFO_TYPE_BASE + 1)
#define SQF_INFO_TYPE_OPALSTATUS	(SQF_INFO_TYPE_BASE + 2)	// Bit 0: lockingSupported, Bit 1: lockingEnabled, Bit 2: locked

#define SQF_INTERFACE_UNKNOWN		0x00
#define SQF_INTERFACE_SATA			0x01
#define SQF_INTERFACE_NVME			0x02
int SQFlash_GetInformation(int index, uint32_t infoType, uint32_t *result);

/* ------------------------------------------ SQFlash SATA SMART ------------------------------------------  */
int SQFlash_ReadSMARTAttribute(int index, unsigned char SmartData[512]);

// SATA SMART
typedef struct SATASMART {
	uint64_t UncorrectableECCCnt;	// 0x01
	uint64_t PowerOnHours;			// 0x09
	uint64_t PowerCycleCnt;			// 0x0C
	uint32_t DeviceCapacity;		// 0x0E, Sector 512Byte
	uint32_t UserCapacity;			// 0x0F, Sector 512Byte
	uint32_t TotalAvailSpareBlk;	// 0x10
	uint32_t RemainingSpareBlk;		// 0x11
	uint32_t TotalEraseCnt;			// 0x64
	uint64_t PHYErrCnt;				// 0xA8
	uint16_t LaterBadBlkCnt;		// 0xAA, 10 ~ 9
	uint16_t EarlyBadBlkCnt;		// 0xAA, 6 ~ 5
	uint16_t AvgEraseCnt;			// 0xAD, 8 ~ 7
	uint16_t MaxEraseCnt;			// 0xAD, 6 ~ 5
	uint32_t UnexpectedPwrLostCnt;	// 0xAE
	uint32_t VolStabilizerTriggerCnt;// 0xAF, 10 ~ 7
	uint8_t GuaranteedFlush;		// 0xAF, 6, 0x01 Enable
	uint8_t DriveStatus;			// 0xAF, 5, 0x00 Normal
	uint16_t UnexpectedPwrLostCnt2;	// 0xC0, same with 0xAE
	uint16_t MaxTemperature;		// 0xC2, 10 ~ 9
	uint16_t MinTemperature;		// 0xC2, 8 ~ 7
	uint16_t CurTemperature;		// 0xC2, 6 ~ 5
	uint8_t SSDLifeUsed;			// 0xCA, %
	uint64_t CRCErrCnt;				// 0xDA
	uint8_t SSDLifeLeft;			// 0xE7, %
	uint64_t TotalNANDRead;			// 0xEA, Sector 512Byte
	uint64_t TotalNANDWritten;		// 0xEB, Sector 512Byte
	uint64_t HostWrite;				// 0xF1, Sector 512Byte
	uint64_t HostRead;				// 0xF2, Sector 512Byte
} SATASMART_t, *PSATASMART_t;
int SQFlash_ReadParsedSMARTAttribute(int index, PSATASMART_t sataSmart);

int SQFlash_ReadSMARTAttributeThresholds(int index, unsigned char AttributeThresholds[512]);

// Vendor SMART
typedef struct VSMARTAttr {
	unsigned int MaxProgram;
	unsigned int AverageProgram;
	unsigned int EnduranceCheck;
	unsigned int PowerOnTime;
	unsigned int EccCount;
	unsigned int MaxReservedBlock;
	unsigned int CurrentReservedBlock;
	unsigned int GoodBlockRate;
} VSMARTAttr_t, *PVSMARTAttr_t;
int SQFlash_ReadVendorSMARTAttributes(int index, PVSMARTAttr_t Attr, int AttrSize);

/* ------------------------------------------ SQFlash NVMe SMART ------------------------------------------  */
int SQFlash_NVMe_StdSMARTRaw(int index, uint8_t stdSmartRaw[512]);
int SQFlash_NVMe_VendorSMARTRaw(int index, uint8_t vendorSmartRaw[512]);

// NVMe Standard SMART
typedef struct NVMeStdSMART {
	uint8_t CriticalWarning;				// 0x01
	uint16_t CompositeTemperature;			// 0x02
	uint8_t AvailableSpare;					// 0x03
	uint8_t AvailableSpareThreshold;		// 0x04
	uint8_t PercentageUsed;					// 0x05
	uint8_t DataUnitsRead[16];				// 0x07
	uint8_t DataUnitsWritten[16];			// 0x08
	uint8_t HostReadCommands[16];			// 0x09
	uint8_t HostWriteCommands[16];			// 0x0A
	uint8_t ControllerBusyTime[16];			// 0x0B
	uint8_t PowerCycles[16];				// 0x0C
	uint8_t PowerOnHours[16];				// 0x0D
	uint8_t UnsafeShutdowns[16];			// 0x0E
	uint8_t MediaErrors[16];				// 0x0F
	uint8_t ErrorLogNumber[16];				// 0x10
	uint32_t WarningCompositeTemperatureTime;	// 0x11
	uint32_t CriticalCompositeTemperatureTime;	// 0x12
	uint16_t TemperatureSensor1;			// 0x13
	uint16_t TemperatureSensor2;			// 0x14
	uint16_t TemperatureSensor3;			// 0x15
	uint16_t TemperatureSensor4;			// 0x16
	uint16_t TemperatureSensor5;			// 0x17
	uint16_t TemperatureSensor6;			// 0x18
	uint16_t TemperatureSensor7;			// 0x19
	uint16_t TemperatureSensor8;			// 0x1A
} NVMeStdSMART_t, *PNVMeStdSMART_t;
int SQFlash_NVMe_StdSMART(int index, PNVMeStdSMART_t stdSmart);

// NVMe Vendor SMART
typedef struct NVMeVenSMART {
	uint8_t FlashReadSector[8];		// 0x1C
	uint8_t FlashWriteSector[8];	// 0x1D
	uint8_t UNCError[8];			// 0x1E
	uint32_t PyhError;				// 0x1F
	uint32_t EarlyBadBlock;			// 0x20
	uint32_t LaterBadBlock;			// 0x21
	uint32_t MaxEraseCount;			// 0x22
	uint32_t AvgEraseCount;			// 0x23
	uint8_t CurPercentSpares[8];	// 0x24
	uint16_t CurTemperature;		// 0x25
	uint16_t LowestTemperature;		// 0x26, K
	uint16_t HighestTemperature;	// 0x27, K
	uint16_t ChipInternalTemperature;// 0x28, K
	uint16_t SpareBlocks;			// 0x29
} NVMeVenSMART_t, *PNVMeVenSMART_t;
int SQFlash_NVMe_VendorSMART(int index, PNVMeVenSMART_t vendorSmart);

// SMART self-test
#define SMART_SHORT_SELF_TEST_OFFLINE_MODE				0x01
#define SMART_EXTENDED_SELF_TEST_OFFLINE_MODE			0x02
int SQFlash_ExecuteSMARTSelfTest(int index, uint8_t Subcommand);

#define SELF_TEST_STATUS_IN_PROGRESS				0x0F
#define SELF_TEST_STATUS_SUCCESS					0x00	// 0x01 ~ 0x08 means different error, check ATA Command Set Spec.
int SQFlash_ReadSMARTSelfTestStatus(int index, uint8_t *progress, uint8_t *result);

// Initialize
int SQFlash_SendAccessCode(int index, const char *SerialNumber);
int SQFlash_SetAccessCode(int index, const char *SerialNumber);
int SQFlash_ChangeAccessCode(int index, const char *orgsn, const char *newsn);

// Security ID
#define SID_CAP_ITEM_ID_MAX_LENGTH					0x00000000		// Content type: int
#define SID_CAP_ITEM_ID2_MAX_LENGTH					0x00001000		// Content type: int
int SQFlash_GetSecurityIDCap(int index, unsigned int Item, void *Content);

int SQFlash_SetSecurityID(int index, const char *SecurityID);
int SQFlash_GetSecurityID(int index, char *SecurityID, int Length);
int SQFlash_SetSecurityID2(int index, const char *SecurityID);
int SQFlash_GetSecurityID2(int index, char *SecurityID, int Length);

// Erase
#define QUICK_ERASE_OPTION_NONE						0
#define QUICK_ERASE_OPTION_CHECK_SUPPORT			(1 << 31)
int SQFlash_QuickErase(int index, int option);

// Flash Vault
//	Password: Maximum length is 32 bytes
int SQFlash_SetVaultEnable(int index, char *Password);
int SQFlash_SetVaultDisable(int index, char *Password);
int SQFlash_SetVaultUnlock(int index, char *Password);

#define VAULT_STATUS_ENABLE							0
#define VAULT_STATUS_DISABLE						1
#define VAULT_STATUS_UNLOCKED						2	// Temporary unlock, re-lock after power reset
#define VAULT_STATUS_CONFLICT						3	// Conflict with other feature
int SQFlash_GetVaultStatus(int index, unsigned char *Status);

// Flash Lock V2 (Windows 32-bit library) (No need access code)
#if defined(_WIN32)
#define LOCK_STATUS_ENABLE							0
#define LOCK_STATUS_DISABLE							1
int SQFlash_LockV2GetStatus(int index,  unsigned char *Status);

int SQFlash_LockV2Enable(int index, char *Password);
int SQFlash_LockV2Disable(int index, char *Password);

#endif /* _WIN32 */

//#ifdef __cplusplus
//}
//#endif /*__cplusplus*/

#endif /* _SQFLIB_H_ */
