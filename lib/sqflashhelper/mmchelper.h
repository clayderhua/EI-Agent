#ifndef _MMC_HELPER_H_
#define _MMC_HELPER_H_

#include <stdbool.h>

//#define INVALID_DEVMON_VALUE  (-999)

typedef struct{
	char mmc_name[128];
	int mmc_index; 
	char mmc_version[64];
	char mmc_health[64];
	unsigned int mmc_health_percent;
}mmc_mon_info_t;

typedef struct mmc_mon_info_node_t{
	mmc_mon_info_t mmcMonInfo;
	struct mmc_mon_info_node_t * next;
}mmc_mon_info_node_t;

typedef mmc_mon_info_node_t * mmc_mon_info_list;

typedef struct{
	int mmcCount;
	mmc_mon_info_list mmcMonInfoList;
}mmc_info_t;

#ifdef __cplusplus
extern "C" {
#endif

	mmc_mon_info_list mmc_CreateMMCInfoList();

	void mmc_DestroyMMCInfoList(mmc_mon_info_list mmcInfoList);

	bool mmc_GetMMCInfo(mmc_info_t * pMMCInfo);

#ifdef __cplusplus
}
#endif


#endif /*_MMC_HELPER_H_*/