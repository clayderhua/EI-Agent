#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util_path.h"
#include "WISEPlatform.h"
#include "moduleconfig.h"

#define MODULE_NUM "ModuleNum"
#define MODULE_NAME "ModuleName"
#define MODULE_PATH "ModulePath"
#define MODULE_ENABLE "ModuleEnable"

typedef struct module_t{
	int index;
	char name[32];
	char path[MAX_PATH];
	char enable[8];
}MODULE_ST;

bool findModule(char* configFile, char* name, struct module_t* module)
{
	char sValue[128] = {0};
	char moduleFile[MAX_PATH]={0};
	if(module == NULL) return false;
	if(module_get(configFile, MODULE_NUM, sValue, sizeof(sValue))>0) 
	{	
		int iNum = atoi(sValue);
		if( iNum > 0 ) 
		{
			for( int i=0; i < iNum; i++ ) 
			{
				char sItemName[32]={0};
				memset(sValue,0,sizeof(sValue));
				snprintf( sItemName, sizeof( sItemName ), "%s%d",MODULE_NAME,i+1 );		
				if( module_get(configFile, sItemName, sValue, sizeof(sValue)) > 0 ) 
				{
					if(strcmp(sValue, name) == 0)
					{
						module->index = i;
						strcpy(module->name, sValue);
						
						memset(sItemName,0,sizeof(sItemName));
						snprintf( sItemName, sizeof( sItemName ), "%s%d",MODULE_ENABLE,i+1 );
						module_get(configFile, sItemName, module->enable, sizeof(module->enable));
						

						memset(sItemName,0,sizeof(sItemName));
						snprintf( sItemName, sizeof( sItemName ), "%s%d",MODULE_PATH,i+1 );
						module_get(configFile, sItemName, module->path, sizeof(module->path));

						return true;
					}
				}
			}
		}
		else {
			printf("Module XML is not any handler module need to load\n");
		}
	} else {
		printf("Can't not load Module XML Config %s\n", configFile );
	}
	return false;
}

void addModule(char* configFile, char* name, char* path, char* enable)
{
	char sValue[128] = {0};
	if(module_get(configFile, MODULE_NUM, sValue, sizeof(sValue))>0) 
	{	
		char buff[32] = {0};
		int iNum = atoi(sValue);
		iNum++;
		sprintf(buff, "%d", iNum);
		module_set(configFile, MODULE_NUM,  buff);
		memset(buff,0,sizeof(buff));

		snprintf( buff, sizeof( buff ), "%s%d",MODULE_NAME,iNum );
		module_set(configFile, buff, name);
		memset(buff,0,sizeof(buff));
		
		snprintf( buff, sizeof( buff ), "%s%d",MODULE_PATH,iNum );
		module_set(configFile, buff, path);
		memset(buff,0,sizeof(buff));

		snprintf( buff, sizeof( buff ), "%s%d",MODULE_ENABLE,iNum );
		module_set(configFile, buff, enable);
		memset(buff,0,sizeof(buff));
	}
	
}

void copyModule(char* sourceFolder, char* targetFolder, char* module)
{
	char name[MAX_PATH] = {0};
	char path[MAX_PATH] = {0};
	char sourceFile[MAX_PATH] = {0};
	
	util_split_path_file(module, path, name);
	util_path_combine(sourceFile, sourceFolder, name);
	if(util_is_file_exist(sourceFile))
	{
		char cmd[512] = {0};
		sprintf(cmd, "cp -d %s* %s", sourceFile, targetFolder);
printf("copy to %s\n",cmd);
		system(cmd);
	}
}



void merge(char* target, char* source)
{
	char sValue[128] = {0};
	char sItemName[32]={0};
	char configFile[MAX_PATH]={0};
	char targetFile[MAX_PATH]={0};
	util_path_combine(configFile,  source, DEF_CONFIG_FILE_NAME);
	util_path_combine(targetFile,  target, DEF_CONFIG_FILE_NAME);
	if(module_get(configFile, MODULE_NUM, sValue, sizeof(sValue))>0) 
	{	
		int iNum = atoi(sValue);
		if( iNum > 0 ) 
		{

			for( int i=0; i < iNum; i++ ) 
			{
				char stModuleName[64]={0};
				char stModuleEnable[8]={0};
				char moduleFile[MAX_PATH]={0};
				struct module_t curmodule;

				memset(sItemName,0,sizeof(sItemName));

				// 1. Name
				snprintf( sItemName, sizeof( sItemName ), "%s%d",MODULE_NAME,i+1 );
				module_get(configFile, sItemName, stModuleName, sizeof(stModuleName));
				memset(sItemName,0,sizeof(sItemName));

				// 2. Enable?
				snprintf( sItemName, sizeof( sItemName ), "%s%d",MODULE_ENABLE,i+1 );
				module_get(configFile, sItemName, stModuleEnable, sizeof(stModuleEnable));
				memset(sItemName,0,sizeof(sItemName));
				
				// 3. Path
				snprintf( sItemName, sizeof( sItemName ), "%s%d",MODULE_PATH,i+1 );
				module_get(configFile, sItemName, moduleFile, sizeof(moduleFile));
				memset(sItemName,0,sizeof(sItemName));
				
				if(findModule(targetFile, stModuleName, &curmodule))
				{
					if(strcasecmp(stModuleEnable, curmodule.enable) != 0)
					{
						snprintf( sItemName, sizeof( sItemName ), "%s%d",MODULE_ENABLE,i+1 );
						module_set(targetFile, sItemName, stModuleEnable);
						memset(sItemName,0,sizeof(sItemName));
					}
					continue;
				}

				addModule(targetFile, stModuleName, moduleFile, stModuleEnable);
				copyModule(source, target, moduleFile); /*does not work*/
			}	// End of For 
		}	// End of cfg_get ( MODULE_NUM )
		else {
			printf("Module XML is not any handler module need to load\n");
		}
	} else {
		printf("Can't not load Module XML Config %s\n", configFile );
	}
}

int main(int argc, char * argv[])
{
	if(argc > 2)
	{
		char sourcePath[MAX_PATH] = {0};
		char targetPath[MAX_PATH] = {0};
	
		
		strcpy(targetPath, argv[1]);
		strcpy(sourcePath, argv[2]);
		printf("Merge to %s\n",targetPath);
		printf("Merge from %s\n", sourcePath);
		merge(targetPath, sourcePath);	
	}
	else
		printf("Usage: ModuleMerge target_folder source_folder\n");
	return 0;
}
