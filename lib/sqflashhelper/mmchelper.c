#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "WISEPlatform.h"
#include "util_string.h"
#include "util_path.h"
#include <fcntl.h>

#include "mmchelper.h"


//---------------------device monitor mmc info list fuction ------------------------------------
mmc_mon_info_node_t * FindMMCInfoNode(mmc_mon_info_list mmcInfoList, char * mmcName)
{
	mmc_mon_info_node_t * findNode = NULL, * head = NULL;
	if(mmcInfoList == NULL || mmcName == NULL) return findNode;
	head = mmcInfoList;
	findNode = head->next;
	while(findNode)
	{
		if(!strcmp(findNode->mmcMonInfo.mmc_name, mmcName)) break;
		findNode = findNode->next;
	}

	return findNode;
}

mmc_mon_info_node_t * FindMMCInfoNodeWithID(mmc_mon_info_list mmcInfoList, int mmcIndex)
{
	mmc_mon_info_node_t * findNode = NULL, * head = NULL;
	if(mmcInfoList == NULL) return findNode;
	head = mmcInfoList;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->mmcMonInfo.mmc_index == mmcIndex) break;
		findNode = findNode->next;
	}
	return findNode;
}

mmc_mon_info_list mmc_CreateMMCInfoList()
{
	mmc_mon_info_node_t * head = NULL;
	head = (mmc_mon_info_node_t *)malloc(sizeof(mmc_mon_info_node_t));
	memset(head, 0, sizeof(mmc_mon_info_node_t));
	if(head)
	{
		head->next = NULL;
		memset(head->mmcMonInfo.mmc_name, 0, sizeof(head->mmcMonInfo.mmc_name));
		head->mmcMonInfo.mmc_index = -1;
		memset(head->mmcMonInfo.mmc_version, 0, sizeof(head->mmcMonInfo.mmc_version));
		memset(head->mmcMonInfo.mmc_health, 0, sizeof(head->mmcMonInfo.mmc_health));
		head->mmcMonInfo.mmc_health_percent = 100;

	}
	return head;
}

int InsertMMCInfoNode(mmc_mon_info_list mmcInfoList, mmc_mon_info_t * mmcMonInfo)
{
	int iRet = -1;
	mmc_mon_info_node_t * newNode = NULL, * findNode = NULL, * head = NULL;
	if(mmcInfoList == NULL || mmcMonInfo == NULL) return iRet;
	head = mmcInfoList;
	findNode = FindMMCInfoNodeWithID(head, mmcMonInfo->mmc_index);
	if(findNode == NULL)
	{
		newNode = (mmc_mon_info_node_t *)malloc(sizeof(mmc_mon_info_node_t));
		memset(newNode, 0, sizeof(mmc_mon_info_node_t));
		strcpy(newNode->mmcMonInfo.mmc_name, mmcMonInfo->mmc_name);
		newNode->mmcMonInfo.mmc_index = mmcMonInfo->mmc_index;
		strcpy(newNode->mmcMonInfo.mmc_version, mmcMonInfo->mmc_version);
		strcpy(newNode->mmcMonInfo.mmc_health, mmcMonInfo->mmc_health);
		newNode->mmcMonInfo.mmc_health_percent = mmcMonInfo->mmc_health_percent;

		newNode->next = head->next;
		head->next = newNode;
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

int InsertMMCInfoNodeEx(mmc_mon_info_list mmcInfoList, mmc_mon_info_t * mmcMonInfo)
{
	int iRet = -1;
	mmc_mon_info_node_t * newNode = NULL, /* * findNode = NULL, */* head = NULL;
	if(mmcInfoList == NULL || mmcMonInfo == NULL) return iRet;
	head = mmcInfoList;
	newNode = (mmc_mon_info_node_t *)malloc(sizeof(mmc_mon_info_node_t));
	memset(newNode, 0, sizeof(mmc_mon_info_node_t));
	strcpy(newNode->mmcMonInfo.mmc_name, mmcMonInfo->mmc_name);
	newNode->mmcMonInfo.mmc_index              = mmcMonInfo->mmc_index;
	strcpy(newNode->mmcMonInfo.mmc_version, mmcMonInfo->mmc_version);
	strcpy(newNode->mmcMonInfo.mmc_health, mmcMonInfo->mmc_health);
	newNode->mmcMonInfo.mmc_health_percent = mmcMonInfo->mmc_health_percent;

	newNode->next = head->next;
	head->next = newNode;
	iRet = 0;
	return iRet;
}

int DeleteMMCInfoNode(mmc_mon_info_list mmcInfoList, char * mmcName)
{
	int iRet = -1;
	mmc_mon_info_node_t * delNode = NULL, * head = NULL;
	mmc_mon_info_node_t * p = NULL;
	if(mmcInfoList == NULL || mmcName == NULL) return iRet;
	head = mmcInfoList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(!strcmp(delNode->mmcMonInfo.mmc_name, mmcName))
		{
			p->next = delNode->next;
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		p = delNode;
		delNode = delNode->next;
	}
	if(iRet == -1) iRet = 1;
	return iRet;
}

int DeleteMMCInfoNodeWithID(mmc_mon_info_list mmcInfoList, int mmcIndex)
{
	int iRet = -1;
	mmc_mon_info_node_t * delNode = NULL, * head = NULL;
	mmc_mon_info_node_t * p = NULL;
	if(mmcInfoList == NULL) return iRet;
	head = mmcInfoList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->mmcMonInfo.mmc_index == mmcIndex)
		{
			p->next = delNode->next;
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		p = delNode;
		delNode = delNode->next;
	}
	if(iRet == -1) iRet = 1;
	return iRet;
}

int DeleteAllMMCInfoNode(mmc_mon_info_list mmcInfoList)
{
	int iRet = -1;
	mmc_mon_info_node_t * delNode = NULL, *head = NULL;
	if(mmcInfoList == NULL) return iRet;
	head = mmcInfoList;
	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		free(delNode);
		delNode = head->next;
	}
	iRet = 0;
	return iRet;
}

void mmc_DestroyMMCInfoList(mmc_mon_info_list mmcInfoList)
{
	if(NULL == mmcInfoList) return;
	DeleteAllMMCInfoNode(mmcInfoList);
	free(mmcInfoList); 
	mmcInfoList = NULL;
}

bool IsMMCInfoListEmpty(mmc_mon_info_list mmcInfoList)
{
	bool bRet = true;
	mmc_mon_info_node_t * curNode = NULL, *head = NULL;
	if(mmcInfoList == NULL) return bRet;
	head = mmcInfoList;
	curNode = head->next;
	if(curNode != NULL) bRet = false;
	return bRet;
}
//----------------------------------------------------------------------------------------------


#ifdef WIN32
bool mmc_GetMMCInfo(mmc_info_t * pMMCInfo)
{
	bool bRet = false;
	return bRet ;
}

#else
#include <sys/ioctl.h>
#include "mmc.h"

#define  MAX_DETECT_DEVICE 			10
#define  SDA_DEV_TYPE				"mmcblk"
int g_mmcblk_mask[MAX_DETECT_DEVICE] = { 0 };

static int read_extcsd(int fd, char *ext_csd)
{
	int ret = -1;
	struct mmc_ioc_cmd idata;

	memset(&idata, 0, sizeof(idata));
	memset(ext_csd, 0, sizeof(char) * 512);
	idata.write_flag = 0;
	idata.opcode = MMC_SEND_EXT_CSD;
	idata.arg = 0;
	idata.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	idata.blksz = 512;
	idata.blocks = 1;
	mmc_ioc_cmd_set_data(idata, ext_csd);

	ret = ioctl(fd, MMC_IOC_CMD, &idata);
	if (ret)
		perror("ioctl");

	return ret;
}

static bool GetMmcMonInfoFromExtCSD(mmc_mon_info_t * mmcMonInfo, char * ext_csd)
{
	bool bRet = false;
	const char *version;
	const char *health;
	int liftTime;
	int ext_csd_tmp;
	if(mmcMonInfo == NULL || ext_csd == NULL) return bRet;
	
	// mmc_version EXT_CSD_REV ext_csd[192];
	ext_csd_tmp = ext_csd[EXT_CSD_REV];
	switch (ext_csd_tmp) {
	case 8:
		version = "5.1";
		break;
	case 7:
		version = "5.0";
		break;
	case 6:
		version = "4.5";
		break;
	case 5:
		version = "4.41";
		break;
	case 3:
		version = "4.3";
		break;
	case 2:
		version = "4.2";
		break;
	case 1:
		version = "4.1";
		break;
	case 0:
		version = "4.0";
		break;
	default:
		version = "Unknown";;
	}

	if(strncmp(version, "Unk", 3) == 0)
	{
		sprintf(mmcMonInfo->mmc_version, "%s", version);
	}
	else
	{
	    sprintf(mmcMonInfo->mmc_version, "Rev 1.%d (MMC %s)", ext_csd_tmp, version);
	}
	
	//mmc_health EXT_CSD_PRE_EOL_INFO ext_csd [267]
	ext_csd_tmp = ext_csd[EXT_CSD_PRE_EOL_INFO];
	switch (ext_csd_tmp) {
	case 3:
		health = "Urgent";
		break;
	case 2:
		health = "Warning";
		break;
	case 1:
		health = "Normal";
		break;
	default:
		health = "Unknown";;
	}

	sprintf(mmcMonInfo->mmc_health, "%s", health);
	
	// mmc_health_percent 
	// EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B 	269	
	// EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A 	268
	if(ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A] > ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B])
		liftTime = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A];
	else
		liftTime = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B];

	mmcMonInfo->mmc_health_percent = (11 - liftTime) * 10;
	
	bRet = true;
	return bRet;
}

static int GetMmcMonInfo(mmc_mon_info_list mmcMonInfoList, char * devType)
{
	int mmcCnt = 0, iRet = -1;
	int i = 0;
	int fd;
	char ext_csd[512];
	
	if(mmcMonInfoList == NULL || devType == NULL) return mmcCnt;
	{
		for(i = 0; i < MAX_DETECT_DEVICE; i++)
		{
			char devPath[256] = {0};
			if (g_mmcblk_mask[i])
				continue;
			snprintf(devPath, sizeof(devPath), "/dev/%s%d", devType, i);
			fd = open(devPath, O_RDONLY);
			if (fd < 0) 
			{
				g_mmcblk_mask[i] = 1;
				continue;
			}
			
			iRet = read_extcsd(fd, ext_csd);
			if(iRet == 0)
			{
				mmc_mon_info_t * pCurMmcMonInfo = NULL;
				pCurMmcMonInfo = (mmc_mon_info_t *)malloc(sizeof(mmc_mon_info_t));
				memset((char*)pCurMmcMonInfo, 0, sizeof(mmc_mon_info_t));
				
				//dump_ext_csd(ext_csd);
				sprintf(pCurMmcMonInfo->mmc_name, "%s%d", devType, i);
				pCurMmcMonInfo->mmc_index = i;
				GetMmcMonInfoFromExtCSD(pCurMmcMonInfo, ext_csd);
				InsertMMCInfoNodeEx(mmcMonInfoList, pCurMmcMonInfo);

				free(pCurMmcMonInfo);
				pCurMmcMonInfo = NULL;
				mmcCnt++;
			}
			else
			{
				g_mmcblk_mask[i] = 1;
			}
			close(fd);
			
		}
	}
	
	return mmcCnt;
}

bool mmc_GetMMCInfo(mmc_info_t * pMMCInfo)
{
	bool bRet = false;
	if(pMMCInfo == NULL) return bRet;

	{
		int tmpMmcCnt = 0;
		pMMCInfo->mmcCount = 0;
		if(pMMCInfo->mmcMonInfoList == NULL)
		{
			pMMCInfo->mmcMonInfoList = mmc_CreateMMCInfoList();
		}
		else
		{
			DeleteAllMMCInfoNode(pMMCInfo->mmcMonInfoList);
		}

		//mmcIndex = 0;
		tmpMmcCnt = GetMmcMonInfo(pMMCInfo->mmcMonInfoList, SDA_DEV_TYPE);
		pMMCInfo->mmcCount += tmpMmcCnt;

		bRet = true;
	}
	return bRet ;
}
#endif


//----------------------------------------------------------------------------------------------

