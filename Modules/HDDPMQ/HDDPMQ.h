#ifndef _HDDPMQ_H_
#define _HDDPMQ_H_

#include "pthread.h"

#define DEBUG_SWITCH		false
#define REPORT_STR(x)		MSG_PrintUnformatted(x)
#define DEBUG_PRINTF(x)		DEBUG_SWITCH? printf("\n\n========== JSON String ==========\n%s\n\n", x):printf("")

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define CAGENT_REQUEST_CUSTOM			2102	// define the request ID for V3.0, not used on V3.1 or later
#define CAGENT_CUSTOM_ACTION			31002	// define the action ID for V3.0, not used on V3.1 or later

#define ENABLE_UPDATE_PATH				"HDD_PMQ/param/enable report"			
#define UPDATE_PERIOD_PATH				"HDD_PMQ/param/report interval"


typedef struct disk_check
{
	int count;
	long long nextChkTs;
	struct disk_obj *disk;
} disk_check;

typedef struct hdd_context_t
{
	hdd_info_t hddInfo;
	pthread_t hddMutex;
} hdd_context;

typedef struct handler_context_t
{
	void *threadHandler;
	bool bThreadRunning;
	bool bHasSQFlash;
	hdd_context hddCtx;
} handler_context;


#endif