#include "DiskPMQInfo.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "PMQPredict.h"
#include "iniNode.h"
#include "version.h"

piniFileNode_t HDDPMQConfig = NULL;

bool GetHDDEventID(pDiskPMQ diskpmq)
{
	unsigned char nowEventID = EVENTID_CLR;
	unsigned int remainingDay;
	unsigned int maxProg;

	if (diskpmq == NULL)
		return false;

	switch (diskpmq->type)
	{
	case HddTypeUnknown:
		break;

	case SQFlash:
		if (diskpmq->predictVal >= 0.67)
			nowEventID |= EVENT7;
		else if (diskpmq->predictVal >= 0.55)
			nowEventID |= EVENT6;
		else
			nowEventID |= EVENT1;
		break;

	case StdDisk:
		if (diskpmq->smart.smart5.val > SMART5_THR || diskpmq->smart.smart197.val > SMART197_THR)
			nowEventID |= EVENT2;
		else if (diskpmq->smart.smart9.val > SMART9_THR)
			nowEventID |= EVENT3;
		else if (diskpmq->smart.smart187.val > SMART187_THR)
			nowEventID |= EVENT4;
		else if (diskpmq->smart.smart192.val > SMART192_THR)
			nowEventID |= EVENT5;
		else
			nowEventID |= EVENT1;
		break;

	default:
		break;
	}

	if (diskpmq->eventID == nowEventID)
		diskpmq->state = isKeep;
	else
		diskpmq->state = isChange;

	diskpmq->eventID = nowEventID;

	return true;
}

bool GetPrediectVal(pDiskPMQ diskpmq)
{
	unsigned int smartList[6];

	if (diskpmq == NULL)
		return false;

	switch (diskpmq->type)
	{
	case HddTypeUnknown:
		break;

	case SQFlash:
		smartList[0] = diskpmq->smart.smart9.val;
		smartList[1] = diskpmq->smart.smart173.val;
		PMQ_SQFPredict(smartList, diskpmq->max_program, SQF_SMART_LEN, diskpmq->sqfType, &diskpmq->predictVal);
		break;

	case StdDisk:
		smartList[0] = 1;
		smartList[1] = diskpmq->smart.smart5.val;
		smartList[2] = diskpmq->smart.smart9.val;
		smartList[3] = diskpmq->smart.smart187.val;
		smartList[4] = diskpmq->smart.smart192.val;
		smartList[5] = diskpmq->smart.smart197.val;
		PMQ_HddPredict(smartList, HDD_SMART_LEN, &diskpmq->predictVal);
		break;

	default:
		diskpmq->predictVal = 1;
	}
	
	return true;
}

bool GetSMARTFromHDDInfo(pSMART smart, hdd_mon_info_node_t *source)
{
	smart_attri_info_node_t* sourSMARTTemp;
	unsigned int val;

	if (source == NULL)
		return false;

	memset(smart, 0, sizeof(SMART));

	switch (source->hddMonInfo.hdd_type)
	{
	case HddTypeUnknown:
		break;

	case SQFlash:
		smart->smart9.smartNum = 9;
		smart->smart9.val = source->hddMonInfo.power_on_time;
		strcpy(smart->smart9.msg, "Power-On Hours");

		smart->smart173.smartNum = 173;
		smart->smart173.val = source->hddMonInfo.average_program;
		strcpy(smart->smart173.msg, "Erase count");
		break;

	case StdDisk:
		if (source->hddMonInfo.smartAttriInfoList)
		{
			sourSMARTTemp = source->hddMonInfo.smartAttriInfoList->next;
			while (sourSMARTTemp)
			{
				val = ((sourSMARTTemp->sata.attriVendorData[1] & 0xff) << 8)
					+ (sourSMARTTemp->sata.attriVendorData[0] & 0xff);

				switch (sourSMARTTemp->sata.attriType)
				{
				case ReallocatedSectorsCount:
					smart->smart5.smartNum = 5;
					smart->smart5.val = val;
					strcpy(smart->smart5.msg, "Reallocated Sector Count");
					break;

				case PowerOnHoursPOH:
					smart->smart9.smartNum = 9;
					smart->smart9.val = val;
					strcpy(smart->smart9.msg, "Power-On Hours");
					break;
					
				case ReportedUncorrectableErrors:
					smart->smart187.smartNum = 187;
					smart->smart187.val = val;
					strcpy(smart->smart187.msg, "Reported Uncorrectable Errors");
					break;

				case GSenseErrorRate:
					smart->smart191.smartNum = 191;
					smart->smart191.val = val;
					strcpy(smart->smart191.msg, "G-sense Error Rate");
					break;

				case PoweroffRetractCount:
					smart->smart192.smartNum = 192;
					smart->smart192.val = val;
					strcpy(smart->smart192.msg, "Power-off Retract Count");
					break;

				case Temperature:
					smart->smart194.smartNum = 194;
					smart->smart194.val = val;
					strcpy(smart->smart194.msg, "Temperature");
					break;

				case CurrentPendingSectorCount:
					smart->smart197.smartNum = 197;
					smart->smart197.val = val;
					strcpy(smart->smart197.msg, "Current Pending Sector Count");
					break;

				case UncorrectableSectorCount:
					smart->smart198.smartNum = 198;
					smart->smart198.val = val;
					strcpy(smart->smart198.msg, "Uncorrectable Sector Count");
					break;
		
				case UltraDMACRCErrorCount:
					smart->smart199.smartNum = 199;
					smart->smart199.val = val;
					strcpy(smart->smart199.msg, "UltraDMA CRC Error Count");
					break;

				default:
					break;
				}

				sourSMARTTemp = sourSMARTTemp->next; 
			}
		}
		else 
			return false;
		break;

	default:
		break;
	}

	return true;
}

bool GetHDDType(pDiskPMQ diskpmq)
{
	if (diskpmq == NULL)
		return false;

	if (diskpmq->type == SQFlash)
	{
		if (diskpmq->diskname[7] == 'M')
			diskpmq->sqfType = MLC;
		else if (diskpmq->diskname[7] == 'U')
			diskpmq->sqfType = ULC;
		else if (diskpmq->diskname[7] == 'S')
			diskpmq->sqfType = SLC;
	}

	return true;
}

bool UpdatePMQInfoFromHDDInfo(pDiskPMQ diskpmq, hdd_info_t *source)
{
	hdd_mon_info_node_t *sourceTemp;
	pDiskPMQ diskpmqTemp;

	if (source == NULL)
		return false;

	if (diskpmq == NULL)
		diskpmq = (pDiskPMQ)calloc(1, sizeof(DiskPMQ));

	memset(diskpmq, 0, sizeof(diskpmq));

	sourceTemp = source->hddMonInfoList->next;
	if (sourceTemp)
	{
		while (sourceTemp)
		{
			if (strcmp(sourceTemp->hddMonInfo.hdd_name, "") != EQUAL)
			{
				diskpmqTemp = diskpmq;
				
				while (strcmp(diskpmqTemp->diskname, sourceTemp->hddMonInfo.hdd_name) != EQUAL)
				{
					if (strcmp(diskpmqTemp->diskname, "") == EQUAL)
						break;

					if (diskpmqTemp->next == NULL)
					{
						diskpmqTemp->next = (pDiskPMQ)calloc(1, sizeof(DiskPMQ));
						diskpmqTemp = diskpmqTemp->next;
						break;
					}

					diskpmqTemp = diskpmqTemp->next;
				}

				strcpy(diskpmqTemp->diskname, sourceTemp->hddMonInfo.hdd_name);
				diskpmqTemp->type = sourceTemp->hddMonInfo.hdd_type;
				diskpmqTemp->max_program = sourceTemp->hddMonInfo.max_program;

				if (GetHDDType(diskpmqTemp) == false)
				{
					printf("Get HDD type Error!\n");
					return false;
				}

				if (GetSMARTFromHDDInfo(&diskpmqTemp->smart, sourceTemp) == false)
				{
					printf("Get SMART value Error!\n");
					return false;
				}

				if (GetPrediectVal(diskpmqTemp) == false)
				{
					printf("Get prediect value Error!\n");
					return false;
				}

				if (GetHDDEventID(diskpmqTemp) == false)
				{
					printf("Get event ID Error!\n");
					return false;
				}
			}
			
			sourceTemp = sourceTemp->next;
		}
	}
	else
		return false;

	return true;
}

bool SavePMQParameter(pHandlerConfig pmqConfig)
{
	psectionNode_t sectionNode = NULL;
	pkeyNode_t keyNode = NULL;

	if (pmqConfig == NULL)
		return false;

	sectionNode = getSection(HDDPMQConfig, SECTIONNAME);

	keyNode = getKeyinSection(sectionNode, INTERVALKEY);
	sprintf(keyNode->val, "%d", pmqConfig->paramInfo.reportInterval);

	keyNode = getKeyinSection(sectionNode, REPORTKEY);
	if (pmqConfig->paramInfo.enableReport == true)
		sprintf(keyNode->val, "%s", "True");
	else
		sprintf(keyNode->val, "%s", "False");

	Set_iniFile(HDDPMQConfig);

	return true;
}

bool LoadPMQParameter(pHandlerConfig pmqConfig)
{
	char version[INFOSTRLEN];
	psectionNode_t sectionNode = NULL;
	psectionNode_t checkSection = NULL;
	pkeyNode_t keyNode = NULL;
	
	sprintf(version, "%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD);

	HDDPMQConfig = Get_iniFile(pmqConfig->iniDir, pmqConfig->iniFileName);

	if (HDDPMQConfig == NULL)
	{
		HDDPMQConfig = New_iniFile(pmqConfig->iniDir, pmqConfig->iniFileName);
		LoadDefaultConfig(HDDPMQConfig);
		Set_iniFile(HDDPMQConfig);
	}
	else
	{
		checkSection = getSection(HDDPMQConfig, SECTIONNAME);

		if (checkSection == NULL)
		{
			UnInitFileNode(HDDPMQConfig);
			HDDPMQConfig = NULL;
			HDDPMQConfig = New_iniFile(pmqConfig->iniDir, pmqConfig->iniFileName);
			LoadDefaultConfig(HDDPMQConfig);
			Set_iniFile(HDDPMQConfig);
		}
	}
	
	sectionNode = getSection(HDDPMQConfig, SECTIONNAME);
	keyNode = getKeyinSection(sectionNode, INTERVALKEY);
	pmqConfig->paramInfo.reportInterval = atoi(keyNode->val);

	keyNode = getKeyinSection(sectionNode, REPORTKEY);
	if (keyNode->val[0] == 'T' || keyNode->val[0] == 't')
		pmqConfig->paramInfo.enableReport = true;
	else if (keyNode->val[0] == 'F' || keyNode->val[0] == 'f')
		pmqConfig->paramInfo.enableReport = false;

	strcpy(pmqConfig->infoConfig.type, "PMQ");
	strcpy(pmqConfig->infoConfig.name, "HDD_PMQ");
	strcpy(pmqConfig->infoConfig.description, "This service is HDD PMQ Service");
	strcpy(pmqConfig->infoConfig.version, version);
	pmqConfig->infoConfig.confidence = 83.12;
	strcpy(pmqConfig->infoConfig.update, "");
	pmqConfig->infoConfig.eventNotify = true;

	return true;
}

bool UnInitPMQParameter()
{
	if (HDDPMQConfig == NULL)
		return false;

	if (UnInitFileNode(HDDPMQConfig))
		return true;
	else
		return false;
}




