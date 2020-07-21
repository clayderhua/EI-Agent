#include "GenMessage.h"
#include "DiskPMQInfo.h"
#include "IoTMessageGenerate.h"
#include "cJSON.h"
#include "PMQPredict.h"

GenStatus_t PMQMsgSetParam(MSG_CLASSIFY_T *msg, pParamInfo info)
{
	MSG_CLASSIFY_T* *groupTemp = NULL;
	MSG_ATTRIBUTE_T *attrTemp = NULL;

	if (info == NULL)
		return GenFail;

	groupTemp = IoT_FindGroup(msg, PARAMETER_GROUP_NAME);
    if (groupTemp == NULL)
        groupTemp = IoT_AddGroup(msg, PARAMETER_GROUP_NAME);

	attrTemp = IoT_FindSensorNode(groupTemp, "report interval");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "report interval");
	IoT_SetDoubleValueWithMaxMin(attrTemp, info->reportInterval, IoT_READWRITE, 3600, 10, "sec");

	attrTemp = IoT_FindSensorNode(groupTemp, "enable report");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "enable report");
	IoT_SetBoolValue(attrTemp, info->enableReport, IoT_READWRITE);

	return GenSuccess;
}

GenStatus_t PMQMsgSetAction(MSG_CLASSIFY_T *msg, pActionInfo info)
{
	MSG_CLASSIFY_T* *groupTemp = NULL;
	MSG_ATTRIBUTE_T *attrTemp = NULL;

	if (info == NULL)
		return GenFail;

	groupTemp = IoT_FindGroup(msg, ACTION_GROUP_NAME);
    if (groupTemp == NULL)
        groupTemp = IoT_AddGroup(msg, ACTION_GROUP_NAME);

	attrTemp = IoT_FindSensorNode(groupTemp, "a1");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "a1");
    IoT_SetBoolValue(attrTemp, false, IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Lower system temperature ( < 40 Celsius ).");

	attrTemp = IoT_FindSensorNode(groupTemp, "a2");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "a2");
    IoT_SetBoolValue(attrTemp, false, IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Backup data to new disk.");

	attrTemp = IoT_FindSensorNode(groupTemp, "a3");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "a3");
    IoT_SetBoolValue(attrTemp, false, IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Check power source.");

	attrTemp = IoT_FindSensorNode(groupTemp, "ActionLog");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "ActionLog");
    IoT_SetStringValue(attrTemp, "", IoT_READONLY);

	return GenSuccess;
}

GenStatus_t PMQMsgSetEvent(MSG_CLASSIFY_T *msg, pEventInfo info)
{
	MSG_CLASSIFY_T* *groupTemp = NULL;
	MSG_ATTRIBUTE_T *attrTemp = NULL;

	if (info == NULL)
		return GenFail;

	groupTemp = IoT_FindGroup(msg, EVENT_GROUP_NAME);
	if (groupTemp == NULL)
        groupTemp = IoT_AddGroup(msg, EVENT_GROUP_NAME);
	
	attrTemp = IoT_FindSensorNode(groupTemp, "e1");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "e1");
    IoT_SetStringValue(attrTemp, "HDD back to Normal.", IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "actionlist", "");

	attrTemp = IoT_FindSensorNode(groupTemp, "e2");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "e2");
    IoT_SetStringValue(attrTemp, "Over temperature.", IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "actionlist", "a1");

	attrTemp = IoT_FindSensorNode(groupTemp, "e3");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "e3");
    IoT_SetStringValue(attrTemp, "Disk aging.", IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "actionlist", "a2");

	attrTemp = IoT_FindSensorNode(groupTemp, "e4");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "e4");
    IoT_SetStringValue(attrTemp, "Disk read/writes error frequently.", IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "actionlist", "a2");

	attrTemp = IoT_FindSensorNode(groupTemp, "e5");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "e5");
    IoT_SetStringValue(attrTemp, "Power failure.", IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "actionlist", "a3");

	attrTemp = IoT_FindSensorNode(groupTemp, "e6");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "e6");
    IoT_SetStringValue(attrTemp, "Remaining 60 Day.", IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "actionlist", "a2");

	attrTemp = IoT_FindSensorNode(groupTemp, "e7");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "e7");
    IoT_SetStringValue(attrTemp, "Remaining one month.", IoT_READONLY);
    MSG_AppendIoTSensorAttributeString(attrTemp, "actionlist", "a2");

	return GenSuccess;
}

GenStatus_t PMQMsgSetPMQInfo(MSG_CLASSIFY_T *msg, pDiskPMQ diskpmq)
{
	MSG_CLASSIFY_T *dataGroup = NULL, *predictGroup = NULL, *groupTemp = NULL;
	MSG_ATTRIBUTE_T *attrTemp = NULL;
	pDiskPMQ diskpmqTemp;
	double predictResult = 0, reMapVal = 0;

	if (diskpmq == NULL)
		return GenFail;

	groupTemp = IoT_FindGroup(msg, DATA_GROUP_NAME);
    if (groupTemp == NULL)
        groupTemp = IoT_AddGroup(msg, DATA_GROUP_NAME);

	dataGroup = IoT_FindGroup(groupTemp, "list");
    if (dataGroup == NULL)
        dataGroup = IoT_AddGroupArray(groupTemp, "list");

	groupTemp = IoT_FindGroup(msg, PREDICT_GROUP_NAME);
    if (groupTemp == NULL)
        groupTemp = IoT_AddGroup(msg, PREDICT_GROUP_NAME);

	predictGroup = IoT_FindGroup(groupTemp, "list");
    if (predictGroup == NULL)
        predictGroup = IoT_AddGroupArray(groupTemp, "list");

	diskpmqTemp = diskpmq;
	while (diskpmqTemp != NULL)
	{
		groupTemp = IoT_FindGroup(dataGroup, diskpmqTemp->diskname); // Add HDD name group in data/list group
        if (groupTemp == NULL)
            groupTemp = IoT_AddGroup(dataGroup, diskpmqTemp->diskname);

		switch (diskpmqTemp->type)
		{
		case HddTypeUnknown:
			break;

		case SQFlash:
			attrTemp = IoT_FindSensorNode(groupTemp, "smart 9");
			if (attrTemp == NULL)
				attrTemp = IoT_AddSensorNode(groupTemp, "smart 9");
			IoT_SetDoubleValueWithMaxMin(attrTemp, diskpmqTemp->smart.smart9.val, IoT_READONLY, 35000, 0, NULL);
			MSG_AppendIoTSensorAttributeDouble(attrTemp, "threshold", 26280);
			MSG_AppendIoTSensorAttributeString(attrTemp, "u", "hr");
			MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Power-On Hours");

			attrTemp = IoT_FindSensorNode(groupTemp, "smart 173");
			if (attrTemp == NULL)
				attrTemp = IoT_AddSensorNode(groupTemp, "smart 173");
			IoT_SetDoubleValueWithMaxMin(attrTemp, diskpmqTemp->smart.smart173.val, IoT_READONLY, 100000, 0, NULL);
			MSG_AppendIoTSensorAttributeDouble(attrTemp, "threshold", 26280);
			MSG_AppendIoTSensorAttributeString(attrTemp, "u", "count");
			MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Erase count");
			break;

		case StdDisk:
			attrTemp = IoT_FindSensorNode(groupTemp, "smart 5");
			if (attrTemp == NULL)
				attrTemp = IoT_AddSensorNode(groupTemp, "smart 5");
			IoT_SetDoubleValueWithMaxMin(attrTemp, diskpmqTemp->smart.smart5.val, IoT_READONLY, 20, 0, NULL);
			MSG_AppendIoTSensorAttributeDouble(attrTemp, "threshold", 10);
			MSG_AppendIoTSensorAttributeString(attrTemp, "u", "count");
			MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Reallocated Sector Count");

			attrTemp = IoT_FindSensorNode(groupTemp, "smart 9");
			if (attrTemp == NULL)
				attrTemp = IoT_AddSensorNode(groupTemp, "smart 9");
			IoT_SetDoubleValueWithMaxMin(attrTemp, diskpmqTemp->smart.smart9.val, IoT_READONLY, 35000, 0, NULL);
			MSG_AppendIoTSensorAttributeDouble(attrTemp, "threshold", 26280);
			MSG_AppendIoTSensorAttributeString(attrTemp, "u", "hr");
			MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Power-On Hours");

			attrTemp = IoT_FindSensorNode(groupTemp, "smart 187");
			if (attrTemp == NULL)
				attrTemp = IoT_AddSensorNode(groupTemp, "smart 187");
			IoT_SetDoubleValueWithMaxMin(attrTemp, diskpmqTemp->smart.smart187.val, IoT_READONLY, 5, 0, NULL);
			MSG_AppendIoTSensorAttributeDouble(attrTemp, "threshold", 1);
			MSG_AppendIoTSensorAttributeString(attrTemp, "u", "count");
			MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Reported Uncorrectable Errors");

			attrTemp = IoT_FindSensorNode(groupTemp, "smart 192");
			if (attrTemp == NULL)
				attrTemp = IoT_AddSensorNode(groupTemp, "smart 192");
			IoT_SetDoubleValueWithMaxMin(attrTemp, diskpmqTemp->smart.smart192.val, IoT_READONLY, 400, 0, NULL);
			MSG_AppendIoTSensorAttributeDouble(attrTemp, "threshold", 190);
			MSG_AppendIoTSensorAttributeString(attrTemp, "u", "number");
			MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Power-off Retract Count");

			attrTemp = IoT_FindSensorNode(groupTemp, "smart 197");
			if (attrTemp == NULL)
				attrTemp = IoT_AddSensorNode(groupTemp, "smart 197");
			IoT_SetDoubleValueWithMaxMin(attrTemp, diskpmqTemp->smart.smart197.val, IoT_READONLY, 10, 0, NULL);
			MSG_AppendIoTSensorAttributeDouble(attrTemp, "threshold", 2);
			MSG_AppendIoTSensorAttributeString(attrTemp, "u", "count");
			MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Current Pending Sector Count");

			attrTemp = IoT_FindSensorNode(groupTemp, "smart 198");
			if (attrTemp == NULL)
				attrTemp = IoT_AddSensorNode(groupTemp, "smart 198");
			IoT_SetDoubleValueWithMaxMin(attrTemp, diskpmqTemp->smart.smart198.val, IoT_READONLY, 40, 0, NULL);
			MSG_AppendIoTSensorAttributeDouble(attrTemp, "threshold", 10);
			MSG_AppendIoTSensorAttributeString(attrTemp, "u", "count");
			MSG_AppendIoTSensorAttributeString(attrTemp, "msg", "Uncorrectable Sector Count");
			break;

		default:
			break;
		}
		
		groupTemp = IoT_FindGroup(predictGroup, diskpmqTemp->diskname); // Add HDD name group in predict/list group
        if (groupTemp == NULL)
            groupTemp = IoT_AddGroup(predictGroup, diskpmqTemp->diskname);

		predictResult = diskpmqTemp->predictVal;

		attrTemp = IoT_FindSensorNode(groupTemp, "Failure rate");
        if (attrTemp == NULL)
            attrTemp = IoT_AddSensorNode(groupTemp, "Failure rate");
		switch (diskpmqTemp->type)
		{
		case HddTypeUnknown:
			break;

		case SQFlash:
			IoT_SetDoubleValueWithMaxMin(attrTemp, predictResult * 100, IoT_READONLY, 100,	0, NULL);
			break;

		case StdDisk:
			if (predictResult <= HDD_THR)
			{
				reMapVal = NormalizeValue(predictResult, 0, (HDD_THR - 0.001), 0, 66);
				IoT_SetDoubleValueWithMaxMin(attrTemp, reMapVal, IoT_READONLY, 100,	0, NULL);
			}   
			else
			{
				reMapVal = NormalizeValue(predictResult, HDD_THR, 1, 67, 100);
				IoT_SetDoubleValueWithMaxMin(attrTemp, reMapVal, IoT_READONLY, 100, 0, NULL);
			}
			break;

		default:
			break;
		}
				
        attrTemp = IoT_FindSensorNode(groupTemp, "hddpredict");
        if (attrTemp == NULL)
            attrTemp = IoT_AddSensorNode(groupTemp, "hddpredict");
        IoT_SetDoubleValueWithMaxMin(attrTemp, predictResult, IoT_READONLY, 1, 0, NULL);
        MSG_AppendIoTSensorAttributeDouble(attrTemp, "threshold", HDD_THR);

		diskpmqTemp = diskpmqTemp->next;
	}

	return GenSuccess;
}

GenStatus_t PMQMsgSetInfo(MSG_CLASSIFY_T *msg, pInfoConfig info)
{
	MSG_CLASSIFY_T *groupTemp = NULL;
	MSG_ATTRIBUTE_T *attrTemp = NULL;

	if (info == NULL)
		return GenFail;

	groupTemp = IoT_FindGroup(msg, INFO_GROUP_NAME);
	if (groupTemp == NULL)
		groupTemp = IoT_AddGroup(msg, INFO_GROUP_NAME);

	attrTemp = IoT_FindSensorNode(groupTemp, "type");
	if (attrTemp == NULL)
		attrTemp = IoT_AddSensorNode(groupTemp, "type");
	IoT_SetStringValue(attrTemp, info->type, IoT_READONLY);

	attrTemp = IoT_FindSensorNode(groupTemp, "name");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "name");
    IoT_SetStringValue(attrTemp, info->name, IoT_READONLY);

	attrTemp = IoT_FindSensorNode(groupTemp, "description");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "description");
    IoT_SetStringValue(attrTemp, info->description, IoT_READONLY);

	attrTemp = IoT_FindSensorNode(groupTemp, "version");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "version");
	IoT_SetStringValue(attrTemp, info->version, IoT_READONLY);

	attrTemp = IoT_FindSensorNode(groupTemp, "confidence level");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "confidence level");
	IoT_SetDoubleValue(attrTemp, 83.12, IoT_READONLY, "%");

	attrTemp = IoT_FindSensorNode(groupTemp, "update");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "update");
    IoT_SetStringValue(attrTemp, info->update, IoT_READWRITE);

	attrTemp = IoT_FindSensorNode(groupTemp, "eventNotify");
    if (attrTemp == NULL)
        attrTemp = IoT_AddSensorNode(groupTemp, "eventNotify");
	IoT_SetBoolValue(attrTemp, info->eventNotify, IoT_READONLY);

	return GenSuccess;
}

GenStatus_t UpdatePMQMsg(MSG_CLASSIFY_T *msg, pDiskPMQ diskpmq, pHandlerConfig pmqConfig)
{
	if (msg == NULL || diskpmq == NULL)
		return GenFail;

	if (PMQMsgSetInfo(msg, &pmqConfig->infoConfig) == GenFail)
		return GenFail;

	if (PMQMsgSetPMQInfo(msg, diskpmq) == GenFail)
		return GenFail;

	if (PMQMsgSetEvent(msg, &pmqConfig->eventInfo) == GenFail)
		return GenFail;

	if (PMQMsgSetAction(msg, &pmqConfig->actInfo) == GenFail)
		return GenFail;

	if (PMQMsgSetParam(msg, &pmqConfig->paramInfo) == GenFail)
		return GenFail;
	
	return GenSuccess;
}

GenStatus_t GenEventStrbyByteCode(char *msgStr, char *idStr, unsigned char byteCode)
{
	int i;
	
	if (byteCode == NULL)
		return GenFail;

	if (msgStr == NULL)
		msgStr = (char *)calloc(1024, sizeof(char));

	if (idStr == NULL)
		idStr = (char *)calloc(64, sizeof(char));

	memset(msgStr, 0, sizeof(msgStr));
	memset(idStr, 0, sizeof(idStr));

	for (i = 0; i < EVENT_MAX_NUM; i++)
	{
		if ((byteCode & (1 << i)) == (1 << i))
		{
			switch (i)
			{
			case 0:
				strcat(idStr, "e1,");
				strcat(msgStr, "HDD back to Normal.");
				strcat(msgStr, " ");
				break;
			case 1:
				strcat(idStr, "e2,");
				strcat(msgStr, "Over temperature.");
				strcat(msgStr, " ");
				break;
			case 2:
				strcat(idStr, "e3,");
				strcat(msgStr, "Disk aging.");
				strcat(msgStr, " ");
				break;
			case 3:
				strcat(idStr, "e4,");
				strcat(msgStr, "Disk read/writes error frequently.");
				strcat(msgStr, " ");
				break;
			case 4:
				strcat(idStr, "e5,");
				strcat(msgStr, "Power failure.");
				strcat(msgStr, " ");
				break;
			case 5:
				strcat(idStr, "e6,");
				strcat(msgStr, "Remaining 60 Day.");
				strcat(msgStr, " ");
				break;
			case 6:
				strcat(idStr, "e7,");
				strcat(msgStr, "Remaining one month.");
				strcat(msgStr, " ");
				break;
			case 7:
				strcat(idStr, "e8,");
				strcat(msgStr, "");
				strcat(msgStr, " ");
				break;
			default:
				memset(msgStr, 0, sizeof(msgStr));
				memset(idStr, 0, sizeof(idStr));
			}
		}
	}

	idStr[strlen(idStr) - 1] = 0;
	msgStr[strlen(msgStr) - 1] = 0;
    
	return GenSuccess;
}

char *CreateEventMsg(pDiskPMQ diskpmq)
{
	cJSON *root = NULL;
	cJSON *extNode = NULL;
	char *buff = NULL;
	char msg[512], eventID[64];

	if (diskpmq == NULL)
		return NULL;

	if ((diskpmq->type == StdDisk && diskpmq->predictVal > HDD_THR) ||
		(diskpmq->type == SQFlash && diskpmq->predictVal > SQF_THR))
	{
		if (GenEventStrbyByteCode(msg, eventID, diskpmq->eventID) == GenSuccess)
		{
			root = cJSON_CreateObject();
			cJSON_AddStringToObject(root, "subtype", "predict");
			cJSON_AddStringToObject(root, "msg", msg);

			extNode = cJSON_CreateObject();
			cJSON_AddItemToObject(root, "extMsg", extNode); //fixed to 'general'
			cJSON_AddStringToObject(extNode, "n", diskpmq->diskname);
			cJSON_AddStringToObject(extNode, "eventID", eventID);
		}

		buff = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
	}

	return buff;
}

MSG_CLASSIFY_T* CreatePMQMsg()
{
	MSG_CLASSIFY_T *msgTemp = NULL;
	MSG_CLASSIFY_T *groupTemp = NULL;

	if (msgTemp == NULL)
		msgTemp = IoT_CreateRoot("HDD_PMQ");

	groupTemp = IoT_FindGroup(msgTemp, INFO_GROUP_NAME);
	if (groupTemp == NULL)
		groupTemp = IoT_AddGroup(msgTemp, INFO_GROUP_NAME);

	groupTemp = IoT_FindGroup(msgTemp, DATA_GROUP_NAME);
	if (groupTemp == NULL)
		groupTemp = IoT_AddGroup(msgTemp, DATA_GROUP_NAME);

	groupTemp = IoT_FindGroup(msgTemp, PREDICT_GROUP_NAME);
	if (groupTemp == NULL)
		groupTemp = IoT_AddGroup(msgTemp, PREDICT_GROUP_NAME);

	groupTemp = IoT_FindGroup(msgTemp, EVENT_GROUP_NAME);
	if (groupTemp == NULL)
		groupTemp = IoT_AddGroup(msgTemp, EVENT_GROUP_NAME);

	groupTemp = IoT_FindGroup(msgTemp, ACTION_GROUP_NAME);
	if (groupTemp == NULL)
		groupTemp = IoT_AddGroup(msgTemp, ACTION_GROUP_NAME);

	groupTemp = IoT_FindGroup(msgTemp, PARAMETER_GROUP_NAME);
	if (groupTemp == NULL)
		groupTemp = IoT_AddGroup(msgTemp, PARAMETER_GROUP_NAME);

	return msgTemp;
}









