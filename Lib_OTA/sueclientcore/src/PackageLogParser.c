#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "util_path.h"
#include "PackageLogParser.h"

#ifdef linux
#define strtok_s(s1,s2,s3) strtok_r((char *)(s1),(char *)(s2),(char **)(s3))
#define strcpy_s(s1,s2,s3) strncpy((char *)(s1),(char *)(s3),(int)(s2))
#endif

static bool ParseInitProperty(char * line, section *tmpSec);

int SUEPkgDeployCheckCB(const void * const checkHandle, NotifyDPCheckMsgCB notifyDpMsgCB, int * isExit, void * userData){
    int iRet = 0;
    FILE *fp=NULL;
	Section_Head secHeader; 
	bool isRun = true;
	int curSection = SECTION_NULL_NUM;
    char * tmpPath = (char *)(userData);
    char *path = NULL;
    int len = 0;
    len = strlen(tmpPath) + strlen(LOG_NAME) + 2;
    path = (char *)malloc(len);
    memset(path, 0, len);
    sprintf(path, "%s%c%s", tmpPath, FILE_SEPARATOR, LOG_NAME);
	secHeader.next = NULL;
	printf("file path is %s",path);

	fp = fopen(path,"r");
	if(fp == NULL){
		//sprintf(str,"Can not open OTAPackageLog.log");
		//notifyDpMsgCB(checkHandle,str,strlen(str)+1);
		//printf("Can not open OTAPackageLog.log, last error code is %d\n",error);
        iRet = -1;
        goto done1;
	}
	char line[READ_LEN];
	char *pstr = NULL;

	while(isRun){
		curSection = SECTION_NULL_NUM;
		section *tmpSection = NULL;
		//[property]
		while(isRun){
			pstr = fgets(line,READ_LEN,fp);
			if(line[0] == '\n') continue;
			if(pstr == NULL){
				//int error = GetLastError();
				//printf("Read error code is: %d\n",error);
				isRun = false;  //end of file.
				break;
			}
			/*
			else if(line[strlen(line)-1] != '\n'){//user process have not writed whole line. 
				fseek(fp,(-1)*(strlen(line)+1),SEEK_CUR);
				cp_sleep(0);
				continue;
			}*/
			//if(curSection == SECTION_NULL_NUM && (strstr(line,SECTION_STATUS)|| strstr(line,SECTION_END))){//check [log] and [end]
			if (strstr(line, "[log]")){
				pstr = fgets(line, READ_LEN, fp);
				if (pstr == NULL){
					//int error = GetLastError();
					//printf("Read error code is: %d\n",error);
					isRun = false;  //end of file.
					char statusMsg[SW_NAME_LEN + 2 + SW_VERSION_LEN + 2 + (SW_RESULT_LEN * 2) + 2 + MESSAGE_LEN + STATUS_LEN + 4];
					sprintf(statusMsg, "[%s][%s][result]failed,Process stopped unexpectedly;!", tmpSection->name, tmpSection->version);
					notifyDpMsgCB(checkHandle, statusMsg, strlen(statusMsg) + 1);
				}
			}
			if((curSection == SECTION_NULL_NUM && strstr(line,SECTION_END))||
				(curSection == SECTION_PROPERTY_NUM && strstr(line,SECTION_PROPERTY))){
				char str[64] = "Log is illegal";
				notifyDpMsgCB(checkHandle,str,strlen(str)+1);
                iRet = -2;
                goto done1;
			}
			else if(strstr(line,SECTION_PROPERTY) && curSection == SECTION_NULL_NUM){
				//else if(char *s = strstr(str, substr);)
				tmpSection = (section *)malloc(sizeof(section));
				memset(tmpSection,0,sizeof(section));
				tmpSection->next = NULL;
				curSection = SECTION_PROPERTY_NUM;
				if(secHeader.next == NULL) secHeader.next = tmpSection;
				else{
					section *ptr=secHeader.next;
					while(ptr->next != NULL) ptr = ptr->next;
					ptr->next = tmpSection;            //Add the end of linker
				}
				//printf("Read [property]\n");
			}	
			else if(strstr(line,SECTION_STATUS) && curSection == SECTION_PROPERTY_NUM){
				curSection = SECTION_STATUS_NUM;
				//printf("There is not [status],Will entry section [end]\n");
				break;
			}
			else if(strstr(line,SECTION_END) && curSection == SECTION_PROPERTY_NUM){
				curSection = SECTION_END_NUM;
				//printf("There is not [status],Will entry section [end]\n");
				break;
			}
			else if(curSection == SECTION_PROPERTY_NUM){
				ParseInitProperty(line, tmpSection);
			}
		}
		//[status]
		//char statusMsg[STATUS_LEN+SW_NAME_LEN+SW_VERSION_LEN+6];
		char statusStr[STATUS_LEN];
		//int len = sizeof(statusStr);
		memset(statusStr,0,sizeof(statusStr));
		while(isRun && curSection == SECTION_STATUS_NUM){
			pstr = fgets(line,READ_LEN,fp);
			if(line[0] == '\n') continue;
			if(pstr == NULL){
				isRun = false;  //end of file.
				break;
			}
			if(strstr(line,SECTION_STATUS)||strstr(line,SECTION_PROPERTY)){
				char str[64] = "Log is illegal";
				notifyDpMsgCB(checkHandle,str,strlen(str)+1);
				return -2;
			}
			else if(strstr(line,SECTION_END)){
				curSection = SECTION_END_NUM;
				//printf("Will entry section [end]\n");
				//sprintf(statusStr,"[%s][%s]:%s",tmpSection->name,tmpSection->version,line);
				//notifyDpMsgCB(checkHandle,statusStr,strlen(statusStr)+1);
				break;
			}
			else{
				int lineLen = strlen(line);
				int strLen = strlen(statusStr);
				if((lineLen + strLen) < STATUS_LEN){
					strcat(statusStr,line);
				}
			}
		}
		//[end]
		while(isRun && curSection == SECTION_END_NUM){
			pstr = fgets(line,READ_LEN,fp);
			if(line[0] == '\n') continue;
			if(pstr == NULL){
				//int error = GetLastError();
				//printf("Read error code is: %d\n",error);
				isRun = false;  //end of file.
				break;
			}
			if((curSection == SECTION_END_NUM && strstr(line,SECTION_PROPERTY))
				|| (curSection == SECTION_END_NUM && strstr(line,SECTION_END))){
				char str[64] = "Log is illegal";
				notifyDpMsgCB(checkHandle,str,strlen(str)+1);
                iRet = -2;
                goto done1;
			}
			else if(strstr(line,SECTION_EXIT)){  //Check new sw information
				char statusMsg[SW_NAME_LEN+2+SW_VERSION_LEN+2+(SW_RESULT_LEN*2)+2+MESSAGE_LEN+STATUS_LEN+4];
				curSection = SECTION_NULL_NUM;
				if(strlen(tmpSection->result) == 0)
					strcpy_s(tmpSection->result,SW_RESULT_LEN,VALUE_SW_RESULT_FAILED);
				if(strlen(tmpSection->message)>0)
					sprintf(statusMsg,"[%s][%s][result]%s,%s;%s",tmpSection->name,tmpSection->version,tmpSection->result,tmpSection->message,statusStr);
				else
					sprintf(statusMsg,"[%s][%s][result]%s;%s",tmpSection->name,tmpSection->version,tmpSection->result,statusStr);
				notifyDpMsgCB(checkHandle,statusMsg,strlen(statusMsg)+1);
				break;
			}else if(curSection == SECTION_END_NUM){
				ParseInitProperty(line, tmpSection);
			}
		}
	}
done1:
	if(fp != NULL)fclose(fp);
	fp = NULL;
	//free source
	section *secPtr=secHeader.next;
	while(secPtr != NULL){
		secHeader.next = secPtr->next;
		free(secPtr);
		secPtr=secHeader.next;
	}
    if (path) free(path);
    path = NULL;
	return iRet;
}

static bool ParseInitProperty(char * line, section *tmpSec)
{
	char *subLine;
	char *ptrSemi;
	char *ptrEqual;
	bool ret = true;

	if(line == NULL || tmpSec == NULL)
		return false;

	if(strstr(line,SEMI_FLAG)) {
		subLine = strtok_s(line,SEMI_FLAG,&ptrSemi);
		while(subLine != NULL){ //splite by ";"
			if(strstr(subLine,EQUAL_FLAG)){
				char * p; 
				p = strtok_s(subLine,EQUAL_FLAG,&ptrEqual); 
				while(p!=NULL) { //splite by "="
					if(strstr(subLine,KEY_SW_NAME)){
						p = strtok_s(NULL,EQUAL_FLAG,&ptrEqual);
						if(p!=NULL)strcpy_s(tmpSec->name,SW_NAME_LEN,p);
						if(tmpSec->name[strlen(tmpSec->name)-1]=='\n') tmpSec->name[strlen(tmpSec->name)-1] = '\0';
						//printf("sw name is:%s\n",tmpSec->name);
						break;
					}else if(strstr(subLine,KEY_SW_VERSION)){
						p = strtok_s(NULL,EQUAL_FLAG,&ptrEqual);
						if(p!=NULL)strcpy_s(tmpSec->version,SW_VERSION_LEN,p);
						if(tmpSec->version[strlen(tmpSec->version)-1]=='\n') tmpSec->version[strlen(tmpSec->version)-1] = '\0';
						//printf("sw version is:%s\n",tmpSec->version);
						break;
					}else if(strstr(subLine,KEY_SW_RESULT)){
						p = strtok_s(NULL,EQUAL_FLAG,&ptrEqual);
						if(p!=NULL){
							if(strstr(p,VALUE_SW_RESULT_SUCCESS))
								strcpy_s(tmpSec->result,SW_RESULT_LEN,VALUE_SW_RESULT_SUCCESS);
							else
								strcpy_s(tmpSec->result,SW_RESULT_LEN,VALUE_SW_RESULT_FAILED);
						}
						if(tmpSec->result[strlen(tmpSec->result)-1]=='\n') tmpSec->result[strlen(tmpSec->result)-1] = '\0';
						//printf("sw result is:%s\n",tmpSec->result);
						break;
					}else if(strstr(subLine,KEY_SW_MESSAGE)){
						p = strtok_s(NULL,EQUAL_FLAG,&ptrEqual);
						if(p!=NULL)strcpy_s(tmpSec->message,MESSAGE_LEN,p);
						if(tmpSec->message[strlen(tmpSec->message)-1]=='\n') tmpSec->message[strlen(tmpSec->message)-1] = '\0';
						//printf("sw message is:%s\n",tmpSec->message);
						break;
					}
				}
			}
			subLine = strtok_s(NULL,SEMI_FLAG,&ptrSemi);
		}
	} else if(strstr(line,EQUAL_FLAG)) {
		char * p; 
		p = strtok_s(line,EQUAL_FLAG,&ptrEqual); 
		while(p!=NULL) { //splite by "="
			if(strstr(line,KEY_SW_NAME)){
				p = strtok_s(NULL,EQUAL_FLAG,&ptrEqual);
				if(p!=NULL)strcpy_s(tmpSec->name,SW_NAME_LEN,p);
				if(tmpSec->name[strlen(tmpSec->name)-1]=='\n') tmpSec->name[strlen(tmpSec->name)-1] = '\0';
				//printf("sw name is:%s\n",tmpSec->name);
				break;
			}else if(strstr(line,KEY_SW_VERSION)){
				p = strtok_s(NULL,EQUAL_FLAG,&ptrEqual);
				if(p!=NULL)strcpy_s(tmpSec->version,SW_VERSION_LEN,p);
				if(tmpSec->version[strlen(tmpSec->version)-1]=='\n') tmpSec->version[strlen(tmpSec->version)-1] = '\0';
				//printf("sw version is:%s\n",tmpSec->version);
				break;
			}else if(strstr(line,KEY_SW_RESULT)){
				p = strtok_s(NULL,EQUAL_FLAG,&ptrEqual);
				if(p!=NULL){ //strcpy_s(tmpSec->result,SW_RESULT_LEN,p);
					if(strstr(p,VALUE_SW_RESULT_SUCCESS))
						strcpy_s(tmpSec->result,SW_RESULT_LEN,VALUE_SW_RESULT_SUCCESS);
					else
						strcpy_s(tmpSec->result,SW_RESULT_LEN,VALUE_SW_RESULT_FAILED);
				}
				if(tmpSec->result[strlen(tmpSec->result)-1]=='\n') tmpSec->result[strlen(tmpSec->result)-1] = '\0';
				//printf("sw result is:%s\n",tmpSec->result);
				break;
			}else if(strstr(line,KEY_SW_MESSAGE)){
				p = strtok_s(NULL,EQUAL_FLAG,&ptrEqual);
				if(p!=NULL)strcpy_s(tmpSec->message,MESSAGE_LEN,p);
				if(tmpSec->message[strlen(tmpSec->message)-1]=='\n') tmpSec->message[strlen(tmpSec->message)-1] = '\0';
				//printf("sw message is:%s\n",tmpSec->message);
				break;
			}
		}
	} else {
		ret = false;
	}
	return ret;
}
