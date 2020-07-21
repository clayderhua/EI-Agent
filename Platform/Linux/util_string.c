#include "util_string.h"
#include <string.h>
#include <unistd.h>
#include <time.h>

void TrimStr(char * str)
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

wchar_t * ANSIToUnicode(const char * str)
{
	int textLen = 0;
	wchar_t * wcRet = NULL;
	int n = 0;
	textLen = mbstowcs(wcRet, str, 0)+1;  
	wcRet = (wchar_t*)malloc((textLen+1)*sizeof(wchar_t));
	memset(wcRet, 0, (textLen)*sizeof(wchar_t));
	n = mbstowcs(wcRet, str, textLen);  
	if(-1 == n)
	{
		free(wcRet);
		wcRet = NULL;
	} 
	return wcRet;
}

char *UnicodeToANSI(const wchar_t *str)
{
	char *cRet = NULL;
	int len = wcstombs(cRet, str, 0)+1; 
	int n = 0;
	cRet = (char*)malloc((len)*sizeof(char));
	memset(cRet,0,sizeof(char)*(len));
	n = wcstombs(cRet, str, len);
	if(-1 == n)
	{
		free(cRet);
		cRet = NULL;
	}    
	return cRet;
}

wchar_t *UTF8ToUnicode(const char* str)
{
	wchar_t * wcRet = NULL;
	/*todo*/
	wcRet = strdup(str);
	return wcRet;
}

char* UnicodeToUTF8(const wchar_t* str)
{
	char * cRet = NULL;
	/*todo*/
	cRet = strdup(str);
	return cRet;
}


char* ANSIToUTF8(const char* str)
{
	char * cRet = NULL;
	/*todo*/
	cRet = strdup(str);
	return cRet;
}

char* UTF8ToANSI(const char* str)
{
	char * cRet = NULL;
	/*todo*/
	cRet = strdup(str);
	return cRet;
}

bool IsUTF8(const char * string)
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
unsigned long long GetTickCount()
{
	struct timespec ts;  
	clock_gettime(CLOCK_MONOTONIC, &ts);  
	return (ts.tv_sec * 1000 + ts.tv_nsec/1000000); 
}

bool GetRandomStr(char *randomStrBuf, int bufSize)
{
    int strLen = 0, i = 0;
	if(NULL == randomStrBuf || bufSize <= 0) return false;

	srand((unsigned int)GetTickCount());  //10ms
	usleep(10*1000);  //10ms deviation

	//srand((unsigned int)time(NULL)); //sec

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
	return true; 
}

char*  StringReplace(const char *orig, const char *rep, const char *with)
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
    for (count = 0; (tmp = strstr(ins, rep)) != NULL; ++count) {
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

void StringFree(char *str) {
	free(str);
}
