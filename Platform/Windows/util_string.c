#include "util_string.h"
#include <string.h>
#include <Windows.h>

WISEPLATFORM_API void TrimStr(char * str)
{
	char *p, *stPos, *edPos;
	if(NULL == str) return;

	for (p = str; (*p == ' ' || *p == '\t') && *p != '\0'; p++);
	edPos = stPos = p;
	for (; *p != '\0'; p++)
	{
		if(*p != ' ' && *p != '\t') edPos = p;   
	}
	memmove(str, stPos, edPos - stPos + 1);
	*(str + (edPos - stPos + 1)) = '\0' ;
}

WISEPLATFORM_API wchar_t * ANSIToUnicode(const char * str)
{
	int textLen = 0;
	wchar_t * wcRet = NULL;
	textLen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	wcRet = (wchar_t*)malloc((textLen+1)*sizeof(wchar_t));
	memset(wcRet, 0, (textLen+1)*sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)wcRet, textLen);
	return wcRet;
}

WISEPLATFORM_API char * UnicodeToANSI(const wchar_t *str)
{
	char *cRet = NULL;
	int textLen = 0;
	textLen = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	cRet = (char*)malloc((textLen+1)*sizeof(char));
	memset(cRet,0,sizeof(char)*(textLen+1));
	WideCharToMultiByte(CP_ACP, 0, str, -1, cRet, textLen,NULL,NULL);
	return cRet;
}

WISEPLATFORM_API wchar_t * UTF8ToUnicode(const char* str)
{
	int textLen = 0;
	wchar_t * wcRet = NULL;
	textLen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	wcRet = (wchar_t *)malloc((textLen+1)*sizeof(wchar_t));
	memset(wcRet,0,(textLen+1)*sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, str, -1, (LPWSTR)wcRet, textLen);
	return wcRet;
}

WISEPLATFORM_API char * UnicodeToUTF8(const wchar_t* str)
{
	char * cRet = NULL;
	int textLen = 0;
	textLen = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
	cRet = (char*)malloc((textLen+1)*sizeof(char));
	memset(cRet,0,sizeof(char)*(textLen+1));
	WideCharToMultiByte(CP_UTF8, 0, str, -1, cRet, textLen, NULL, NULL);
	return cRet;
}

WISEPLATFORM_API char * ANSIToUTF8(const char* str)
{
	char * cRet = NULL;
	wchar_t * wcTmpStr = NULL;
	wcTmpStr = ANSIToUnicode(str);
	if(wcTmpStr != NULL)
	{
		cRet = UnicodeToUTF8(wcTmpStr);
		free(wcTmpStr);
	}
	return cRet;
}

WISEPLATFORM_API char * UTF8ToANSI(const char* str)
{
	char * cRet = NULL;
	wchar_t * wcTmpStr = NULL;
	wcTmpStr = UTF8ToUnicode(str);
	if(wcTmpStr != NULL)
	{
		cRet = UnicodeToANSI(wcTmpStr);
		free(wcTmpStr);
	}
	return cRet;
}

WISEPLATFORM_API bool IsUTF8(const char * string)
{
	const unsigned char * bytes = NULL;
    if(!string)
        return false;

   bytes = (const unsigned char *)string;
    while(*bytes)
    {
        if( (// ASCII
             // use bytes[0] <= 0x7F to allow ASCII control characters
                bytes[0] == 0x09 ||
                bytes[0] == 0x0A ||
                bytes[0] == 0x0D ||
                (0x20 <= bytes[0] && bytes[0] <= 0x7E)
            )
        ) {
            bytes += 1;
            continue;
        }

        if( (// non-overlong 2-byte
                (0xC2 <= bytes[0] && bytes[0] <= 0xDF) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF)
            )
        ) {
            bytes += 2;
            continue;
        }

        if( (// excluding overlongs
                bytes[0] == 0xE0 &&
                (0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
            ) ||
            (// straight 3-byte
                ((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
                    bytes[0] == 0xEE ||
                    bytes[0] == 0xEF) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
            ) ||
            (// excluding surrogates
                bytes[0] == 0xED &&
                (0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
            )
        ) {
            bytes += 3;
            continue;
        }

        if( (// planes 1-3
                bytes[0] == 0xF0 &&
                (0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
            ) ||
            (// planes 4-15
                (0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
            ) ||
            (// plane 16
                bytes[0] == 0xF4 &&
                (0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
            )
        ) {
            bytes += 4;
            continue;
        }

        return false;
    }

    return true;
}

WISEPLATFORM_API bool GetRandomStr(char *randomStrBuf, int bufSize)
{
	int strLen = 0, i = 0;
	LARGE_INTEGER perfCnt = {0};
	if(NULL == randomStrBuf || bufSize <= 0) return FALSE;

	QueryPerformanceCounter(&perfCnt);
	srand((unsigned int)perfCnt.QuadPart); //us

	strLen = bufSize - 1;

	while(i < strLen)
	{
		int flag = rand()%3;
		switch(flag)
		{
		case 0:
			{
				randomStrBuf[i] = 'a' + rand()%26;
				break;
			}
		case 1:
			{
				randomStrBuf[i] = '0' + rand()%10;
				break;
			}
		case 2:
			{
				randomStrBuf[i] = 'A' + rand()%26;
				break;
			}
		default: break;
		}
		i++;
	}
	randomStrBuf[strLen] = '\0';
	return TRUE; 
}

WISEPLATFORM_API char*  StringReplace(const char *orig, const char *rep, const char *with)
{
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

WISEPLATFORM_API void StringFree(char *str) {
	free(str);
}
