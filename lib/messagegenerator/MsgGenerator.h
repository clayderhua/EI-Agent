#ifndef _MESSAGE_GENERATE_H_
#define _MESSAGE_GENERATE_H_
#include <stdbool.h>

#define DEF_NAME_SIZE				260
#define DEF_CLASSIFY_SIZE			32
#define DEF_UNIT_SIZE				64
#define DEF_VERSION_SIZE			16
#define DEF_ASM_SIZE				3

#define TAG_BASE_NAME		"bn"
#define TAG_ATTR_NAME		"n"
#define TAG_VERSION			"ver"
#define TAG_E_NODE			"e"
#define TAG_VALUE			"v"
#define TAG_BOOLEAN			"bv"
#define TAG_STRING			"sv"
#define TAG_TIME_ARRAY		"at"
#define TAG_VALUE_ARRAY		"av"
#define TAG_BOOLEAN_ARRAY	"abv"
#define TAG_STRING_ARRAY	"asv"
#define TAG_MAX				"max"
#define TAG_MIN				"min"
#define TAG_ASM				"asm"
#define TAG_UNIT			"u"
#define TAG_STATUS_CODE		"StatusCode"
#define TAG_DATE			"$date"
#define TAG_TIMESTAMP		"$timestamp"
#define TAG_OPTS			"opTS"

typedef enum {
	attr_type_unknown = 0,
	attr_type_numeric = 1,
	attr_type_boolean = 2,
	attr_type_string = 3,
	attr_type_date = 4,
	attr_type_timestamp = 5,
	attr_type_numeric_array = 6,
	attr_type_boolean_array = 7,
	attr_type_string_array = 8,
} attr_type;

typedef enum {
	class_type_root = 0,
	class_type_object = 1,
	class_type_array = 2,
} class_type;

typedef void  (*AttributeChangedCbf) ( void* attribute, void* pRev1); 

typedef struct ext_attr{
	char name[DEF_NAME_SIZE];
	attr_type type;
	union
	{
		double v;
		bool bv;
		char* sv;
	};
	char strvalue[DEF_NAME_SIZE];
	long strlen;
	struct ext_attr *next;
}EXT_ATTRIBUTE_T;

typedef struct msg_attr{
	char name[DEF_NAME_SIZE];
	char readwritemode[DEF_ASM_SIZE];
	attr_type type;
	union
	{
		double v;
		bool bv;
		char* sv;
	};
	union
	{
		double *av;
		bool *abv;
		char **asv;
	};
	int* at;
	char strvalue[DEF_NAME_SIZE];
	long strlen;
	double max;
	double min;
	char unit[DEF_UNIT_SIZE];
	bool bRange;
	bool bSensor;
	bool bNull;
	int value_array_cap; // the capacity of value_array
	int value_array_len; // the length of value_array
	struct msg_attr *next;
	struct ext_attr *extra;

	AttributeChangedCbf on_datachanged;
	void *pRev1;

}MSG_ATTRIBUTE_T;

typedef struct msg_class{
	char classname[DEF_NAME_SIZE];
	char version[DEF_VERSION_SIZE];
	bool bIoTFormat;
	class_type type;
	struct msg_attr *attr_list;
	struct msg_class *sub_list;
	struct msg_class *next;

	AttributeChangedCbf on_datachanged;
	void *pRev1;

}MSG_CLASSIFY_T;

#ifdef __cplusplus
extern "C" {
#endif
	long long MSG_GetTimeTick();

	MSG_CLASSIFY_T* MSG_CreateRoot();
	MSG_CLASSIFY_T* MSG_CreateRootEx(AttributeChangedCbf onchanged, void* pRev1);
	void MSG_ReleaseRoot(MSG_CLASSIFY_T* classify);
	MSG_CLASSIFY_T* MSG_Clone(MSG_CLASSIFY_T* classify, bool bRecursive);


	MSG_CLASSIFY_T* MSG_AddClassify(MSG_CLASSIFY_T *pNode, char const* name, char const* version, bool bArray, bool isIoT);
	MSG_CLASSIFY_T* MSG_FindClassify(MSG_CLASSIFY_T* pNode, char const* name);
	bool MSG_DelClassify(MSG_CLASSIFY_T* pNode, char* name);

	MSG_ATTRIBUTE_T* MSG_AddAttribute(MSG_CLASSIFY_T* pClass, char const* attrname, bool isSensorData);
	MSG_ATTRIBUTE_T* MSG_FindAttribute(MSG_CLASSIFY_T* root, char const* attrname, bool isSensorData);
	bool MSG_DelAttribute(MSG_CLASSIFY_T* pNode, char* name, bool isSensorData);

	bool MSG_SetFloatValue(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, char *unit);
	bool MSG_SetFloatValueWithMaxMin(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, float max, float min, char *unit);
	bool MSG_SetDoubleValue(MSG_ATTRIBUTE_T* attr, double value, char* readwritemode, char *unit);
	bool MSG_SetDoubleValueWithMaxMin(MSG_ATTRIBUTE_T* attr, double value, char* readwritemode, double max, double min, char *unit);
	bool MSG_SetBoolValue(MSG_ATTRIBUTE_T* attr, bool bvalue, char* readwritemode);
	bool MSG_SetStringValue(MSG_ATTRIBUTE_T* attr, char *svalue, char* readwritemode);

	void MSG_SetMaxArrayValueSize(int v);
	bool MSG_AppendDoubleValueFull(MSG_ATTRIBUTE_T* attr,
								   double value,
								   int time,
								   char* readwritemode,
								   char *unit,
								   bool hasMaxMin,
								   double max,
								   double min);
	bool MSG_AppendDoubleValue(MSG_ATTRIBUTE_T* attr, double value, int time);
	bool MSG_ResetDoubleValues(MSG_ATTRIBUTE_T* attr);

	bool MSG_AppendBoolValueFull(MSG_ATTRIBUTE_T* attr,
								 bool value,
								 int time,
								 char* readwritemode);
	bool MSG_AppendBoolValue(MSG_ATTRIBUTE_T* attr, bool value, int time);
	bool MSG_ResetBoolValues(MSG_ATTRIBUTE_T* attr);

	bool MSG_AppendStringValueFull(MSG_ATTRIBUTE_T* attr,
								   char* value,
								   int time,
								   char* readwritemode);
	bool MSG_AppendStringValue(MSG_ATTRIBUTE_T* attr, char* value, int time);
	bool MSG_ResetStringValues(MSG_ATTRIBUTE_T* attr);

	bool MSG_SetTimestampValue(MSG_ATTRIBUTE_T* attr, unsigned int value, char* readwritemode);
	bool MSG_SetDateValue(MSG_ATTRIBUTE_T* attr, char *svalue, char* readwritemode);
	bool MSG_SetNULLValue(MSG_ATTRIBUTE_T* attr, char* readwritemode);

	MSG_ATTRIBUTE_T* MSG_FindAttributeWithPath(MSG_CLASSIFY_T *msg,char *path, bool isSensorData);
	bool MSG_IsAttributeExist(MSG_CLASSIFY_T *msg,char *path, bool isSensorData);

	char* MSG_JSONPrintUnformatted(MSG_CLASSIFY_T* msg, bool bflat, bool bsimulate);
	char *MSG_JSONPrintWithFiltered(MSG_CLASSIFY_T* msg, char** filter, int length, bool bflat, bool bsimulate);
	char *MSG_JSONPrintSelectedWithFiltered(MSG_CLASSIFY_T* msg, char** filter, int length, char* reqItems, bool bflat, bool bsimulate);

#define MSG_PrintUnformatted(msg) MSG_JSONPrintUnformatted(msg, false, false);
#define MSG_PrintWithFiltered(msg, filter, length) MSG_JSONPrintWithFiltered(msg, filter, length, false, false);
#define MSG_PrintSelectedWithFiltered(msg, filter, length, reqItems) MSG_JSONPrintSelectedWithFiltered(msg, filter, length, reqItems, false, false);

#define MSG_PrintFlatUnformatted(msg, simulate) MSG_JSONPrintUnformatted(msg, true, simulate);
#define MSG_PrintFlatWithFiltered(msg, filter, length, simulate) MSG_JSONPrintWithFiltered(msg, filter, length, true, simulate);
#define MSG_PrintSelectedFlatWithFiltered(msg, filter, length, reqItems, simulate) MSG_JSONPrintSelectedWithFiltered(msg, filter, length, reqItems, true, simulate);

	void MSG_SetDataChangeCallback(MSG_CLASSIFY_T* msg, AttributeChangedCbf on_datachanged, void* pRev1);
	
// Add new classify node as child of pNode.
#define MSG_AddJSONClassify(pNode, name, version, bArray) MSG_AddClassify(pNode, name, version, bArray, false);
// Add new IOT classify node as child of pNode.
#define MSG_AddIoTClassify(pNode, name, version, bArray) MSG_AddClassify(pNode, name, version, bArray, true);

#define MSG_AddJSONAttribute(pClass, attrname) MSG_AddAttribute(pClass, attrname, false);
#define MSG_AddIoTSensor(pClass, attrname) MSG_AddAttribute(pClass, attrname, true);
	bool MSG_AppendIoTSensorAttributeDouble(MSG_ATTRIBUTE_T* attr, const char* attrname, double value);
	bool MSG_AppendIoTSensorAttributeBool(MSG_ATTRIBUTE_T* attr, const char* attrname, bool bvalue);
	bool MSG_AppendIoTSensorAttributeString(MSG_ATTRIBUTE_T* attr, const char* attrname, char *svalue);
	bool MSG_AppendIoTSensorAttributeTimestamp(MSG_ATTRIBUTE_T* attr, const char* attrname, unsigned int value);
	bool MSG_AppendIoTSensorAttributeDate(MSG_ATTRIBUTE_T* attr, const char* attrname, char *svalue);

#define MSG_FindJSONAttribute(pClass, attrname) MSG_FindAttribute(pClass, attrname, false);
#define MSG_FindIoTSensor(pClass, attrname) MSG_FindAttribute(pClass, attrname, true);

#define MSG_DelJSONAttribute(pClass, attrname) MSG_DelAttribute(pClass, attrname, false);
#define MSG_DelIoTSensor(pClass, attrname) MSG_DelAttribute(pClass, attrname, true);

#ifdef __cplusplus
}
#endif
#endif