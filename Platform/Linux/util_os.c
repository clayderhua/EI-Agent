#include "util_os.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <inttypes.h>

#define MEM_TOTAL_LINE 1
#define MEM_FREE_LINE 2

char *trimwhitespace(char *str)
{
  char *end = str + strlen(str) - 1;

  // Trim leading space
  while(str < end && isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

bool read_line(FILE* fp,char* buff,int b_l,int l)
{
    if (!fp)
        return false;
    
    char line_buff[b_l];
    int i;
    for (i = 0; i < l-1; i++)
    {
        if (!fgets (line_buff, sizeof(line_buff), fp))
        {
            return false;
        }
    }

    if (!fgets (line_buff, sizeof(line_buff), fp))
    {
        return false;
    }

    memcpy(buff,line_buff,b_l);

    return true;
}

bool util_os_get_os_name(char * pOSNameBuf, unsigned long * bufLen)
{
	FILE *fp = NULL;
	char osname[128] = {0};
	char *p, *q;
	//if(osVersionBuf == NULL) return false;
#ifndef ANDROID
	fp = popen("lsb_release -d", "r");
	if(!fp) return false;
	p = fgets(osname, 128, fp);
	if (!p) {
		osname[0] = '\0';
	}
	pclose(fp);
#else
	sprintf(osname, "Android");
#endif
	p = strtok(osname, ":");
	if(!p) return false;
	p = strtok(NULL, ":");
	if(!p) return false;
	q = trimwhitespace(p);
	if(pOSNameBuf)
		strncpy(pOSNameBuf, q, strlen(q)+1);
	if(bufLen)
		*bufLen = strlen(q)+1;
	return true;
}

bool util_os_get_processor_name(char * pProcessorNameBuf, unsigned long * bufLen)
{
	FILE *fp = NULL;
	char modelname[128] = {0};
	char *p, *q;
	fp = popen("cat /proc/cpuinfo | grep 'model name'", "r");
	if(!fp) return false;
	if (fgets(modelname, 128, fp) == NULL) modelname[0] = '\0';
	pclose(fp);
	p = strtok(modelname, ":");
	if(!p) return false;
	p = strtok(NULL, ":");
	if(!p) return false;
	q = trimwhitespace(p);
	if(pProcessorNameBuf)
		strncpy(pProcessorNameBuf, q, strlen(q)+1);
	if(bufLen)
		*bufLen = strlen(q)+1;
	return true;
}
bool util_os_get_architecture(char * pArchBuf, int * bufLen)
{
	FILE *fp = NULL;
	char buff[32] = {0};
	char *p;
	fp = popen("uname -m", "r");
	if(!fp) return false;
	if (fgets(buff, 32, fp) == NULL) buff[0] = '\0';
	pclose(fp);
	p = strtok(buff, "\n");
	if(!p) return false;
	if(pArchBuf)
		strncpy(pArchBuf, p, strlen(p)+1);
	if(bufLen)
		*bufLen = strlen(p)+1;
	return true;
}

bool util_os_get_free_memory(uint64_t *totalPhysMemKB, uint64_t *availPhysMemKB)
{
	char proc_pic_path[128] = {0};
	const int bufLength = 256;
	sprintf(proc_pic_path,"/proc/meminfo");
	FILE * fp = NULL;
	fp = fopen(proc_pic_path,"r");
	if(NULL != fp)
	{
		char buf[256] = {0};
		if (read_line(fp,buf,bufLength,MEM_TOTAL_LINE))
    	{
			if(totalPhysMemKB)
			{
				char name[32] = {0};
				sscanf(buf,"%31s %"PRIu64,name, totalPhysMemKB);
			}
    	} 
		fseek(fp,0,SEEK_SET);
		if (read_line(fp,buf,bufLength,MEM_FREE_LINE))
    	{
			if(availPhysMemKB)
			{
				char name[32] = {0};
				sscanf(buf,"%31s %"PRIu64,name, availPhysMemKB);
			}
    	} 
		fclose(fp);
	}
	return true;
}

unsigned long long util_os_get_tick_count()
{
	struct timespec ts;  
	clock_gettime(CLOCK_MONOTONIC, &ts);  
	return (ts.tv_sec * 1000 + ts.tv_nsec/1000000); 
}
