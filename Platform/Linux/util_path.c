#include "util_path.h"
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

void util_split_path_file(char const *filepath, char* path, char* file)
{
	char* dirc = strdup(filepath);
       char* basec = strdup(filepath);
	char* dir = dirname(dirc);
	char* name = basename(basec);
	strcpy(path, dir);
	strcpy(file, name);
	free(dirc);
	free(basec);
}

void util_path_combine(char* destination, const char* path1, const char* path2)
{
	if(path1 == NULL && path2 == NULL) {
		strcpy(destination, "");;
	}
	else if(path2 == NULL || strlen(path2) == 0) {
		strcpy(destination, path1);
	}
	else if(path1 == NULL || strlen(path1) == 0) {
		strcpy(destination, path2);
	} 
	else {
		char directory_separator[] = {FILE_SEPARATOR, 0};
		const char *last_char, *temp = path1;
		const char *skip_char = path2;
		int append_directory_separator = 0;

		while(*temp != '\0')
		{
			last_char = temp;
			temp++;        
		}
		
		if(strcmp(last_char, directory_separator) != 0) {
			append_directory_separator = 1;
		}
		strcpy(destination, path1);
		if(append_directory_separator)
		{
			if(strncmp(path2, directory_separator, strlen(directory_separator))!= 0)
				strcat(destination, directory_separator);
		}
		else
		{
			if(*skip_char == FILE_SEPARATOR)
				skip_char++;   
		}
		strcat(destination, skip_char);
	}
}

int util_module_path_get(char * moudlePath)
{
	int iRet = 0;
	char * lastSlash = NULL;
	char tempPath[MAX_PATH] = {0};
	if(NULL == moudlePath) return iRet;
	
	if (readlink("/proc/self/exe", tempPath, sizeof(tempPath)) == -1)
		return iRet;

	if( 0 == access( tempPath, F_OK ) )
	{
		lastSlash = strrchr(tempPath, FILE_SEPARATOR);
		if(NULL != lastSlash)
		{
			iRet = lastSlash - tempPath + 1;
			memcpy(moudlePath, tempPath, iRet);
			moudlePath[iRet] = '\0';
		}	
	}
	return iRet;
}

bool util_create_directory(char* path)
{
    struct stat st;
    bool status = true;
    mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path, mode) != 0)
            status = false;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        status = false;
    }

    return(status);
}

unsigned long util_temp_path_get(char* lpBuffer, int nBufferLength)
{
#ifdef ANDROID
	int len = 0;
	len = strlen("/cache/");
   	strncpy(lpBuffer, "/cache/", strlen("/cache/")+1);	
	return len;
#else
	int len = 0;
	char const *folder = getenv("TMPDIR");
	if (folder == 0)
	{
		len = strlen("/tmp/");
		strncpy(lpBuffer, "/tmp/", strlen("/tmp/")+1);
	}
	else
	{
		len = strlen(folder);
		if(nBufferLength < len+1)
		{
			strncpy(lpBuffer, folder, nBufferLength);
			len = nBufferLength;
		}
		else
			strncpy(lpBuffer, folder, len+1);
	}
	return len;
#endif
}

bool util_is_file_exist(char const *filepath)
{
	if(NULL == filepath) return false;
	return access(filepath, F_OK) == 0 ? true : false;
}

void util_remove_file(char const *filepath)
{
	if(access(filepath, F_OK) == 0)
		remove(filepath);
}

bool util_copy_file(char const *srcFilepath, char const *destFilepath)
{
	int read_fd;
	int write_fd;
	struct stat stat_buf;
	off_t offset = 0;
	if(NULL == srcFilepath) return false;
	if(NULL == destFilepath) return false;

	/* Open the input file. */
	read_fd = open (srcFilepath, O_RDONLY);
	/* Stat the input file to obtain its size. */
	fstat (read_fd, &stat_buf);
	/* Open the output file for writing, with the same permissions as the
	  source file. */
	write_fd = open (destFilepath, O_WRONLY | O_CREAT, stat_buf.st_mode);
	/* Blast the bytes from one file to the other. */
	sendfile (write_fd, read_fd, &offset, stat_buf.st_size);
	/* Close up. */
	close (read_fd);
	close (write_fd);
	return true;
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
	size = fread(buff, length, 1, fp);
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
	if (realpath(path, buf)) return -1;
	
	return 0;
}

WISEPLATFORM_API int util_is_dir_exist(const char * dirPath)
{
	if(access(dirPath, 0) == 0) {
        struct stat status;
        stat(dirPath, &status);
        return (status.st_mode & S_IFDIR) != 0? 1: 0;
    }
    return 0;
}


WISEPLATFORM_API int util_rm_dir(char * dirPath)
{
	int iRet = 0;
	if(NULL != dirPath) {
		DIR *dp;
		if((dp = opendir(dirPath)) != NULL && iRet == 0) {
			struct dirent entry;
			struct dirent *result;
			char *path;

			while(0 == readdir_r(dp, &entry, &result) && result != NULL)
			{
				if(strcmp(".", result->d_name) == 0 || strcmp("..", result->d_name) == 0) continue;

				path = (char *) malloc(PATH_MAX);
				sprintf(path, "%s/%s", dirPath, result->d_name);
				if(DT_DIR == result->d_type) {
						iRet = util_rm_dir(path);
				} else {
						iRet = remove(path);
				}
				free(path);
			}
			closedir(dp);
		}
		else
			iRet = -1;
		if(iRet == 0)
			iRet = rmdir(dirPath);
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
	while(si < strlen(src)) 
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

	for (i = pi; i < strlen(pattern); i++) 
	{
		if (*(pattern+i) != '*') return false;		
	}
	return true;
}

WISEPLATFORM_API int util_file_iterate(const char * dirPath, const char * pattern, unsigned int depth, 
    bool isJustSpacLayer, util_file_iterate_cb cbFunc, void * userData)
{	
	int foundCnt = 0;
	static unsigned int realDepth = 0;

	if (dirPath && pattern) {
		DIR * dp = opendir(dirPath);
		if (dp) {
			struct dirent entry;
			struct dirent * result;
			
			realDepth++;
			while (0 == readdir_r(dp, &entry, &result) && result != NULL) {
				const char * tname = result->d_name;

				if (!strcmp(tname, ".") || !strcmp(tname, "..")) {
					continue;
				} 
				
				if (DT_DIR == result->d_type && 1 != depth) { 
					unsigned int tdepth = depth > 1 ? (depth - 1) : 0; // Note: depth can't be 1 here!
                    char *path = (char *)malloc(PATH_MAX);
                    
                    sprintf(path, "%s/%s/", dirPath, tname);
                    
					cbFunc(realDepth, path, userData);

					foundCnt += util_file_iterate(path, pattern, tdepth, 
                                    isJustSpacLayer, cbFunc, userData);
                    free(path);
					continue;
				} 
				
				if (depth > 1 && isJustSpacLayer) {
					continue;
				} 
				
                // WildcardMatch return 0 means it not found
				if (WildcardMatch(tname, pattern, 0)) {
					int len = strlen(dirPath) + strlen(tname) + 2;					
					char * distFilePath = malloc(len); 
                    
					sprintf(distFilePath, "%s/%s", dirPath, tname);
                    cbFunc(realDepth, distFilePath, userData);
                    
                    free(distFilePath);
                    foundCnt++;
				}
			}
			closedir(dp);
			realDepth--;
		}	
	}
	return foundCnt;
}