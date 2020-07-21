#include "MiniUnzipLib.h"
#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
#endif

#ifdef unix
# include <unistd.h>
# include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
# include <direct.h>
# include <io.h>
#endif

#include <inttypes.h>

#include "mz_compat.h"
#include "mz_os.h"
#include "mz_zip.h"

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)

#ifdef WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif

#define change_file_date        mz_os_set_file_date

/*
void change_file_date(const char *filename, uLong dosdate)//, tm_unz tmu_date)
{
#ifdef WIN32
	HANDLE hFile;
	FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;

	hFile = CreateFile(filename,GENERIC_READ | GENERIC_WRITE,
		0,NULL,OPEN_EXISTING,0,NULL);
	GetFileTime(hFile,&ftCreate,&ftLastAcc,&ftLastWrite);
	DosDateTimeToFileTime((WORD)(dosdate>>16),(WORD)dosdate,&ftLocal);
	LocalFileTimeToFileTime(&ftLocal,&ftm);
	SetFileTime(hFile,&ftm,&ftLastAcc,&ftm);
	CloseHandle(hFile);
#else
#ifdef unix
	struct utimbuf ut;
	struct tm newdate;

//	newdate.tm_sec = tmu_date.tm_sec;
//	newdate.tm_min=tmu_date.tm_min;
//	newdate.tm_hour=tmu_date.tm_hour;
//	newdate.tm_mday=tmu_date.tm_mday;
//	newdate.tm_mon=tmu_date.tm_mon;
//	if (tmu_date.tm_year > 1900)
//		newdate.tm_year=tmu_date.tm_year - 1900;
//	else
//		newdate.tm_year=tmu_date.tm_year ;
//	newdate.tm_isdst=-1;

	//int32_t iRet = mz_zip_dosdate_to_tm(dosdate,&newdate);
	if(dosdate_to_tm(dosdate,&newdate) == 0){ //MZ_OK 0
	    ut.actime=ut.modtime=mktime(&newdate);
	    utime(filename,&ut);
	}
#endif
#endif
}
*/
int mymkdir(const char* dirname)
{
	int ret=0;
#ifdef WIN32
	ret = mkdir(dirname);
#else
#ifdef unix
	ret = mkdir (dirname,0775);
#endif
#endif
	return ret;
}

int makedir(char *newdir)
{
	char *buffer ;
	char *p;
	int  len = (int)strlen(newdir);

	if (len <= 0)
		return 0;

	buffer = (char*)malloc(len+1);
	strcpy(buffer,newdir);

	if (buffer[len-1] == '/') {
		buffer[len-1] = '\0';
	}
	if (mymkdir(buffer) == 0)
	{
		free(buffer);
		return 1;
	}

	p = buffer+1;
	while (1)
	{
		char hold;

		while(*p && *p != '\\' && *p != '/')
			p++;
		hold = *p;
		*p = 0;
		if ((mymkdir(buffer) == -1) && (errno == ENOENT))
		{
			LOGE("Couldn't create directory %s.",buffer);
			free(buffer);
			return 0;
		}
		if (hold == 0)
			break;
		*p++ = hold;
	}
	free(buffer);
	return 1;
}

int do_extract_currentfile(unzFile uf, const int* popt_extract_without_path, int* popt_overwrite, char* basePath, const char* password)
{
	char filename_inzip[256];
	char* filename_withoutpath;
	char* p;
	int err=UNZ_OK;
	FILE *fout=NULL;
	void* buf;
	unsigned int size_buf;
	char * curRealDestPath = NULL;
	int len = 0;

	unz_file_info file_info;
	err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
	if (err!=UNZ_OK)
	{
		LOGE("Error %d with zipfile in unzGetCurrentFileInfo.",err);
		return err;
	}

	size_buf = WRITEBUFFERSIZE;
	buf = (void*)malloc(size_buf);
	if (buf==NULL)
	{
		LOGE("Error allocating memory.");
		return UNZ_INTERNALERROR;
	}

	p = filename_withoutpath = filename_inzip;
	while ((*p) != '\0')
	{
		if (((*p)=='/') || ((*p)=='\\'))
			filename_withoutpath = p+1;
		p++;
	}

	if ((*filename_withoutpath)=='\0')
	{
		if ((*popt_extract_without_path)==0)
		{
			len = strlen(basePath) + strlen(filename_inzip) + 2;
			curRealDestPath = (char *)malloc(len);
			memset(curRealDestPath, 0, len);
			sprintf(curRealDestPath, "%s/%s", basePath, filename_inzip);
			LOGD("Creating directory: %s.", curRealDestPath);
			mymkdir(curRealDestPath);
			if(access(curRealDestPath,0)==0){
				chmod(curRealDestPath,0775);
			}
			//LOGD("Creating directory: %s.", filename_inzip);
			//mymkdir(filename_inzip);
		}
	}
	else
	{
		char* write_filename;
		int skip=0;

		/*if ((*popt_extract_without_path)==0)
			write_filename = filename_inzip;
		else
			write_filename = filename_withoutpath;*/
	
		if ((*popt_extract_without_path)==0)
		{
			len = strlen(basePath) + strlen(filename_inzip) + 2;
			curRealDestPath = (char *)malloc(len);
			memset(curRealDestPath, 0, len);
			sprintf(curRealDestPath, "%s/%s", basePath, filename_inzip);
		}
		else
		{
			len = strlen(basePath) + strlen(filename_withoutpath) + 2;
			curRealDestPath = (char *)malloc(len);
			memset(curRealDestPath, 0, len);
			sprintf(curRealDestPath, "%s/%s", basePath, filename_withoutpath);
		}
		write_filename = curRealDestPath;
		err = unzOpenCurrentFilePassword(uf,password);
		if (err!=UNZ_OK)
		{
			LOGE("Error %d with zipfile in unzOpenCurrentFilePassword.",err);
		}

		if (((*popt_overwrite)==0) && (err==UNZ_OK))
		{
			char rep=0;
			FILE* ftestexist;
			ftestexist = fopen(write_filename,"rb");
			if (ftestexist!=NULL)
			{
				fclose(ftestexist);
				do
				{
					char answer[128];
					int ret;

					printf("The file %s exists. Overwrite ? [y]es, [n]o, [A]ll: ",write_filename);
					ret = scanf("%1s",answer);
					if (ret != 1) 
					{
						exit(EXIT_FAILURE);
					}
					rep = answer[0] ;
					if ((rep>='a') && (rep<='z'))
						rep -= 0x20;
				}
				while ((rep!='Y') && (rep!='N') && (rep!='A'));
			}

			if (rep == 'N')
				skip = 1;

			if (rep == 'A')
				*popt_overwrite=1;
		}

		if ((skip==0) && (err==UNZ_OK))
		{
			fout=fopen(write_filename,"wb");

			/* some zipfile don't contain directory alone before file */
			if ((fout==NULL) && ((*popt_extract_without_path)==0) &&
				(filename_withoutpath!=(char*)filename_inzip))
			{
				int needle = strlen(write_filename) - strlen(filename_withoutpath);
				char c=*(write_filename + needle);
				*(write_filename + needle) = '\0';
				makedir(write_filename);
				*(write_filename + needle) = c;
				fout=fopen(write_filename,"wb");
			}

			if (fout==NULL)
			{
				LOGE("Error opening %s.",write_filename);
			}
		}

		if (fout!=NULL)
		{
			LOGI("Extracting: %s.",write_filename);
			do
			{
				err = unzReadCurrentFile(uf,buf,size_buf);
				if (err<0)
				{
					LOGE("Error %d with zipfile in unzReadCurrentFile.",err);
					break;
				}
				if (err>0)
					if (fwrite(buf,err,1,fout)!=1)
					{
						LOGE("Error in writing extracted file.");
						err=UNZ_ERRNO;
						break;
					}
			}
			while (err>0);
			if (fout)
				fclose(fout);

			if (err==0){
				time_t tmtAccessTime;
				time(&tmtAccessTime);
				time_t tmtModiTime = mz_zip_dosdate_to_time_t(file_info.dosDate);
				change_file_date(write_filename, tmtModiTime, tmtAccessTime,0);//Create time is no support.
				//change_file_date(write_filename, file_info.dos_date);// , file_info.tmu_date);
			}
			//文件存在则修改权限
			fout=fopen(write_filename,"rb");
			if(fout!=NULL){
				fclose(fout);
				chmod(write_filename,0775);
			}
		}

		if (err==UNZ_OK)
		{
			err = unzCloseCurrentFile (uf);
			if (err!=UNZ_OK)
			{
				LOGE("Error %d with zipfile in unzCloseCurrentFile\n",err);
			}
		}
		else
			unzCloseCurrentFile(uf); /* don't lose the error */
	}
   if(curRealDestPath) free(curRealDestPath);
	free(buf);
	return err;
}


int do_extract(unzFile uf,int opt_extract_without_path,int opt_overwrite, char* basePath,const char* password)
{
	uint32_t i;
	unz_global_info gi;
	int err;

	err = unzGetGlobalInfo (uf,&gi);
	if (err!=UNZ_OK) LOGE("Error %d with zipfile in unzGetGlobalInfo \n",err);
	else
	{
		for (i=0;i<gi.number_entry;i++)
		{
			err = do_extract_currentfile(uf,&opt_extract_without_path,
				&opt_overwrite, basePath, password);
			if (err != UNZ_OK) break;
			if ((i+1)<gi.number_entry)
			{
				err = unzGoToNextFile(uf);
				if (err!=UNZ_OK)
				{
					LOGE("[MiniUnzipLib]Error %d with zipfile in unzGoToNextFile,basepath:%s\n",err, basePath);
					break;
				}
			}
		}
	}
	return err;
}

/*
	return 0 for legacy OTA zip file. 1 for new OTA zip, -1 for fail
*/
int CheckZipComment(char * srcZipPath, char * pkgComment, int size)
{
	int iRet = 1;

	if (NULL == srcZipPath)
		return -1;

	{
#ifdef USEWIN32IOAPI
		zlib_filefunc_def ffunc;
#endif
		unzFile uf = NULL;
		char *fileNameTry = NULL;
		int len = strlen(srcZipPath) + 16;
		fileNameTry = (char *)malloc(len);
		memset(fileNameTry, 0, len);
		strcpy(fileNameTry, srcZipPath);

#ifdef USEWIN32IOAPI
		fill_win32_filefunc(&ffunc);
		uf = unzOpen2(srcZipPath, &ffunc);
#else
		uf = unzOpen(srcZipPath);
#endif
		if (uf == NULL)
		{
			strcat(fileNameTry, ".zip");
#ifdef USEWIN32IOAPI
			uf = unzOpen2(fileNameTry, &ffunc);
#else
			uf = unzOpen(fileNameTry);
#endif
		}
		if (uf == NULL)
		{
			LOGE("[MiniUnzipLib]UnzOpen failed!uf:NULL!\n");
		}
		else
		{
			LOGD("[MiniUnzipLib]UnzOpen success!\n");
			//get zipcomment
			char curComment[500];
			memset(curComment, 0, sizeof(curComment));
			unzGetGlobalComment(uf, curComment, sizeof(curComment));
			if (curComment[0] != '\0')
			{
				LOGD("[MiniUnzipLib]GetComment success!, curComment=[%s]\n", curComment);
				if (pkgComment != NULL && size != 0)
					strncpy(pkgComment, curComment, size);
				iRet = 1;
			}
			else
			{
				LOGD("[MiniUnzipLib]GetComment failed!\n");
				iRet = 0;
				//LOGE("GetComment failed!");
			}
			//if (pkgComment != NULL && size != 0)unzGetGlobalComment(uf, pkgComment, size);
			unzClose(uf);
		}
		if (fileNameTry != NULL)
			free(fileNameTry);
	}
	return iRet;
}

int MiniUnzip(char * srcZipPath, char * dstUnzipPath, char * password)
{
#ifdef MEM_LEAK_CHECK
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1);
#endif
	int iRet = -1;
	if(NULL == srcZipPath || dstUnzipPath == NULL) return iRet;
	{
#ifdef USEWIN32IOAPI
		zlib_filefunc_def ffunc;
#endif
		unzFile uf=NULL;
		char *fileNameTry = NULL;
		int len = strlen(srcZipPath)+16;
		fileNameTry = (char *)malloc(len);
		memset(fileNameTry, 0, len);
		strcpy(fileNameTry, srcZipPath);

#ifdef USEWIN32IOAPI
		fill_win32_filefunc(&ffunc);
		uf = unzOpen2(srcZipPath,&ffunc);
#else
		uf = unzOpen(srcZipPath);
#endif
		if (uf==NULL)
		{
			strcat(fileNameTry,".zip");
#ifdef USEWIN32IOAPI
			uf = unzOpen2(fileNameTry,&ffunc);
#else
			uf = unzOpen(fileNameTry);
#endif
		}
		if(uf == NULL)
		{
			LOGE("[MiniUnzipLib]UnzOpen failed!uf:NULL!\n");
		}
		else
		{
			LOGD("[MiniUnzipLib]UnzOpen success!\n");
			LOGD("[MiniUnzipLib]SrcZip:%s,DstUnzip:%s\n", srcZipPath, dstUnzipPath);
			iRet = do_extract(uf, 0, 1, dstUnzipPath, password);
			unzClose(uf);
			LOGD("[MiniUnzipLib]DoExtract result:%d!\n", iRet);
			//unzCloseCurrentFile(uf);
		}
		if(fileNameTry != NULL) free(fileNameTry);
	}
#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint(&s2);
	if (_CrtMemDifference(&s3, &s1, &s2)){
		_CrtMemDumpStatistics(&s3);
	}
#endif
	return iRet;
}
