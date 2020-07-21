#include <mntent.h>
#include <sys/vfs.h>
#include <string.h>
#include <stdlib.h>
#include "util_storage.h"

#include <stdbool.h>

/*
change parameter
true  : / to -
false : - to /
*/
char* ReplaceDiskName(char* name , bool change)
{
	char* newname = NULL;
	if(name == NULL)
		return newname;
	
	newname = strdup(name);
	
	if(change == true)
	{
		char *p = newname;
		while(*p)
		{
			if(*p == '/')
				*p = '-';
			++p;
		}
	}
	else
	{
		char *p = newname;
		while(*p)
		{
			if(*p == '-')
				*p = '/';
			++p;
		}
	}
	return newname;
}

DiskItemList* util_storage_getdisklist()
{
	int diskIndex = 0;
	DiskItemList* disklist = NULL;
	DiskItemList* last = NULL;
	struct mntent *ent;
	FILE *aFile = setmntent("/etc/mtab", "r");
		
	if (aFile == NULL)
		return disklist;
	
	while (NULL != (ent = getmntent(aFile)))
	{
		if ((strncmp(ent->mnt_fsname, "/dev", 4) == 0) && diskIndex < MAX_ITEM_COUNT)
		{
			//char* drive = ReplaceDiskName(ent->mnt_dir , true);

			DiskItemList* disk = malloc(sizeof(DiskItemList));
			memset(disk, 0, sizeof(DiskItemList));
			disk->disk = malloc(sizeof(DiskInfoItem));
			memset(disk->disk, 0, sizeof(DiskInfoItem));
			// if(drive)
			// {
			// 	strcpy(disk->disk->name, drive);
			// 	free(drive);
			// }
			strcpy(disk->disk->name, ent->mnt_dir);
			if(disklist == NULL)
				last = disklist = disk;
			else{
				while(last)
				{
					if(last->next)
						last = last->next;
					else
						break;
				}

				last->next = disk;
			}

			diskIndex++;
		}
	}
	endmntent(aFile);
	return disklist;
}

void util_storage_freedisklist(DiskItemList* list)
{
	while(list)
	{
		DiskItemList* current = list;
		list = list->next;
		if(current->disk)
			free(current->disk);
		free(current);
	}
}

unsigned long long util_storage_gettotaldiskspace(char* drive)
{
	struct statfs diskInfo;
    unsigned long long value = 0;
	int result = 0;
	//char* disk_name = ReplaceDiskName(drive , false);
	//if(disk_name)
	//{
		//if (statfs(disk_name, &diskInfo) == 0)
		result = statfs(drive, &diskInfo);
		if (result == 0)
		{
			printf("statfs %s bsize: %lld, nblock: %lld", drive, (unsigned long long)diskInfo.f_bsize, (unsigned long long)diskInfo.f_blocks);
			value = (unsigned long long)diskInfo.f_bsize * (unsigned long long)diskInfo.f_blocks;	
		} 
		else
		{
			printf("statfs return error: %d", result);
		}
		
		//free(disk_name);
	//}
	return value;
}

unsigned long long util_storage_getfreediskspace(char* drive)
{
	struct statfs diskInfo;
	unsigned long long value = 0;
	int result = 0;
 	//char* disk_name = ReplaceDiskName(drive , false);
	//if(disk_name)
	//{
		//if (statfs(disk_name, &diskInfo) == 0)
		result = statfs(drive, &diskInfo);
		if (result == 0)
		{
			printf("statfs %s bsize: %lld, nblock: %lld", drive, (unsigned long long)diskInfo.f_bsize, (unsigned long long)diskInfo.f_blocks);
			value = (unsigned long long)diskInfo.f_bsize * (unsigned long long)diskInfo.f_bavail;	
		}
		else
		{
			printf("statfs return error: %d", result);
		}
		//free(disk_name);
	//}
	return value;
}

int util_storage_getdiskspeed(DiskItemList* list)
{
	/*TODO*/
	return 0;
}