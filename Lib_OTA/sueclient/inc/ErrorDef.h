/**
* @file      ErrorDef.h 
* @brief     SUE Client library error define
* @author    hailong.dang
* @date      2016/4/12 
* @version   1.0.0 
* @copyright Copyright (c) Advantech Co., Ltd. All Rights Reserved                                                              
*/
#ifndef _ERROR_DEF_H_
#define _ERROR_DEF_H_

/**
 * Success code: No error,indicates successful completion of an SUE client operation.
 */
#define SUEC_SUCCESS                   0

/**
 * Error code: Interface parameter error!
 */
#define SUEC_I_PARAMETER_ERROR      2001

/**
 * Error code: SUEC library not init!
 */
#define SUEC_I_NOT_INIT                     2002

/**
 * Error code: SUEC library already start!
 */
#define SUEC_I_ALREADY_INIT                 2003

/**
 * Error code: SUEC library not start!
 */
#define SUEC_I_NOT_START                    2004

/**
 * Error code: SUEC library already start!
 */
#define SUEC_I_ALREADY_START                 2005

/**
 * Error code: Task process startup failed!
 */
#define SUEC_I_TASK_PROCESS_START_FAILED     2006

/**
 * Error code: Task status not checked!
 */
//#define SUEC_I_PKG_TASK_STATUS_UNKNOW       2007

/**
 * Error code: Package task already exist!
 */
//#define SUEC_I_PKG_TASK_EXIST           2008

/**
 * Error code: Operation cannot be performed at the current state of the package!!
 */
#define SUEC_I_PKG_STATUS_NOT_FIT            2009

/**
 * Error code: Package suspend failed!
 */
//#define SUEC_I_PKG_SUSPEND_FAILED            2010

/**
 * Error code: Package is not suspend!
 */
//#define SUEC_I_PKG_NOT_SUSPEND               2011

/**
 * Error code: Package resume failed!
 */
//#define SUEC_I_PKG_RESUME_FAILED             2012

/**
 * Error code: FT init failed!
 */
#define SUEC_I_FT_INIT_FAILED                 2013

/**
 * Error code: Package object to be operated has not been found!
 */
#define SUEC_I_OBJECT_NOT_FOUND              2014

/**
 * Error code: Config read failed!
 */
#define SUEC_I_CFG_READ_FAILED              2015

/**
 * Error code: Output msg process startup failed!
 */
#define SUEC_I_OUT_MSG_PROCESS_START_FAILED     2100

/**
 * Error code: Msg parse failed!
 */
#define SUEC_I_MSG_PARSE_FAILED             2101

/**
 * Error code: Msg cannot output!
 */
#define SUEC_I_MSG_CANNOT_OUTPUT             2102

/**
 * Error code: A request to download is underway!
 */
#define SUEC_I_REQ_DL_TASK_DOING             2103

/**
 * Error code: Request download command output error!
 */
#define SUEC_I_REQ_DL_OUTPUT_ERROR           2104

/**
 * Error code: Request download timeout!
 */
#define SUEC_I_REQ_DL_TIMEOUT              2105

/**
 * Error code: Corresponding request download task not found!
 */
#define SUEC_I_REQ_DL_TASK_NOT_FOUND        2106

/**
 * Error code: Requested download task already exists!
 */
#define SUEC_I_REQ_DL_TASK_ALRADY_EXIST        2107

/**
 * Error code: Requested deploy task already exists!
 */
#define SUEC_I_REQ_DP_TASK_ALRADY_EXIST        2108

/**
 * Error code: Package start download failed!
 */
#define SUEC_S_DL_START_FAILED                2501

/**
 * Error code: Package start deploy failed!
 */
#define SUEC_S_DP_START_FAILED                2502

/**
 * Error code: Package path error!
 */
#define SUEC_S_PKG_PATH_ERROR                2503

/**
 * Error code: Package file exist!
 */
#define SUEC_S_PKG_FILE_EXIST                  2504

/**
 * Error code: Package file transfer error!
 */
#define SUEC_S_PKG_FT_DL_ERROR               2505

/**
 * Error code: Calculate MD5 error!
 */
#define SUEC_S_CALC_MD5_ERROR                2506

/**
 * Error code: MD5 not match!
 */
#define SUEC_S_MD5_NOT_MATCH                 2507

/**
 * Error code: Unzip error!
 */
#define SUEC_S_UNZIP_ERROR                  2508

/**
 * Error code: XML parse error!
 */
#define SUEC_S_XML_PARSE_ERROR               2509

/**
 * Error code: OS not match!
 */
#define SUEC_S_OS_NOT_MATCH                 2510

/**
 * Error code: Arch not match!
 */
#define SUEC_S_ARCH_NOT_MATCH                 2511

/**
 * Error code: Deploy file not exist!
 */
#define SUEC_S_DPFILE_NOT_EXIST             2512

/**
 * Error code: Deploy exec failed!
 */
#define SUEC_S_DP_EXEC_FAILED                2513

/**
 * Error code: Package file not exist!
 */
#define SUEC_S_PKG_FILE_NOT_EXIST             2514

/**
 * Error code: Base64 or DES decode error!
 */
#define SUEC_S_BS64DESDECODE_ERROR          2515

/**
 * Error code: Deploy wait process exit failed!
 */
#define SUEC_S_DP_WAIT_FAILED                   2516

/**
* Error code: Already config system startup deploy!
*/
#define SUEC_S_CFG_SYSSTARTUP_DP                2517

/**
* Error code: Package back file not found!
*/
#define SUEC_S_BK_FILE_NOT_FOUND                2518

/**
* Error code: Package deploy rollback error!
*/
#define SUEC_S_DP_ROLLBACK_ERROR                2519

/**
* Error code: Package deploy timeout!
*/
#define SUEC_S_DP_TIMEOUT                       2520

/**
 * Error code: Tags not match!
 */
#define SUEC_S_TAGS_NOT_MATCH                   2524

/**
 * Error code: Package object to be operated has not been found.
 */
#define SUEC_S_OBJECT_NOT_FOUND            2601

/**
 * Error code: Package downloading!
 */
#define SUEC_S_PKG_DLING              2602

/**
 * Error code: Schedule time string format error!
 */
#define SUEC_S_SCHE_TIME_FORMAT_ERROR         2603

/**
 * Error code: Schedule startup error!
 */
#define SUEC_S_SCHE_STARTUP_ERROR            2604

/**
 * Error code: Schedule time error!
 */
#define SUEC_S_SCHE_TIME_ERROR            2605

/**
 * Error code: Schedule startup error!
 */
#define SUEC_S_SCHE_ALREADY_EXIST            2606

/**
 * Error code: Schedule type error!
 */
#define SUEC_S_SCHE_TYPE_ERROR             2607

/**
 * Error code: Schedule object not exist!
 */
#define SUEC_S_SCHE_OBJ_NOT_EXIST             2608

/**
 * Error code: Package download task already exists!
 */
#define SUEC_S_PKG_DL_TASK_EXIST              2609

#endif
