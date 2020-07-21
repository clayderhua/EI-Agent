/*
 * Copyright (c) 2016 Advantech Co., Ltd. All rights reserved.
 *
 * Project name	FileTransfer
 * File name	FileTransferLib.h
 * @version 	1.0
 * @author 		guolin.huang
 * @date	 	2016/02/24
 */

#ifndef __FILETRANSFERLIB_H__
#define __FILETRANSFERLIB_H__

//#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>


/**
 * This enum define the operation of FTHandle setting. 
 */
typedef enum {
	/** Upload callback set option. */
	FT_OPT_UPLOADCALLBACK, 
	/** Download callback set option. */
	FT_OPT_DOWNLOADCALLBACK,
} FTSetOption;

/**
 * This enum define the protocol which use for file transfer. 
 */
typedef enum {
	/** FTP uses for file transfer. */
	FT_PROTOCOL_FTP = 1,
	/** Azure blob storage for file transfer. */
	FT_PROTOCOL_AZURE,
	/** S3 compatible storage for file transfer. */
	FT_PROTOCOL_S3C,
	/** AWS S3 storage for file transfer. */
	FT_PROTOCOL_S3,
} FTProtocol;

/**
 * This enum define the security level which use for file transfer.
 */
typedef enum {
	/** None security option. */
	FT_SECURITY_NONE = 100,
	/** Implicit SSL/TLS security option. */
	FT_SECURITY_SSL,
	/** Explicit SSL/TLS security option. */
	FT_SECURITY_ESSL,
} FTSecurity;

/**
 *  The FTHandle structure define.  
 */
typedef struct _FTHandle FTHandle;

/**
 *  Define The function prototype of upload & download callback. 
 */
typedef void (* FTTransferCallback)(void *userData, int64_t totalSize, int64_t nowSize, int64_t averageSpeed);


#ifdef __cplusplus
extern "C" {
#endif 

/**
 * This function initialize FileTransferLib. 
 *
 * This function is not thread safe, it must be called just once within a 
 * program before the program calls any other function in FileTransferLib.
 */
int32_t FTGlobalInit(void);

/**
 * This function releases resources acquired by FTGlobalInit().
 *
 * This function is not thread safe also. When the program no longer uses
 * FileTransferLib, it should be call and just once.
 */
void FTGlobalCleanup(void);

/**
 * This function must be call before any other file operation functions, 
 * and it returns a FTHandle pointer that you must use as input to other 
 * file operation functions. 
 *
 * This call MUST have a corresponding call to FTCloseHandle() when the
 * operation is complete.
 */
FTHandle * FTNewHandle(void);

/**
 * This function releases resources acquired by FTNewHandle().
 * 
 * When the program no longer uses the FTHandle create by FTNewHandle()
 * in same thread, it should be call.
 *
 * @param  handle The FTHandle need to close.
 */
void FTCloseHandle(FTHandle * handle);

/**
 * This function uses to set transfer callback and user data to the FTHandle.
 *
 * @param  handle The FTHandle need to set.
 * @param  option The option you want to set.
 * @param  pFunc The function pointer you specified. 
 * @param  pData The data you want to give the callback function.
 * @return  pData The data you want to give the callback function.
 */
int32_t FTSet(FTHandle * handle, FTSetOption option, void *pFunc, void *pData);

/**
 * This function download a single file from the remote file server.
 * Call this function will be block until it's finished or failed.
 *
 * @param handle The FTHandle it need.
 * @param protocol The protocol you want use to transfer file(also see FTProtocol).
 * @param security Set security level you need (also see FTSecurity).
 * @param URL Give the source file's URL.
 * @param locPath The local directory that use to store the remote file.
 * @param isVerbose print debug message of curl or not
 * @return Success will return zero, otherwise fail or abort.
 */
int32_t FTDownload(FTHandle * handle, FTProtocol protocol, FTSecurity security, 
				   const char * URL, const char * locPath, int isVerbose);

/**
 * This function upload a single file to the remote file server.
 * Call this function will be block until it's finished or failed.
 *
 * This function is thread safe when the input FTHandle is new one per thread.
 *
 * @param handle The FTHandle it need.
 * @param protocol The protocol you want use to transfer file(also see FTProtocol).
 * @param security Set security level you need (also see FTSecurity).
 * @param URL Give the remote file path's URL.
 * @param locPath The local file path.
 * @return Success will return zero, otherwise fail or abort.
 */
int32_t FTUpload(FTHandle * handle, FTProtocol protocol, FTSecurity security, 
				 const char * URL, const char * locPath);

/**
 * This function abort an ongoing file transfer task in another thread.
 *
 * @note You must make sure that this function called before the input FTHandle be closed.
 *
 * @param handle The FTHandle uses to the ongoing file transfer task.
 * @return Success will return zero, otherwise no task running need to abort, or FTHandle 
 *	is null.
 */
int32_t FTAbortTransfer(FTHandle * handle);

/**
 * This function returns a string describing the FileTransferLib's error code.
 *
 * @param errCode The FTHandle it need.
 * @return A string describing the error code.
 */
const char * FTErr2Str(int32_t errCode);

#ifdef __cplusplus
}
#endif

#endif /* __FILETRANSFERLIB_H__ */
