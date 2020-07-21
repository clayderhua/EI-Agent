#ifndef __UTIL_H__
#define __UTIL_H__

unsigned char * HMAC_SHA256(const void *key, int32_t keyLen,
				const char *data, int32_t dataLen,
				unsigned char *result, uint32_t *resultLen);
unsigned char * HMAC_SHA1(const void *key, int32_t keyLen,
				const char *data, int32_t dataLen,
				unsigned char *result, uint32_t *resultLen);
void HMAC_SHA256_HASH(const void *key, int32_t keyLen,
		const char *data, int32_t dataLen,
		char *outBuf, int32_t outBufLen);
void SHA256_HASH(char *string, char outputBuffer[65]);
int32_t getLocalFileLength(char * localFilePath);

#endif /* __UTIL_H__ */
