#include "util_path.h"
#include <windows.h>
#include <stdio.h>
#include  <io.h>

#include "unistd.h"

WISEPLATFORM_API void util_split_path_file(char const *filepath, char* path, char* file) 
{
	const char *slash = filepath, *next;
	while ((next = strpbrk(slash + 1, "\\/"))) slash = next;
	if (filepath != slash) slash++;
	strncpy(path, filepath, slash - filepath);
	strcpy(file, slash);
}

WISEPLATFORM_API void util_path_combine(char* destination, const char* path1, const char* path2)
{
	if (destination == NULL)
		return;
	if(path1 == NULL && path2 == NULL) {
		strcpy(destination, "");
	}
	else if(path2 == NULL || strlen(path2) == 0) {
		strcpy(destination, path1);
	}
	else if(path1 == NULL || strlen(path1) == 0) {
		strcpy(destination, path2);
	} 
	else {
		char directory_separator[] = {FILE_SEPARATOR, 0};
		const char *last_char = NULL;
		const char *skip_char = path2;
		int append_directory_separator = 0;
		if (strlen(destination) > 0)
			memset(destination, 0, strlen(destination));
		last_char = path1 + strlen(path1)-1;
		if(strcmp(last_char, directory_separator) != 0) {
			append_directory_separator = 1;
		}
		strncpy(destination, path1, strlen(path1));
		if(append_directory_separator)
		{
			if(strncmp(path2, directory_separator, strlen(directory_separator)) != 0)
				strncat(destination, directory_separator, strlen(directory_separator));
		}
		else
		{
			if(*skip_char == FILE_SEPARATOR)
				skip_char++;   
		}
		strncat(destination, skip_char, strlen(skip_char));
	}
}

WISEPLATFORM_API int util_module_path_get(char * moudlePath)
{
	int iRet = 0;
	char * lastSlash = NULL;
	char tempPath[MAX_PATH] = {0};
	if(NULL == moudlePath) return iRet;
	if(ERROR_SUCCESS != GetModuleFileName(NULL, tempPath, sizeof(tempPath)))
	{
		lastSlash = strrchr(tempPath, FILE_SEPARATOR);
		if(NULL != lastSlash)
		{
			strncpy(moudlePath, tempPath, lastSlash - tempPath + 1);
			moudlePath[lastSlash - tempPath + 1] = '\0';
			iRet = lastSlash - tempPath + 1;
		}
	}
	return iRet;
}

WISEPLATFORM_API bool util_create_directory(char* path)
{
	return CreateDirectory(path,NULL)==TRUE?true:false;
}

WISEPLATFORM_API unsigned long util_temp_path_get(char* lpBuffer, int nBufferLength)
{
	return GetTempPath (nBufferLength, lpBuffer);
}

WISEPLATFORM_API bool util_is_file_exist(char const *filepath)
{
	if(NULL == filepath) return false;
	return _access(filepath, 0) == 0 ? true : false;
}

WISEPLATFORM_API void util_remove_file(char const *filepath)
{
	if(filepath && strlen(filepath))
	{
		remove(filepath);
	}
}

WISEPLATFORM_API bool util_copy_file(char const *srcFilepath, char const *destFilepath)
{
	if(srcFilepath == NULL || destFilepath == NULL)
		return false;
	return CopyFile(srcFilepath, destFilepath, false)?true:false;	 
}

WISEPLATFORM_API long util_file_size_get(char const *filepath)
{
	long size = 0;
	FILE *fp = NULL;
	fp = fopen(filepath, "r");
	if(fp == NULL)
		return size;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fclose(fp);
	return size;
}

WISEPLATFORM_API long util_file_read(char const *filepath, char* buff, long length)
{
	long size = 0;
	FILE *fp = NULL;
	fp = fopen(filepath, "r");
	if(fp == NULL)
		return size;
	size = fread(buff, 1, length, fp);
	fclose(fp);
	return size;
}

WISEPLATFORM_API long util_file_write(char const *filepath, char* buff)
{
	return util_file_write_ex(filepath, buff, true);
}

WISEPLATFORM_API long util_file_write_ex(char const *filepath, char* buff, bool append)
{
	long size = 0;
	FILE *fp = NULL;
	if(append)
		fp = fopen(filepath, "w+");
	else
		fp = fopen(filepath, "w");
	if(fp == NULL)
		return size;
	size = fwrite(buff, 1, strlen(buff), fp);
	fclose(fp);
	return size;
}

WISEPLATFORM_API int util_get_full_path (const char *path, char *buf, int bufLen)
{
	if (!path || !buf) return -1;
	if (GetFullPathName((LPSTR)path, bufLen, (LPSTR)buf, NULL) > 0) return -1;
	
	return 0;
}

WISEPLATFORM_API int util_is_dir_exist(const char * dirPath)
{
	DWORD dwAttrib = GetFileAttributes(dirPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

WISEPLATFORM_API int util_rm_dir(char * dirPath)
{
	int iRet = 0;
	if(NULL != dirPath)
	{
		long findHandle = -1;
		struct _finddata_t fileInfo;
		int len = strlen(dirPath)+3;
		char * tmpFindPath = (char*)malloc(len);

		memset(tmpFindPath, 0, len);
		sprintf(tmpFindPath, "%s%c*", dirPath, FILE_SEPARATOR);
		findHandle = _findfirst(tmpFindPath, &fileInfo);
		if(findHandle != -1) {
			do {
				char * curFilePath = NULL;
				len = strlen(dirPath)+strlen(fileInfo.name)+2;
				curFilePath = (char *)malloc(len);
				memset(curFilePath, 0, len);
				sprintf(curFilePath, "%s%c%s", dirPath, FILE_SEPARATOR, fileInfo.name);
				if (fileInfo.attrib & _A_SUBDIR) {
					if((strcmp(fileInfo.name,".") != 0 ) &&(strcmp(fileInfo.name,"..") != 0)) {
						iRet = util_rm_dir(curFilePath);
					}
				} else {
					iRet = remove(curFilePath);
				}
				free(curFilePath);
			} while (_findnext(findHandle, &fileInfo) == 0 && iRet == 0);
			_findclose(findHandle);
		}
		else
			iRet = -1;

		free(tmpFindPath);

		if (iRet == 0) {
			iRet = _rmdir(dirPath);
		}
	}
	else
		iRet = -1;
	return iRet;
}

#define MATCH_CHAR(c1, c2, igCase)  ((c1 == c2) || ((igCase == 1) && (tolower(c1) == tolower(c2))))
static bool WildcardMatch(const char *src, const char *pattern, int igCase) 
{
	bool asterisk = false;
	int sa = 0, pa = 0;
	char cp = '\0';
	char cs = '\0';
	int i = 0;
	int si = 0, pi = 0;
	while(si < (int) strlen(src)) 
	{
		if(pi == strlen(pattern)) 
		{
			if(asterisk) 
			{
				sa++;
				si = sa;
				pi = pa;
				continue;
			} 
			else 
			{
				return false;
			}
		}

		cp = *(pattern+pi);
		cs = *(src+si);

		if(cp == '*') 
		{
			asterisk = true;
			pi++;
			pa = pi;
			sa = si;
		} 
		else if(cp == '?') 
		{
			si++;
			pi++;
		} 
		else 
		{
			if(MATCH_CHAR(cs,cp,igCase)) 
			{
				si++;
				pi++;
			}
			else if(asterisk) 
			{
				pi = pa;
				sa++;
				si = sa;
			} 
			else 
			{
				return false;
			}
		}
	}

	for (i = pi; i < (int) strlen(pattern); i++) 
	{
		if (*(pattern+i) != '*') return false;		
	}
	return true;
}

WISEPLATFORM_API int util_file_iterate(const char * dirPath, const char * pattern, unsigned int depth, 
    bool isSpecLayer, util_file_iterate_cb cbFunc, void * userData)
{
	int iRet = 0;
	static unsigned int realDepth = 0;

	if(NULL != dirPath && NULL != pattern && cbFunc != NULL)
	{
		long findHandle = -1;
		struct _finddata_t fileInfo;
		int len = strlen(dirPath)+3;
		char * tmpFindPath = (char*) malloc(len);
		memset(tmpFindPath, 0, len);
		sprintf(tmpFindPath, "%s%c*", dirPath, FILE_SEPARATOR);
		findHandle = _findfirst(tmpFindPath, &fileInfo);
		if(findHandle != -1)
		{
			realDepth++;
			do
			{
				if (fileInfo.attrib & _A_SUBDIR )    
				{
					if(depth != 1)
					{
						if((strcmp(fileInfo.name,".") != 0 ) &&(strcmp(fileInfo.name,"..") != 0))   
						{
							unsigned int curDepth = depth >1?depth-1:depth;
							char * curDirPath = NULL;
							len = strlen(dirPath)+strlen(fileInfo.name)+3;
							curDirPath = (char *)malloc(len);
							memset(curDirPath, 0, len);
							sprintf(curDirPath, "%s%c%s%c", dirPath, FILE_SEPARATOR, fileInfo.name, FILE_SEPARATOR);
							cbFunc(realDepth, curDirPath, userData); // add for dirpath
							iRet += util_file_iterate(curDirPath, pattern, curDepth, isSpecLayer, cbFunc, userData);
							free(curDirPath);
						}
					}
				}
				else if(fileInfo.attrib & _A_ARCH)  
				{
					if((depth>1 && !isSpecLayer) || depth <= 1)
					{
						if(WildcardMatch(fileInfo.name, pattern, 0))
						{
							char * findFilePath = NULL;
							len = strlen(dirPath)+strlen(fileInfo.name)+16;
							findFilePath = (char *)malloc(len);
							memset(findFilePath, 0, len);
							sprintf(findFilePath, "%s%c%s", dirPath, FILE_SEPARATOR, fileInfo.name);
							cbFunc(realDepth, findFilePath, userData);
							free(findFilePath);
							findFilePath = NULL;
							iRet++;
						}
					}
				}
			}while (_findnext(findHandle, &fileInfo) == 0);
			_findclose(findHandle);
			realDepth--;
		}
		free(tmpFindPath);
	}
	return iRet;
}
