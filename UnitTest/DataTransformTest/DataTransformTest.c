/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/11/20 by Scott Chang								    */
/* Modified Date: 2017/11/20 by Scott Chang									*/
/* Abstract     : WISE-PaaS data transform									*/
/* Reference    : None														*/
/****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "WISEPlatform.h"

#include "DataTransform.h"

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	_CrtMemCheckpoint( &memStateStart);
#endif

	{ // Verify Device ID
		char oldID[17] = {0};
		char newID[37] = {0};

		GetNewID("0000000BAB548765", newID);
		printf("New ID: %s\n", newID);

		GetOldID(newID, oldID);
		printf("Old ID: %s\n", oldID);


		GetOldID("HDD_PMQ", oldID);
		printf("Old ID: %s\n", oldID);

		GetNewID("HDD_PMQ", newID);
		printf("New ID: %s\n", newID);

		GetOldID("BAB548765BAB548765HDD_PMQ", oldID);
		printf("Old ID: %s\n", oldID);

		GetNewID("BAB548765BAB548765BAB548765HDD_PMQ", newID);
		printf("New ID: %s\n", newID);

		GetOldID("BAB548765HDD_PMQ", oldID);
		printf("Old ID: %s\n", oldID);

		GetNewID("B548765B-AB54-8765-BAB5-48765HDD_PMQ", newID);
		printf("New ID: %s\n", newID);
	}
	{ // Verify Report Data
		char* newData = NULL;
		char* oldData = NULL;
		
		newData = Trans2NewFrom("{\"susiCommData\":{\"data\":{\"SenHub\":{\"SenData\":{\"e\":[{\"n\":\"RoomTemp\",\"v\":5}],\"bn\":\"SenData\"},\"Info\":{\"e\":[],\"bn\":\"Info\"},\"Net\":{\"e\":[],\"bn\":\"Net\"},\"Action\":{\"e\":[],\"bn\":\"Action\"},\"ver\":1}},\"commCmd\":2055,\"requestID\":2001,\"agentID\":\"0001000EC6F0F831\",\"handlerName\":\"general\",\"sendTS\":162872850}}");
		printf("New Data: %s\n", newData);

		oldData = Trans2OldFrom(newData);
		printf("Old Data: %s\n", oldData);
		free(newData);
		free(oldData);
	}
	

	{ // Verify Report Data
		cJSON* root = NULL;
		char* newData = NULL;
		root = cJSON_Parse((char *)"{\"SenHub\":{\"SenData\":{\"e\":[{\"n\":\"RoomTemp\",\"v\":5}],\"bn\":\"SenData\"},\"Info\":{\"e\":[],\"bn\":\"Info\"},\"Net\":{\"e\":[],\"bn\":\"Net\"},\"Action\":{\"e\":[],\"bn\":\"Action\"},\"ver\":1},\"agentID\":\"0001000EC6F0F831\",\"sendTS\":162872850}");
		if(root)
		{
			cJSON* target = cJSON_GetObjectItem(root, "agentID");
			if(target)
			{
				cJSON_DeleteItemFromObject(root, "agentID");
			}
			target = cJSON_GetObjectItem(root, "sendTS");
			if(target)
			{
				cJSON_DeleteItemFromObject(root, "sendTS");
			}
			newData = cJSON_PrintUnformatted(root);
		}
		printf("New Data: %s\n", newData);
		free(newData);
		cJSON_Delete(root);
	}

	{
		char* newData = NULL;
		cJSON* root = cJSON_Parse("{\"IoTGW\":{\"WSN\":{\"0001852CF4B7B0E8\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"0001000EC6F0DE03,0001000EC6F0DE06,0001000EC6F0F831,0001000EC6F0DE02,0001000EC6F0DE04,0001000EC6F0F833,0001000EC6F0DE05,0001000EC6F0DE01\"},{\"n\":\"Neighbor\",\"sv\":\"0001000EC6F0DE03,0001000EC6F0DE06,0001000EC6F0F831,0001000EC6F0DE02,0001000EC6F0DE04,0001000EC6F0F833,0001000EC6F0DE05,0001000EC6F0DE01\"},{\"n\":\"Name\",\"sv\":\"WSN1\"},{\"n\":\"Health\",\"v\":100},{\"n\":\"sw\",\"sv\":\"1.2.1.12\"},{\"n\":\"reset\",\"bv\":false}],\"bn\":\"Info\"},\"bn\":\"0001852CF4B7B0E8\",\"ver\":1},\"bn\":\"WSN\"},\"Ethernet\":{\"00070242AC120002\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"\"},{\"n\":\"Neighbor\",\"sv\":\"\"},{\"n\":\"Name\",\"sv\":\"Ethernet\"}],\"bn\":\"Info\"},\"bn\":\"00070242AC120002\",\"ver\":1},\"bn\":\"Ethernet\"},\"opTS\":{\"$date\":1511752844998}}}");
		if(root)
		{
			cJSON* body = cJSON_GetObjectItem(root, "IoTGW");
			if(body)
			{
				cJSON* interFace = body->child;
				while(interFace)
				{
					cJSON* mac = interFace->child;
					while(mac)
					{
						cJSON* info = cJSON_GetObjectItem(mac, "Info");
						if(info)
						{
							cJSON* items = cJSON_GetObjectItem(info, "e");
							if(items)
							{
								int i=0;
								int size = cJSON_GetArraySize(items);
								for(i=0;i<size;i++)
								{
									cJSON* item = cJSON_GetArrayItem(items, i);
									if(item)
									{
										cJSON* name = cJSON_GetObjectItem(item, "n");
										if(name)
										{
											if(strcmp(name->valuestring,"SenHubList")==0)
											{
												cJSON* value = cJSON_GetObjectItem(item, "sv");
												if(value)
												{
													char* lists = strdup(value->valuestring);
													int length = strlen(lists);
													char* buffer = calloc(length/17+1, 37);
													char* p;
													char* delim=",";
													p = strtok(lists, delim);
													while(p)
													{
														char newID[37] = {0};
														GetNewID(p, newID);
														strcat(buffer, newID);
														p = strtok(NULL, delim);
														if(p)
															strcat(buffer, ",");
													}
													cJSON_DeleteItemFromObject(item, "sv");
													cJSON_AddStringToObject(item, "sv", buffer);
													free(buffer);
													free(lists);
												}
											}
											else if(strcmp(name->valuestring,"Neighbor")==0)
											{
												cJSON* value = cJSON_GetObjectItem(item, "sv");
												if(value)
												{
													char* lists = strdup(value->valuestring);
													int length = strlen(lists);
													char* buffer = calloc(length/17+1, 37);
													char* p;
													char* delim=",";
													p = strtok(lists, delim);
													while(p)
													{
														char newID[37] = {0};
														GetNewID(p, newID);
														strcat(buffer, newID);
														p = strtok(NULL, delim);
														if(p)
															strcat(buffer, ",");
													}
													cJSON_DeleteItemFromObject(item, "sv");
													cJSON_AddStringToObject(item, "sv", buffer);
													free(buffer);
													free(lists);
												}
											}
										}
									}
								}
							}
						}
						mac = mac->next;
					}
					interFace = interFace->next;
				}
			}
			newData = cJSON_PrintUnformatted(root);
			cJSON_Delete(root);
		}
		printf("New Data: %s\n", newData);
		free(newData);
		
	}

	{ // Verify Device Info
		char* newData = NULL;
		char* oldData = NULL;
		
		newData = Trans2NewFrom("{\"susiCommData\":{\"parentID\":\"0000000BAB548765\",\"devID\":\"000014DAE996BE04\",\"hostname\":\"PC001104\",\"sn\":\"14DAE996BE04\",\"mac\":\"14DAE996BE04\",\"version\":\"1.0.0.0\",\"type\":\"IPC\",\"product\":\"\",\"manufacture\":\"\",\"account\":\"anonymous\",\"password\":\"\",\"status\":0,\"commCmd\":1,\"requestID\":21,\"agentID\":\"000014DAE996BE04\",\"handlerName\":\"general\",\"sendTS\":1423536737}}");
		printf("New Data: %s\n", newData);

		oldData = Trans2OldFrom(newData);
		printf("Old Data: %s\n", oldData);
		free(newData);
		free(oldData);
	}

	{ // Verify Update CMD
		char* newData = NULL;
		char* oldData = NULL;
		
		newData = Trans2NewFrom("{\"susiCommData\":{\"params\":{\"userName\":\"test\",\"pwd\":\"123456\",\"port\":2121,\"path\":\"/upgrade/SA30AgentSetupV3.0.999_for_V3.0.26.exe\",\"md5\":\"FC98315DEB2ACD72B1160BC7889CE29C\"},\"commCmd\":111,\"requestID\":16,\"agentID\":\"000014DAE996BE04\",\"handlerName\":\"general\",\"sendTS\":1424765401}}");
		printf("New Data: %s\n", newData);

		oldData = Trans2OldFrom(newData);
		printf("Old Data: %s\n", oldData);
		free(newData);
		free(oldData);
	}

	{ // Verify OS Info
		char* newData = NULL;
		char* oldData = NULL;
		
		newData = Trans2NewFrom("{\"susiCommData\":{\"osInfo\":{\"cagentVersion\":\"1.0.0\",\"osVersion\":\"Windows 7 Service Pack 1\",\"biosVersion\":\"\",\"platformName\":\"\",\"processorName\":\"\",\"osArch\":\"X64\",\"totalPhysMemKB\":8244060,\"macs\":\"14DAE996BE04\",\"IP\":\"127.0.0.1\"},\"commCmd\":116,\"requestID\":16,\"agentID\":\"000014DAE996BE04\",\"handlerName\":\"general\",\"sendTS\":1424765401}}");
		printf("New Data: %s\n", newData);

		oldData = Trans2OldFrom(newData);
		printf("Old Data: %s\n", oldData);
		free(newData);
		free(oldData);
	}

	{ // Verify Threshold Rule
		char* newData = NULL;
		char* oldData = NULL;
		
		newData = Trans2NewFrom("{\"susiCommData\":{\"catalogID\":4,\"Thresholds\":[{\"min\":10,\"bu\":\"\",\"max\":20,\"enable\":\"true\",\"intervalTimeS\":60,\"type\":3,\"lastingTimeS\":10,\"n\":\"NetMonitor/netMonInfoList/Index0/Link SpeedMbps\"},{\"min\":-1,\"bu\":\"\",\"max\":1,\"enable\":\"true\",\"intervalTimeS\":60,\"type\":3,\"lastingTimeS\":10,\"n\":\"NetMonitor/netMonInfoList/Index0/sendDataByte\"}],\"handlerName\":\"RuleEngine\",\"commCmd\":527}}");
		printf("New Data: %s\n", newData);

		oldData = Trans2OldFrom(newData);
		printf("Old Data: %s\n", oldData);
		free(newData);
		free(oldData);
	}

	{ // Verify Autoreport CMD
		char* newData = NULL;
		char* oldData = NULL;
		
		newData = Trans2NewFrom("{\"susiCommData\":{\"requestID\":1001,\"catalogID\":4,\"commCmd\":2056,\"handlerName\":\"general\",\"requestItems\":{\"All\":{}}}}");
		printf("New Data: %s\n", newData);

		oldData = Trans2OldFrom(newData);
		printf("Old Data: %s\n", oldData);
		free(newData);
		free(oldData);
	}

	{ // Verify Response message
		char* newData = NULL;
		char* oldData = NULL;
		
		newData = Trans2NewFrom("{\"susiCommData\":{\"requestID\":1001,\"catalogID\":4,\"commCmd\":2054,\"handlerName\":\"general\",\"result\":\"SUCCESS\"}}");
		printf("New Data: %s\n", newData);

		oldData = Trans2OldFrom(newData);
		printf("Old Data: %s\n", oldData);
		free(newData);
		free(oldData);
	}

	{ // Verify Get/Set CMD
		char* newData = NULL;
		char* oldData = NULL;
		
		newData = Trans2NewFrom("{\"susiCommData\":{\"commCmd\":523,\"handlerName\":\"SUSIControl\",\"catalogID\":4,\"sessionID\":\"1234\",\"sensorIDList\":{\"e\":[{\"n\":\"SUSIControl/Hardware Monitor/Temperature/CPU\"},{\"n\":\"SUSIControl/HardwareMonitor/Voltage/CPU\"}]}}}");
		printf("New Data: %s\n", newData);

		oldData = Trans2OldFrom(newData);
		printf("Old Data: %s\n", oldData);
		free(newData);
		free(oldData);
	}

	{ // Verify Device ID
		char oldTopic[256] = {0};
		char newTopic[256] = {0};
		char oldID[17] = {0};
		char newID[37] = {0};

		Trans2NewTopic("/cagent/admin/0000000BAB548765/willmessage", "RMM", newTopic);
		GetNewIDFromTopic("/cagent/admin/0000000BAB548765/willmessage", newID);
		GetOldIDFromTopic("/cagent/admin/0000000BAB548765/willmessage", oldID);
		printf("New Topic: %s\nnewID: %s\noldID: %s\n", newTopic, newID, oldID);

		Trans2OldTopic(newTopic, oldTopic);
		GetNewIDFromTopic(newTopic, newID);
		GetOldIDFromTopic(newTopic, oldID);
		printf("Old Topic: %s\nnewID: %s\noldID: %s\n", oldTopic, newID, oldID);


		Trans2NewTopic("/cagent/admin/0000000BAB548765/agentinfoack", "RMM", newTopic);
		printf("New Topic: %s\n", newTopic);

		Trans2OldTopic(newTopic, oldTopic);
		printf("Old Topic: %s\n", oldTopic);

		Trans2NewTopic("/cagent/admin/0000000BAB548765/agentactionreq", "RMM", newTopic);
		printf("New Topic: %s\n", newTopic);

		Trans2OldTopic(newTopic, oldTopic);
		printf("Old Topic: %s\n", oldTopic);

		Trans2NewTopic("/cagent/admin/0000000BAB548765/agentcallbackreq", "RMM", newTopic);
		printf("New Topic: %s\n", newTopic);

		Trans2OldTopic(newTopic, oldTopic);
		printf("Old Topic: %s\n", oldTopic);


		Trans2NewTopic("/cagent/admin/0000000BAB548765/deviceinfo", "RMM", newTopic);
		printf("New Topic: %s\n", newTopic);

		Trans2OldTopic(newTopic, oldTopic);
		printf("Old Topic: %s\n", oldTopic);

		Trans2NewTopic("/cagent/admin/0000000BAB548765/eventnotify", "RMM", newTopic);
		printf("New Topic: %s\n", newTopic);

		Trans2OldTopic(newTopic, oldTopic);
		printf("Old Topic: %s\n", oldTopic);

		
		Trans2NewTopic("/cagent/admin/0000000BAB548765/notify", "RMM", newTopic);
		printf("New Topic: %s\n", newTopic);

		Trans2OldTopic(newTopic, oldTopic);
		printf("Old Topic: %s\n", oldTopic);

		Trans2NewTopic("/server/admin/+/agentctrl", "RMM", newTopic);
		GetNewIDFromTopic("/server/admin/+/agentctrl", newID);
		GetOldIDFromTopic("/server/admin/+/agentctrl", oldID);
		printf("New Topic: %s\nnewID: %s\noldID: %s\n", newTopic, newID, oldID);

		Trans2OldTopic(newTopic, oldTopic);
		GetNewIDFromTopic(newTopic, newID);
		GetOldIDFromTopic(newTopic, oldID);
		printf("Old Topic: %s\nnewID: %s\noldID: %s\n", oldTopic, newID, oldID);
	}

	printf("Click enter to exit\n");
	fgetc(stdin);

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif

	return 0;
}
