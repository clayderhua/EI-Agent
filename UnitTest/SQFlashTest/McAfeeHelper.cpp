#include "McAfeeHelper.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <curl/curl.h>
#include "WISEPlatform.h"
#include "util_path.h"
#include "util_libloader.h"
#include "mcafeelib.h"

/*
typedef Status_t (__stdcall* PInitializeMcAfeeEngine)();
typedef Status_t (__stdcall* PUnitializeMcAfeeEngine)();
typedef Status_t (__stdcall* PScanDrives)(char* driverletters, int amount, ScanResult& result);
typedef Status_t (__stdcall* PGetCurrentVirusDefVersion)(int& version);
typedef Status_t (__stdcall* PUpdateAutoInfo)(char* updateDirectory, int currentVersion, UpdateInfo& info);
typedef Status_t (__stdcall* PUpdateAutoVirusDef)(char* updateDirectory, int currentVersion);
typedef Status_t (__stdcall* PSetCallback)(MessageCallback messageCallback);

PInitializeMcAfeeEngine pInitializeMcAfeeEngine = NULL;
PUnitializeMcAfeeEngine pUnitializeMcAfeeEngine = NULL;
PScanDrives pScanDrives = NULL;
PGetCurrentVirusDefVersion pGetCurrentVirusDefVersion = NULL;
PUpdateAutoInfo pUpdateAutoInfo = NULL;
PUpdateAutoVirusDef pUpdateAutoVirusDef = NULL;
PSetCallback pSetCallback = NULL;
*/
char g_Updatefolder[MAX_PATH] = { 0 };
char g_McAfeeDir[MAX_PATH] = { 0 };
int g_Version = 0;
void* g_hMcAfeeLib = NULL;
/*
bool IsExistMcAfeeLib(char* filepath)
{
	if (!util_is_file_exist(filepath))
		return false;

	if (util_dlexist(filepath))
		return true;
	else
	{
		char* error = util_dlerror();
		printf(error);
		util_dlfree_error(error);
	}
	return false;
}

void GetMcAfeeLibFunction(void* hMcAfeeLib)
{
	if (hMcAfeeLib != NULL)
	{
		pInitializeMcAfeeEngine = (PInitializeMcAfeeEngine)util_dlsym(hMcAfeeLib, "_InitializeMcAfeeEngine@0");
		pUnitializeMcAfeeEngine = (PUnitializeMcAfeeEngine)util_dlsym(hMcAfeeLib, "UnitializeMcAfeeEngine");
		pScanDrives = (PScanDrives)util_dlsym(hMcAfeeLib, "ScanDrives");
		pGetCurrentVirusDefVersion = (PGetCurrentVirusDefVersion)util_dlsym(hMcAfeeLib, "GetCurrentVirusDefVersion");
		pUpdateAutoInfo = (PUpdateAutoInfo)util_dlsym(hMcAfeeLib, "UpdateAutoInfo");
		pUpdateAutoVirusDef = (PUpdateAutoVirusDef)util_dlsym(hMcAfeeLib, "UpdateAutoVirusDef");
		pSetCallback = (PSetCallback)util_dlsym(hMcAfeeLib, "SetCallback");
	}
}

bool StartupMcAfeeLib(char* filepath)
{
	bool bRet = false;
	void* hMcAfeeLib = NULL;
	if (util_dlopen(filepath, &hMcAfeeLib))
	{
		g_hMcAfeeLib = hMcAfeeLib;
		GetMcAfeeLibFunction(hMcAfeeLib);
		bRet = true;
	}
	else
	{
		char* error = util_dlerror();
		printf(error);
		util_dlfree_error(error);
	}
	return bRet;
}

bool CleanupMcAfeeLib()
{
	bool bRet = true;

	if (g_hMcAfeeLib != NULL)
	{
		util_dlclose(g_hMcAfeeLib);
		g_hMcAfeeLib = NULL;
		pInitializeMcAfeeEngine = NULL;
		pUnitializeMcAfeeEngine = NULL;
		pScanDrives = NULL;
		pGetCurrentVirusDefVersion = NULL;
		pUpdateAutoInfo = NULL;
		pUpdateAutoVirusDef = NULL;
		pSetCallback = NULL;
	}

	return bRet;
}
*/

static size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
	return written;
}

void fileDownload(char* url, char* outfilename)
{
	CURL* curl;
	FILE* fp;
	CURLcode res;
	if (url == NULL || outfilename == NULL)
		return;

	curl = curl_easy_init();
	if (curl)
	{
		fp = fopen(outfilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
	}
}

void WINAPI mc_MessageCallback(ScanLog log)
{

}

bool mc_Initialize(char* filepath, int* version)
{
	bool bRet = false;
	char fName[MAX_PATH] = { 0 };
	char fullPath[MAX_PATH] = { 0 };
	/*
	if (!IsExistMcAfeeLib(filepath))
		return bRet;

	if(!StartupMcAfeeLib(filepath))
		return bRet;

	if (pInitializeMcAfeeEngine == NULL || pUnitializeMcAfeeEngine == NULL)
		return bRet;

		*/

	util_path_combine(fName, filepath, "mcscan32.dll");

	if (util_is_file_exist(fName))
	{
		char tmpPath[MAX_PATH] = { 0 };
		char tmpName[MAX_PATH] = { 0 };
		if(util_get_full_path(fName, tmpPath, sizeof(tmpPath)))
			util_split_path_file(tmpPath, fullPath, tmpName);
	}
	else
	{
		return bRet;
	}

	if(InitializeMcAfeeEngine(fullPath) != ADV_STATUS_SUCCESS)
		return bRet;
	if(strcmp(g_McAfeeDir, filepath))
		strcpy(g_McAfeeDir, fullPath);
	if (SetCallback(&mc_MessageCallback) != ADV_STATUS_SUCCESS)
		return bRet;

	if (version)
	{
		g_Version = 0;
		//if (pGetCurrentVirusDefVersion)
		{
			if (GetCurrentVirusDefVersion(g_Version) == ADV_STATUS_SUCCESS)
			{
				*version = g_Version;
			}
		}
	}
	//util_split_path_file(filepath, g_McAfeeDir, fName);
	
	bRet = true;
	return bRet;
}

void mc_Uninitialize()
{
	UnitializeMcAfeeEngine(true);
	//CleanupMcAfeeLib();
}

bool mc_GetCurrentVirusVersion(int* version)
{
	bool bRet = false;
	if (version)
	{
		g_Version = 0;
		//if (pGetCurrentVirusDefVersion)
		{
			if (GetCurrentVirusDefVersion(g_Version) == ADV_STATUS_SUCCESS)
			{
				*version = g_Version;
				bRet = true;
			}
		}
	}
	return bRet;
}

bool mc_StartScan(char* driverletters, int amount, int *total, int * infected)
{
	bool bRet = false;
	ScanResult result;

	if (total == NULL || infected == NULL)
		return bRet;

	//if (pScanDrives)
		bRet = ScanDrives(driverletters, amount, result) == ADV_STATUS_SUCCESS;
	if (bRet)
	{
		*total = result.total_count;
		*infected = result.infected_count;
	}
	
	return bRet;
}

bool mc_CheckUpdate()
{
	bool bRet = false;
	//if (pUpdateAutoInfo)
	{
		UpdateInfo info;
		//char buf[MAX_PATH] = { 0 };
		//if (util_temp_path_get(buf, MAX_PATH) > 0)
		{
			char updateFile[MAX_PATH] = { 0 };
			char url[64] = "http://update.nai.com/Products/CommonUpdater/gdeltaavv.ini";
			if (strlen(g_Updatefolder) == 0)
				util_path_combine(g_Updatefolder, g_McAfeeDir, "new_dat");
			if(!util_is_dir_exist(g_Updatefolder))
				util_create_directory(g_Updatefolder);
			util_path_combine(updateFile, g_Updatefolder, "gdeltaavv.ini");
			fileDownload(url, updateFile);

			if (UpdateAutoInfo(g_Updatefolder, g_Version, info) != ADV_STATUS_SUCCESS)
				return bRet;

			if (info.canUpdate == 0)
			{
				for (int i = 0; i < info.numUpdateFile; i++)
				{
					char tmpFile[MAX_PATH] = { 0 };
					util_path_combine(tmpFile, g_Updatefolder, info.updateFileList[i]);
					char tmpUrl[MAX_PATH] = { 0 };
					if (util_is_file_exist(tmpFile))
						continue;
					sprintf(tmpUrl, "http://update.nai.com/Products/CommonUpdater/%s", info.updateFileList[i]);
					fileDownload(tmpUrl, tmpFile);
					if(!util_is_file_exist(tmpFile))
						return bRet;
				}

				bRet = true;
			}
		}		
	}
	return bRet;
}

int file_iterate_cb(unsigned int depth, const char* filePath, void* userData)
{
	char tmp[MAX_PATH] = { 0 };
	char target[MAX_PATH] = { 0 };
	char name[MAX_PATH] = { 0 };
	char path[MAX_PATH] = { 0 };
	util_path_combine(tmp, g_McAfeeDir, "dat");
	util_split_path_file(filePath, path, name);
	util_path_combine(target, tmp, name);
	util_copy_file(filePath, target);
	return 0;
}

bool mc_Update(int *newVersion)
{
	bool bRet = false;

	/* Close Engine */
	UnitializeMcAfeeEngine(false);

	/* Update Automatically */
	//if (pUpdateAutoVirusDef)
	{
		bRet = UpdateAutoVirusDef(g_Updatefolder, g_Version);
	}

	//if (bRet)
	{
		/* Move new dat files */
		util_file_iterate(g_Updatefolder, "*.dat", 1, false, file_iterate_cb, NULL);
	}

	if (util_is_dir_exist(g_Updatefolder))
		util_rm_dir(g_Updatefolder);
	
	/*Initialize Engine*/
	bRet = InitializeMcAfeeEngine(g_McAfeeDir) == ADV_STATUS_SUCCESS;

	if (!bRet)
		return bRet;

	//if(pSetCallback)
		SetCallback(&mc_MessageCallback);

	if (newVersion)
	{
		//if (pGetCurrentVirusDefVersion)
		{
			if (GetCurrentVirusDefVersion(g_Version) == ADV_STATUS_SUCCESS)
				*newVersion = g_Version;
		}
	}
	return bRet;
}