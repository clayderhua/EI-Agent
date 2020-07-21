#ifndef __CJSON_UTIL_H__
#define __CJSON_UTIL_H__

#define cJSON_NewItem(item)  do {\
	item = cJSON_CreateObject();\
	if (!item) { goto error; } \
} while (0)

#define cJSON_AddObjectToObject(parent, name, item) do {\
	item = cJSON_CreateObject();\
	if (!item) { goto error; } \
	cJSON_AddItemToObject(parent, name, item);\
} while (0)

#define cJSON_IPSO_NewRWNumberItem(item, name, value)  do {\
	cJSON_NewItem(item); \
	if (!item) { goto error; } \
	cJSON_AddStringToObject(item, "n", name); \
	cJSON_AddNumberToObject(item, "v", value); \
	cJSON_AddStringToObject(item, "asm", "rw"); \
} while (0)

#define cJSON_IPSO_NewRONumberItem(item, name, value)  do {\
	cJSON_NewItem(item); \
	if (!item) { goto error; } \
	cJSON_AddStringToObject(item, "n", name); \
	cJSON_AddNumberToObject(item, "v", value); \
	cJSON_AddStringToObject(item, "asm", "r"); \
} while (0)

#define cJSON_IPSO_NewRWStringItem(item, name, value)  do {\
	cJSON_NewItem(item); \
	if (!item) { goto error; } \
	cJSON_AddStringToObject(item, "n", name); \
	cJSON_AddStringToObject(item, "sv", value); \
	cJSON_AddStringToObject(item, "asm", "rw"); \
} while (0)

#define cJSON_IPSO_NewROStringItem(item, name, value)  do {\
	cJSON_NewItem(item); \
	if (!item) { goto error; } \
	cJSON_AddStringToObject(item, "n", name); \
	cJSON_AddStringToObject(item, "sv", value); \
	cJSON_AddStringToObject(item, "asm", "r"); \
} while (0)

#endif
