/*
 * Copyright (c) 2016 Advantech Co., Ltd. All rights reserved.
 *
 * Project name	FileTransfer
 * File name	FTSample.c
 * @version 	1.0
 * @author 		guolin.huang
 * @date	 	2016/02/24
 */

#include "FileTransferLib.h"
#include "CPCommon.h"
#include "CPThread.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <curl/curl.h>

#define TEST_TARGET	5

#if TEST_TARGET == 5
// aws s3, v4
#	define DOWNLOADFILE		"aws-s3-test.txt"
//#	define URL				"s3-4://AKIAJJ7CBWETWMED3SAA:BpoOKo0C%7B%n%qWXsvWen95goDicm5frJHTyc8T@ap-southeast-1@/ota-unit-test/" DOWNLOADFILE
#	define URL				"s3-4://AKIAJJ7CBWETWMED3SAA:BpoOKo0C%7B%n%qWXsvWen95goDicm5frJHTyc8T@ap-southeast-1@s3.amazonaws.com/ota-unit-test/" DOWNLOADFILE
//#	define URL				"s3-4://AKIAJJ7CBWETWMED3SAA:BpoOKo0C%7B%n%qWXsvWen95goDicm5frJHTyc8T@ap-southeast-1@s3.ap-southeast-1.amazonaws.com/ota-unit-test/" DOWNLOADFILE
#	define PROTOCOL			FT_PROTOCOL_S3
#	define SECURITY			FT_SECURITY_SSL

#elif TEST_TARGET == 4
// aliyun oss, v2
#	define DOWNLOADFILE		"512k.dat"
#	define URL				"s3c://LTAId9bKqZ2gNvT3:1EKAC22QETxx8gX9HFe5VhGgmro1r4@us-east-1@oss-cn-hangzhou.aliyuncs.com/ota-filetransfer-test/" DOWNLOADFILE
#	define PROTOCOL			FT_PROTOCOL_S3C
#	define SECURITY			FT_SECURITY_NONE

#elif TEST_TARGET == 3
// openstack swift, v2
#	define DOWNLOADFILE		"1m.dat"
#	define URL				"s3c://3f068add35344b1a8a6638af6371c976:51e18b119f3643d4b6661f152ff14758@us-east-1@124.9.14.40:8080/ota-bucket/" DOWNLOADFILE
#	define PROTOCOL			FT_PROTOCOL_S3C
#	define SECURITY			FT_SECURITY_NONE

#elif TEST_TARGET == 2
// azure blob 
#	define DOWNLOADFILE		"512k.dat"
#	define URL				"http://KqKS9p0l+nWyAFFDw3K6KVyZOEwXxSqxc3MiDmjSZXKc6ntT76kjvWlpuIVd10JXNyYZBLDKnoyMSvSEwy4ddQ==@eipaasarchive.blob.core.windows.net/ota-container/" DOWNLOADFILE
#	define PROTOCOL			FT_PROTOCOL_AZURE
#	define SECURITY			FT_SECURITY_SSL

#else 
// ftp 
#	define DOWNLOADFILE		"100m.dat"
#	define URL				"ftp://test:123456@172.21.84.152/download/" DOWNLOADFILE
#	define PROTOCOL			FT_PROTOCOL_FTP
#	define SECURITY			FT_SECURITY_SSL
#endif 

#ifdef WIN32
#	define LOCALPATH		"D:\\ftptest\\download\\"
#else 
#	define LOCALPATH		"/home/ota/Downloads/"
#endif 


void downloadCallback(void *userData, int64_t totalSize, int64_t nowSize, int64_t averageSpeed)
{
#ifdef WIN32
	fprintf(stdout, "%s > totalSize: %I64d, nowSize: %I64d, averageSpeed: %I64d\n", 
		userData, totalSize, nowSize, averageSpeed);
#else
	fprintf(stdout, "%s > totalSize: %lld, nowSize: %lld, averageSpeed: %lld\n", 
		userData, totalSize, nowSize, averageSpeed);
#endif 
}

CP_PTHREAD_ENTRY(AbortThread, arg_name)
{
	int32_t res;
	FTHandle * handle = arg_name;

	fprintf(stderr, "AbortThread start!\n");

	while (!cp_is_file_exist(LOCALPATH DOWNLOADFILE)) {
		cp_sleep(100);
	}

	cp_sleep(100);
	//printf("Abort in thread!(arg_name=%d)\n", arg_name);
	res = FTAbortTransfer(handle);
	fprintf(stderr, "AbortThread-FTAbortTransfer() told us %s(%d)\n",  FTErr2Str(res), res);

	cp_sleep(1000);

	/* Resume the task */
	handle = NULL;
	handle = FTNewHandle(); // new one 
	if (handle) {
		res = FTSet(handle, FT_OPT_DOWNLOADCALLBACK, downloadCallback, "Resume"DOWNLOADFILE);
		fprintf(stderr, "AbortThread-FTSet() told us %s(%d)\n",  FTErr2Str(res), res);

		res = FTDownload(handle, PROTOCOL, SECURITY, URL, LOCALPATH);
		fprintf(stderr, "AbortThread-FTDownload() told us %s(%d)\n",  FTErr2Str(res), res);

        res = FTAbortTransfer(handle);
        fprintf(stderr, "AbortThread-FTAbortTransfer() told us %s(%d)\n", FTErr2Str(res), res);

        FTCloseHandle(handle);
        handle = NULL;
	}

    fprintf(stderr, "AbortThread exit!\n");
    return 0;
}



int main(void)
{
	int32_t res;
	const char * locPath = LOCALPATH;
	
	char taskInfo[64] = {0};
	FTHandle * handle = NULL;
	CP_THREAD_T thread;

	/* Global init, you must call this just once! */
	FTGlobalInit();

	/* New a FTHandle to use */
	handle = FTNewHandle();
	if(handle) {
		/* Set callback */
		res = FTSet(handle, FT_OPT_DOWNLOADCALLBACK, downloadCallback, taskInfo);
		fprintf(stderr, "FTSet() told us %s\n",  FTErr2Str(res));

		//cp_thread_create(&thread, AbortThread, handle);

		/* Download file */
		fprintf(stdout, "FTDownload() start!\n");
		strcpy(taskInfo, DOWNLOADFILE);
		res = FTDownload(handle, PROTOCOL, SECURITY, URL, locPath);
		fprintf(stderr, "FTDownload() told us %s(%d)\n",  FTErr2Str(res), res);

		//fprintf(stdout, "Thread join start!\n");
		//cp_thread_join(thread);
		//fprintf(stdout, "Thread join complete!\n");

		/* Close the FTHandle if you don't use it any more. */
		FTCloseHandle(handle);
		fprintf(stdout, "closed 1st handle!\n");
	}


	fprintf(stdout, "All done! Press any key to exit!\n");
	getchar();
	/* 
	 * Global cleanup, you must call this just once! 
	 * And make sure now don't need FileTransferLib any more!
	 */
	FTGlobalCleanup();

	return 0;
}
