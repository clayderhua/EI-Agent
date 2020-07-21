#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "fplatform.h"
#include "tool.h"

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

void GetNowTimeString(char *string, int length, const char *fmt) {
	static char timeStr[64] = {0};
	time_t t = 0;
	struct tm timetm;

	if ((t = TimeEventWithSecond(0,1)) != 0) {
		localtime_r(&t, &timetm);
		strftime(timeStr, sizeof(timeStr), fmt, &timetm);
	}

	strncpy(string, timeStr, length);
}

/*void ReduceBackGroundAndDumpString(const char *buffer, char **output) {
	if(strchr(buffer,'\n') != NULL) {
		int count = 0;
		int len = strlen(buffer);
		char *saveptr;
		char *temp = strdup(buffer);
		char *set;
		char *pos = temp;
		
		pos = strchr(pos, '\n');
		while (pos != NULL) {
			pos++;
			count++;
			pos = strchr(pos, '\n');
		}
		*output = (char *)malloc(len + 6 * count + 1);
		pos = *output;
		set = strtok_r(temp, "\n", &saveptr);
		while (set != NULL) {
			pos += sprintf(pos,"%s%s\n",set,__COLOR_NONE);
			set = strtok_r(NULL, "\n", &saveptr);
		}
		free(temp);
		len = strlen(*output);
		(*output)[len] = 0;
	} else {
		*output = NULL;
	}

	return;
}*/


int GetFileSize(FILE *fp) {
	int fd = fileno(fp);
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

char *fmalloc(const char *filename) {
	char *end;
	int ret;
	char *buffer;
	FILE *fp = fopen(filename,"r");
	if(fp == NULL) return NULL;
	int size = GetFileSize(fp);
	
	if(size > 4096 || size <= 0) {
		fclose(fp);
		return NULL; //4k
	}

	buffer = (char *)calloc(1,size+1);
	ret = fread(buffer,size,1,fp);
	end = strrchr (buffer,'}');
	if(end != NULL) *(end+1) = 0;
	else {
		free(buffer);
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	return buffer;
}
