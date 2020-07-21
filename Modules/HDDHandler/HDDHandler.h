#ifndef _HDD_HANDLER_H_
#define _HDD_HANDLER_H_


#include <cJSON.h>
#include <sqflashhelper.h>
#include <mmchelper.h>
#include <pthread.h>
#include "util_storage.h"
//#include "ThresholdHelper.h"
//#include "Parser.h"



#define cagent_request_hdd_monitoring 19
#define cagent_action_hdd_monitoring 113
//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define MyTopic	 "HDDMonitor"

int Handler_Uninitialize();

#endif /* _HDD_HANDLER_H_ */
