#ifndef _WIN_SERVICE_H_
#define _WIN_SERVICE_H_


#include "export.h"
#ifdef WIN32
#include <Windows.h>
#define DEF_SERVICE_NAME             "AgentService_31"
#else
#define DEF_SERVICE_NAME             "saagent"
#endif
#define DEF_SRVC_LOG_FILE_NAME       "AgentServiceLog.txt"
#define SERVICE_CONTROL_USER         128
#define MAX_CMD_LEN                  32
#define DEF_SERVICE_VERSION          "1.0"
#define HELP_SERVICE_CMD             "-h"
#define DSVERSION_SERVICE_CMD        "-v"
#define INSTALL_SERVICE_CMD          "-i"
#define UNINSTALL_SERVICE_CMD        "-u"
#define QUIT_CMD                     "-q"
#define NSRV_RUN_CMD                 "-n"
#define STOP_NSER_RUN_CMD            "-s"

/*
 * Callback function "APP_START_CB" and "APP_STOP_CB"
 * return 0 is success, else are fail.
 */
typedef int (*APP_START_CB) ();
typedef int (*APP_STOP_CB) ();

#ifdef __cplusplus
extern "C" {
#endif

WISEPLATFORM_API int ServiceInit(char * pSrvcName, char * pVersion, APP_START_CB pStart, APP_STOP_CB pStop, void * logHandle);
WISEPLATFORM_API int ServiceUninit();
WISEPLATFORM_API int LaunchService();
WISEPLATFORM_API int ExecuteCmd(char* cmd);

#ifdef __cplusplus
}
#endif

#endif
