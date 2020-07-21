#include "SUEClientCore.h"

#define SUEC_CORE_SUCCESS_STR                         "Success!"

#define SUEC_CORE_I_PARAMETER_ERROR_STR               "Interface parameter error!"

#define SUEC_CORE_I_NOT_INIT_STR                      "SUECCore library not init!"

#define SUEC_CORE_I_ALREADY_INIT_STR                  "SUECCore library already start!"

#define SUEC_CORE_I_NOT_START_STR                     "SUECCore library not start!"

#define SUEC_CORE_I_ALREADY_START_STR                 "SUECCore library already start!"

#define SUEC_CORE_I_TASK_PROCESS_START_FAILED_STR     "Task process startup failed!"

#define SUEC_CORE_I_PKG_TASK_STATUS_UNKNOW_STR        "Task status not checked!"

#define SUEC_CORE_I_PKG_TASK_EXIST_STR                "Package task already exist!"

#define SUEC_CORE_I_PKG_STATUS_NOT_FIT_STR            "Operation cannot be performed at the current state of the package!"

#define SUEC_CORE_I_PKG_SUSPEND_FAILED_STR            "Package suspend failed!"

#define SUEC_CORE_I_PKG_NOT_SUSPEND_STR               "Package is not suspend!"

#define SUEC_CORE_I_PKG_RESUME_FAILED_STR             "Package resume failed!"

#define SUEC_CORE_I_FT_INIT_FAILED_STR                "FT library init failed!"

#define SUEC_CORE_I_OBJECT_NOT_FOUND_STR              "Package object to be operated has not been found!"

#define SUEC_CORE_I_CFG_READ_FAILED_STR               "Config read failed!"

#define SUEC_CORE_I_PKG_FILE_EXIST_STR                 "Package file exist!"

#define SUEC_CORE_I_DLING_STR                      "Package Downloading!"

#define SUEC_CORE_S_DL_START_FAILED_STR               "Package start download failed!"

#define SUEC_CORE_S_DP_START_FAILED_STR               "Package start deploy failed!"

#define SUEC_CORE_S_PKG_PATH_ERROR_STR                "Package path error!"

#define SUEC_CORE_S_PKG_FILE_EXIST_STR                "Package file exist!"

#define SUEC_CORE_S_PKG_FT_DL_ERROR_STR               "Package file prepare transfer error!"

#define SUEC_CORE_S_CALC_MD5_ERROR_STR                "Calculate MD5 error!"

#define SUEC_CORE_S_MD5_NOT_MATCH_STR                 "MD5 not match!"

#define SUEC_CORE_S_UNZIP_ERROR_STR                   "Unzip error!"

#define SUEC_CORE_S_XML_PARSE_ERROR_STR               "XML parse error!"

#define SUEC_CORE_S_OS_NOT_MATCH_STR                  "OS not match!"

#define SUEC_CORE_S_ARCH_NOT_MATCH_STR                "Arch not match!"

#define SUEC_CORE_S_TAGS_NOT_MATCH_STR                "Tags not match!"

#define SUEC_CORE_S_DPFILE_NOT_EXIST_STR              "Deploy file not exist!"

#define SUEC_CORE_S_DP_EXEC_FAILED_STR                "Deploy file exec failed!"

#define SUEC_CORE_S_PKG_FILE_NOT_EXIST_STR             "Package file not exist!"

#define SUEC_CORE_S_BS64DESDECODE_ERROR_STR          "Base64 or DES decode error!"

#define SUEC_CORE_S_DP_WAIT_FAILED_STR                   "Deploy wait process exit failed!"

#define SUEC_CORE_S_CFG_SYSSTARTUP_DP_STR               "Already config to deployed at system startup!"

#define SUEC_CORE_S_BK_FILE_NOT_FOUND_STR              "Package back file not found!"

#define SUEC_CORE_S_DP_ROLLBACK_ERROR_STR              "Package deploy rollback error!"

#define SUEC_CORE_S_DP_TIMEOUT_STR                     "Package deploy timeout!"

#define SUEC_CORE_S_RETCHECK_EXEC_ERROR_STR              "Package result check script exec failed!"

#define SUEC_CORE_S_RETCHECK_EXEC_TIMEOUT_STR            "Package result check script exec timeout!"

#define SUEC_CORE_S_DL_ABORT_STR                         "Package Download abort!"

char * SuecCoreGetErrorMsg(unsigned int errorCode)
{
	char * errMsg = 0;

	switch(errorCode)
	{
	case SUEC_CORE_SUCCESS:
		{
			errMsg = SUEC_CORE_SUCCESS_STR;
			break;
		}
	case SUEC_CORE_I_PARAMETER_ERROR:
		{
			errMsg = SUEC_CORE_I_PARAMETER_ERROR_STR;
			break;
		}
	case SUEC_CORE_I_NOT_INIT:
		{
			errMsg = SUEC_CORE_I_NOT_INIT_STR;
			break;
		}
	case SUEC_CORE_I_ALREADY_INIT:
		{
			errMsg = SUEC_CORE_I_ALREADY_INIT_STR;
			break;
		}
	case SUEC_CORE_I_NOT_START:
		{
			errMsg = SUEC_CORE_I_NOT_START_STR;
			break;
		}
	case SUEC_CORE_I_ALREADY_START:
		{
			errMsg = SUEC_CORE_I_ALREADY_START_STR;
			break;
		}
	case SUEC_CORE_I_TASK_PROCESS_START_FAILED:
		{
			errMsg = SUEC_CORE_I_TASK_PROCESS_START_FAILED_STR;
			break;
		}
	case SUEC_CORE_I_PKG_TASK_STATUS_UNKNOW:
		{
			errMsg = SUEC_CORE_I_PKG_TASK_STATUS_UNKNOW_STR;
			break;
		}
	case SUEC_CORE_I_PKG_TASK_EXIST:
		{
			errMsg = SUEC_CORE_I_PKG_TASK_EXIST_STR;
			break;
		}
	case SUEC_CORE_I_PKG_STATUS_NOT_FIT:
		{
			errMsg = SUEC_CORE_I_PKG_STATUS_NOT_FIT_STR;
			break;
		}
	case SUEC_CORE_I_PKG_SUSPEND_FAILED:
		{
			errMsg = SUEC_CORE_I_PKG_SUSPEND_FAILED_STR;
			break;
		}
	case SUEC_CORE_I_PKG_NOT_SUSPEND:
		{
			errMsg = SUEC_CORE_I_PKG_NOT_SUSPEND_STR;
			break;
		}
	case SUEC_CORE_I_PKG_RESUME_FAILED:
		{
			errMsg = SUEC_CORE_I_PKG_RESUME_FAILED_STR;
			break;
		}
	case SUEC_CORE_I_FT_INIT_FAILED:
		{
			errMsg = SUEC_CORE_I_FT_INIT_FAILED_STR;
			break;
		}
	case SUEC_CORE_I_OBJECT_NOT_FOUND:
		{
			errMsg = SUEC_CORE_I_OBJECT_NOT_FOUND_STR;
			break;
		}
	case SUEC_CORE_I_CFG_READ_FAILED:
		{
			errMsg = SUEC_CORE_I_CFG_READ_FAILED_STR;
			break;
		}
	case SUEC_CORE_I_PKG_FILE_EXIST:
		{
			errMsg = SUEC_CORE_I_PKG_FILE_EXIST_STR;
			break;
		}
	case  SUEC_CORE_I_DLING:
		{
			errMsg =  SUEC_CORE_I_DLING_STR;
			break;
		}
	case SUEC_CORE_S_DL_START_FAILED:
		{
			errMsg = SUEC_CORE_S_DL_START_FAILED_STR;
			break;
		}
	case SUEC_CORE_S_DP_START_FAILED:
		{
			errMsg = SUEC_CORE_S_DP_START_FAILED_STR;
			break;
		}
	case SUEC_CORE_S_PKG_PATH_ERROR:
		{
			errMsg = SUEC_CORE_S_PKG_PATH_ERROR_STR;
			break;
		}
	case SUEC_CORE_S_PKG_FILE_EXIST:
		{
			errMsg = SUEC_CORE_S_PKG_FILE_EXIST_STR;
			break;
		}
	case SUEC_CORE_S_PKG_FT_DL_ERROR:
		{
			errMsg = SUEC_CORE_S_PKG_FT_DL_ERROR_STR;
			break;
		}
	case SUEC_CORE_S_CALC_MD5_ERROR:
		{
			errMsg = SUEC_CORE_S_CALC_MD5_ERROR_STR;
			break;
		}
	case SUEC_CORE_S_MD5_NOT_MATCH:
		{
			errMsg = SUEC_CORE_S_MD5_NOT_MATCH_STR;
			break;
		}
	case SUEC_CORE_S_UNZIP_ERROR:
		{
			errMsg = SUEC_CORE_S_UNZIP_ERROR_STR;
			break;
		}
	case SUEC_CORE_S_XML_PARSE_ERROR:
		{
			errMsg = SUEC_CORE_S_XML_PARSE_ERROR_STR;
			break;
		}
	case SUEC_CORE_S_OS_NOT_MATCH:
		{
			errMsg = SUEC_CORE_S_OS_NOT_MATCH_STR;
			break;
		}
	case SUEC_CORE_S_ARCH_NOT_MATCH:
		{
			errMsg = SUEC_CORE_S_ARCH_NOT_MATCH_STR;
			break;
		}
	case SUEC_CORE_S_TAGS_NOT_MATCH:
		{
			errMsg = SUEC_CORE_S_TAGS_NOT_MATCH_STR;
			break;
		}
	case SUEC_CORE_S_DPFILE_NOT_EXIST:
		{
			errMsg = SUEC_CORE_S_DPFILE_NOT_EXIST_STR;
			break;
		}
	case SUEC_CORE_S_DP_EXEC_FAILED:
		{
			errMsg = SUEC_CORE_S_DP_EXEC_FAILED_STR;
			break;
		}
	case SUEC_CORE_S_PKG_FILE_NOT_EXIST:
		{
			errMsg = SUEC_CORE_S_PKG_FILE_NOT_EXIST_STR;
			break;
		}
	case SUEC_CORE_S_BS64DESDECODE_ERROR:
		{
			errMsg = SUEC_CORE_S_BS64DESDECODE_ERROR_STR;
			break;
		}
	case  SUEC_CORE_S_DP_WAIT_FAILED:
		{
			errMsg =  SUEC_CORE_S_DP_WAIT_FAILED_STR;
			break;
		}
    case SUEC_CORE_S_CFG_SYSSTARTUP_DP:
        {
            errMsg = SUEC_CORE_S_CFG_SYSSTARTUP_DP_STR;
            break;
        }
    case SUEC_CORE_S_BK_FILE_NOT_FOUND:
        {
            errMsg = SUEC_CORE_S_BK_FILE_NOT_FOUND_STR;
            break;
        }
    case SUEC_CORE_S_DP_ROLLBACK_ERROR:
        {
            errMsg = SUEC_CORE_S_DP_ROLLBACK_ERROR_STR;
            break;
        }
    case SUEC_CORE_S_DP_TIMEOUT:
        {
            errMsg = SUEC_CORE_S_DP_TIMEOUT_STR;
            break;
        }
    case SUEC_CORE_S_RETCHECK_EXEC_ERROR:
        {
            errMsg = SUEC_CORE_S_RETCHECK_EXEC_ERROR_STR;
            break;
        }
    case SUEC_CORE_S_RETCHECK_EXEC_TIMEOUT:
        {
            errMsg = SUEC_CORE_S_RETCHECK_EXEC_TIMEOUT_STR;
            break;
        }
    case SUEC_CORE_S_DL_ABORT:
        {
            errMsg = SUEC_CORE_S_DL_ABORT_STR;
            break;
        }
	default:break;
	}
	return errMsg;
}