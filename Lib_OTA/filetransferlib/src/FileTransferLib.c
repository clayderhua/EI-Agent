/*
 * Copyright (c) 2016-2017 Advantech Co., Ltd.
 * All rights reserved.
 *
 * Project name	FileTransfer
 * File name	FileTransferLib.c
 * @version 	2.0
 * @author 		guolin.huang
 * @create	 	2016/02/24
 * @update      2017/09/18
 */

#include "FileTransferLib.h"
#include "Util.h"
#ifndef ANDROID
#include "base64.h"
#endif
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include <curl/curl.h>



/*======================= Macro define ========================*/
#define _FT_DEBUG

#ifdef _FT_DEBUG
#	define debug(...) fprintf (stderr, __VA_ARGS__)
#else
#	define debug(...)
#endif /* _FT_DEBUG */

#define FT_CALLBACK_INTERVAL    0.5
#define FT_FILESIZE_UNKNOWN     -1L

#define AWS_S3_DEFAULT_HOST		"s3.amazonaws.com"
/*==============================================================*/


/*======================= Type define =========================*/
typedef enum {
	FT_ERR_OK = 0,
	FT_ERR_NULLHANDLE = -1,
	FT_ERR_BUSY = -2,
	FT_ERR_UNKNOWNOPTION = -3,
	FT_ERR_NONEEDABORT = -4,
} FTErrCode;

typedef enum {
    FT_OP_UPLOAD = 0,
    FT_OP_DOWNLOAD,
} FTOperation;

typedef struct {
	FTTransferCallback uploadCallback;
    FTTransferCallback downloadCallback;
} FTCallback_st;

typedef struct {
	void * uploadUserData;
	void * downloadUserData;
} FTCallbackUserData_st;

typedef struct {
    pthread_mutex_t lock;
    bool data;
}LockData_st;

typedef struct {
    FTOperation operation;
    FTProtocol protocol;
    FTSecurity security;
    const char *url;
    const char *locPath;
    curl_off_t resumeAt;
    curl_off_t needTransferSize;
    double lastCallbackTime;
    bool isDone;
} OneTimeTaskData_st;

struct _FTHandle {
    CURL *curl;
    LockData_st isBusy;
    LockData_st isAbort;
    FTCallback_st *callback;
	FTCallbackUserData_st *userData;
    OneTimeTaskData_st * taskData;
};
/*==============================================================*/



/*=============== Local funcation declaration ==================*/
static char* makeLocalFilePath(const char* URL, const char* locPath);
static char* makeTargetUrl(FTProtocol protocol, FTSecurity security, const char* URL);
static void setFTPSecurity(CURL * curl, FTSecurity security);
static void setHeader4Azure(CURL * curl, const char * URL);
static bool onlyVirtualHostedStyle(const char *URL);
static char * locateTheChar(const char *str, char ch, int nth);
static char * strRepalce(char *str, char old, char new);
static void replace_string(char *result, char *source, char* s1, char *s2);
static void escape_string(char * string, char * string_copy);
/*==============================================================*/




int32_t FTGlobalInit(void)
{
#ifdef WIN32
    return curl_global_init(CURL_GLOBAL_DEFAULT);
#else // Linux
    return curl_global_init(CURL_GLOBAL_SSL);
#endif
}

void FTGlobalCleanup(void)
{
    curl_global_cleanup();
}

FTHandle * FTNewHandle()
{
    CURL *curl;
    FTHandle *pHandle = NULL;

    curl = curl_easy_init();
    if (curl) {
        if ( (pHandle = malloc(sizeof(*pHandle))) != NULL ) {
            memset(pHandle, 0, sizeof (*pHandle));

            /* Init this FTHandle */
            pHandle->curl = curl;
			pHandle->isBusy.data = false;
            pthread_mutex_init(&(pHandle->isBusy.lock), NULL);
            pHandle->isAbort.data = false;
            pthread_mutex_init(&(pHandle->isAbort.lock), NULL);
            pHandle->callback = NULL;
			pHandle->userData = NULL;
            pHandle->taskData = NULL;
        }
    }

    return pHandle;
}

// Use this function carefully
static void WaitHandleToFreeState(FTHandle * handle) {

	if (handle) {
		pthread_mutex_t *isBusyLock = &(handle->isBusy.lock);

		bool isBusy = true;
		while(isBusy) {
			usleep(1000*100);

			// Get state
			pthread_mutex_lock(isBusyLock);
			isBusy = handle->isBusy.data;
			pthread_mutex_unlock(isBusyLock);
		}
	}
}

void  FTCloseHandle(FTHandle * handle)
{
    /* Release all FTHandle's resources */
    if (handle) {
		pthread_mutex_t *isBusyLock = &(handle->isBusy.lock);
		pthread_mutex_t *isAbortLock = &(handle->isAbort.lock);

        if (handle->curl) {
			bool isBusy = false;

			// Get state
			pthread_mutex_lock(isBusyLock);
			isBusy = handle->isBusy.data;
			pthread_mutex_unlock(isBusyLock);

			if (isBusy) {
				// Abort task!
				FTAbortTransfer(handle);

				// Need wait handle to free state(not busy)!
				WaitHandleToFreeState(handle);
			}

            curl_easy_cleanup(handle->curl);
        }
		pthread_mutex_destroy(isBusyLock);
        pthread_mutex_destroy(isAbortLock);
		if (handle->callback) {
			free(handle->callback);
		}
		if (handle->userData) {
			free(handle->userData);
		}
        /* pHandle->taskData is a stack resource, doesn't need free() */

        free(handle);
    }
}

int32_t FTSet(FTHandle * handle, FTSetOption option, void *pFunc, void *pData)
{
	int32_t res = FT_ERR_OK;

	if (!handle)  return FT_ERR_NULLHANDLE;

    pthread_mutex_lock(&(handle->isBusy.lock));
	if (handle->isBusy.data)  return FT_ERR_BUSY;
    pthread_mutex_unlock(&(handle->isBusy.lock));

	if (!handle->callback) {
		handle->callback = malloc(sizeof(*(handle->callback)) );
		memset(handle->callback, 0, sizeof (*(handle->callback)) );
	}
	if (!handle->userData) {
		handle->userData = malloc(sizeof(*(handle->userData)) );
		memset(handle->userData, 0, sizeof (*(handle->userData)) );
	}


	switch (option) {
	case FT_OPT_UPLOADCALLBACK:
		handle->callback->uploadCallback = pFunc;
		handle->userData->uploadUserData = pData;
		break;

	case FT_OPT_DOWNLOADCALLBACK:
		handle->callback->downloadCallback = pFunc;
		handle->userData->downloadUserData = pData;
		break;

	default:
		res = FT_ERR_UNKNOWNOPTION;
		break;
	}

	return res;
}

struct FTFile {
    const char *locPath;
	bool resume;
    FILE *stream;
};

static size_t ft_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
    struct FTFile *out = (struct FTFile *)stream;
    if (out && !out->stream) {

		/* open file for writing */
		if (out->resume)
        	out->stream = fopen(out->locPath, "ab");
		else
			out->stream = fopen(out->locPath, "wb");

        if (!out->stream)
            return -1; /* failure, can't open file to write */
    }
    return fwrite(buffer, size, nmemb, out->stream);
}

static int xferinfo(void *p,
    curl_off_t dltotal, curl_off_t dlnow,
    curl_off_t ultotal, curl_off_t ulnow)
{
    FTHandle *handle = (FTHandle *)p;
	OneTimeTaskData_st *taskData = handle->taskData;
	FTTransferCallback userCallback = NULL;
	void * userData = NULL;
    double interval = 0;
    double curtime = 0;
    double speed = 0;

	//debug("Callback =======start=======.\n");

    pthread_mutex_lock(&(handle->isAbort.lock));
	if (handle->isAbort.data) {
		//debug("Callback ------abort------!\n");

    	pthread_mutex_unlock(&(handle->isAbort.lock));
		/* Abort now! */
		return 1;
	}
    pthread_mutex_unlock(&(handle->isAbort.lock));

	/* Callback interval get */
    curl_easy_getinfo(handle->curl, CURLINFO_TOTAL_TIME, &curtime);
    interval = curtime - taskData->lastCallbackTime;
    if (interval < 0) {
        taskData->lastCallbackTime = curtime;
    }

    switch (taskData->operation) {
    case FT_OP_UPLOAD:
		if (handle->callback) {
			userCallback = handle->callback->uploadCallback;
			if (handle->userData) userData = handle->userData->uploadUserData;
		}

        if ((interval >= FT_CALLBACK_INTERVAL) ||
            (taskData->needTransferSize == ulnow)) {
            curl_easy_getinfo(handle->curl, CURLINFO_SPEED_UPLOAD, &speed);
            if (!taskData->isDone) {
                taskData->lastCallbackTime = curtime;

				debug("Debug-upload:"
					" ulnow=%" CURL_FORMAT_CURL_OFF_T " of"
					" ultotal=%" CURL_FORMAT_CURL_OFF_T
					" resumeAt=%" CURL_FORMAT_CURL_OFF_T
					" callback interval=%f\r\n",
					ulnow, ultotal, taskData->resumeAt, interval);

                /* User upload callback function */
				if (userCallback) {
					userCallback(userData,
						(taskData->needTransferSize),
						(ulnow + taskData->resumeAt), (int64_t)speed);
				}

                if (taskData->needTransferSize == ulnow) {
                    taskData->isDone = true;
                }
            }
        }
        break;

    case FT_OP_DOWNLOAD:
		if (handle->callback) {
			userCallback = handle->callback->downloadCallback;
			if (handle->userData) userData = handle->userData->downloadUserData;
		}

        if ((FT_FILESIZE_UNKNOWN == taskData->needTransferSize) &&
            (dltotal > 0)) {
            curl_off_t willTransferSize = dltotal;
            if (taskData->resumeAt > 0 && (willTransferSize > taskData->resumeAt)) {
                willTransferSize -= taskData->resumeAt;
            }
            taskData->needTransferSize = willTransferSize;
        }

        if ((FT_FILESIZE_UNKNOWN != taskData->needTransferSize) &&
            (interval >= FT_CALLBACK_INTERVAL ||
                taskData->needTransferSize == dlnow) ) {
            curl_easy_getinfo(handle->curl, CURLINFO_SPEED_DOWNLOAD, &speed);
            if (!taskData->isDone) {
                taskData->lastCallbackTime = curtime;

				debug("Debug-download:"
					" dlnow=%" CURL_FORMAT_CURL_OFF_T " of"
					" dltotal=%" CURL_FORMAT_CURL_OFF_T
					" resumeAt=%" CURL_FORMAT_CURL_OFF_T
					" callback interval=%f\r\n",
					dlnow, dltotal, taskData->resumeAt, interval);

                /* User download callback function */
				if (userCallback) {
					(userCallback)(userData,
						(taskData->needTransferSize + taskData->resumeAt),
						(dlnow + taskData->resumeAt), (int64_t)speed);
				}


                if (taskData->needTransferSize == dlnow) {
                    taskData->isDone = true;
                }
            }
        }
        break;
    }

	//debug("Callback ======end======.\n");

    return 0;
}

static void setFTPSecurity(CURL * curl, FTSecurity security)
{
	if (FT_SECURITY_NONE != security) {
		debug("Enable SSL.\n");
		debug("SSL VERSION: %s\n", curl_version_info(CURL_VERSION_SSL)->ssl_version);

		/* Disable SSL CA verify */
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		/* We activate SSL and we require it for both control and data */
		curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	}
}

static char* makeLocalFilePath(const char* URL, const char* locPath)
{
	char * splitName = strrchr(URL, '/') + 1;
	char tmpFName[256];
	char * fileName = splitName;
	if (strchr(splitName, '?')){
		memset(tmpFName, 0, sizeof(char) * 256);
		strcpy(tmpFName, splitName);
		char* p = strtok(tmpFName, "?");
		fileName = p;
	}

	char * filePath = malloc(strlen(locPath) + strlen(fileName) + 2); // add '\\' + '\0' size
#ifdef WIN32
	char pathSeparator = '\\';
#else
	char pathSeparator = '/'; // Linux
#endif
	sprintf(filePath, "%s%c%s", locPath, pathSeparator, fileName);
	return filePath;
}

// only use for S3C, service list:
// aliyun oss, ref: https://help.aliyun.com/document_detail/64919.html
static bool onlyVirtualHostedStyle(const char *URL)
{
#define ONLY_VIRTUAL_HOST_ALIYUN_FLAG	"oss"

	// url format: s3c-${signerType}://${accessKey}:${secretKey}@${region}@${endpoint}/${bucket}/path/to/file
	char host[256] = {0};
	const char *start = locateTheChar(URL, '@', 1) + 1;
	const char *end = locateTheChar(URL, '/', 3);
	strncpy(host, start, end - start);
	return strstr(host, ONLY_VIRTUAL_HOST_ALIYUN_FLAG) != NULL;
}

static char* makeTargetUrl(FTProtocol protocol, FTSecurity security, const char* URL)
{
	char * targetURL = malloc(strlen(URL) + 2);
	const char *p = NULL;

	switch (protocol) {

	case FT_PROTOCOL_FTP:
		// url format: ftp(s)://user:password@host:port/path/to/file
		if (FT_SECURITY_SSL == security)
			sprintf(targetURL, "ftp%s", URL + strlen("ftp"));
		else
			sprintf(targetURL, "%s", URL);
		break;

	case FT_PROTOCOL_AZURE:
		if (strstr(URL, "azure:"))
		{
			// url format: azure://${accessKey}@${account}.blob.core.windows.net/${container}/path/to/file
			p = strrchr(URL, '@') + 1;
			if (FT_SECURITY_NONE == security)
				sprintf(targetURL, "http://%s", p);
			else
				sprintf(targetURL, "https://%s", p);
		}
		else
		{
			//For SASURL: https://eipaasarchive.blob.core.windows.net/adv-ota-stg-112961855/CreateDir-v1.0.0.2-a9d6612910a37dee7be162ca5bd985ff.zip?st=2018-08-10T05%3A11%3A21Z&se=2019-02-02T05%3A11%3A00Z&sp=rl&sv=2018-03-28&sr=c&sig=xs4z3SkOGJutYfc9AuRW33ujEqicSwJY%2BfKv1s6dmgs%3D
			sprintf(targetURL, "%s", URL);
		}

		break;

	case FT_PROTOCOL_S3C:
		// url format: s3c-${signerType}://${accessKey}:${secretKey}@${region}@${endpoint}/${bucket}/path/to/file
		p = locateTheChar(URL, '@', 2) + 1;
		if (onlyVirtualHostedStyle(URL)) {
			char host[64] = {0};
			strncpy(host, p, locateTheChar(URL, '/', 3) - p);
			p = locateTheChar(URL, '/', 4);
			if (FT_SECURITY_NONE == security)
				sprintf(targetURL, "http://%s%s", host, p);
			else
				sprintf(targetURL, "https://%s%s", host, p);
		} else {
			if (FT_SECURITY_NONE == security)
				sprintf(targetURL, "http://%s", p);
			else
				sprintf(targetURL, "https://%s", p);
		}
		break;

	case FT_PROTOCOL_S3:
		// url format: s3-v4://${accessKey}:${secretKey}@${region}@${endpoint}/${bucket}/path/to/file
		p = locateTheChar(URL, '@', 2) + 1;
		if (p == locateTheChar(URL, '/', 3)) {
			// endpoint is null
			if (FT_SECURITY_NONE == security)
				sprintf(targetURL, "http://%s%s", AWS_S3_DEFAULT_HOST, p);
			else
				sprintf(targetURL, "https://%s%s", AWS_S3_DEFAULT_HOST, p);
		} else {
			// has endpoint
			if (FT_SECURITY_NONE == security)
				sprintf(targetURL, "http://%s", p);
			else
				sprintf(targetURL, "https://%s", p);
		}
		break;

	default:
		debug("Unknown protocol type: %d", protocol);
		break;
	}
	debug("url: %.4s\n", targetURL); // print protocol of url
	return targetURL;
}

static char* locateTheChar(const char *str, char ch, int nth)
{
	char * p = (char*) str;
	nth = nth > 0 ? nth : 1; // invaild 'nth' will auto set to 1
	while (p != NULL && nth > 0) {
		/*
		 * p+1 avoid continuous.
		 * e.g:
		 *  locate the second "/", and give the 'str': "//"
		 */
		p = strchr(p+1, ch);
		nth--;
	}
	return p;
}

static void setHeader4Azure(CURL * curl, const char * URL)
{
	// azure signature ref:
	// https://docs.microsoft.com/en-us/rest/api/storageservices/authentication-for-the-azure-storage-services
	struct curl_slist *chunk = NULL;

	time_t rawtime;
	struct tm *timeSt;
	char timeBuf[64] = {0};
	char dateBuf[128] = {0};
	char versionBuf[] = "x-ms-version:2014-02-14";
	char *p = NULL;
	char key[256] = {0};
	char key_copy[256] = {0};
	char name[64] = {0};
	char name_copy[64] = {0};
	char restURL[256] = {0};
	char canonicalizedHeaders[256] = {0};
	char canonicalizedResource[256] = {0};
	uint32_t len = 0;
	char stringToSign[512] = {0};
	char auth[128] = {0};
	char authBuf[128] = {0};

	// date header
	time(&rawtime);
	timeSt = gmtime(&rawtime);
	strftime(timeBuf, sizeof(timeBuf), "%a, %d %b %Y %H:%M:%S GMT", timeSt);
	sprintf(dateBuf, "x-ms-date:%s", timeBuf);
	chunk = curl_slist_append(chunk, dateBuf);
	debug("azure date header: %s\n", dateBuf);

	// version header
	chunk = curl_slist_append(chunk, versionBuf);
	debug("azure version header: %s\n", versionBuf);

	// authorization header
	// url format: azure://accessKey@account.blob.core.windows.net/container/path/to/file
	p = locateTheChar(URL, '/', 2)+1;
	strncpy(key, p, strchr(URL, '@')-p);
	escape_string(key, key_copy);
	//strRepalce(key, '%', '/');
	debug("azure account key: %s\n", key);

	p = strchr(URL, '@')+1;
	strncpy(name, p, strchr(URL, '.')-p);
	escape_string(name, name_copy);
	debug("azure account name: %s\n", name);

	p = locateTheChar(URL, '/', 3)+1;
	strcpy(restURL, p);
	sprintf(canonicalizedHeaders, "%s\n%s", dateBuf, versionBuf);
	sprintf(canonicalizedResource, "/%s/%s", name, restURL);
	sprintf(stringToSign, "GET\n\n\n\n\n\n\n\n\n\n\n\n%s\n%s",
			canonicalizedHeaders, canonicalizedResource);
	debug("azure string to sign: %s\n", stringToSign);

	Base64Decode(key, strlen(key), &p, (int*) &len);
	HMAC_SHA256(p, len,
			stringToSign, strlen(stringToSign),
			(unsigned char*) auth, &len);
	free(p);
	Base64Encode(auth, len, &p, (int*) &len);
	memset(auth, 0, sizeof(auth));
	memcpy(auth, p, len);
	free(p);
	sprintf(authBuf, "Authorization: SharedKey %s:%s", name, auth);
	debug("azure auth: %s\n", authBuf);

	curl_slist_append(chunk, authBuf);

	/* set our custom set of headers */
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
}

static char * strRepalce(char *str, char old, char new)
{
	if (str) {
		char *p = str;
		while (*p) {
			if (*p == old) *p = new;
			p++;
		}
	}
	return str;
}

static void replace_string(char *result, char *source, char *s1, char *s2)
{
	char *q = NULL;
	char *p = NULL;

	p = source;
	while ((q = strstr(p, s1)) != NULL)
	{
		strncpy(result, p, q - p);
		result[q - p] = '\0';//very important, must attention!
		strcat(result, s2);
		strcat(result, q + strlen(s1));
		strcpy(p, result);
	}
	strcpy(result, p);
}

static void escape_string(char *string,char *string_copy)
{
	if (strstr(string, "%2F"))replace_string(string_copy, string, "%2F", "/");
	if (strchr(string, '%'))strRepalce(string, '%', '/');
}

static void setHeader4S3C(CURL * curl, const char * URL)
{
	// s3c v2 signature ref:
	// https://docs.aws.amazon.com/zh_cn/AmazonS3/latest/dev/RESTAuthentication.html

	struct curl_slist *chunk = NULL;

	time_t rawtime;
	struct tm *timeSt;
	char timeBuf[64] = {0};
	char dateBuf[128] = {0};
	char *p = NULL;
	char accessKey[256] = {0};
	char accessKey_copy[256] = {0};
	char secretKey[256] = {0};
	char secretKey_copy[256] = {0};
	char restURL[256] = {0};
	uint32_t len = 0;
	char stringToSign[512] = {0};
	char auth[128] = {0};
	char authBuf[128] = {0};
	char hostBuf[256] = {0};

	// date header
	time(&rawtime);
	timeSt = gmtime(&rawtime);
	strftime(timeBuf, sizeof(timeBuf), "%a, %d %b %Y %H:%M:%S GMT", timeSt);
	sprintf(dateBuf, "Date:%s", timeBuf);
	chunk = curl_slist_append(chunk, dateBuf);
	debug("s3c date header: %s\n", dateBuf);

	// authorization header
	// url format: s3c-${signerType}://${accessKey}:${secretKey}@${region}@${endpoint}/${bucket}/path/to/file
	p = locateTheChar(URL, '/', 2)+1;
	strncpy(accessKey, p, locateTheChar(URL, ':', 2)-p);
	escape_string(accessKey, accessKey_copy);
	debug("s3c access key: %s\n", accessKey);

	p = locateTheChar(URL, ':', 2)+1;
	strncpy(secretKey, p, strchr(URL, '@')-p);
	escape_string(secretKey, secretKey_copy);
	debug("s3c secret key: %s\n", secretKey);

	p = locateTheChar(URL, '/', 3);
	strcpy(restURL, p);
	sprintf(stringToSign, "GET\n\n\n%s\n%s", timeBuf, restURL);
	debug("s3c string to sign: %s\n", stringToSign);

	HMAC_SHA1(secretKey, strlen(secretKey),
			stringToSign, strlen(stringToSign),
			(unsigned char*) auth, &len);
	Base64Encode(auth, len, &p, (int*) &len);
	memset(auth, 0, sizeof(auth));
	memcpy(auth, p, len);
	free(p);
	sprintf(authBuf, "Authorization: AWS %s:%s", accessKey, auth);
	debug("s3c auth: %s\n", authBuf);

	curl_slist_append(chunk, authBuf);

	// Virtual host style need Host header
	if (onlyVirtualHostedStyle(URL)) {
		char bucket[64] = {0};
		char host[256] = {0};

		// get bucket name
		p = locateTheChar(URL, '/', 3) + 1;
		strncpy(bucket, p, locateTheChar(URL, '/', 4) - p);

		// get host name
		p = locateTheChar(URL, '@', 2) + 1;
		strncpy(host, p, locateTheChar(URL, '/', 3) - p);

		// generate Host header
		sprintf(hostBuf, "Host: %s.%s", bucket, host);
		debug("s3c host: %s\n", hostBuf);
		curl_slist_append(chunk, hostBuf);
	}

	/* set our custom set of headers */
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
}

static void setAwsV4Header(
		char *file, char *bucket,
		char *accessKey, char *secretKey,
		char *host, char *region,
		CURL *curl)
{
	// aws v4 signature ref:
	// https://docs.aws.amazon.com/zh_cn/AmazonS3/latest/API/sigv4-auth-using-authorization-header.html
	// https://docs.aws.amazon.com/zh_cn/AmazonS3/latest/API/sig-v4-header-based-auth.html

	struct curl_slist *chunk = NULL;

	char contentHash[65] = {0};
	char date[32] = {0};
	char shortDate[16] = {0};

	{ // host header
		char hostBuf[128] = {0};
		sprintf(hostBuf, "Host:%s", host);
		// first time need
		chunk = curl_slist_append(chunk, hostBuf);
		debug("aws v4 host header: %s\n", hostBuf);
	}

	{ // x-amz-content-sha256 header
		char contentHashBuf[128] = {0};
		SHA256_HASH("", contentHash);
		sprintf(contentHashBuf, "x-amz-content-sha256:%s", contentHash);
		curl_slist_append(chunk, contentHashBuf);
		debug("aws v4 x-amz-content-sha256 header: %s\n", contentHashBuf);
	}

	{ // date header
		time_t rawtime;
		struct tm *timeSt;
		char dateBuf[64] = {0};

		time(&rawtime);
		timeSt = gmtime(&rawtime);
		strftime(date, sizeof(date), "%Y%m%dT%H%M%SZ", timeSt);
		strncpy(shortDate, date, 8);

		sprintf(dateBuf, "x-amz-date:%s", date);
		curl_slist_append(chunk, dateBuf);
		debug("aws v4 date header: %s\n", dateBuf);
	}

	{ // Authorization header
		const char *canonicalRequestFormat = "%s\n%s\n%s\n%s\n%s\n%s";

		const char *service = "s3";

		// canonical request
		const char *httpMethod = "GET";
		char canonicalURI[256] = {0};
		const char *canonicalQueryString = ""; // empty
		char canonicalHeaders[512] = {0};
		const char *signedHeaders ="host;x-amz-content-sha256;x-amz-date";
		const char *hashedPayload = contentHash;
		char canonicalRequest[1024] = {0};

		// string to sign
		char scope[64] = {0};
		char hashedCanonicalRequest[65] = {0};
		char stringToSign[512] = {0};

		// signing key & signature
		char inKey[128] = {0}, outKey[128] = {0};
		uint32_t len1 = 0, len2 = 0;
		const char *inData[4] = {0};
		int32_t dataLen = 4;
		char signature[128] = {0};

		// authorization
		char authorizationBuf[1024] = {0};
		int i;

		// compute canonical request
		sprintf(canonicalURI, "/%s/%s", bucket, file);
		sprintf(canonicalHeaders,
				"host:%s\nx-amz-content-sha256:%s\nx-amz-date:%s\n",
				host, contentHash, date);
		sprintf(canonicalRequest, canonicalRequestFormat,
				httpMethod,
				canonicalURI,
				canonicalQueryString,
				canonicalHeaders,
				signedHeaders,
				hashedPayload);
		debug("aws v4 canonicalRequest: %s\n", canonicalRequest);

		// compute the string to sign
		sprintf(scope, "%s/%s/%s/aws4_request",
				shortDate, region, service);
		SHA256_HASH(canonicalRequest, hashedCanonicalRequest);
		sprintf(stringToSign, "AWS4-HMAC-SHA256\n%s\n%s\n%s",
				date, scope, hashedCanonicalRequest);
		debug("aws v4 stringToSign: %s\n", stringToSign);

		// compute signing key & signature
		sprintf(inKey, "AWS4%s", secretKey);
		len1 = strlen(inKey);
		inData[0] = shortDate;
		inData[1] = region;
		inData[2] = service;
		inData[3] = "aws4_request";
		for (i = 0; i < dataLen; i++) {
			HMAC_SHA256(inKey, len1,
					inData[i], strlen(inData[i]),
					(unsigned char*) outKey, &len2);
			len1 = len2;
			strncpy(inKey, outKey, len2);
		}
		HMAC_SHA256_HASH(outKey, len2,
				stringToSign, strlen(stringToSign),
				signature, sizeof(signature));
		debug("aws v4 signature: %s\n", signature);

		// compute authorization
		sprintf(authorizationBuf,
				"Authorization: AWS4-HMAC-SHA256 Credential=%s/%s,SignedHeaders=%s,Signature=%s",
				accessKey, scope, signedHeaders, signature);

		curl_slist_append(chunk, authorizationBuf);
		debug("aws v4 authorization header: %s\n", authorizationBuf);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	}
}

static void setHeader4S3(CURL * curl, const char * URL)
{
	char file[256] = {0};
	char bucket[64] = {0};
	char host[64] = {0};
	char accessKey[256] = {0};
	char accessKey_copy[256] = {0};
	char secretKey[256] = {0};
	char secretKey_copy[256] = {0};
	char region[16] = {0};

	char *p = NULL;

	// url format: s3-v4://${accessKey}:${secretKey}@${region}@${endpoint}/${bucket}/path/to/file

	// file
	p = locateTheChar(URL, '/', 4) + 1;
	strcpy(file, p);
	debug("s3 file: %s\n", file);

	// bucket
	p = locateTheChar(URL, '/', 3) + 1;
	strncpy(bucket, p, locateTheChar(URL, '/', 4) - p);
	debug("s3 bucket: %s\n", bucket);

	// secretKey
	p = locateTheChar(URL, ':', 2) + 1;
	strncpy(secretKey, p, strchr(URL, '@') - p);
	escape_string(secretKey, secretKey_copy);
	debug("s3 secret key: %s\n", secretKey);

	// host
	p = locateTheChar(URL, '@', 2) + 1;
	if (locateTheChar(URL, '/', 3) == p) {
		// no endpoint
		strcpy(host, AWS_S3_DEFAULT_HOST);
	} else {
		// has endpoint
		strncpy(host, p, locateTheChar(URL, '/', 3) - p);
	}
	debug("s3 host: %s\n", host);

	// accesskey
	p = locateTheChar(URL, '/', 2) + 1;
	strncpy(accessKey, p, locateTheChar(URL, ':', 2) - p);
	escape_string(accessKey, accessKey_copy);
	debug("s3 access key: %s\n", accessKey);

	// region
	p = locateTheChar(URL, '@', 1) + 1;
	strncpy(region, p, locateTheChar(URL, '@', 2) - p);
	debug("s3 region %s\n", region);

	setAwsV4Header(file, bucket,
			accessKey, secretKey,
			host, region,
			curl);
}

int32_t FTDownload(FTHandle * handle,
				   FTProtocol protocol,
				   FTSecurity security,
				   const char * URL,
				   const char * locPath,
				   int isVerbose
		)
{
    int32_t result;

    if (handle) {
		char * targetURL = makeTargetUrl(protocol, security, URL);
		bool isHttp = !(bool)strncmp(targetURL, "http", 4);
		char * locFilePath = makeLocalFilePath(URL, locPath);
		CURL * curl;
        struct FTFile ftFile = {0};
		OneTimeTaskData_st taskData = {0};
        pthread_mutex_t *isBusyLock = &(handle->isBusy.lock);
        pthread_mutex_t *isAbortLock = &(handle->isAbort.lock);

		ftFile.locPath = locFilePath;
        curl = handle->curl;

		/* Init taskData */
        taskData.operation = FT_OP_DOWNLOAD;
        taskData.protocol = protocol;
        taskData.security = security;
		taskData.url = targetURL;
        taskData.locPath = locFilePath;
        taskData.resumeAt = getLocalFileLength(locFilePath);
        taskData.needTransferSize = FT_FILESIZE_UNKNOWN; // Unknown flag!
        taskData.isDone = false;
		handle->taskData = &taskData;

		/* === This FTHandle is busy now! === */
		pthread_mutex_lock(isBusyLock);
        handle->isBusy.data = true;
        pthread_mutex_unlock(isBusyLock);

        pthread_mutex_lock(isAbortLock);
        /* Always false before task running! */
        handle->isAbort.data = false;
        pthread_mutex_unlock(isAbortLock);

		/*=== Custom process ===*/
		switch (protocol) {
		case FT_PROTOCOL_FTP:
			setFTPSecurity(curl, security);

			/* Support resume and set resume point */
			ftFile.resume = true;
			curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, taskData.resumeAt);
			break;

		case FT_PROTOCOL_AZURE:
			if (strstr(URL, "azure:")){//AccessKey mode
				setHeader4Azure(curl, URL);
			}//else SAS Mode
			/* azure not support transfer resume */
			ftFile.resume = false;
			taskData.resumeAt = 0;
#ifdef	WIN32
			// Disable ssl CA certificates verify windows only
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#ifdef  ANDROID
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			break;

		case FT_PROTOCOL_S3C:
			setHeader4S3C(curl, URL);
			ftFile.resume = false;
			taskData.resumeAt = 0;
#ifdef	WIN32
			// Disable ssl CA certificates verify windows only
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#ifdef  ANDROID
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			break;

		case FT_PROTOCOL_S3:
			setHeader4S3(curl, URL);
			ftFile.resume = false;
			taskData.resumeAt = 0;
#ifdef	WIN32
			// Disable ssl CA certificates verify windows only
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#ifdef  ANDROID
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			break;
		}

        /* Set URL always */
        curl_easy_setopt(curl, CURLOPT_URL, targetURL);

        /* Define our callback to get called when there's data to be written */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ft_fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftFile); // Send data to the callback

        /* Set callback function (this callback contain the user callback)*/
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, handle); // Send data to the callback

        /* Set low speed set control */
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L); // Set 10 seconds timeout

		/*======= Switch on full protocol/debug output =======*/
		if (isVerbose) {
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		}

        /* Enable progress callback */
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        /* Task running! */
		debug("FTDownload() start download\n");
        result = curl_easy_perform(curl);
		debug("FTDownload() download result: %s\n", curl_easy_strerror(result));

		if (result == CURLE_OK && isHttp) {
			long responseCode;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			debug("FTDownload() HTTP response code: %ld\n", responseCode);
			if (responseCode >= 400 && responseCode < 600) {
				// http error code 4xx-5xx
				result = (int32_t)responseCode;
			}
		}

		/* Reset curl */
		curl_easy_reset(curl);

        /* Release resources */
		if (locFilePath) free(locFilePath);
		if (targetURL) free(targetURL);
		handle->taskData = NULL;
		if(ftFile.stream) {
            /* close the local file */
            int32_t ret = 0;
			ret = fclose(ftFile.stream);
            debug("FTLib file fclose:%d\n", ret);
		}

        /* === This FTHandle is free now! === */
        pthread_mutex_lock(isBusyLock);
        handle->isBusy.data = false;
        pthread_mutex_unlock(isBusyLock);
    } else {
        /* Set error code */
		result = FT_ERR_NULLHANDLE;
    }

	debug("FTDownload() quit now!\n");

    return result;
}

int32_t FTAbortTransfer(FTHandle * handle)
{
    int32_t res;

    if (handle) {
        pthread_mutex_t *isBusyLock = &(handle->isBusy.lock);
        pthread_mutex_t *isAbortLock = &(handle->isAbort.lock);

        pthread_mutex_lock(isBusyLock);
		printf("handle->isBusy.data: %d\n", handle->isBusy.data);
	    if (handle->isBusy.data) {
            /* Abort task! */
            pthread_mutex_lock(isAbortLock);
            handle->isAbort.data = true;
            pthread_mutex_unlock(isAbortLock);

            res = FT_ERR_OK;
	    } else {
            res = FT_ERR_NONEEDABORT;
	    }
        pthread_mutex_unlock(isBusyLock);
    } else {
        res = FT_ERR_NULLHANDLE;
    }
    return res;
}

const char * FTErr2Str(int32_t errCode)
{
	if (errCode >= 0) {
		if (errCode >= 400) {
			return "HTTP error!";
		}
		return curl_easy_strerror(errCode);
	} else {
		switch (errCode) {
		case FT_ERR_NULLHANDLE:
			return "FTHandle is NULL!";
			break;

		case FT_ERR_BUSY:
			return "File transfer module is busy now!";
			break;

		case FT_ERR_UNKNOWNOPTION:
			return "Unknown option!";
			break;

		case FT_ERR_NONEEDABORT:
			return "No task need to abort!";
			break;

		default:
			return "File transfer module internal error!";
			break;
		}
	}
}




