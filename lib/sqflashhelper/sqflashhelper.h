
#ifndef _SQFLASH_HELPER_H_
#define _SQFLASH_HELPER_H_
#include <stdbool.h>

#ifdef WIN32
#include "stdint.h"
#include "sqflib.h"

// Vendor SMART Extend
typedef struct ExtSMARTAttr {
	/*Old SQFlash Struct*/
	unsigned int MaxReservedBlock;
	unsigned int CurrentReservedBlock;
	unsigned int GoodBlockRate;
	/*New SQFlash Struct*/
	uint64_t UncorrectableECCCnt;	// 0x01 EccCount
	uint64_t PowerOnHours;			// 0x09 PowerOnTime
	uint64_t PowerCycleCnt;			// 0x0C
	uint32_t DeviceCapacity;		// 0x0E, Sector 512Byte
	uint32_t UserCapacity;			// 0x0F, Sector 512Byte
	uint32_t TotalAvailSpareBlk;	// 0x10
	uint32_t RemainingSpareBlk;		// 0x11
	uint32_t TotalEraseCnt;			// 0x64
	uint64_t PHYErrCnt;				// 0xA8
	uint16_t LaterBadBlkCnt;		// 0xAA, 10 ~ 9
	uint16_t EarlyBadBlkCnt;		// 0xAA, 6 ~ 5
	uint16_t AvgEraseCnt;			// 0xAD, 8 ~ 7 AverageProgram
	uint16_t MaxEraseCnt;			// 0xAD, 6 ~ 5 MaxProgram
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
	uint8_t SSDLifeLeft;			// 0xE7, % EnduranceCheck
	uint64_t TotalNANDRead;			// 0xEA, Sector 512Byte
	uint64_t TotalNANDWritten;		// 0xEB, Sector 512Byte
	uint64_t HostWrite;				// 0xF1, Sector 512Byte
	uint64_t HostRead;				// 0xF2, Sector 512Byte
	char smartAttriBuf[512];
} ExtSMARTAttr_t, * PExtSMARTAttr_t;


typedef struct ExtNVMeSMART {
	// NVMe Standard SMART
	uint8_t	CriticalWarning;				// 0x01
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
	// NVMe Vendor SMART
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
	//char smartAttriStd[512];
	//char smartAttriVen[512];
} ExtNVMeSMART_t, * PExtNVMeSMART_t;
#else

typedef struct _ATA_SMART_ATTR_TABLE
{
   unsigned int dwMaxProgram;
   unsigned int dwAverageProgram;
   unsigned int dwEnduranceCheck;
   unsigned int dwPowerOnTime;
   unsigned int dwEccCount;
   unsigned int dwMaxReservedBlock;
   unsigned int dwCurrentReservedBlock;
   unsigned int dwGoodBlockRate;
} ATA_SMART_ATTR_TABLE, *PATA_SMART_ATTR_TABLE;

#endif

#define INVALID_DEVMON_VALUE  (-999)
#define DEFAULT_REMAIN_DAYS (-1) 

typedef enum{
	HddTypeUnknown = 0,
	SQFlash,
	StdDisk,
	SQFNVMe,
}hdd_type_t;

typedef enum{
	SmartAttriTypeUnknown       = 0x00,
	ReadErrorRate               = 0x01,
	ThroughputPerformance       = 0x02,
	SpinUpTime                  = 0x03,
	StartStopCount              = 0x04,
	ReallocatedSectorsCount     = 0x05,
	ReadChannelMargin           = 0x06,
	SeekErrorRate               = 0x07,
	SeekTimePerformance         = 0x08,
	PowerOnHoursPOH             = 0x09,
	SpinRetryCount              = 0x0A,
	CalibrationRetryCount       = 0x0B,
	PowerCycleCount             = 0x0C,
	SoftReadErrorRate           = 0x0D,
	SQDeviceCapacity			= 0x0E,       //SQ Flash
	SQUserCapacity				= 0x0F,       //SQ Flash
	SQInitialSpareBlocksAvailable = 0x10,     //SQ Flash
	SQSpareBlocksRemaining		= 0x11,       //SQ Flash
	SQTotalEraseCount			= 0x64,       //SQ Flash
	SQSATAPHYErrorCount			= 0XA8,       //SQ Flash
	SQBadBlockCount				= 0xAA,       //SQ Flash
	SQEraseCount				= 0xAD,       //SQ Flash: old SQAverageProgram
	SQUnexpectedPowerLossCount	= 0xAE,       //SQ Flash
	SQPowerFailureProtectionStatus = 0xAF,    //SQ Flash
	SATADownshiftErrorCount     = 0xB7,
	EndtoEnderror               = 0xB8,
	HeadStability               = 0xB9,
	InducedOpVibrationDetection = 0xBA,
	ReportedUncorrectableErrors = 0xBB,
	CommandTimeout              = 0xBC,
	HighFlyWrites               = 0xBD,
	AirflowTemperatureWDC       = 0xBE,
//TemperatureDifferencefrom100  = 0xBE,
	GSenseErrorRate             = 0xBF,
	PoweroffRetractCount        = 0xC0,       //SQ Flash: UnexpectedPowerLossCount
	LoadCycleCount              = 0xC1,
	Temperature                 = 0xC2,
	HardwareECCRecovered        = 0xC3,
	ReallocationEventCount      = 0xC4,
	CurrentPendingSectorCount   = 0xC5,
	UncorrectableSectorCount    = 0xC6,
	UltraDMACRCErrorCount       = 0xC7,
	MultiZoneErrorRate          = 0xC8,
//WriteErrorRateFujitsu         = 0xC8,
	OffTrackSoftReadErrorRate   = 0xC9,
	DataAddressMarkerrors       = 0xCA,       //SQ Flash: PercentageOfSparesRemaining
	RunOutCancel                = 0xCB,
	SoftECCCorrection           = 0xCC,
	ThermalAsperityRateTAR      = 0xCD,
	FlyingHeight                = 0xCE,
	SpinHighCurrent             = 0xCF,
	SpinBuzz                    = 0xD0,
	OfflineSeekPerformance      = 0xD1,
	VibrationDuringWrite        = 0xD3,
	ShockDuringWrite            = 0xD4,
	SQCRCError					= 0xDA,       //SQ Flash
	DiskShift                   = 0xDC,
	GSenseErrorRateAlt          = 0xDD,
	LoadedHours                 = 0xDE,
	LoadUnloadRetryCount        = 0xDF,
	LoadFriction                = 0xE0,
	LoadUnloadCycleCount        = 0xE1,
	LoadInTime                  = 0xE2,
	TorqueAmplificationCount    = 0xE3,
	PowerOffRetractCycle        = 0xE4,
	GMRHeadAmplitude            = 0xE6,
	DriveTemperature            = 0xE7,       //SQ Flash: SSD Life Remaining
	SQEnduranceRemainLife       = 0xE8,       //SQ Flash Old
	SQPowerOnTime               = 0xE9,       //SQ Flash Old
	SQTotalNANDRead				= 0xEA,       //SQ Flash: Old SQECCLog
	SQTotalNANDWritten			= 0xEB,       //SQ Flash: Old GoodBlockRate
	HeadFlyingHours             = 0xF0,
//TransferErrorRateFujitsu      = 0xF0,
	TotalLBAsWritten            = 0xF1,       //SQFlash: TotalHostWrite
	TotalLBAsRead               = 0xF2,       //SQFlash: TotalHostRead
	ReadErrorRetryRate          = 0xFA,
	FreeFallProtection          = 0xFE,
}smart_attri_type_t;

typedef enum {
	// NVMe Standard SMART
	CriticalWarning = 0x01,
	CompositeTemperature = 0x02,
	AvailableSpare = 0x03,
	AvailableSpareThreshold = 0x04,
	PercentageUsed = 0x05,
	DataUnitsRead = 0x07,
	DataUnitsWritten = 0x08,
	HostReadCommands = 0x09,
	HostWriteCommands = 0x0A,
	ControllerBusyTime = 0x0B,
	PowerCycles = 0x0C,
	PowerOnHours = 0x0D,
	UnsafeShutdowns = 0x0E,
	MediaErrors = 0x0F,
	ErrorLogNumber = 0x10,
	WarningCompositeTemperatureTime = 0x11,
	CriticalCompositeTemperatureTime = 0x12,
	TemperatureSensor1 = 0x13,
	TemperatureSensor2 = 0x14,
	TemperatureSensor3 = 0x15,
	TemperatureSensor4 = 0x16,
	TemperatureSensor5 = 0x17,
	TemperatureSensor6 = 0x18,
	TemperatureSensor7 = 0x19,
	TemperatureSensor8 = 0x1A,
// NVMe Vendor SMART
	FlashReadSector = 0x1C,
	FlashWriteSector = 0x1D,
	UNCError = 0x1E,
	PyhError = 0x1F,
	EarlyBadBlock = 0x20,
	LaterBadBlock = 0x21,
	MaxEraseCount = 0x22,
	AvgEraseCount = 0x23,
	CurPercentSpares = 0x24,
	CurTemperature = 0x25,
	LowestTemperature = 0x26,
	HighestTemperature = 0x27,
	ChipInternalTemperature = 0x28,
	SpareBlocks = 0x29,
}nvme_smart_attri_type_t;

typedef struct{
	smart_attri_type_t attriType;
	char attriName[64];
	int attriFlags;
	int attriWorst;
	int attriThresh;
	int attriValue;
	char attriVendorData[6];
}smart_attri_info_t;

typedef struct {
	nvme_smart_attri_type_t attriType;
	char attriName[64];
	long attriValue;
	int datalen;
	char attriVendorData[16];
}nvme_attri_info_t;

typedef struct smart_attri_info_node_t{
	union
	{
		smart_attri_info_t sata;
		nvme_attri_info_t nvme;
	};
	bool bNVMe;
	struct smart_attri_info_node_t * next;
}smart_attri_info_node_t;

typedef smart_attri_info_node_t * smart_attri_info_list;


typedef struct{
	hdd_type_t hdd_type;
	char hdd_name[128];
	int hdd_index; 
	unsigned int max_program;
	unsigned int average_program;
	//unsigned int endurance_check;
	unsigned int power_on_time;
	unsigned int ecc_count;
	//unsigned int max_reserved_block;
	//unsigned int current_reserved_block;
	//unsigned int good_block_rate;
	unsigned int hdd_health_percent;
	unsigned int remain_days;
	unsigned int power_cycle;
	unsigned int unexpected_power_loss_count;
	unsigned int later_bad_block;
	unsigned int crc_error;
	unsigned int opal_status;
	unsigned long long nand_write;
	unsigned long long nand_read;
	unsigned long long host_write;
	unsigned long long host_read;
	int hdd_temp;
	int max_temp;
	char SerialNumber[21];
	char FirmwareRevision[9];
	smart_attri_info_list smartAttriInfoList;
}hdd_mon_info_t;

typedef struct hdd_mon_info_node_t{
	hdd_mon_info_t hddMonInfo;
	struct hdd_mon_info_node_t * next;
}hdd_mon_info_node_t;

typedef hdd_mon_info_node_t * hdd_mon_info_list;

typedef struct{
	int hddCount;
	hdd_mon_info_list hddMonInfoList;
}hdd_info_t;

#ifdef __cplusplus
extern "C" {
#endif

	bool hdd_IsExistSQFlashLib();

	bool hdd_StartupSQFlashLib();

	bool hdd_CleanupSQFlashLib();

	hdd_mon_info_list hdd_CreateHDDInfoList();

	void hdd_DestroyHDDInfoList(hdd_mon_info_list hddInfoList);

	bool hdd_GetHDDInfo(hdd_info_t * pHDDInfo);

	bool hdd_SelfTest(int devID);

	bool hdd_GetSelfTestStatus(int devID, unsigned int* progress, unsigned int* status);

#ifdef __cplusplus
}
#endif

#endif /* _SQFLASH_HELPER_H_ */