#include <string.h>

#ifdef WIN32
#include <Windows.h>
#include <stdio.h>
#else

#define  MAX_PATH 260

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#endif

#include "ReadINI.h"

char g_szConfigPath[MAX_PATH];

//Get Current Path
int GetCurrentPath(char buf[],char *pFileName)
{
    char * p;
#ifdef WIN32
    GetModuleFileName(NULL,buf,MAX_PATH);
#else
    char pidfile[64];
    int bytes;
    int fd;

    sprintf(pidfile, "/proc/%d/cmdline", getpid());

    fd = open(pidfile, O_RDONLY, 0);
    bytes = read(fd, buf, 256);
    close(fd);
    buf[MAX_PATH] = '\0';
    if (bytes == -1) { // error
        return -1;
    }

#endif
    p = &buf[strlen(buf)];
    do
    {
        *p = '\0';
        p--;
#ifdef WIN32
    } while( '\\' != *p );
#else
    } while( '/' != *p );
#endif

    p++;

    memcpy(p,pFileName,strlen(pFileName));
    return 0;
}

//Get a String From INI file
char *GetIniKeyString(const char *title, const char *key, const char *filename)
{
    FILE *fp;
    char szLine[1024];
    static char tmpstr[1024];
    int rtnval;
    int i = 0;
    int flag = 0;
    char *tmp;

    if((fp = fopen(filename, "r")) == NULL)
    {
        printf("have   no   such   file \n");
        return "";
    }

    while(!feof(fp))
    {
        rtnval = fgetc(fp);
        if (rtnval == '\r') {
            continue;
        }

        if(rtnval == EOF || rtnval == '\n')
        {
            szLine[i] = '\0';

            // find comment and skip
            i = 0;
            while (szLine[i] != '\0') {
                if (szLine[i] == ';') {
                    szLine[i] = '\0';
                    break;
                }
                i++;
            }

            i = 0;
            tmp = strchr(szLine, '=');

            if(( tmp != NULL )&&(flag == 1))
            {
                szLine[tmp-szLine] = '\0';
                if(strcmp(szLine,key)==0)
                {
                    if ('#' == szLine[0])
                    {
                    }
                    else if ( '/' == szLine[0] && '/' == szLine[1] )
                    {

                    }
                    else
                    {
                        strncpy(tmpstr,tmp+1, sizeof(tmpstr));
                        fclose(fp);
                        return tmpstr;
                    }
                }
            }
            else
            {
                strcpy(tmpstr,"[");
                strcat(tmpstr,title);
                strcat(tmpstr,"]");
                if( strncmp(tmpstr,szLine,strlen(tmpstr)) == 0 )
                {
                    flag = 1;
                }
            }
        }
        else
        {
            szLine[i++] = rtnval;
        }
    }
    fclose(fp);
    return "";
}

//Get a Int Value From INI file
int GetIniKeyInt(const char *title, const char *key, const char *filename)
{
    char buffer[1024];
    return strtol(GetIniKeyStringDef(title, key, filename, buffer, sizeof(buffer), "0"), NULL, 10);
}

int GetIniKeyIntDef(const char *title, const char *key, const char *filename, int def)
{
    char buffer[1024];
    char *endptr = NULL;
    const char *str = GetIniKeyStringDef(title, key, filename, buffer, sizeof(buffer), NULL);
    int ret;

    if (str == NULL) { // not found
        return def;
    }

    ret = strtol(str, &endptr, 10);
    if (str == endptr) { // no digit
        return def;
    }
    return ret;
}


double GetIniKeyDoubleDef(const char *title, const char *key, const char *filename, double def)
{
    char buffer[1024];
    char *endptr = NULL;
    const char *str = GetIniKeyStringDef(title, key, filename, buffer, sizeof(buffer), NULL);
    double ret;

    if (str == NULL) { // not found
        return def;
    }

    ret = strtod(str, &endptr);
    if (str == endptr) { // no digit
        return def;
    }
    return ret;
}


/*
    Get a String From INI file
*/
char *GetIniKeyStringDef(const char *title,
                         const char *key,
                         const char *filename,
                         char* buffer,
                         unsigned long bufferSize,
                         const char* def)
{
    FILE *fp;
    char szLine[1024];
    char title_buffer[512];
    int rtnval;
    int i = 0;
    int flag = 0;
    char *tmp;

    if((fp = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "have no such file \n");
        if (def) {
            strncpy(buffer, def, bufferSize-1);
            buffer[bufferSize-1] = '\0';
            return buffer;
        } else {
            return NULL;
        }
    }

    while(!feof(fp))
    {
        rtnval = fgetc(fp);
        if (rtnval == '\r') {
            continue;
        }

        if(rtnval == EOF || rtnval == '\n')
        {
            szLine[i] = '\0';

            // find comment and skip
            i = 0;
            while (szLine[i] != '\0') {
                if (szLine[i] == ';') {
                    szLine[i] = '\0';
                    break;
                }
                i++;
            }

            i = 0;
            tmp = strchr(szLine, '=');

            if(( tmp != NULL )&&(flag == 1))
            {
                szLine[tmp-szLine] = '\0';
                if(strcmp(szLine,key)==0)
                {
                    if ('#' == szLine[0])
                    {
                    }
                    else if ( '/' == szLine[0] && '/' == szLine[1] )
                    {
                    }
                    else
                    {
                        if (bufferSize < strlen(tmp+1) + 1) {
                            fprintf(stderr, "buffer is not enough, bufferSize=%lu, require=%lu\n", bufferSize, strlen(tmp+1) + 1);
                            if (def) {
                                strncpy(buffer, def, bufferSize-1);
                                buffer[bufferSize-1] = '\0';
                                return buffer;
                            } else {
                                return NULL;
                            }
                        }
                        strcpy(buffer, tmp+1);
                        fclose(fp);
                        return buffer;
                    }
                }
            }
            else
            {
                if (sizeof(title_buffer) < strlen(title) + 3) { // +3, "[]\n"
                    fprintf(stderr, "buffer is not enough, bufferSize=%lu, require=%lu\n", sizeof(title_buffer), strlen(title) + 3);
                    if (def) {
                        strncpy(buffer, def, bufferSize-1);
                        buffer[bufferSize-1] = '\0';
                        return buffer;
                    } else {
                        return NULL;
                    }
                }
                strcpy(title_buffer,"[");
                strcat(title_buffer, title);
                strcat(title_buffer, "]");
                if( strncmp(title_buffer, szLine, strlen(title_buffer)) == 0 )
                {
                    flag = 1;
                }
            }
        }
        else
        {
            szLine[i++] = rtnval;
        }
    }
    fclose(fp);
    if (def) {
        strncpy(buffer, def, bufferSize-1);
        buffer[bufferSize-1] = '\0';
        return buffer;
    } else {
        return NULL;
    }
    return buffer;
}