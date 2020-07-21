#ifndef _DISKPMQINFO_H_
#define _DISKPMQINFO_H_

#include <stdbool.h>
#include <sqflashhelper.h>

#define INFOSTRLEN		64
#define STRLENGTH		128
#define EQUAL			0

#define EVENT_MAX_NUM		8
#define EVENTID_CLR			0
#define EVENTID_BASE		1
#define EVENT1				(EVENTID_BASE << 0)
#define EVENT2				(EVENTID_BASE << 1)
#define EVENT3				(EVENTID_BASE << 2)
#define EVENT4				(EVENTID_BASE << 3)
#define EVENT5				(EVENTID_BASE << 4)
#define EVENT6				(EVENTID_BASE << 5)
#define EVENT7				(EVENTID_BASE << 6)
#define EVENT8				(EVENTID_BASE << 7)

#define SMART5_THR			10
#define SMART9_THR			26280
#define SMART187_THR		1
#define SMART192_THR		190
#define SMART197_THR		2

#define HDD_THR				0.385
#define SQF_THR				0.66

#define PLUGIN_NAME				"HDD_PMQ"

#define INFO_GROUP_NAME			"info"
#define DATA_GROUP_NAME			"data"
#define PREDICT_GROUP_NAME		"predict"
#define EVENT_GROUP_NAME		"event"
#define ACTION_GROUP_NAME		"action"
#define PARAMETER_GROUP_NAME	"param"

#define HDD_SMART_LEN		6
#define SQF_SMART_LEN		2

#define SEC2HOUR			3600

#define SQF_MLC_MAX_PROG	3000
#define SQF_SLC_MAX_PROG	100000

#define LOADPATH				"../config/"
#define LOADFILENAME			"HDDPMQ.ini"
#define SECTIONNAME				"ReportConfig"
#define INTERVALKEY				"ReportInterval"
#define REPORTKEY				"EnableReport"

typedef enum
{
	MLC = 0,
	ULC,
	SLC,
} SQFType_t;

typedef enum
{
	isKeep = 0,
	isChange,
	isErr,
} State_t;

typedef enum
{
	HDD = 0,
	SQF,
	SSD,
} DiskType_t;

typedef struct param
{
	unsigned int reportInterval;
	bool enableReport;

	struct param *next;
} ParamInfo, *pParamInfo;

typedef struct actinfo
{
	char name[STRLENGTH];
	char msg[STRLENGTH];
	bool enable;

	struct actinfo *next;
} ActionInfo, *pActionInfo;

typedef struct eventinfo
{
	char name[STRLENGTH];
	char msg[STRLENGTH];
	unsigned int actionNum;

	struct eventinfo *next;
} EventInfo, *pEventInfo;

typedef struct attrnode
{
	unsigned int smartNum;
	unsigned int val;
	char msg[STRLENGTH];

	struct attrnode *next;
} AttrNode, *pAttrNode;

typedef struct smart
{
	AttrNode smart5;
	AttrNode smart9;
	AttrNode smart173;
	AttrNode smart187;
	AttrNode smart191;
	AttrNode smart192;
	AttrNode smart194;
	AttrNode smart197;
	AttrNode smart198;
	AttrNode smart199;
	AttrNode smart243;
} SMART, *pSMART;

typedef struct diskPMQ
{
	char diskname[STRLENGTH];			// done
	unsigned int max_program;			// done
	hdd_type_t type;					// done
	SMART smart;						// done

	unsigned char eventID;				// done
	double predictVal;					// done
	State_t state;						// done
	SQFType_t sqfType;
	
	struct diskPMQ *old;
	struct diskPMQ *next;
} DiskPMQ, *pDiskPMQ;

typedef struct InfoConfig
{
	char type[INFOSTRLEN];
	char name[INFOSTRLEN];
	char description[INFOSTRLEN];
	char version[INFOSTRLEN];
	char update[INFOSTRLEN];
	double confidence;
	//char confidence[INFOSTRLEN];
	bool eventNotify;
} InfoConfig, *pInfoConfig;

typedef struct HandlerConfig
{
	char iniDir[STRLENGTH];
	char iniFileName[STRLENGTH];

	InfoConfig infoConfig;
	EventInfo eventInfo;
	ActionInfo actInfo;
	ParamInfo paramInfo;
} HandlerConfig, *pHandlerConfig;

bool GetHDDEventID(pDiskPMQ diskpmq);
bool GetPrediectVal(pDiskPMQ diskpmq);
bool GetSMARTFromHDDInfo(pSMART smart, hdd_mon_info_node_t *source);
bool GetHDDType(pDiskPMQ diskpmq);
bool UpdatePMQInfoFromHDDInfo(pDiskPMQ diskpmq, hdd_info_t *source);
bool SavePMQParameter(pHandlerConfig pmqConfig);
bool LoadPMQParameter(pHandlerConfig pmqConfig);
bool UnInitPMQParameter(void);

#endif