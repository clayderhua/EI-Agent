// ConfigTest.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GPSMessageGenerate.h"
#include "DeviceMessageGenerate.h"
#include "IoTMessageGenerate.h"
#include "MsgGenerator.h"
#include "WISEPlatform.h"

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------


/*
{ 
   "Test":{ 
      "Voltage":{ 
         "bn":"Voltage",
         "bu":"V",
         "e":[ 
            { 
               "n":"Volt",
               "av":[ 
                  6.100000,
                  6.200000,
                  6.300000,
                  6.400000
               ],
               "at":[ 
                  1100,
                  1200,
                  1300,
                  1400
               ],
               "max":5.500000,
               "min":4.500000,
               "asm":"r",
               "u":"V"
            },
            { 
               "n":"Enabled",
               "abv":[ 
                  false,
                  false,
                  false,
                  false
               ],
               "at":[ 
                  1100,
                  1200,
                  1300,
                  1400
               ],
               "asm":"r"
            }
         ]
      },
      "Weather":{ 
         "bn":"Weather",
         "e":[ 
            { 
               "n":"Today",
               "asv":[ 
                  "Cloudy",
                  "Sun"
               ],
               "at":[ 
                  200,
                  300
               ],
               "asm":"r"
            }
         ]
      }
   }
}
*/
int main(int argc, char *argv[])
{
    int iRet = 0;
    char *buffer = NULL;

    MSG_ATTRIBUTE_T *attrVolt = NULL, *enableVolt = NULL, *today = NULL;
    MSG_CLASSIFY_T *myCapability = IoT_CreateRoot("Test");

    // Voltage
    MSG_CLASSIFY_T *myVoltGroup = IoT_AddGroup(myCapability, "Voltage");
    MSG_ATTRIBUTE_T* attr = IoT_AddGroupAttribute(myVoltGroup, "bu");
    if(attr)
        IoT_SetStringValue(attr, "V", IoT_READONLY);

    attrVolt = IoT_AddSensorNode(myVoltGroup, "Volt");
    if(attrVolt) {
        int i, time = 0;
        double value = 5.0;

        IoT_AppendDoubleValueFull(attrVolt, 4.9, 0, IoT_READONLY, "V", true, 5.5, 4.5);
        for (i = 1; i < 15; i++) {
            value += 0.1;
            time += 100;
            IoT_AppendDoubleValue(attrVolt, value, time);
            if (i%5==0) {
                IoT_ResetDoubleValues(attrVolt);
            }
        }
    }

    enableVolt = IoT_AddSensorNode(myVoltGroup, "Enabled");
    if(enableVolt) {
        int i, time = 0;

        IoT_AppendBoolValueFull(enableVolt, true, 0, IoT_READONLY);
        for (i = 1; i < 15; i++) {
            time += 100;
            IoT_AppendBoolValue(enableVolt, (i%10==0), time);
            if (i%5==0) {
                IoT_ResetBoolValues(enableVolt);
            }
        }
    }

    // weather
    MSG_CLASSIFY_T *weatherGroup = IoT_AddGroup(myCapability, "Weather");

    today = IoT_AddSensorNode(weatherGroup, "Today");
    if(today) {
        IoT_AppendStringValueFull(today, "Sun", 0, IoT_READONLY);
        IoT_AppendStringValue(today, "Rain", 100);
        IoT_ResetStringValues(today);
        IoT_AppendStringValue(today, "Cloudy", 200);
        IoT_AppendStringValue(today, "Sun", 300);
    }

    // dump
    if(MSG_IsAttributeExist(myCapability, "Test/Voltage/Volt", true))
    {
        buffer = IoT_PrintCapability(myCapability);
        printf("IoT Format:\r\n%s\r\n", buffer);
        free(buffer);
    }

    IoT_ReleaseAll(myCapability);


    return iRet;
}
