#include "ActionTrigger.h"
#include "HandlerThreshold.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "wrapper.h" //redefine strtok_r to strtok_s for VC
#include "util_string.h"

#define CAPAB_ACTION	                            "action"

void Action_On_Threshold_Triggered(HANDLE const handler, bool isNormal, char* sensorname, double value, MSG_ATTRIBUTE_T* attr, void *pRev)
{
}

char* GetFunctionList(MSG_CLASSIFY_T *pCapability)
{
	MSG_CLASSIFY_T *pInfo = NULL;
	MSG_CLASSIFY_T *pCurrentNode = NULL;
	
	if(pCapability == NULL)
		return NULL;

	pCurrentNode = pCapability;
	while(pCurrentNode)
	{
		pInfo = IoT_FindGroup(pCurrentNode, "Information");
		if(pInfo)
		{
			MSG_ATTRIBUTE_T* pFunc = IoT_FindSensorNode(pInfo, "functionList");
			if(pFunc)
			{
				if(pFunc->type == attr_type_string)
					return strdup(pFunc->sv);
			}
		}
		else
		{
			char* result = GetFunctionList(pCurrentNode->sub_list);
			if(result)
				return result;
		}
		pCurrentNode = pCurrentNode->next;
	}
	return NULL;
}

MSG_CLASSIFY_T* GetActionNode(MSG_CLASSIFY_T *pCapability, char* name)
{
	MSG_CLASSIFY_T* dev = NULL;
	MSG_CLASSIFY_T* plugin = NULL;
	MSG_CLASSIFY_T* action = NULL;

	if(pCapability == NULL)
		return action;

	dev = pCapability->sub_list;

	while(dev)
	{
		plugin = MSG_FindClassify(dev, name);

		if(plugin == NULL)
			goto NEXT_ACTION;

		action = IoT_FindGroup(plugin, CAPAB_ACTION);

		if(action != NULL)
			break;

NEXT_ACTION:
		dev = dev->next;
	}

	return action;
}

void SetPowerOnOffAction(char* name, MSG_CLASSIFY_T *pCapability, MSG_CLASSIFY_T* pAction)
{
	
	char* strFuncList = NULL;
	char* delim=",";
	char *p = NULL;
	char *token = NULL;

	if(pCapability == NULL || pAction == NULL )
		return;
	strFuncList = GetFunctionList(pCapability);
	if(strFuncList == NULL)
	{
		free(strFuncList);
		return;
	}
	p = strtok_r(strFuncList, delim, &token);
	while(p)
	{
		if(strlen(p) > 0)
		{
			MSG_ATTRIBUTE_T* pSensor = NULL;
			char msg[256] = {0};
			pSensor = IoT_AddSensorNode(pAction, p);
			
			if(strcmp(p, "wol") == 0)
			{
				sprintf(msg, "System Power on");
				IoT_SetStringValue(pSensor, "na", IoT_WRITEONLY);
			}
			else if(strcmp(p, "shutdown") == 0)
			{
				sprintf(msg, "System Power off");
				IoT_SetStringValue(pSensor, "na", IoT_WRITEONLY);
			}
			else if(strcmp(p, "restart") == 0)
			{
				sprintf(msg, "System Restart");
				IoT_SetStringValue(pSensor, "na", IoT_WRITEONLY);
			}
			else if(strcmp(p, "hibernate") == 0)
			{
				sprintf(msg, "System Hibernate");
				IoT_SetStringValue(pSensor, "na", IoT_WRITEONLY);
			}
			else if(strcmp(p, "suspend") == 0)
			{
				sprintf(msg, "System Sleep");
				IoT_SetStringValue(pSensor, "na", IoT_WRITEONLY);
			}
			else
			{
				sprintf(msg, "%s the system", p);
				IoT_SetBoolValue(pSensor, false, IoT_WRITEONLY);
			}
			MSG_AppendIoTSensorAttributeString(pSensor, "msg", msg);
		}
		p = strtok_r(NULL, delim, &token);
	}
	free(strFuncList);
}

bool IsProtectionSupported(MSG_CLASSIFY_T *pCapability)
{
	MSG_CLASSIFY_T *pInfo = NULL;
	MSG_CLASSIFY_T *pCurrentNode = NULL;
	bool bRet = false;
	if(pCapability == NULL)
		return bRet;

	pCurrentNode = pCapability;
	while(pCurrentNode)
	{
		pInfo = IoT_FindGroup(pCurrentNode, "Information");
		if(pInfo)
		{
			MSG_ATTRIBUTE_T* pInstalled = NULL;
			//printf("find Information\n");
			pInstalled = IoT_FindSensorNode(pInfo, "prttIsInstalled");
			if(pInstalled)
			{
				//printf("find prttIsInstalled %d %s\n", pInstalled->type, pInstalled->bv?"true":"false");
				if(pInstalled->type == attr_type_boolean)
				{
					if(pInstalled->bv)
					{
						MSG_ATTRIBUTE_T* pExpired = IoT_FindSensorNode(pInfo, "prttIsExpired");
						if(pExpired)
						{
							//printf("find prttIsExpired %d %s\n", pExpired->type, pExpired->bv?"true":"false");
							if(pExpired->type == attr_type_boolean)
							{
								if(pExpired->bv)
									bRet = false;
								else
									bRet = true;
							}
						}
					}
				}		
			}
			//printf("Result: %s\n", bRet?"true":"false");
			return bRet;
		}
		else
		{
			//printf("not find Information, %s\n", pCurrentNode->sub_list->classname);
			bRet = IsProtectionSupported(pCurrentNode->sub_list);
			//printf("not find Result: %s\n", bRet?"true":"false");
			if(bRet)
				return bRet;
		}
		pCurrentNode = pCurrentNode->next;
	}
	//printf("outside Result: %s\n", bRet?"true":"false");
	return bRet;
}

void SetProtectAction(char* name, MSG_CLASSIFY_T *pCapability, MSG_CLASSIFY_T* pAction)
{
	char* strFuncList = NULL;
	char* delim=",";
	char *p = NULL;
	char *token = NULL;

	if(pCapability == NULL || pAction == NULL )
		return;

	if(!IsProtectionSupported(pCapability))
		return;

	strFuncList = GetFunctionList(pCapability);
	if(strFuncList == NULL)
	{
		free(strFuncList);
		return;
	}	

	p = strtok_r(strFuncList, delim, &token);
	while(p)
	{
		if(strcmp(p, "protect") == 0)
		{
			MSG_ATTRIBUTE_T* pSensor = IoT_AddSensorNode(pAction, p);
			IoT_SetBoolValue(pSensor, false, IoT_WRITEONLY);
			MSG_AppendIoTSensorAttributeString(pSensor, "msg", "Protect the system");
		}
		else if(strcmp(p, "unprotect") == 0)
		{
			MSG_ATTRIBUTE_T* pSensor = IoT_AddSensorNode(pAction, p);
			IoT_SetBoolValue(pSensor, false, IoT_WRITEONLY);
			MSG_AppendIoTSensorAttributeString(pSensor, "msg", "Unprotect the system");
		}
		
		p = strtok_r(NULL, delim, &token);
	}
	free(strFuncList);
}

bool IsRecoverySupported(MSG_CLASSIFY_T *pCapability)
{
	MSG_CLASSIFY_T *pInfo = NULL;
	MSG_CLASSIFY_T *pCurrentNode = NULL;
	bool bRet = false;
	if(pCapability == NULL)
		return bRet;

	pCurrentNode = pCapability;
	while(pCurrentNode)
	{
		pInfo = IoT_FindGroup(pCurrentNode, "Information");
		if(pInfo)
		{	
			MSG_ATTRIBUTE_T* pInstalled = NULL;
			//printf("find Information\n");
			pInstalled = IoT_FindSensorNode(pInfo, "rcvyIsInstalled");
			if(pInstalled)
			{
				//printf("find rcvyIsInstalled %d %s\n", pInstalled->type, pInstalled->bv?"true":"false");
				if(pInstalled->type == attr_type_boolean)
				{
					if(pInstalled->bv)
					{
						MSG_ATTRIBUTE_T* pExpired = IoT_FindSensorNode(pInfo, "rcvyIsExpired");
						if(pExpired)
						{
							//printf("find rcvyIsExpired %d %s\n", pExpired->type, pExpired->bv?"true":"false");
							if(pExpired->type == attr_type_boolean)
							{
								if(pExpired->bv)
									bRet = false;
								else
									bRet = true;
							}
						}
					}
				}		
			}
			//printf("Result: %s\n", bRet?"true":"false");
			return bRet;
		}
		else
		{
			//printf("not find Information, %s\n", pCurrentNode->sub_list->classname);
			bRet = IsRecoverySupported(pCurrentNode->sub_list);
			//printf("not find Result: %s\n", bRet?"true":"false");
			if(bRet)
				return bRet;
		}
		pCurrentNode = pCurrentNode->next;
	}
	//printf("outside Result: %s\n", bRet?"true":"false");
	return bRet;
}

void SetRecoveryAction(char* name, MSG_CLASSIFY_T *pCapability, MSG_CLASSIFY_T* pAction)
{
	char* strFuncList = NULL;
	char* delim=",";
	char *p = NULL;
	char *token = NULL;

	if(pCapability == NULL || pAction == NULL )
		return;

	if(!IsRecoverySupported(pCapability))
		return;

	strFuncList = GetFunctionList(pCapability);
	if(strFuncList == NULL)
	{
		free(strFuncList);
		return;
	}

	p = strtok_r(strFuncList, delim, &token);
	while(p)
	{
		if(strcmp(p, "backup") == 0)
		{
			MSG_ATTRIBUTE_T* pSensor = IoT_AddSensorNode(pAction, p);
			IoT_SetBoolValue(pSensor, false, IoT_WRITEONLY);
			MSG_AppendIoTSensorAttributeString(pSensor, "msg", "Backup the system");
		}
		else if(strcmp(p, "recovery") == 0)
		{
			MSG_ATTRIBUTE_T* pSensor = IoT_AddSensorNode(pAction, p);
			IoT_SetBoolValue(pSensor, false, IoT_WRITEONLY);
			MSG_AppendIoTSensorAttributeString(pSensor, "msg", "Recovery the system from latest backup");
		}
		
		p = strtok_r(NULL, delim, &token);
	}
	free(strFuncList);
}

void SetProcessAction(char* name, MSG_CLASSIFY_T *pCapability, MSG_CLASSIFY_T* pAction)
{
	MSG_ATTRIBUTE_T* pSensor = NULL;

	if(pCapability == NULL || pAction == NULL )
		return;

	pSensor = IoT_AddSensorNode(pAction,"restart");
	IoT_SetDoubleValue(pSensor, 0, IoT_WRITEONLY, NULL);
	MSG_AppendIoTSensorAttributeString(pSensor, "msg", "Restart the process with PID");

	pSensor = IoT_AddSensorNode(pAction,"kill");
	IoT_SetDoubleValue(pSensor, 0, IoT_WRITEONLY, NULL);
	MSG_AppendIoTSensorAttributeString(pSensor, "msg", "Kill the process with PID");

	pSensor = IoT_AddSensorNode(pAction, "exec");
	IoT_SetStringValue(pSensor, "\"prcPath\":\"<file_path>\",\"prcArg\":\"<arguments>\",\"prcHidden\":false", IoT_WRITEONLY);
	MSG_AppendIoTSensorAttributeString(pSensor, "msg", "Execute application with arguments");
}

void SetScreenShotAction(char* name, MSG_CLASSIFY_T *pCapability, MSG_CLASSIFY_T* pAction)
{
	MSG_ATTRIBUTE_T* pSensor = NULL;

	if(pCapability == NULL || pAction == NULL )
		return;

	pSensor = IoT_AddSensorNode(pAction,"screenshot");
	IoT_SetBoolValue(pSensor, false, IoT_WRITEONLY);
	MSG_AppendIoTSensorAttributeString(pSensor, "msg", "Take a snapshot");
}

void SetSetAction(char* name, MSG_CLASSIFY_T *pCapability, MSG_CLASSIFY_T* pAction)
{
	MSG_ATTRIBUTE_T* pSensor = NULL;
	MSG_ATTRIBUTE_T* lastaction = NULL;
	MSG_CLASSIFY_T* pNode = NULL;
	MSG_CLASSIFY_T* dev = NULL;
	bool bHasWritable = false;

	if(pCapability == NULL || pAction == NULL )
		return;

	if(pAction->type == class_type_root)
		pNode = pAction->sub_list;
	else
		pNode = pAction;
	lastaction = pNode->attr_list;
	while(lastaction)
	{
		if(lastaction->next)
			lastaction = lastaction->next;
		else
			break;
	}
	
	

	dev = pCapability->sub_list;

	while (dev)
	{
		MSG_CLASSIFY_T* target = NULL;
		MSG_CLASSIFY_T* plugin = MSG_FindClassify(dev, name);

		if (plugin == NULL)
			goto NEXT_ACTION;

		target = plugin->sub_list;

		while (target)
		{
			if (!strcmp(target->classname, CAPAB_ACTION)) {

				MSG_ATTRIBUTE_T* pact = target->attr_list;

				while (pact)
				{
					MSG_ATTRIBUTE_T* newaction = NULL;
					bool bWriteable = false;
					if (strcmp(pact->readwritemode, "w") == 0)
					{
						bWriteable = true;
					}
					else if (strcmp(pact->readwritemode, "rw") == 0)
					{
						bWriteable = true;
					}
					if (bWriteable)
					{
						newaction = MSG_CloneAttribute(pact, true);
						if (lastaction == NULL)
							pNode->attr_list = newaction;
						else
							lastaction->next = newaction;
						lastaction = newaction;
					}
					pact = pact->next;
				}
			}
			else 
			{

				MSG_CLASSIFY_T* curClass = target;

				if(curClass)
				{
					if(curClass->attr_list)
					{
						struct msg_attr *curAttr = curClass->attr_list;
						while(curAttr)
						{
							if(strcmp(curAttr->readwritemode, "w") == 0)
							{
								bHasWritable = true;
							}
							else if(strcmp(curAttr->readwritemode, "rw") == 0)
							{
								bHasWritable = true;
							}
							curAttr = curAttr->next;	
						}
					}
					if(IoT_HasWritableSensorNode(curClass->sub_list))
						bHasWritable = true;
				}					
			}
			target = target->next;
		}

	NEXT_ACTION:
		dev = dev->next;
	}
	if (bHasWritable)
	{
		pSensor = IoT_AddSensorNode(pAction,"Set");
		IoT_SetBoolValue(pSensor, false, IoT_WRITEONLY);
		MSG_AppendIoTSensorAttributeString(pSensor, "msg", "Set value to specific sesnor data");
	}
}
void Action_Set_Action_Capability(char* devID, char* name, MSG_CLASSIFY_T *pCapability, MSG_CLASSIFY_T** pAction)
{

	if(strcmp(name, "power_onoff") == 0)
	{
		if(*pAction == NULL)
			*pAction = IoT_CreateRoot(name);
		SetPowerOnOffAction(name, pCapability, *pAction);
	}
	else if(strcmp(name, "protection") == 0)
	{
		if(*pAction == NULL)
			*pAction = IoT_CreateRoot(name);
		SetProtectAction(name, pCapability, *pAction);
	}
	else if(strcmp(name, "recovery") == 0)
	{
		if(*pAction == NULL)
			*pAction = IoT_CreateRoot(name);
		SetRecoveryAction(name, pCapability, *pAction);
	}
	else if(strcmp(name, "ProcessMonitor") == 0)
	{
		if(*pAction == NULL)
			*pAction = IoT_CreateRoot(name);
		SetProcessAction(name, pCapability, *pAction);
	}
	else if(strcmp(name, "screenshot") == 0)
	{
		if(*pAction == NULL)
			*pAction = IoT_CreateRoot(name);
		SetScreenShotAction(name, pCapability, *pAction);
	}
	else if(strcmp(name, "remote_kvm") == 0)
	{
		
	}
	else if(strcmp(name, "terminal") == 0)
	{
		
	}
	else
	{
		if(*pAction == NULL)
			*pAction = IoT_CreateRoot(name);
		SetSetAction(name, pCapability, *pAction);
	}
}

bool PowerOnOffCommand(char* name, thr_action_t* pAction, char* command)
{
	if(name == NULL || command == NULL || pAction == NULL)
		return false;
	if(strcmp(name, "wol") == 0)
	{
		if(pAction->type == action_type_string && strlen(pAction->sv) > 0)
			sprintf(command, "{\"susiCommData\":{\"commCmd\":85,\"handlerName\":\"power_onoff\",\"macs\":\"%s\"}}", pAction->sv);
		else
			return false;
	}
	else if(strcmp(name, "shutdown") == 0)
	{
		if(pAction->type == action_type_boolean && pAction->bv)
			strcpy(command, "{\"susiCommData\":{\"commCmd\":77,\"handlerName\":\"power_onoff\"}}");
		else
			return false;
	}
	else if(strcmp(name, "restart") == 0)
	{
		if(pAction->type == action_type_boolean && pAction->bv)
			strcpy(command, "{\"susiCommData\":{\"commCmd\":79,\"handlerName\":\"power_onoff\"}}");
		else
			return false;
	}
	else if(strcmp(name, "hibernate") == 0)
	{
		if(pAction->type == action_type_boolean && pAction->bv)
			strcpy(command, "{\"susiCommData\":{\"commCmd\":83,\"handlerName\":\"power_onoff\"}}");
		else
			return false;
	}
	else if(strcmp(name, "suspend") == 0)
	{
		if(pAction->type == action_type_boolean && pAction->bv)
			strcpy(command, "{\"susiCommData\":{\"commCmd\":81,\"handlerName\":\"power_onoff\"}}");
		else
			return false;
	}
	else
		return false;
	return true;
}

bool ProtectCommand(char* name, thr_action_t* pAction, char* command)
{
	if(name == NULL || command == NULL || pAction == NULL)
		return false;

	if(strcmp(name, "protect") == 0)
	{
		if(pAction->type == action_type_boolean && pAction->bv)
			strcpy(command, "{\"susiCommData\":{\"commCmd\":11,\"handlerName\":\"protection\"}}");
		else
			return false;
	}
	else if(strcmp(name, "unprotect") == 0)
	{
		if(pAction->type == action_type_boolean && pAction->bv)
			strcpy(command, "{\"susiCommData\":{\"commCmd\":13,\"handlerName\":\"protection\"}}");
		else
			return false;
	}
	else
		return false;
	return true;
}

bool RecoveryCommand(char* name, thr_action_t* pAction, char* command)
{
	if(name == NULL || command == NULL || pAction == NULL)
		return false;

	if(strcmp(name, "backup") == 0)
	{
		if(pAction->type == action_type_boolean && pAction->bv)
			strcpy(command, "{\"susiCommData\":{\"commCmd\":41,\"handlerName\":\"recovery\"}}");
		else
			return false;
	}
	else if(strcmp(name, "recovery") == 0)
	{
		if(pAction->type == action_type_boolean && pAction->bv)
			strcpy(command, "{\"susiCommData\":{\"commCmd\":43,\"handlerName\":\"recovery\"}}");
		else
			return false;
	}
	else
		return false;
	return true;
}

bool ProcessCommand(char* name, thr_action_t* pAction, char* command)
{
	if(name == NULL || command == NULL || pAction == NULL)
		return false;

	if(strcmp(name, "restart") == 0)
	{
		if(pAction->type == action_type_numeric)
			sprintf(command, "{\"susiCommData\":{\"commCmd\":167,\"handlerName\":\"ProcessMonitor\",\"pids\":[%f]}}", pAction->v);
		else
			return false;
	}
	else if(strcmp(name, "kill") == 0)
	{
		if(pAction->type == action_type_numeric)
			sprintf(command, "{\"susiCommData\":{\"commCmd\":169,\"handlerName\":\"ProcessMonitor\",\"pids\":[%f]}}", pAction->v);
		else
			return false;
	}
	else if (strcmp(name, "exec") == 0)
	{
		if (pAction->type == action_type_string)
		{
			char* param = StringReplace(pAction->sv, "\\", "\\\\");
			if (param)
			{
				sprintf(command, "{\"susiCommData\":{\"commCmd\":171,\"handlerName\":\"ProcessMonitor\",%s}}", param);
				StringFree(param);
			}
		}
		else
			return false;
	}
	else
		return false;
	return true;
}

bool ScreenShotCommand(char* name, thr_action_t* pAction, char* command)
{
	if(name == NULL || command == NULL || pAction == NULL)
		return false;

	if(strcmp(name, "screenshot") == 0)
	{
		if(pAction->type == action_type_boolean && pAction->bv)
			strcpy(command, "{\"susiCommData\":{\"commCmd\":403,\"handlerName\":\"screenshot\"}}");
		else
			return false;
	}
	else
		return false;
	return true;
}

bool SetCommand(char* name, char* sensor, thr_action_t* pAction, char* command)
{
	bool bRet = false;
	if(name == NULL || command == NULL || pAction == NULL)
		return bRet;
	switch(pAction->type)
	{
		case action_type_numeric:
			sprintf(command, "{\"susiCommData\":{\"commCmd\":525,\"sessionID\":\"1234\",\"handlerName\":\"%s\",\"sensorIDList\":{\"e\":[{\"n\":\"%s/%s\",\"v\":%f}]}}}", name, name, sensor, pAction->v);
			bRet = true;
			break;
		case action_type_boolean:
			sprintf(command, "{\"susiCommData\":{\"commCmd\":525,\"sessionID\":\"1234\",\"handlerName\":\"%s\",\"sensorIDList\":{\"e\":[{\"n\":\"%s/%s\",\"bv\":%s}]}}}", name, name, sensor, pAction->bv?"true":"false");
			bRet = true;
			break;
		case action_type_string:
			if(strlen(pAction->sv) > 0)
			{
				sprintf(command, "{\"susiCommData\":{\"commCmd\":525,\"sessionID\":\"1234\",\"handlerName\":\"%s\",\"sensorIDList\":{\"e\":[{\"n\":\"%s/%s\",\"sv\":\"%s\"}]}}}", name, name, sensor, pAction->sv);
				bRet = true;
			}
			break;
	}
	return bRet;
}

bool Action_Trigger_Command(char* name, char* sensor, thr_action_t* pAction, char* command)
{
	if(name == NULL || sensor == NULL || command == NULL || pAction == NULL)
		return false;

	if(strcmp(name, "power_onoff") == 0)
	{
		return PowerOnOffCommand(sensor, pAction, command);
	}
	else if(strcmp(name, "protection") == 0)
	{
		return ProtectCommand(sensor, pAction, command);
	}
	else if(strcmp(name, "recovery") == 0)
	{
		return RecoveryCommand(sensor, pAction, command);
	}
	else if(strcmp(name, "ProcessMonitor") == 0)
	{
		return ProcessCommand(sensor, pAction, command);
	}
	else if(strcmp(name, "screenshot") == 0)
	{
		return ScreenShotCommand(sensor, pAction, command);
	}
	else
	{
		/*TODO: generate getset command*/
		return SetCommand(name, sensor, pAction, command);
	}
	return false;
}