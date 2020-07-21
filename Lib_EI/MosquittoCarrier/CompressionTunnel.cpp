/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2018/12/24 by Fred Chang								    */
/* Modified Date: 2018/12/24 by Fred Chang									*/
/* Abstract     : Compression Tunnel            							*/
/* Reference    : None														*/
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WISEPlatform.h"
#include "AdvCompression.h"
#include "AdvCC.h"
#include "AdvJSON.h"
//#include "AdvLog.h"
#include "CompressionTunnel.h"
#include "ExternalTranslator.h"

typedef struct {
	char *topic;
	char *payload;
} atomic_message;
/*
int GetMaxCompressedLen(int nLenSrc)
{
	int n16kBlocks = (nLenSrc + 16383) / 16384; // round up any fraction of a block
	return (nLenSrc + 6 + (n16kBlocks * 5));
}
int CompressData(const unsigned char* abSrc, int nLenSrc, unsigned char* abDst, int nLenDst)
{
	int nErr = -1;
	int nRet = -1;
	z_stream zInfo = { 0 };
	zInfo.total_in = zInfo.avail_in = nLenSrc;
	zInfo.total_out = zInfo.avail_out = nLenDst;
	zInfo.next_in = (unsigned char*)abSrc;
	zInfo.next_out = abDst;

	nErr = deflateInit(&zInfo, Z_DEFAULT_COMPRESSION); // zlib function
	if (nErr == Z_OK) {
		nErr = deflate(&zInfo, Z_FINISH);              // zlib function
		if (nErr == Z_STREAM_END) {
			nRet = zInfo.total_out;
		}
	}
	deflateEnd(&zInfo);    // zlib function
	return(nRet);
}

int UncompressData(const unsigned char* abSrc, int nLenSrc, unsigned char* abDst, int nLenDst)
{
	int nErr = -1;
	int nRet = -1;

	z_stream zInfo = { 0 };
	zInfo.total_in = zInfo.avail_in = nLenSrc;
	zInfo.total_out = zInfo.avail_out = nLenDst;
	zInfo.next_in = (unsigned char*)abSrc;
	zInfo.next_out = abDst;

	nErr = inflateInit(&zInfo);               // zlib function
	if (nErr == Z_OK) {
		nErr = inflate(&zInfo, Z_FINISH);     // zlib function
		if (nErr == Z_STREAM_END) {
			nRet = zInfo.total_out;
		}
	}
	inflateEnd(&zInfo);   // zlib function
	return(nRet); // -1 or len of output
}
*/

time_t TimeEventWithSecond(time_t check, time_t interval) {
	static time_t oldt = 0;
	static unsigned int count = 0;
	if (check == 0 || oldt == 0 || count % (4*interval) == 0) {
		time_t t = time(NULL);
		if (t - interval >= oldt) {
			oldt = t;
			return oldt;
		}
		return 0;
	}
	else if (check < 0) {
		return oldt;
	} 
	else {
		if (check - interval >= oldt) return oldt;
		else return 0;
	}
}

time_t AdvLog_TimesUp(time_t check, time_t interval) {
	if (check <= 0) return TimeEventWithSecond(-1, interval);
	return TimeEventWithSecond(check, interval);
}

void *memdup(const void *src, size_t n)
{
	void *dest;

	dest = malloc(n);
	if (dest == NULL)
		return NULL;

	return memcpy(dest, src, n);
}

char *CT_QueueToPayload(void **pq, time_t *time, char *topic, char *payload, int payloadlen, int *resultLen, int second, char *method) {
	static char *compress = NULL;
	static unsigned int compressSize = 1024;
	static char *temp = NULL;
	static unsigned int tempSize = 1024;
	static unsigned int amount = 0;

	QueueHandler q = *(QueueHandler *)pq;
	atomic_message atomic;
	int len = 0;
	int count = 0;
	int pos = 0;
	unsigned int maxDst = 0;
	int srcLen = 0;
	bool forcezip = false;
	time_t result;

	if (compress == NULL) {
		compress = (char *)malloc(1024);
	}
	if (temp == NULL) {
		temp = (char *)malloc(1024);
	}

	if (q == NULL) {
		Queue_Init(&q);
		*pq = q;
	}

	amount += strlen(topic) + payloadlen;

	atomic.topic = strdup(topic);
	atomic.payload = strdup(payload);
	Queue_Add(q, &atomic, sizeof(atomic_message));

	if (Queue_GetAmount(q) >= 20 || amount > 250000) {
		forcezip = true;
		result = 1;
		amount = 0;
	} else 
		result = second != 0 ? AdvLog_TimesUp(*time, *time == 0 ? second > 3 ? 3 : second : second) : 1;

	if (result) {
		printf("*time = %lld, result = %lld, diff = %lld\n",*time,result, result - *time);
		*time = result;

		pos = snprintf(temp, tempSize, "{\"method\":\"%s\",\"packets\":[", forcezip ? "gzip" : method);
		while (CON_RESULT_NORMAL == Queue_Get(q, &atomic)) {
			len = strlen(atomic.topic) + strlen(atomic.payload) + 32;
			if (strncmp(method, "none", 4) == 0 && !forcezip) {
				if (pos + len > 250000) {
					if (count == 0 && len >= 250000) {
						printf("============================================================ The package is over 250000.");
						forcezip = true;
						pos = snprintf(temp, tempSize, "{\"method\":\"gzip\",\"packets\":[");
					}
					else {
						Queue_Redo(q, &atomic, sizeof(atomic_message));
						break;
					}
				}
			}
			if (pos + len >= tempSize) {
				tempSize += len + 32;
				temp = (char *)realloc(temp, tempSize);
			}
			pos += snprintf(temp + pos, tempSize, count == 0 ? "{\"topic\":\"%s\",\"payload\":%s}" : ",{\"topic\":\"%s\",\"payload\":%s}", atomic.topic, atomic.payload);
			free(atomic.topic);
			free(atomic.payload);
			count++;
		}
		pos += snprintf(temp + pos, tempSize, "],\"count\":%d}", count);

		srcLen = strlen(temp);

		if (strncmp(method, "none", 4) == 0 && !forcezip) {
			*resultLen = srcLen;
			printf("@@@@@@@@@@@@@@@@ ori: %d, compress: %d, percentage: %0.2f%%\n", srcLen, srcLen, (float)srcLen * 100 / srcLen);
			return (char *)temp;
		}

		maxDst = AdvZ_GetMaxCompressedLen(method, (const unsigned char *)temp, srcLen);
		if (compressSize < maxDst + 32) {
			compressSize = maxDst + 32;
			compress = (char *)realloc(compress, compressSize);
		}

		/*
		Nion {
			(4)"orig":(4)size,
			(4)"gzip":(size)binary
		}
		*/
		compress[0] = 0x10;
		compress[1] = 0x64;
		sprintf((char *)compress + 2, "orig");
		compress[6] = 0x34;
		*((int *)compress + 7) = srcLen;
		
		compress[11] = 0x64;
		sprintf((char *)compress + 12, method);
		maxDst = AdvZ_CompressData(method, (const unsigned char *)temp, srcLen, (unsigned char *)compress + 21, maxDst);
		compress[16] = 0x40 + ((maxDst >> 16) & 0xF);
		compress[17] = 0x40 + ((maxDst >> 12) & 0xF);
		compress[18] = 0x40 + ((maxDst >> 8) & 0xF);
		compress[19] = 0x40 + ((maxDst >> 4) & 0xF);
		compress[20] = 0xA0 + (maxDst & 0xF);
		maxDst += 21;
		compress[maxDst] = 0;
		maxDst += 1;

		

		/*maxDst = CompressData((const unsigned char *)temp, srcLen, compress + 10, maxDst);*/
		//srcLen = UncompressData((const unsigned char *)compress + 10, maxDst, (unsigned char *)temp, tempSize);
		//printf("srcLen = %d\n", srcLen);

		/*if (maxDst <= *len) {
			*len = maxDst;
			memcpy(message, temp, maxDst);
		}*/
		printf("@@@@@@@@@@@@@@@@ ori: %d, compress: %d, percentage: %0.2f%%\n", srcLen, maxDst, (float)maxDst * 100 / srcLen);
		//free(temp);
		*resultLen = maxDst;
		// bulk out
		return (char *)compress;
	}
	*resultLen = 0;
	return NULL;
}
