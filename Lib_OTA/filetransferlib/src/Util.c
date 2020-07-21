#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#if _MSC_VER>=1900
_ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned);
#ifdef __cplusplus 
extern "C"
#endif 
FILE* __cdecl __iob_func(unsigned i) {
	return __acrt_iob_func(i);
}
#endif /* _MSC_VER>=1900 */

unsigned char * HMAC_SHA256(const void *key, int32_t keyLen,
		const unsigned char *data, int32_t dataLen,
		unsigned char *result, uint32_t *resultLen)
{
	return HMAC(EVP_sha256(), key, keyLen, data, dataLen, result, resultLen);
}

unsigned char * HMAC_SHA1(const void *key, int32_t keyLen,
		const unsigned char *data, int32_t dataLen,
		unsigned char *result, uint32_t *resultLen)
{
	return HMAC(EVP_sha1(), key, keyLen, data, dataLen, result, resultLen);
}

void HMAC_SHA256_HASH(const void *key, int32_t keyLen,
		const unsigned char *data, int32_t dataLen,
		char *outBuf, int32_t outBufLen)
{
	unsigned char result[128] = {0};
	uint32_t resultLen;
	int i;
	
	HMAC(EVP_sha256(), key, keyLen, data, dataLen, result, &resultLen);

	memset(outBuf, 0, outBufLen);
	for(i = 0; i < resultLen; i++)
	{
		sprintf(outBuf + (i * 2), "%02x", result[i]);
	}
}

void SHA256_HASH(char *string, char outputBuffer[65])
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	int i = 0;
	
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, string, strlen(string));
	SHA256_Final(hash, &sha256);
	
	for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
	}
	outputBuffer[64] = 0;
}


int32_t getLocalFileLength(char * localFilePath) 
{
	int32_t size = 0;
	FILE * fp = fopen(localFilePath, "rb");
	if (fp) {
		fseek(fp, 0L, SEEK_END);
		size = ftell(fp);
		fclose(fp);
	}
	return size;
}
