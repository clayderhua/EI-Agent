/**
* @file      SUEClient.h 
* @brief     SUE Client interface define
* @author    hailong.dang
* @date      2016/4/12 
* @version   1.0.0 
* @copyright Copyright (c) Advantech Co., Ltd. All Rights Reserved                                                              
*/
#ifndef _SUE_CLIENT_H_
#define _SUE_CLIENT_H_

#include "SUEClientCoreData.h"

/**
*Enumeration values....
*/
typedef enum{
	/** This value...*/
	OPAT_SUSPEND = 1, 
	/** This value...*/
	OPAT_CONTINUE = 2,
}OutputActType;

#ifdef __cplusplus
extern "C" {
#endif

	/**
	* Function brief description
	* @param msg
	* @param msgLen
	* @param userData
	* @return 
	*/
	typedef int (*SUECOutputMsgCB)(char *msg, int msgLen, void * userData);

	/**
	* Function brief description
	* @param pTaskStatusInfo
	* @param userData
	* @return 
	*/
	typedef int (*SUECTaskStatusCB)(PTaskStatusInfo pTaskStatusInfo, void * userData);

	/**
	* Function brief description
	* @param checkHandle
	* @param msg
	* @param msgLen
	* @return 
	*/
	typedef int (*SUECNotifyDPCheckMsgCB)(const void * const checkHandle, char * msg, unsigned int msgLen);

	/**
	* Function brief description
	* @param checkHandle
	* @param notifyDpMsgCB
	* @param isQuit
	* @param userData
	* @return 
	*/
	typedef int (*SUECDeployCheckCB)(const void * const checkHandle, SUECNotifyDPCheckMsgCB notifyDpMsgCB, int * isQuit, void * userData);

	/**
	* Function brief description
	* @param devID
	* @param cfgFile
	* @param tags
	* @return 
	*  ::SUEC_SUCCESS <br>
	*  ::SUEC_I_PARAMETER_ERROR  <br>
	*  ::SUEC_I_ALREADY_INIT     <br>
	*  ::SUEC_I_CFG_READ_FAILED  <br>
	*  ::SUEC_I_FT_INIT_FAILED   <br>
	* @note
	*/
	int SUECInit(char *devID, char *cfgFile, char* tags);

	/**
	* Function brief description
	* @return 
	*  ::SUEC_SUCCESS <br>
	* @note
	*/
	int SUECUninit();

	/**
	* Function brief description 
	* @param rsvParam 
	* @return 
	*  ::SUEC_SUCCESS <br>
	*  ::SUEC_I_NOT_INIT  <br>
	*  ::SUEC_I_ALREADY_START     <br>
	*  ::SUEC_I_OUT_MSG_PROCESS_START_FAILED  <br>
	*  ::SUEC_I_TASK_PROCESS_START_FAILED   <br>
	* @note
	*/
	int SUECStart(void * rsvParam);

	/**
	* Function brief description
	* @return 
	*  ::SUEC_SUCCESS <br>
	*  ::SUEC_I_NOT_INIT <br>
	*  ::SUEC_I_NOT_START  <br>
	* @note
	*/
	int SUECStop();

	/**
	* Function brief description
	* @param outputMsgCB
	* @param userData
	* @return 
	*  ::SUEC_SUCCESS <br>
	*  ::SUEC_I_NOT_INIT  <br>
	*  ::SUEC_I_ALREADY_START     <br>
	* @note
	*/
	int SUECSetOutputMsgCB(SUECOutputMsgCB outputMsgCB, void * userData);

	/**
	* Function brief description
	* @param taskStatusCB
	* @param userData
	* @return 
	*  ::SUEC_SUCCESS <br>
	*  ::SUEC_I_NOT_INIT  <br>
	*  ::SUEC_I_ALREADY_START     <br>
	* @note
	*/
	int SUECSetTaskStatusCB(SUECTaskStatusCB taskStatusCB, void * userData);

	/**
	* Function brief description
	* @param pkgType
	* @param dpCheckCB
	* @param userData
	* @return 
	*  ::SUEC_SUCCESS <br>
	*  ::SUEC_I_PARAMETER_ERROR  <br>
	*  ::SUEC_I_NOT_INIT  <br>
	*  ::SUEC_I_ALREADY_START     <br>
	* @note
	*/
	int SUECSetDpCheckCB(char* pkgType, SUECDeployCheckCB dpCheckCB,void * userData);

	/**
	* Function brief description
	* @param msg
	* @param msgLen
	* @return 
	*  ::SUEC_SUCCESS    <br>
	*  ::SUEC_I_PARAMETER_ERROR  <br>
	*  ::SUEC_I_NOT_INIT     <br>
	*  ::SUEC_I_MSG_PARSE_FAILED     <br>
	* @note
	*/
	int SUECInputMsg(char * msg, int msgLen);

	/**
	* Function brief description
	* @param actType
	* @return 
	*  ::SUEC_SUCCESS    <br>
	*  ::SUEC_I_PARAMETER_ERROR  <br>
	*  ::SUEC_I_NOT_INIT     <br>
	* @note
	*/
	int SUECSetOutputAct(OutputActType actType);

	/**
	* Function brief description
	* @param pkgName
	* @param timeoutMS
	* @return 
	*  ::SUEC_SUCCESS    <br>
	*  ::SUEC_I_PARAMETER_ERROR  <br>
	*  ::SUEC_I_NOT_INIT  <br>
	*  ::SUEC_I_REQ_DL_TASK_ALRADY_EXIST     <br>
	*  ::SUEC_I_MSG_CANNOT_OUTPUT     <br>
	*  ::SUEC_I_REQ_DL_TASK_DOING     <br>
	*  ::SUEC_I_REQ_DL_OUTPUT_ERROR     <br>
	*  ::SUEC_I_REQ_DL_TASK_NOT_FOUND     <br>
	*  ::SUEC_I_REQ_DL_TIMEOUT     <br>
	* @note
	*/
	int SUECRequestDownload(char * pkgName, unsigned int timeoutMS);

	/**
	* Function brief description
	* @param pkgName
	* @return 
	*  ::SUEC_SUCCESS    <br>
	*  ::SUEC_I_PARAMETER_ERROR  <br>
	*  ::SUEC_I_NOT_INIT  <br>
	*  ::SUEC_I_REQ_DP_TASK_ALRADY_EXIST     <br>
	*  ::SUEC_I_OBJECT_NOT_FOUND     <br>
	* @note
	*/
	int SUECRequestDeploy(char * pkgName);

	/**
	* Function brief description
	* @param errCode
	* @return 
	*/
	char * SUECGetErrMsg(int errCode);

	/**
	* Function brief description
	* @param osBuf
	* @param bufLen
	* @return
	* @note
	*/
	int SUEGetSysOS(char * osBuf, int bufLen);

	/**
	* Function brief description
	* @param archBuf
	* @param bufLen
	* @return
	* @note
	*/
	int SUEGetSysArch(char * archBuf, int bufLen);

    /**
    * Function brief description
    * @param pkgName
    * @param timeoutMS
    * @param retryCnt
    * @return
    *  ::SUEC_SUCCESS    <br>
    *  ::SUEC_I_PARAMETER_ERROR  <br>
    *  ::SUEC_I_NOT_INIT  <br>
    *  ::SUEC_I_REQ_DL_TASK_ALRADY_EXIST     <br>
    *  ::SUEC_I_MSG_CANNOT_OUTPUT     <br>
    *  ::SUEC_I_REQ_DL_TASK_DOING     <br>
    *  ::SUEC_I_REQ_DL_OUTPUT_ERROR     <br>
    *  ::SUEC_I_REQ_DL_TASK_NOT_FOUND     <br>
    *  ::SUEC_I_REQ_DL_TIMEOUT     <br>
    * @note
    */
    int SUECRequestDownloadWithNorm(char * pkgName, unsigned int timeoutMS, unsigned int retryCnt);

    /**
    * Function brief description
    * @param pkgName
    * @param retryCnt
    * @param isRollback
    * @return
    *  ::SUEC_SUCCESS    <br>
    *  ::SUEC_I_PARAMETER_ERROR  <br>
    *  ::SUEC_I_NOT_INIT  <br>
    *  ::SUEC_I_REQ_DP_TASK_ALRADY_EXIST     <br>
    *  ::SUEC_I_OBJECT_NOT_FOUND     <br>
    * @note
    */
    int SUECRequestDeployWithNorm(char * pkgName, unsigned int retryCnt, unsigned int isRollback);

#ifdef __cplusplus
};
#endif

#endif
