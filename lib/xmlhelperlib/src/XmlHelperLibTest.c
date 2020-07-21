#include <stdio.h>
#include "XmlHelperLib.h"

#define DEF_TEST_XML_PATH                        "XmlTest.xml"
#define DEF_XML_ACTIVE_COMM_TYPE_PATH            "/CommSettings/ActiveCommType"
#define DEF_XML_MQTT_KEEPALIVE_PATH              "/CommSettings/MqttStettings/KeepAlive"
#define DEF_XML_MQTT_QOS_PATH                    "/CommSettings/MqttStettings/Qos"

int main(int argc, char *argv[])
{
	PXmlDoc doc = CreateXmlDoc();
	if(doc)
	{
		printf("Create doc...\n");
		if(SetItemValueToDoc(doc, DEF_XML_ACTIVE_COMM_TYPE_PATH, "Mqtt")==0)
		{
			printf("Set:%s---%s\n", DEF_XML_ACTIVE_COMM_TYPE_PATH, "Mqtt");
			if(SetItemValueToDoc(doc, DEF_XML_MQTT_KEEPALIVE_PATH, "60")==0)
			{
				printf("Set:%s---%s\n", DEF_XML_MQTT_KEEPALIVE_PATH, "60");
				if(SetItemValueToDoc(doc, DEF_XML_MQTT_QOS_PATH, "0")==0)
				{
					printf("Set:%s---%s\n", DEF_XML_MQTT_QOS_PATH, "0");
					SaveXmlDocToFile(doc, DEF_TEST_XML_PATH);
					printf("Save OK!\n");
				}
			}
		}
		DestoryXmlDoc(doc);
	}

	doc = CreateXmlDocFromFile(DEF_TEST_XML_PATH);
	if(doc)
	{
		char itemValue[32] = {0};
		printf("Create doc...\n");
		if(GetItemValueFormDoc(doc, DEF_XML_ACTIVE_COMM_TYPE_PATH, itemValue, 32)==0)
		{
			printf("Get:%s---%s\n", DEF_XML_ACTIVE_COMM_TYPE_PATH, itemValue);
			if(GetItemValueFormDoc(doc, DEF_XML_MQTT_KEEPALIVE_PATH, itemValue, 32)==0)
			{
				printf("Get:%s---%s\n", DEF_XML_MQTT_KEEPALIVE_PATH, itemValue);
				if(GetItemValueFormDoc(doc, DEF_XML_MQTT_QOS_PATH, itemValue, 32)==0)
				{
					printf("Get:%s---%s\n", DEF_XML_MQTT_QOS_PATH, itemValue);
					SaveXmlDocToFile(doc, DEF_TEST_XML_PATH);
					printf("Save OK!\n");
				}
			}
		}
		DestoryXmlDoc(doc);
	}
	printf("Press any key quit!\n");
	getchar();
	return 0;
}