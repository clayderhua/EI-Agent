#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CODEC_LEN	1024
#define DEF_DES_KEY	"#otasue*"

#include "base64.h"
#include "des.h"

// destBuf = DESDecodeEx( Base64Decode(srcBuf) )
static int DES_BASE64Decode(char * srcBuf, char *destBuf)
{
    int iRet = -1;
    char plaintext[MAX_CODEC_LEN] = { 0 };
    if (srcBuf == NULL || destBuf == NULL) return iRet;
    {
        char *base64Dec = NULL;
        int decLen = 0;
        int len = strlen(srcBuf);
        iRet = Base64Decode(srcBuf, len, &base64Dec, &decLen);
        if (iRet == 0)
        {
            iRet = DESDecodeEx(DEF_DES_KEY, DEF_DES_KEY, base64Dec, decLen, plaintext);
            if (iRet == 0)
            {
                len = strlen(plaintext);
                /*
                ** 根据实际长度判断需要拷贝多少个字节
                */
                //memcpy(destBuf, plaintext, len + 1);
                if(len<sizeof(plaintext)){
                    memcpy(destBuf, plaintext, len);
					destBuf[len] = '\0';
                }else{
                    memcpy(destBuf,plaintext,sizeof(plaintext));
					destBuf[sizeof(plaintext)-1] = '\0';
                }
            }
        }
        if (base64Dec) free(base64Dec);
    }

    return iRet;
}


// destBuf = Base64Encode( DESEncodeEx(srcBuf) )
static int DES_BASE64Encode(char* srcBuf, char *destBuf)
{
    int iRet = -1;
    char ciphertext[MAX_CODEC_LEN] = { 0 };
    int cipherLen = 0;
    if (srcBuf == NULL || destBuf == NULL) return iRet;
    iRet = DESEncodeEx(DEF_DES_KEY, DEF_DES_KEY, srcBuf, ciphertext, &cipherLen);
    if (iRet == 0)
    {
        char *base64Enc = NULL;
        int encLen = 0;
        int len = cipherLen;
        iRet = Base64Encode(ciphertext, len, &base64Enc, &encLen);
        if (iRet == 0)
        {
            len = strlen(base64Enc);
            if (len > 0)
            {
                memcpy(destBuf, base64Enc, len);
				destBuf[len] = '\0';
            }
        }
        if (base64Enc) free(base64Enc);
    }
    return iRet;
}




int main(int argc, char* argv[])
{
	//char url[] = {"pqTXkY9yklDacR43bPyBtYba+1fgmrPZyvT2ZpClg7WqU9gcJ1cz6v8/Cq6LYz18kTT2bcDjCGV5g37vh8eFNVroUP5uV+rtM4/BU+3p3TWqWYF6/p6EYsa8xHiYHVWl"};
	char text[MAX_CODEC_LEN];

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [-d|-e] [input text]\n", argv[0]);
		fprintf(stderr, "\t-d: decode\n");
		fprintf(stderr, "\t-e: encode\n");
		return 0;
	}
	text[0] = '\0';
	
	if (strcmp(argv[1], "-e") == 0) {
		DES_BASE64Encode(argv[2], text);
	} else {
		DES_BASE64Decode(argv[2], text);
	}
	fprintf(stderr, "%s\n", text);
	
	return 0;
}