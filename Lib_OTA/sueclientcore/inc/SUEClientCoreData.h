/**
* @file      SUEClientCoreData.h  
* @brief     SUE Client core data define
* @author    hailong.dang
* @date      2016/4/12 
* @version   1.0.0 
* @copyright Copyright (c) Advantech Co., Ltd. All Rights Reserved                                                              
*/
#ifndef _SUE_CLIENT_CORE_DATA_
#define _SUE_CLIENT_CORE_DATA_

/**
*Enumeration values...
*/
typedef enum{
	/**The value ...*/
	SC_UNKNOW = 0,
	/**The value ...*/
	SC_QUEUE = 1,
	/**The value ...*/
	SC_START = 2,
	/**The value ...*/
	SC_DOING = 3,
	/**The value ...*/
	SC_SUSPEND = 4,
	/**The value ...*/
	SC_FINISHED = 5,
	/**The value ...*/
	SC_ERROR = 6,
}TaskStatusCode;

/**
*Enumeration values...
*/
typedef enum{
	/** This value...*/
	TASK_UNKNOW = 0,
	/** This value...*/
	TASK_DL = 1,   //Download Task
	/** This value...*/
	TASK_DP = 2,   //Deploy Task
}TaskType;

typedef enum{
    TA_UNKNOW = 0,
    TA_NORMAL = 1,
    TA_RETRY = 2,
    TA_ROLLBACK = 3,
}TaskAction;

/**
*This struct...
*/
typedef struct{
	/** This value...*/
	TaskType taskType; 
	/** This value...*/
	char * pkgName; 
	/** This value...*/
	int statusCode; 
	/** This value...*/
	union u{
		/** This value...*/
		int dlPercent; 
		/** This value...*/
		char * msg;  
	}u;
	/** This value...*/
	int errCode; 
    TaskAction taskAction;
    int retryCnt;
	long pkgOwnerId;
}TaskStatusInfo;
/**
*This struct...
*/
typedef TaskStatusInfo * PTaskStatusInfo;

/**
*This struct...
*/
typedef struct{
	/** This value...*/
	char * pkgType;
	/** This value...*/
    char * url;
	/** This value...*/
	char * pkgName;

	char * *subDevices;
	int subDeviceSize;
	/** This value...*/
	int isDeploy;

    int dlRetry;
    int dpRetry;
    int isRollBack;
    int sercurity;
    int protocal;
	long pkgOwnerId;
}DLTaskInfo;
/**
*This struct...
*/
typedef DLTaskInfo *PDLTaskInfo;

/**
*This struct...
*/
typedef struct{
	/** This value...*/
	char * pkgType;
	/** This value...*/
	char * pkgName;

    int dpRetry;
    int isRollBack;
	long pkgOwnerId;
}DPTaskInfo;
/**
*This struct...
*/
typedef DPTaskInfo *PDPTaskInfo;

struct SWInfo{
    char * name;
    char * version;
    int usable;
};
typedef struct SWInfo SWInfo;
typedef SWInfo* PSWInfo;

struct DelPkgRetInfo{
    char * name;
    int errCode;
    long pkgOwnerId;
};
typedef struct DelPkgRetInfo DelPkgRetInfo;
typedef DelPkgRetInfo * PDelPkgRetInfo;

#endif
