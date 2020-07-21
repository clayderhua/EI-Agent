#include "ErrorDef.h"

#define SUEC_SUCCESS_STR                       "Success!"

#define SUEC_I_PARAMETER_ERROR_STR             "Interface parameter error!"

#define SUEC_I_NOT_INIT_STR                    "SUEC library not init!"

#define SUEC_I_ALREADY_INIT_STR                "SUEC library already start"

#define SUEC_I_NOT_START_STR                   "SUEC library not start!"

#define SUEC_I_ALREADY_START_STR               "SUECCore library already start"

#define SUEC_I_TASK_PROCESS_START_FAILED_STR   "Task process startup failed!"

#define SUEC_I_PKG_STATUS_NOT_FIT_STR          "Operation cannot be performed at the current state of the package!"

#define SUEC_I_FT_INIT_FAILED_STR              "FT Library init failed!"

#define SUEC_I_OBJECT_NOT_FOUND_STR            "Package object to be operated has not been found!"

#define SUEC_I_CFG_READ_FAILED_STR                 "Config read failed!"

#define SUEC_I_OUT_MSG_PROCESS_START_FAILED_STR     "Output msg process startup failed!"

#define SUEC_I_MSG_PARSE_FAILED_STR             "Msg parse failed!"

#define SUEC_I_MSG_CANNOT_OUTPUT_STR             "Msg cannot output!"

#define SUEC_I_REQ_DL_TASK_DOING_STR              "A request to download is underway!"

#define SUEC_I_REQ_DL_OUTPUT_ERROR_STR            "Request download command output error!"

#define SUEC_I_REQ_DL_TIMEOUT_STR              "Request download timeout!"

#define SUEC_I_REQ_DL_TASK_NOT_FOUND_STR        "Corresponding request download task not found!"

#define SUEC_I_REQ_DL_TASK_ALRADY_EXIST_STR        "Requested download task already exists!"

#define SUEC_I_REQ_DP_TASK_ALRADY_EXIST_STR        "Requested deploy task already exists!"


#define SUEC_S_DL_START_FAILED_STR                "Package start download failed!"

#define SUEC_S_DP_START_FAILED_STR                "Package start deploy failed!"

#define SUEC_S_PKG_PATH_ERROR_STR                "Package path error!"

#define SUEC_S_PKG_FILE_EXIST_STR                "Package file exist!"

#define SUEC_S_PKG_FT_DL_ERROR_STR               "Package file prepare transfer error!"

#define SUEC_S_CALC_MD5_ERROR_STR                "Calculate MD5 error!"

#define SUEC_S_MD5_NOT_MATCH_STR                 "MD5 not match!"

#define SUEC_S_UNZIP_ERROR_STR                  "Unzip error!"

#define SUEC_S_XML_PARSE_ERROR_STR              "XML parse error!"

#define SUEC_S_OS_NOT_MATCH_STR                "OS not match!"

#define SUEC_S_ARCH_NOT_MATCH_STR              "Arch not match!"

#define SUEC_S_TAGS_NOT_MATCH_STR               "Tags not match!"

#define SUEC_S_DPFILE_NOT_EXIST_STR             "Deploy file not exist!"

#define SUEC_S_DP_EXEC_FAILED_STR                "Deploy exec failed!"

#define SUEC_S_PKG_FILE_NOT_EXIST_STR             "Package file not exist!"

#define SUEC_S_OBJECT_NOT_FOUND_STR             "Package object to be operated has not been found."

#define SUEC_S_PKG_DLING_STR                   "Package downloading!"

#define SUEC_S_BS64DESDECODE_ERROR_STR          "Base64 or DES decode error!"

#define SUEC_S_DP_WAIT_FAILED_STR                   "Deploy wait process exit failed!"

#define SUEC_S_CFG_SYSSTARTUP_DP_STR               "Already config to deployed at system startup!"

#define SUEC_S_BK_FILE_NOT_FOUND_STR               "Package back file not found!"

#define SUEC_S_DP_ROLLBACK_ERROR_STR               "Package deploy rollback error!"

#define SUEC_S_DP_TIMEOUT_STR                      "Package deploy timeout!"

#define SUEC_S_SCHE_OBJ_NOT_EXIST_STR             "Schedule object not exist!"

#define SUEC_S_PKG_DL_TASK_EXIST_STR            "Package download task already exists!"

char * SuecGetErrorMsg(unsigned int errorCode)
{
	char * errMsg = 0;

	switch(errorCode)
	{
	case SUEC_SUCCESS:
		{
			errMsg = SUEC_SUCCESS_STR;
			break;
		}
	case SUEC_I_PARAMETER_ERROR:
		{
			errMsg = SUEC_I_PARAMETER_ERROR_STR;
			break;
		}
	case SUEC_I_NOT_INIT:
		{
			errMsg = SUEC_I_NOT_INIT_STR;
			break;
		}
	case SUEC_I_ALREADY_INIT:
		{
			errMsg = SUEC_I_ALREADY_INIT_STR;
			break;
		}
	case SUEC_I_NOT_START:
		{
			errMsg = SUEC_I_NOT_START_STR;
			break;
		}
	case SUEC_I_ALREADY_START:
		{
			errMsg = SUEC_I_ALREADY_START_STR;
			break;
		}
	case SUEC_I_TASK_PROCESS_START_FAILED:
		{
			errMsg = SUEC_I_TASK_PROCESS_START_FAILED_STR;
			break;
		}
	case SUEC_I_PKG_STATUS_NOT_FIT:
		{
			errMsg = SUEC_I_PKG_STATUS_NOT_FIT_STR;
			break;
		}
	case SUEC_I_FT_INIT_FAILED:
		{
			errMsg = SUEC_I_FT_INIT_FAILED_STR;
			break;
		}
	case SUEC_I_OBJECT_NOT_FOUND:
		{
			errMsg = SUEC_I_OBJECT_NOT_FOUND_STR;
			break;
		}
	case SUEC_I_CFG_READ_FAILED:
		{
			errMsg = SUEC_I_CFG_READ_FAILED_STR;
			break;
		}
	case SUEC_I_OUT_MSG_PROCESS_START_FAILED:
		{
			errMsg = SUEC_I_OUT_MSG_PROCESS_START_FAILED_STR;
			break;
		}
	case SUEC_I_MSG_PARSE_FAILED:
		{
			errMsg = SUEC_I_MSG_PARSE_FAILED_STR;
			break;
		}
	case SUEC_I_MSG_CANNOT_OUTPUT:
		{
			errMsg = SUEC_I_MSG_CANNOT_OUTPUT_STR;
			break;
		}
	case SUEC_I_REQ_DL_TASK_DOING:
		{
			errMsg = SUEC_I_REQ_DL_TASK_DOING_STR;
			break;
		}
	case SUEC_I_REQ_DL_OUTPUT_ERROR:
		{
			errMsg = SUEC_I_REQ_DL_OUTPUT_ERROR_STR;
			break;
		}
	case SUEC_I_REQ_DL_TIMEOUT:
		{
			errMsg = SUEC_I_REQ_DL_TIMEOUT_STR;
			break;
		}
	case SUEC_I_REQ_DL_TASK_NOT_FOUND:
		{
			errMsg = SUEC_I_REQ_DL_TASK_NOT_FOUND_STR;
			break;
		}
	case SUEC_I_REQ_DL_TASK_ALRADY_EXIST:
		{
			errMsg = SUEC_I_REQ_DL_TASK_ALRADY_EXIST_STR;
			break;
		}
	case SUEC_I_REQ_DP_TASK_ALRADY_EXIST:
		{
			errMsg = SUEC_I_REQ_DP_TASK_ALRADY_EXIST_STR;
			break;
		}
	case SUEC_S_DL_START_FAILED:
		{
			errMsg = SUEC_S_DL_START_FAILED_STR;
			break;
		}
	case SUEC_S_DP_START_FAILED:
		{
			errMsg = SUEC_S_DP_START_FAILED_STR;
			break;
		}
	case SUEC_S_PKG_PATH_ERROR:
		{
			errMsg = SUEC_S_PKG_PATH_ERROR_STR;
			break;
		}
	case SUEC_S_PKG_FILE_EXIST:
		{
			errMsg = SUEC_S_PKG_FILE_EXIST_STR;
			break;
		}
	case SUEC_S_PKG_FT_DL_ERROR:
		{
			errMsg = SUEC_S_PKG_FT_DL_ERROR_STR;
			break;
		}
	case SUEC_S_CALC_MD5_ERROR:
		{
			errMsg = SUEC_S_CALC_MD5_ERROR_STR;
			break;
		}
	case SUEC_S_MD5_NOT_MATCH:
		{
			errMsg = SUEC_S_MD5_NOT_MATCH_STR;
			break;
		}
	case SUEC_S_UNZIP_ERROR:
		{
			errMsg = SUEC_S_UNZIP_ERROR_STR;
			break;
		}
	case SUEC_S_XML_PARSE_ERROR:
		{
			errMsg = SUEC_S_XML_PARSE_ERROR_STR;
			break;
		}
	case SUEC_S_OS_NOT_MATCH:
		{
			errMsg = SUEC_S_OS_NOT_MATCH_STR;
			break;
		}
	case SUEC_S_ARCH_NOT_MATCH:
		{
			errMsg = SUEC_S_ARCH_NOT_MATCH_STR;
			break;
		}
	case SUEC_S_TAGS_NOT_MATCH:
		{
			errMsg = SUEC_S_TAGS_NOT_MATCH_STR;
			break;
		}
	case SUEC_S_DPFILE_NOT_EXIST:
		{
			errMsg = SUEC_S_DPFILE_NOT_EXIST_STR ;
			break;
		}
	case SUEC_S_DP_EXEC_FAILED:
		{
			errMsg = SUEC_S_DP_EXEC_FAILED_STR;
			break;
		}
	case SUEC_S_PKG_FILE_NOT_EXIST:
		{
			errMsg = SUEC_S_PKG_FILE_NOT_EXIST_STR;
			break;
		}
	case SUEC_S_OBJECT_NOT_FOUND:
		{
			errMsg = SUEC_S_OBJECT_NOT_FOUND_STR;
			break;
		}
	case SUEC_S_PKG_DLING:
		{
			errMsg = SUEC_S_PKG_DLING_STR;
			break;
		}
	case SUEC_S_BS64DESDECODE_ERROR:
		{
			errMsg = SUEC_S_BS64DESDECODE_ERROR_STR;
			break;
		}
	case SUEC_S_DP_WAIT_FAILED:
		{
			errMsg = SUEC_S_DP_WAIT_FAILED_STR;
			break;
		}
    case SUEC_S_CFG_SYSSTARTUP_DP:
        {
            errMsg = SUEC_S_CFG_SYSSTARTUP_DP_STR;
            break;
        }
    case SUEC_S_BK_FILE_NOT_FOUND:
        {
            errMsg = SUEC_S_BK_FILE_NOT_FOUND_STR;
            break;
        }
    case SUEC_S_DP_ROLLBACK_ERROR:
        {
            errMsg = SUEC_S_DP_ROLLBACK_ERROR_STR;
            break;
        }
    case SUEC_S_DP_TIMEOUT:
        {
            errMsg = SUEC_S_DP_TIMEOUT_STR;
            break;
        }
	case SUEC_S_SCHE_OBJ_NOT_EXIST:
		{
			errMsg = SUEC_S_SCHE_OBJ_NOT_EXIST_STR;
			break;
		}
	case SUEC_S_PKG_DL_TASK_EXIST:
		{
			errMsg = SUEC_S_PKG_DL_TASK_EXIST_STR;
			break;
		}
	default:break;
	}
	return errMsg;
}