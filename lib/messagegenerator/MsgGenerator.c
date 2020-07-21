#include "MsgGenerator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "WISEPlatform.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "util_string.h"

#define DEF_VALUE_ARRAY_CAP_SIZE		10
#define DEF_VALUE_ARRAY_MAX_CAP_SIZE	100

static int g_maxArrayValueSize = DEF_VALUE_ARRAY_MAX_CAP_SIZE;

long long MSG_GetTimeTick()
{
	long long tick = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tick = (long long)tv.tv_sec*1000 + (long long)tv.tv_usec/1000;
	return tick;
}

#pragma region Add_Resource
MSG_CLASSIFY_T* MSG_CreateRoot()
{
	MSG_CLASSIFY_T *pMsg = malloc(sizeof(MSG_CLASSIFY_T));
	if(pMsg)
	{
		memset(pMsg, 0, sizeof(MSG_CLASSIFY_T));
		pMsg->type = class_type_root;
	}
	return pMsg;
}

MSG_CLASSIFY_T* MSG_CreateRootEx(AttributeChangedCbf onchanged, void* pRev1)
{
	MSG_CLASSIFY_T *pMsg = malloc(sizeof(MSG_CLASSIFY_T));
	if(pMsg)
	{
		memset(pMsg, 0, sizeof(MSG_CLASSIFY_T));
		pMsg->type = class_type_root;
		pMsg->on_datachanged = onchanged;
		pMsg->pRev1 = pRev1;
	}
	return pMsg;
}

MSG_CLASSIFY_T* MSG_AddClassify(MSG_CLASSIFY_T *pNode, char const* name, char const* version, bool bArray, bool isIoT)
{
	MSG_CLASSIFY_T *pCurNode = NULL;
	if(!pNode || !name)
		return pNode;

	pCurNode = malloc(sizeof(MSG_CLASSIFY_T));
	if(pCurNode)
	{
		memset(pCurNode, 0, sizeof(MSG_CLASSIFY_T));
		pCurNode->type = bArray ? class_type_array : class_type_object;
		if(name)
		{
			char* newname = StringReplace(name, "/", "@2F");
			if(newname) {
				strncpy(pCurNode->classname, newname, sizeof(pCurNode->classname));
				StringFree(newname);
			}
			//strncpy(pCurNode->classname, name, strlen(name));
		}

		if(version)
		{
			strncpy(pCurNode->version, version, sizeof(pCurNode->version));
		}

		pCurNode->bIoTFormat = isIoT;

		if(!pNode->sub_list)
		{
			pNode->sub_list = pCurNode;
		}
		else
		{
			MSG_CLASSIFY_T *pLastNode = pNode->sub_list;
			while(pLastNode->next)
			{
				pLastNode = pLastNode->next;
			}
			pLastNode->next = pCurNode;

			pCurNode->on_datachanged = pNode->on_datachanged;
			pCurNode->pRev1 = pNode->pRev1;
		}
	}
	return pCurNode;
}

MSG_ATTRIBUTE_T* MSG_AddAttribute(MSG_CLASSIFY_T* pClass, char const* attrname, bool isSensorData)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* lastAttr = NULL;
	if(!pClass)
		return curAttr;
	curAttr = malloc(sizeof(MSG_ATTRIBUTE_T));
	if(curAttr)
	{
		memset(curAttr, 0, sizeof(MSG_ATTRIBUTE_T));

		if(attrname && strlen(attrname) > 0)
		{
			char* newname = StringReplace(attrname, "/", "@2F");
			if(newname) {
				strncpy(curAttr->name, newname, sizeof(curAttr->name));
				StringFree(newname);
			}
			//strncpy(curAttr->name, attrname, strlen(attrname));
		}
		curAttr->bSensor = isSensorData;

		if(pClass->attr_list == NULL)
		{
			pClass->attr_list = curAttr;
		}
		else
		{
			lastAttr = pClass->attr_list;
			while(lastAttr->next)
			{
				lastAttr = lastAttr->next;
			}
			lastAttr->next = curAttr;
			curAttr->on_datachanged = pClass->on_datachanged;
			curAttr->pRev1 = pClass->pRev1;
		}
	}
	return curAttr;
}
#pragma endregion Add_Resource

#pragma region Release_Resource
void ReleaseAttribute(MSG_ATTRIBUTE_T* attr)
{
	int i;

	if(!attr)
		return;

	if((attr->type == attr_type_date || attr->type == attr_type_string) && !attr->bNull)
	{
		if(attr->sv && (attr->strlen >= sizeof(attr->strvalue)))
		{
			free(attr->sv);
		}
		attr->sv = NULL;
		memset(attr->strvalue, 0, sizeof(attr->strvalue));
		attr->strlen = 0;
		attr->on_datachanged = NULL;
		attr->pRev1 = NULL;
	}

	if (attr->type == attr_type_numeric_array && attr->av) {
		free(attr->av);
		attr->av = NULL;
	} else if (attr->type == attr_type_boolean_array && attr->abv) {
		free(attr->abv);
		attr->abv = NULL;
	} else if (attr->type == attr_type_string_array) {
		if (attr->asv) {
			for (i = 0; i < attr->value_array_len; i++) {
				if (attr->asv[i]) {
					free(attr->asv[i]);
					attr->asv[i] = NULL;
				}
			}
			free(attr->asv);
			attr->asv = NULL;
		}
		if (attr->sv) {
			free(attr->sv);
			attr->sv = NULL;
		}
	}
	if (attr->at) {
		free(attr->at);
		attr->at = NULL;
	}

	{
		EXT_ATTRIBUTE_T* extattr = attr->extra;
		while(extattr)
		{
			EXT_ATTRIBUTE_T* extnext = extattr->next;
			if(extattr->type == attr_type_date || extattr->type == attr_type_string)
			{
				if(extattr->sv && (extattr->strlen >= sizeof(extattr->strvalue)))
				{
					free(extattr->sv);
				}
				extattr->sv = NULL;

				memset(extattr->strvalue, 0, sizeof(extattr->strvalue));
				extattr->strlen = 0;
			}
			free(extattr);
			extattr = extnext;
		}
	}

	free(attr);
}

void ReleaseClassify(MSG_CLASSIFY_T* classify)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* nxtAttr = NULL;

	MSG_CLASSIFY_T* curSubtype = NULL;
	MSG_CLASSIFY_T* nxtSubtype = NULL;

	if(!classify)
		return;
	curAttr = classify->attr_list;
	while (curAttr)
	{
		nxtAttr = curAttr->next;
		ReleaseAttribute(curAttr);
		curAttr = nxtAttr;
	}

	curSubtype = classify->sub_list;
	while (curSubtype)
	{
		nxtSubtype = curSubtype->next;
		ReleaseClassify(curSubtype);
		curSubtype = nxtSubtype;
	}

	classify->on_datachanged = NULL;
	classify->pRev1 = NULL;
	free(classify);
}

bool MSG_DelAttribute(MSG_CLASSIFY_T* pNode, char* name, bool isSensorData)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* nxtAttr = NULL;
	char* newname = NULL;
	if(!pNode || !name)
		return false;

	newname = StringReplace(name, "/", "@2F");
	if(newname == NULL) {
		return false;
	}
	curAttr = pNode->attr_list;
	if(curAttr->bSensor == isSensorData)
	{
		if(!strcmp(curAttr->name, newname))
		{
			pNode->attr_list = curAttr->next;
			ReleaseAttribute(curAttr);
			StringFree(newname);
			return true;
		}
	}

	nxtAttr = curAttr->next;
	while (nxtAttr)
	{
		if(nxtAttr->bSensor == isSensorData)
		{
			if(!strcmp(nxtAttr->name, newname))
			{
				curAttr->next = nxtAttr->next;
				ReleaseAttribute(nxtAttr);
				StringFree(newname);
				return true;
			}
		}
		curAttr = nxtAttr;
		nxtAttr = curAttr->next;
	}
	StringFree(newname);
	return false;
}

bool MSG_DelClassify(MSG_CLASSIFY_T* pNode, char* name)
{
	MSG_CLASSIFY_T* curClass = NULL;
	MSG_CLASSIFY_T* nxtClass = NULL;
	char* newname = NULL;
	if(!pNode || !name)
		return false;
	newname = StringReplace(name, "/", "@2F");
	if(newname == NULL) {
		return false;
	}
	curClass = pNode->sub_list;
	if(!strcmp(curClass->classname, newname))
	{
		pNode->sub_list = curClass->next;
		ReleaseClassify(curClass);
		StringFree(newname);
		return true;
	}

	nxtClass = curClass->next;
	while (nxtClass)
	{
		if(!strcmp(nxtClass->classname, newname))
		{
			curClass->next = nxtClass->next;
			ReleaseClassify(nxtClass);
			StringFree(newname);
			return true;
		}
		curClass = nxtClass;
		nxtClass = curClass->next;
	}
	StringFree(newname);
	return false;
}

void MSG_ReleaseRoot(MSG_CLASSIFY_T* classify)
{
	ReleaseClassify(classify);
}

EXT_ATTRIBUTE_T * MSG_CloneExtraAttribute(struct ext_attr *extra, bool bRecursive)
{
	EXT_ATTRIBUTE_T *target = NULL;
	if(!extra)
		return target;

	target = malloc(sizeof(EXT_ATTRIBUTE_T));
	memset(target, 0, sizeof(EXT_ATTRIBUTE_T));

	strcpy(target->name, extra->name);
	target->type = extra->type;
	switch (target->type)
	{
	case attr_type_numeric:
		target->v = extra->v;
		break;
	case attr_type_boolean:
		target->bv = extra->bv;
		break;
	case attr_type_string:
	case  attr_type_date:
	case  attr_type_timestamp:
		target->strlen = extra->strlen;
		if(extra->sv && (extra->strlen >= sizeof(extra->strvalue)))
		{
			target->sv = strdup(extra->sv);
		} else {
			memset(target->strvalue, 0, sizeof(target->strvalue));
			memcpy(target->strvalue, extra->strvalue, extra->strlen);
			target->sv = target->strvalue;
		}

		break;
	default:
		break;
	}
	return target;
}

MSG_ATTRIBUTE_T* MSG_CloneAttribute(MSG_ATTRIBUTE_T* attribute, bool bRecursive)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;
	struct ext_attr *curextra = NULL;
	if(!attribute)
		return curAttr;

	curAttr = malloc(sizeof(MSG_ATTRIBUTE_T));
	if(curAttr)
	{
		memset(curAttr, 0, sizeof(MSG_ATTRIBUTE_T));

		if( strlen(attribute->name)>0 )
		{
			strncpy(curAttr->name, attribute->name, strlen(attribute->name));
		}
		strcpy(curAttr->readwritemode, attribute->readwritemode);
		curAttr->type = attribute->type;
		switch (curAttr->type)
		{
		case attr_type_numeric:
			curAttr->v = attribute->v;
			break;
		case attr_type_boolean:
			curAttr->bv = attribute->bv;
			break;
		case attr_type_string:
		case  attr_type_date:
		case  attr_type_timestamp:
			curAttr->strlen = attribute->strlen;
			if(curAttr->strlen >= sizeof(curAttr->strvalue))
				curAttr->sv = strdup(attribute->sv);
			else
			{
				strcpy(curAttr->strvalue, attribute->strvalue);
				curAttr->sv = curAttr->strvalue;
			}
			break;
		case attr_type_numeric_array:
			curAttr->value_array_cap = attribute->value_array_cap;
			curAttr->value_array_len = attribute->value_array_len;
			// allocate data
			curAttr->av = (double*) malloc(sizeof(double) * curAttr->value_array_cap);
			curAttr->at = (int*) malloc(sizeof(int) * curAttr->value_array_cap);
			// copy data
			memcpy(curAttr->av, attribute->av, sizeof(double) * curAttr->value_array_len);
			memcpy(curAttr->at, attribute->at, sizeof(int) * curAttr->value_array_len);
			break;

		case attr_type_boolean_array:
			curAttr->value_array_cap = attribute->value_array_cap;
			curAttr->value_array_len = attribute->value_array_len;
			// allocate data
			curAttr->abv = (bool*) malloc(sizeof(bool) * curAttr->value_array_cap);
			curAttr->at = (int*) malloc(sizeof(int) * curAttr->value_array_cap);
			// copy data
			memcpy(curAttr->abv, attribute->av, sizeof(bool) * curAttr->value_array_len);
			memcpy(curAttr->at, attribute->at, sizeof(int) * curAttr->value_array_len);
			break;

		default:
			break;
		}
		curAttr->max = attribute->max;
		curAttr->min = attribute->min;
		strcpy(curAttr->unit, attribute->unit);
		curAttr->bRange = attribute->bRange;
		curAttr->bSensor = attribute->bSensor;
		curAttr->bNull = attribute->bNull;
		curAttr->on_datachanged = attribute->on_datachanged;
		curAttr->pRev1 = attribute->pRev1;

	}

	curextra = attribute->extra;
	while(curextra)
	{
		EXT_ATTRIBUTE_T *clonedextra = MSG_CloneExtraAttribute(curextra, bRecursive);

		if(curAttr->extra == NULL)
			curAttr->extra = clonedextra;
		else
		{
			EXT_ATTRIBUTE_T* tmpextra = curAttr->extra;
			while(tmpextra)
			{
				if(tmpextra->next == NULL)
				{
					tmpextra->next = clonedextra;
					break;
				}
				tmpextra = tmpextra->next;
			}
		}

		curextra = curextra->next;
	}
	return curAttr;
}

MSG_CLASSIFY_T* MSG_Clone(MSG_CLASSIFY_T* classify, bool bRecursive)
{
	MSG_CLASSIFY_T* clone = NULL;
	if(classify == NULL)
		return clone;

	clone = malloc(sizeof(MSG_CLASSIFY_T));
	if(clone)
	{
		memset(clone, 0, sizeof(MSG_CLASSIFY_T));
		strcpy(clone->classname, classify->classname);
		strcpy(clone->version, classify->version);
		clone->bIoTFormat = classify->bIoTFormat;
		clone->type = classify->type;
		clone->on_datachanged = classify->on_datachanged;
		clone->pRev1 = classify->pRev1;
	}
	if(bRecursive)
	{
		MSG_CLASSIFY_T* child = classify->sub_list;
		MSG_ATTRIBUTE_T* attr = classify->attr_list;

		while(child)
		{
			MSG_CLASSIFY_T* clonedchild = NULL;
			clonedchild = MSG_Clone(child, bRecursive);

			if(clone->sub_list == NULL)
				clone->sub_list = clonedchild;
			else
			{
				MSG_CLASSIFY_T* tmpchild = clone->sub_list;
				while(tmpchild)
				{
					if(tmpchild->next == NULL)
					{
						tmpchild->next = clonedchild;
						break;
					}
					tmpchild = tmpchild->next;
				}
			}

			child = child->next;
		}


		while(attr)
		{
			MSG_ATTRIBUTE_T* clonedattr = NULL;
			clonedattr = MSG_CloneAttribute(attr, bRecursive);

			if(clone->attr_list == NULL)
				clone->attr_list = clonedattr;
			else
			{
				MSG_ATTRIBUTE_T* tmpattr = clone->attr_list;
				while(tmpattr)
				{
					if(tmpattr->next == NULL)
					{
						tmpattr->next = clonedattr;
						break;
					}
					tmpattr = tmpattr->next;
				}
			}

			attr = attr->next;
		}

	}
	return clone;
}

#pragma endregion Release_Resource

#pragma region Find_Resource
MSG_CLASSIFY_T* MSG_FindClassifyWithoutPrefix(MSG_CLASSIFY_T* pNode, char const* name)
{
	char* index = 0;
	MSG_CLASSIFY_T* curClass = NULL;
	MSG_CLASSIFY_T* nxtClass = NULL;
	char* newname = NULL;
	int offset = strlen("@2D");
	if(!pNode || !name)
		return curClass;
	newname = StringReplace(name, "/", "@2F");
	if(newname == NULL) {
		return curClass;
	}
	curClass = pNode->sub_list;
	while (curClass)
	{
		char* prefix = NULL;
		nxtClass = curClass->next;

		index = strstr(curClass->classname, "@2D");
		if(index == 0)
		{
			curClass = nxtClass;
			continue;
		}

		prefix = index+offset;

		if(!strncmp(prefix, newname, strlen(index)-offset))
		{
			StringFree(newname);
			return curClass;
		}
		curClass = nxtClass;
	}
	StringFree(newname);
	return NULL;
}

MSG_CLASSIFY_T* MSG_FindClassifyWithPrefix(MSG_CLASSIFY_T* pNode, char const* name)
{
	char* index = 0;
	MSG_CLASSIFY_T* curClass = NULL;
	MSG_CLASSIFY_T* nxtClass = NULL;
	char* newname = NULL;
	if(!pNode || !name)
		return curClass;
	newname = StringReplace(name, "/", "@2F");
	if(newname == NULL) {
		return curClass;
	}
	curClass = pNode->sub_list;
	while (curClass)
	{
		nxtClass = curClass->next;

		index = strstr(curClass->classname, "@2D");
		if(index == 0)
		{
			curClass = nxtClass;
			continue;
		}

		if(!strncmp(curClass->classname, newname, index - curClass->classname))
		{
			StringFree(newname);
			return curClass;
		}
		curClass = nxtClass;
	}
	StringFree(newname);
	return MSG_FindClassifyWithoutPrefix(pNode, name);
}

MSG_CLASSIFY_T* MSG_FindClassify(MSG_CLASSIFY_T* pNode, char const* name)
{
	MSG_CLASSIFY_T* curClass = NULL;
	MSG_CLASSIFY_T* nxtClass = NULL;
	char* newname = NULL;
	if(!pNode || !name)
		return curClass;
	newname = StringReplace(name, "/", "@2F");
	if(newname == NULL) {
		return curClass;
	}
	curClass = pNode->sub_list;
	while (curClass)
	{
		nxtClass = curClass->next;
		if(!strcmp(curClass->classname, newname)) {
			StringFree(newname);
			return curClass;
		}
		curClass = nxtClass;
	}
	StringFree(newname);
	return MSG_FindClassifyWithPrefix(pNode, name);
}

MSG_ATTRIBUTE_T* MSG_FindAttribute(MSG_CLASSIFY_T* root, char const* senname, bool isSensorData)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* nxtAttr = NULL;
	char* newname = NULL;
	if(!root || !senname)
		return curAttr;
	newname = StringReplace(senname, "/", "@2F");
	if(newname == NULL) {
		return curAttr;
	}
	curAttr = root->attr_list;
	while (curAttr)
	{
		nxtAttr = curAttr->next;
		if(curAttr->bSensor == isSensorData)
		{
			if(!strcmp(curAttr->name, newname)) {
				StringFree(newname);
				return curAttr;
			}
		}
		curAttr = nxtAttr;
	}
	StringFree(newname);
	return NULL;
}

bool MSG_SetFloatValue(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, char *unit)
{
	return MSG_SetDoubleValue(attr, value, readwritemode, unit);
}

bool MSG_SetFloatValueWithMaxMin(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, float max, float min, char *unit)
{
	return MSG_SetDoubleValueWithMaxMin(attr, value, readwritemode, max, min, unit);
}

bool MSG_SetDoubleValue(MSG_ATTRIBUTE_T* attr, double value, char* readwritemode, char *unit)
{
	bool bNotify = false;
	if(!attr)
		return false;
	if(attr->type != attr_type_numeric)
		bNotify = true;
	else if(attr->v != value)
		bNotify = true;

	if((attr->type == attr_type_date || attr->type == attr_type_string) && !attr->bNull)
	{
		if(attr->sv && (attr->strlen >= sizeof(attr->strvalue)))
			free(attr->sv);
		attr->sv = NULL;
		memset(attr->strvalue, 0, sizeof(attr->strvalue));
		attr->strlen = 0;
	}
	attr->v = value;
	attr->type = attr_type_numeric;
	attr->bRange = false;
	attr->bNull = false;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	if(unit)
		strncpy(attr->unit, unit, strlen(unit));

	if(bNotify)
		if(attr->on_datachanged)
			attr->on_datachanged(attr, attr->pRev1);

	return true;
}

bool MSG_SetDoubleValueWithMaxMin(MSG_ATTRIBUTE_T* attr, double value, char* readwritemode, double max, double min, char *unit)
{
	bool bNotify = false;
	if(!attr)
		return false;
	if(attr->type != attr_type_numeric)
		bNotify = true;
	else if(attr->v != value)
		bNotify = true;

	if((attr->type == attr_type_date || attr->type == attr_type_string) && !attr->bNull)
	{
		if(attr->sv && (attr->strlen >= sizeof(attr->strvalue)))
			free(attr->sv);
		attr->sv = NULL;
		memset(attr->strvalue, 0, sizeof(attr->strvalue));
		attr->strlen = 0;
	}
	attr->v = value;
	attr->type = attr_type_numeric;
	attr->bRange = true;
	attr->bNull = false;
	attr->max = max;
	attr->min = min;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	if(unit)
		strncpy(attr->unit, unit, strlen(unit));

	if(bNotify)
		if(attr->on_datachanged)
			attr->on_datachanged(attr, attr->pRev1);
	return true;
}

void MSG_SetMaxArrayValueSize(int v)
{
	g_maxArrayValueSize = v;
}

/*
	Append a double value to 'av' field. (value_array)
	readwritemode: read write mode string or
	hasMaxMin: true will set max/min, else ignore it
*/
bool MSG_AppendDoubleValueFull(MSG_ATTRIBUTE_T* attr,
							   double value,
							   int time,
							   char* readwritemode,
							   char *unit,
							   bool hasMaxMin,
							   double max,
							   double min)
{
	if(!attr)
		return false;
	if(attr->type != attr_type_unknown && attr->type != attr_type_numeric_array) {
		fprintf(stderr, "MSG_AppendDoubleValueFull invalid attr->type=%d\n", attr->type);
		return false;
	}

	// save latest value for live report and capability
	attr->v = value;

	// allocate new array space for this
	if (!attr->av) {
		attr->value_array_cap = DEF_VALUE_ARRAY_CAP_SIZE;
		attr->value_array_len = 0;
		attr->type = attr_type_numeric_array;
		attr->bRange = false;
		attr->bNull = false;
		attr->av = (double*) malloc(attr->value_array_cap * sizeof(double));
		attr->at = (int*) malloc(attr->value_array_cap * sizeof(int));
	}
	
	if (attr->value_array_len >= g_maxArrayValueSize) {
		return false; // reach the max size limit
	}

	// reallocate array if necessary
	if (attr->value_array_len >= attr->value_array_cap) {
		double *newAv = NULL;
		int *newAt = NULL;

		// reallocate
		attr->value_array_cap = ((2 * attr->value_array_cap) > g_maxArrayValueSize)? g_maxArrayValueSize: (2 * attr->value_array_cap);
		newAv = (double*) malloc(attr->value_array_cap * sizeof(double));
		newAt = (int*) malloc(attr->value_array_cap * sizeof(int));

		// copy data
		memcpy(newAv, attr->av, attr->value_array_len * sizeof(double));
		memcpy(newAt, attr->at, attr->value_array_len * sizeof(int));

		// free data
		free(attr->av);
		free(attr->at);

		// assign data
		attr->av = newAv;
		attr->at = newAt;
	}

	// append value
	attr->av[attr->value_array_len] = value;
	attr->at[attr->value_array_len] = time;
	attr->value_array_len++;

	if (readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	if (unit)
		strncpy(attr->unit, unit, strlen(unit));
	if (hasMaxMin) {
		attr->bRange = true;
		attr->max = max;
		attr->min = min;
	}

	// append is always change
	if(attr->on_datachanged)
		attr->on_datachanged(attr, attr->pRev1);

	return true;
}

bool MSG_AppendDoubleValue(MSG_ATTRIBUTE_T* attr, double value, int time)
{
	return MSG_AppendDoubleValueFull(attr, value, time, NULL, NULL, 0, 0, 0);
}

bool MSG_ResetDoubleValues(MSG_ATTRIBUTE_T* attr)
{
	if(!attr || attr->type != attr_type_numeric_array)
		return false;

	attr->value_array_len = 0; // reset data
	return true;
}

/*
	Append a boolean value to 'abv' field. (value_array)
	readwritemode: read write mode string or
	hasMaxMin: true will set max/min, else ignore it
*/
bool MSG_AppendBoolValueFull(MSG_ATTRIBUTE_T* attr,
							 bool value,
							 int time,
							 char* readwritemode)
{
	if(!attr)
		return false;

	if(attr->type != attr_type_unknown && attr->type != attr_type_boolean_array) {
		fprintf(stderr, "MSG_AppendBoolValueFull invalid attr->type=%d\n", attr->type);
		return false;
	}

	// save latest value for live report and capability
	attr->bv = value;

	// allocate new array space for this
	if (!attr->abv) {
		attr->value_array_cap = DEF_VALUE_ARRAY_CAP_SIZE;
		attr->value_array_len = 0;
		attr->type = attr_type_boolean_array;
		attr->bRange = false;
		attr->bNull = false;
		attr->abv = (bool*) malloc(attr->value_array_cap * sizeof(bool));
		attr->at = (int*) malloc(attr->value_array_cap * sizeof(int));
	}

	if (attr->value_array_len >= g_maxArrayValueSize) {
		return false; // reach the max size limit
	}

	// reallocate array if necessary
	if (attr->value_array_len >= attr->value_array_cap) {
		bool *newAbv = NULL;
		int *newAt = NULL;

		// reallocate
		attr->value_array_cap = ((2 * attr->value_array_cap) > g_maxArrayValueSize)? g_maxArrayValueSize: (2 * attr->value_array_cap);
		newAbv = (bool*) malloc(attr->value_array_cap * sizeof(bool));
		newAt = (int*) malloc(attr->value_array_cap * sizeof(int));

		// copy data
		memcpy(newAbv, attr->abv, attr->value_array_len * sizeof(bool));
		memcpy(newAt, attr->at, attr->value_array_len * sizeof(int));

		// free data
		free(attr->abv);
		free(attr->at);

		// assign data
		attr->abv = newAbv;
		attr->at = newAt;
	}

	// append value
	attr->abv[attr->value_array_len] = value;
	attr->at[attr->value_array_len] = time;
	attr->value_array_len++;

	if (readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));

	// append is always change
	if(attr->on_datachanged)
		attr->on_datachanged(attr, attr->pRev1);

	return true;
}

bool MSG_AppendBoolValue(MSG_ATTRIBUTE_T* attr, bool value, int time)
{
	return MSG_AppendBoolValueFull(attr, value, time, NULL);
}

bool MSG_ResetBoolValues(MSG_ATTRIBUTE_T* attr)
{
	if(!attr || attr->type != attr_type_boolean_array)
		return false;

	attr->value_array_len = 0; // reset data
	return true;
}

/*
	Append a boolean value to 'asv' field. (value_array)
	readwritemode: read write mode string or
	hasMaxMin: true will set max/min, else ignore it
*/
bool MSG_AppendStringValueFull(MSG_ATTRIBUTE_T* attr,
							   char* value,
							   int time,
							   char* readwritemode)
{
	char *toRelease = NULL, *newAlloc;

	if(!attr)
		return false;

	if(attr->type != attr_type_unknown && attr->type != attr_type_string_array) {
		fprintf(stderr, "MSG_AppendBoolValueFull invalid attr->type=%d\n", attr->type);
		return false;
	}

	// save latest value for live report and capability
	toRelease = attr->sv;
	newAlloc = (char*) malloc(strlen(value)+1);
	strcpy(newAlloc, value);
	attr->sv = newAlloc;
	if (toRelease) {
		free(toRelease);
	}

	// allocate new array space for this
	if (!attr->asv) {
		attr->value_array_cap = DEF_VALUE_ARRAY_CAP_SIZE;
		attr->value_array_len = 0;
		attr->type = attr_type_string_array;
		attr->bRange = false;
		attr->bNull = false;
		attr->asv = (char**) malloc(attr->value_array_cap * sizeof(char**));
		attr->at = (int*) malloc(attr->value_array_cap * sizeof(int));
	}

	if (attr->value_array_len >= g_maxArrayValueSize) {
		return false; // reach the max size limit
	}

	// reallocate array if necessary
	if (attr->value_array_len >= attr->value_array_cap) {
		char **newAsv = NULL;
		int *newAt = NULL;

		// reallocate
		attr->value_array_cap = ((2 * attr->value_array_cap) > g_maxArrayValueSize)? g_maxArrayValueSize: (2 * attr->value_array_cap);
		newAsv = (char**) malloc(attr->value_array_cap * sizeof(char**));
		newAt = (int*) malloc(attr->value_array_cap * sizeof(int));

		// copy data
		memcpy(newAsv, attr->asv, attr->value_array_len * sizeof(char**));
		memcpy(newAt, attr->at, attr->value_array_len * sizeof(int));

		// free data
		free(attr->asv);
		free(attr->at);

		// assign datat
		attr->asv = newAsv;
		attr->at = newAt;
	}

	// append value
	attr->asv[attr->value_array_len] = (char*) malloc(strlen(value)+1);
	strcpy(attr->asv[attr->value_array_len], value);
	attr->at[attr->value_array_len] = time;
	attr->value_array_len++;

	if (readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));

	// append is always change
	if(attr->on_datachanged)
		attr->on_datachanged(attr, attr->pRev1);

	return true;
}

bool MSG_AppendStringValue(MSG_ATTRIBUTE_T* attr, char* value, int time)
{
	return MSG_AppendStringValueFull(attr, value, time, NULL);
}

bool MSG_ResetStringValues(MSG_ATTRIBUTE_T* attr)
{
	int i;

	if(!attr || attr->type != attr_type_string_array)
		return false;

	for (i = 0; i < attr->value_array_len; i++) {
		if (attr->asv[i]) {
			free(attr->asv[i]);
			attr->asv[i] = NULL;
		}
	}
	attr->value_array_len = 0; // reset data
	return true;
}

bool MSG_SetBoolValue(MSG_ATTRIBUTE_T* attr, bool bvalue, char* readwritemode)
{
	bool bNotify = false;
	if(!attr)
		return false;
	if(attr->type != attr_type_boolean)
		bNotify = true;
	else if(attr->bv != bvalue)
		bNotify = true;

	if((attr->type == attr_type_date || attr->type == attr_type_string) && !attr->bNull)
	{
		if(attr->sv && (attr->strlen >= sizeof(attr->strvalue)))
			free(attr->sv);
		attr->sv = NULL;
		memset(attr->strvalue, 0, sizeof(attr->strvalue));
		attr->strlen = 0;
	}
	attr->bv = bvalue;
	attr->type = attr_type_boolean;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	attr->bRange = false;
	attr->bNull = false;

	if(bNotify)
		if(attr->on_datachanged)
			attr->on_datachanged(attr, attr->pRev1);
	return true;
}

bool MSG_SetStringValue(MSG_ATTRIBUTE_T* attr, char *svalue, char* readwritemode)
{
	bool bNotify = false;
	if(!attr)
		return false;
	if(!svalue)
		return false;
	if(attr->type != attr_type_string)
		bNotify = true;
	else if(strcmp(attr->sv,svalue)!=0)
		bNotify = true;

	if((attr->type == attr_type_date || attr->type == attr_type_string) && !attr->bNull)
	{
		if(attr->sv && (attr->strlen >= sizeof(attr->strvalue)))
			free(attr->sv);
		attr->sv = NULL;
		memset(attr->strvalue, 0, sizeof(attr->strvalue));
		attr->strlen = 0;
	}

	if(svalue)
	{
		attr->strlen = strlen(svalue);
		if(attr->strlen >= sizeof(attr->strvalue))
			attr->sv = strdup(svalue);
		else
		{
			strncpy(attr->strvalue, svalue, sizeof(attr->strvalue));
			attr->sv = attr->strvalue;
		}
		attr->bNull = false;
	}
	else
		attr->bNull = true;

	attr->type = attr_type_string;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	attr->bRange = false;

	if(bNotify)
		if(attr->on_datachanged)
			attr->on_datachanged(attr, attr->pRev1);
	return true;
}

bool MSG_SetTimestampValue(MSG_ATTRIBUTE_T* attr, unsigned int value, char* readwritemode)
{
	bool bNotify = false;
	if(!attr)
		return false;
	if(attr->type != attr_type_timestamp)
		bNotify = true;
	else if(attr->v != value)
		bNotify = true;

	if((attr->type == attr_type_date || attr->type == attr_type_string) && !attr->bNull)
	{
		if(attr->sv && (attr->strlen >= sizeof(attr->strvalue)))
			free(attr->sv);
		attr->sv = NULL;
		memset(attr->strvalue, 0, sizeof(attr->strvalue));
		attr->strlen = 0;
	}
	attr->v = value;
	attr->type = attr_type_timestamp;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	attr->bRange = false;
	attr->bNull = false;

	if(bNotify)
		if(attr->on_datachanged)
			attr->on_datachanged(attr, attr->pRev1);
	return true;
}

bool MSG_SetDateValue(MSG_ATTRIBUTE_T* attr, char *svalue, char* readwritemode)
{
	bool bNotify = false;
	if(!attr)
		return false;
	if(!svalue)
		return false;
	if(attr->type != attr_type_date)
		bNotify = true;
	else if(strcmp(attr->sv,svalue)!=0)
		bNotify = true;

	if((attr->type == attr_type_date || attr->type == attr_type_string) && !attr->bNull)
	{
		if(attr->sv && (attr->strlen >= sizeof(attr->strvalue)))
			free(attr->sv);
		attr->sv = NULL;
		memset(attr->strvalue, 0, sizeof(attr->strvalue));
		attr->strlen = 0;
	}

	if(svalue)
	{
		attr->strlen = strlen(svalue);
		if(attr->strlen >= sizeof(attr->strvalue))
			attr->sv = strdup(svalue);
		else
		{
			strncpy(attr->strvalue, svalue, sizeof(attr->strvalue));
			attr->sv = attr->strvalue;
		}
		attr->bNull = false;
	}
	else
		attr->bNull = true;

	attr->type = attr_type_date;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	attr->bRange = false;

	if(bNotify)
		if(attr->on_datachanged)
			attr->on_datachanged(attr, attr->pRev1);
	return true;
}

bool MSG_SetNULLValue(MSG_ATTRIBUTE_T* attr, char* readwritemode)
{
	bool bNotify = false;
	if(!attr)
		return false;
	if(attr->type != attr_type_string)
		bNotify = true;
	else if(!attr->bNull)
		bNotify = true;

	if((attr->type == attr_type_date || attr->type == attr_type_string) && !attr->bNull)
	{
		if(attr->sv && attr->strlen >= sizeof(attr->strvalue))
			free(attr->sv);
		attr->sv = NULL;
		memset(attr->strvalue, 0, sizeof(attr->strvalue));
		attr->strlen = 0;

	}
	attr->bNull = true;

	attr->type = attr_type_string;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	attr->bRange = false;

	if(bNotify)
		if(attr->on_datachanged)
			attr->on_datachanged(attr, attr->pRev1);
	return true;
}
#pragma endregion Find_Resource

#pragma region Generate_JSON
bool MatchFilterString(char* target, char** filter, int filterlength)
{
	int i=0;

	/*if(!filter)
		return true;*/

	if(!target)
		return false;
	if (filterlength == 0)
	{
		if (!strcasecmp(target, TAG_VALUE_ARRAY)) { // match rule
			return false;
		}
		else if (!strcasecmp(target, TAG_BOOLEAN_ARRAY)) { // match rule
			return false;
		}
		else if (!strcasecmp(target, TAG_STRING_ARRAY)) { // match rule
			return false;
		}
		else if (!strcasecmp(target, TAG_TIME_ARRAY)) { // match rule
			return false;
		}
		else {
			return true;
		}
		
	}
	else
	{
		for (i = 0; i < filterlength; i++)
		{
			if (!strcasecmp(target, filter[i])) { // match rule
				return true;
			}
		}
	}
	
	return false;
}

bool AddJSONAttribute(cJSON *pClass, MSG_ATTRIBUTE_T *attr_list, char** filter, int length, bool bflat, bool bsimulate)
{
	cJSON* pAttr = NULL;
	cJSON* pENode = NULL;
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* nxtAttr = NULL;

	if(!pClass || !attr_list)
		return false;

	srand(time(NULL));

	curAttr = attr_list;
	while (curAttr)
	{
		nxtAttr = curAttr->next;

		if(curAttr->bSensor && !bflat)
		{
			if(!pENode)
			{
				pENode = cJSON_CreateArray();
				cJSON_AddItemToObject(pClass, TAG_E_NODE, pENode);
			}
			pAttr = cJSON_CreateObject();
			cJSON_AddItemToArray(pENode, pAttr);
			if(MatchFilterString(TAG_ATTR_NAME, filter, length))
				cJSON_AddStringToObject(pAttr, TAG_ATTR_NAME, curAttr->name);
			switch (curAttr->type)
			{
			case attr_type_numeric:
				{
					if(MatchFilterString(TAG_VALUE, filter, length))
					{
						if(curAttr->bNull)
							cJSON_AddNullToObject(pAttr, TAG_VALUE);
						else
						{
							if(bsimulate)
							{
								if(curAttr->bRange)
								{
									curAttr->v = (rand() % (int)(curAttr->max-curAttr->min))+curAttr->min;
								}
								else
								{
									curAttr->v = (rand() % 100);
								}
							}
							cJSON_AddNumberToObject(pAttr, TAG_VALUE, curAttr->v);
						}
					}
					if(curAttr->bRange)
					{
						if(MatchFilterString(TAG_MAX, filter, length))
							cJSON_AddNumberToObject(pAttr, TAG_MAX, curAttr->max);
						if(MatchFilterString(TAG_MIN, filter, length))
							cJSON_AddNumberToObject(pAttr, TAG_MIN, curAttr->min);
					}
					if(strlen(curAttr->readwritemode)>0)
						if(MatchFilterString(TAG_ASM, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);


					if(strlen(curAttr->unit)>0)
					{
						if(MatchFilterString(TAG_UNIT, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_UNIT, curAttr->unit);
					}
				}
				break;
			case attr_type_boolean:
				{
					if(MatchFilterString(TAG_BOOLEAN, filter, length))
					{
						if(curAttr->bNull)
							cJSON_AddNullToObject(pAttr, TAG_BOOLEAN);
						else
						{
							if(bsimulate)
							{
								int val =  (rand() % 2);
								curAttr->bv = val>0?true:false;
							}
							if(curAttr->bv)
								cJSON_AddTrueToObject(pAttr, TAG_BOOLEAN);
							else
								cJSON_AddFalseToObject(pAttr, TAG_BOOLEAN);
						}
					}
					if(strlen(curAttr->readwritemode)>0)
						if(MatchFilterString(TAG_ASM, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);
				}
				break;
			case attr_type_string:
				{
					//if(curAttr->bNull || strlen(curAttr->sv)==0)
					if(curAttr->bNull)
					{
						if(MatchFilterString(TAG_VALUE, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_STRING, "");
					}
					//else if(strlen(curAttr->sv)>0)
					else if(curAttr->strlen>=0)
					{
						if(MatchFilterString(TAG_STRING, filter, length))
						{
							cJSON_AddStringToObject(pAttr, TAG_STRING, curAttr->sv);
						}
					}
					if(strlen(curAttr->readwritemode)>0)
						if(MatchFilterString(TAG_ASM, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);
				}
				break;
			case  attr_type_date:
				{
					if(curAttr->bNull || curAttr->strlen==0)
					{
						if(MatchFilterString(TAG_VALUE, filter, length))
							cJSON_AddNullToObject(pAttr, TAG_VALUE);
					}
					else if(curAttr->strlen>0)
					{
						if(MatchFilterString(TAG_VALUE, filter, length))
						{
							cJSON* pDateRoot = cJSON_CreateObject();
							cJSON_AddItemToObject(pAttr, TAG_VALUE, pDateRoot);
							cJSON_AddStringToObject(pDateRoot, TAG_DATE, curAttr->sv);
						}
					}
					if(strlen(curAttr->readwritemode)>0)
						if(MatchFilterString(TAG_ASM, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);
				}
				break;
			case  attr_type_timestamp:
				{
					if(MatchFilterString(TAG_VALUE, filter, length))
					{
						if(curAttr->bNull)
							cJSON_AddNullToObject(pAttr, TAG_VALUE);
						else
						{
							cJSON* pDateRoot = cJSON_CreateObject();
							cJSON_AddItemToObject(pAttr, TAG_VALUE, pDateRoot);
							cJSON_AddNumberToObject(pDateRoot, TAG_TIMESTAMP, curAttr->v);
						}
					}
					if(strlen(curAttr->readwritemode)>0)
						if(MatchFilterString(TAG_ASM, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);
				}
				break;
			case attr_type_numeric_array:
				{
					if(MatchFilterString(TAG_VALUE_ARRAY, filter, length))
					{
						if(curAttr->bNull)
							cJSON_AddNullToObject(pAttr, TAG_VALUE_ARRAY);
						else {
							cJSON *value_array, *time_array;
							value_array = cJSON_CreateDoubleArray(curAttr->av, curAttr->value_array_len);
							cJSON_AddItemToObject(pAttr, TAG_VALUE_ARRAY, value_array);
							// time array
							time_array = cJSON_CreateIntArray(curAttr->at, curAttr->value_array_len);
							cJSON_AddItemToObject(pAttr, TAG_TIME_ARRAY, time_array);
						}
					} else if (MatchFilterString(TAG_VALUE, filter, length)) {
						cJSON_AddNumberToObject(pAttr, TAG_VALUE, curAttr->v); // add latest cache value
					}
					if(curAttr->bRange)
					{
						if(MatchFilterString(TAG_MAX, filter, length))
							cJSON_AddNumberToObject(pAttr, TAG_MAX, curAttr->max);
						if(MatchFilterString(TAG_MIN, filter, length))
							cJSON_AddNumberToObject(pAttr, TAG_MIN, curAttr->min);
					}
					if(strlen(curAttr->readwritemode)>0)
						if(MatchFilterString(TAG_ASM, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);


					if(strlen(curAttr->unit)>0)
					{
						if(MatchFilterString(TAG_UNIT, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_UNIT, curAttr->unit);
					}
				}
				break;

			case attr_type_boolean_array:
				{
					int i;
					if(MatchFilterString(TAG_BOOLEAN_ARRAY, filter, length))
					{
						if(curAttr->bNull)
							cJSON_AddNullToObject(pAttr, TAG_BOOLEAN_ARRAY);
						else {
							cJSON *value_array, *time_array, *value;
							value_array = cJSON_CreateArray();
							if (!value_array) {
								break;
							}
							for (i = 0; i < curAttr->value_array_len; i++) {
								value = cJSON_CreateBool(curAttr->abv[i]);
								cJSON_AddItemToArray(value_array, value);
							}
							cJSON_AddItemToObject(pAttr, TAG_BOOLEAN_ARRAY, value_array);
							// time array
							time_array = cJSON_CreateIntArray(curAttr->at, curAttr->value_array_len);
							cJSON_AddItemToObject(pAttr, TAG_TIME_ARRAY, time_array);
						}
					}
					else if (MatchFilterString(TAG_BOOLEAN, filter, length)) { // add bv
						if(curAttr->bv) // add latest cache value
							cJSON_AddTrueToObject(pAttr, TAG_BOOLEAN);
						else
							cJSON_AddFalseToObject(pAttr, TAG_BOOLEAN);
					}
					if(strlen(curAttr->readwritemode)>0)
						if(MatchFilterString(TAG_ASM, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);
				}
				break;
			case attr_type_string_array:
				{
					if(MatchFilterString(TAG_STRING_ARRAY, filter, length))
					{
						if(curAttr->bNull)
							cJSON_AddNullToObject(pAttr, TAG_STRING_ARRAY);
						else {
							cJSON *value_array, *time_array;
							value_array = cJSON_CreateStringArray((const char**)curAttr->asv, curAttr->value_array_len);
							cJSON_AddItemToObject(pAttr, TAG_STRING_ARRAY, value_array);
							// time array
							time_array = cJSON_CreateIntArray(curAttr->at, curAttr->value_array_len);
							cJSON_AddItemToObject(pAttr, TAG_TIME_ARRAY, time_array);
						}
					}
					else if (MatchFilterString(TAG_STRING, filter, length)) {
						if (curAttr->sv) {
							cJSON_AddStringToObject(pAttr, TAG_STRING, curAttr->sv); // add latest cache value
						} else {
							cJSON_AddStringToObject(pAttr, TAG_STRING, "");
						}
					}
					if(strlen(curAttr->readwritemode)>0)
						if(MatchFilterString(TAG_ASM, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);
				}
				break;
			default:
				{
				}
				break;
			}

			{
				EXT_ATTRIBUTE_T* extattr = curAttr->extra;
				while(extattr)
				{
					switch (extattr->type)
					{
					case attr_type_numeric:
						if(bsimulate)
						{
							extattr->v = (rand() % 100);
						}
						cJSON_AddNumberToObject(pAttr, extattr->name, extattr->v);
						break;
					case attr_type_boolean:
						if(bsimulate)
						{
							int val =  (rand() % 2);
							extattr->bv = val>0?true:false;
						}

						if(extattr->bv)
							cJSON_AddTrueToObject(pAttr, extattr->name);
						else
							cJSON_AddFalseToObject(pAttr, extattr->name);
						break;
					case attr_type_string:
							cJSON_AddStringToObject(pAttr, extattr->name, extattr->sv);
						break;
					case  attr_type_date:
						{
							cJSON* pDateRoot = cJSON_CreateObject();
							cJSON_AddItemToObject(pAttr, extattr->name, pDateRoot);
							cJSON_AddStringToObject(pDateRoot, TAG_DATE, extattr->sv);
						}
						break;
					case  attr_type_timestamp:
						{
							cJSON* pDateRoot = cJSON_CreateObject();
							cJSON_AddItemToObject(pAttr, extattr->name, pDateRoot);
							cJSON_AddNumberToObject(pDateRoot, TAG_TIMESTAMP, extattr->v);
						}
						break;
					default:
						{
						}
						break;
					}
					extattr = extattr->next;
				}

			}
		}
		else
		{
			switch (curAttr->type)
			{
			case attr_type_numeric:
				{
					if(MatchFilterString(curAttr->name, filter, length) || bflat)
					{
						if(curAttr->bNull)
							cJSON_AddNullToObject(pClass, curAttr->name);
						else
						{
							if(bsimulate)
							{
								if(curAttr->bRange)
								{
									curAttr->v = (rand() % (int)(curAttr->max-curAttr->min))+curAttr->min;
								}
								else
								{
									curAttr->v = (rand() % 100);
								}
							}
							cJSON_AddNumberToObject(pClass, curAttr->name, curAttr->v);
						}
					}
				}
				break;
			case attr_type_boolean:
				{
					if(MatchFilterString(curAttr->name, filter, length) || bflat)
					{
						if(curAttr->bNull)
							cJSON_AddNullToObject(pClass, curAttr->name);
						else
						{
							if(bsimulate)
							{
								int val =  (rand() % 2);
								curAttr->bv = val>0?true:false;
							}

							if(curAttr->bv)
								cJSON_AddTrueToObject(pClass, curAttr->name);
							else
								cJSON_AddFalseToObject(pClass, curAttr->name);
						}
					}
				}
				break;
			case attr_type_string:
				{
					if(curAttr->bNull || curAttr->strlen==0)
					{
						if(MatchFilterString(TAG_VALUE, filter, length) || bflat)
							cJSON_AddNullToObject(pClass, curAttr->name);
					}
					else if(curAttr->strlen>0)
					{
						if(MatchFilterString(curAttr->name, filter, length) || bflat)
						{
							cJSON_AddStringToObject(pClass, curAttr->name, curAttr->sv);
						}
					}
				}
				break;
			case  attr_type_date:
				{
					if(curAttr->bNull || curAttr->strlen==0)
					{
						if(MatchFilterString(curAttr->name, filter, length) || bflat)
							cJSON_AddNullToObject(pClass, curAttr->name);
					}
					else if(curAttr->strlen>0)
					{
						if(MatchFilterString(curAttr->name, filter, length) || bflat)
						{
							cJSON* pDateRoot = cJSON_CreateObject();
							cJSON_AddItemToObject(pClass, curAttr->name, pDateRoot);
							cJSON_AddStringToObject(pDateRoot, TAG_DATE, curAttr->sv);
						}
					}
				}
				break;
			case  attr_type_timestamp:
				{
					if(MatchFilterString(curAttr->name, filter, length) || bflat)
					{

						if(curAttr->bNull)
							cJSON_AddNullToObject(pClass, curAttr->name);
						else
						{
							cJSON* pDateRoot = cJSON_CreateObject();
							cJSON_AddItemToObject(pClass, curAttr->name, pDateRoot);
							cJSON_AddNumberToObject(pDateRoot, TAG_TIMESTAMP, curAttr->v);
						}
					}
				}
				break;
			default:
				{
				}
				break;
			}
		}

		curAttr = nxtAttr;
	};

	return true;
}

bool AddJSONClassify(cJSON *pRoot, MSG_CLASSIFY_T* msg, char** filter, int length, bool bflat, bool bsimulate)
{
	cJSON* pClass = NULL;

	MSG_CLASSIFY_T* curClass = NULL;
	MSG_CLASSIFY_T* nxtClass = NULL;

	if(!pRoot || !msg)
		return false;

	if(msg->type == class_type_root)
		pClass = pRoot;
	else
	{
		if(msg->type == class_type_array)
			pClass = cJSON_CreateArray();
		else
		{
			pClass = cJSON_CreateObject();
			if(msg->bIoTFormat && !bflat)
			{
				if(MatchFilterString(TAG_BASE_NAME, filter, length) && !bflat)
					cJSON_AddStringToObject(pClass, TAG_BASE_NAME, msg->classname);

				if(MatchFilterString(TAG_VERSION, filter, length) && !bflat)
					if(strlen(msg->version)>0)
						cJSON_AddStringToObject(pClass, TAG_VERSION, msg->version);
			}
		}

		if(pRoot->type == cJSON_Array)
			cJSON_AddItemToArray(pRoot, pClass);
		else {
			cJSON_AddItemToObject(pRoot, msg->classname, pClass);
		}
	}

	//if(msg->type != class_type_array)
	//{
		if(msg->attr_list)
		{
			AddJSONAttribute(pClass, msg->attr_list, filter, length, bflat, bsimulate);
		}
	//}

	if(msg->sub_list)
	{
		curClass = msg->sub_list;
		while (curClass)
		{
			nxtClass = curClass->next;

			AddJSONClassify(pClass, curClass, filter, length, bflat, bsimulate);

			curClass = nxtClass;
		};
	}
/*
	if(msg->next)
	{
		curClass = msg->next;
		while (curClass)
		{
			nxtClass = curClass->next;

			AddJSONClassify(pClass, curClass);

			curClass = nxtClass;
		};
	}
*/
	return true;
}

bool AddSingleJSONAttribute(cJSON *pClass, MSG_ATTRIBUTE_T *attr, char** filter, int length, bool bflat, bool bsimulate)
{
	cJSON* pAttr = NULL;
	cJSON* pENode = NULL;
	int i;

	if(!pClass || !attr)
		return false;

	srand(time(NULL));

	if(attr->bSensor && !bflat)
	{
		pENode = cJSON_GetObjectItem(pClass, TAG_E_NODE);
		if(!pENode)
		{
			pENode = cJSON_CreateArray();
			cJSON_AddItemToObject(pClass, TAG_E_NODE, pENode);
		}


		{
			int size = cJSON_GetArraySize(pENode);
			int i=0;
			for(i=0; i<size;i++)
			{
				cJSON* pNode = cJSON_GetArrayItem(pENode, i);
				if(pNode)
				{
					pNode = cJSON_GetObjectItem(pNode, TAG_ATTR_NAME);
					if(pNode)
					{
						if(strcmp(pNode->valuestring, attr->name) == 0)
							return true;
					}

				}
			}
		}

		pAttr = cJSON_CreateObject();
		cJSON_AddItemToArray(pENode, pAttr);
		if(MatchFilterString(TAG_ATTR_NAME, filter, length))
			cJSON_AddStringToObject(pAttr, TAG_ATTR_NAME, attr->name);
		switch (attr->type)
		{
		case attr_type_numeric:
			{
				if(MatchFilterString(TAG_VALUE, filter, length))
				{
					if(attr->bNull)
						cJSON_AddNullToObject(pAttr, TAG_VALUE);
					else
					{
						if(bsimulate)
						{
							if(attr->bRange)
							{
								attr->v = (rand() % (int)(attr->max-attr->min))+attr->min;
							}
							else
							{
								attr->v = (rand() % 100);
							}
						}
						cJSON_AddNumberToObject(pAttr, TAG_VALUE, attr->v);
					}
				}
				if(attr->bRange)
				{
					if(MatchFilterString(TAG_MAX, filter, length))
						cJSON_AddNumberToObject(pAttr, TAG_MAX, attr->max);
					if(MatchFilterString(TAG_MIN, filter, length))
						cJSON_AddNumberToObject(pAttr, TAG_MIN, attr->min);
				}
				if(strlen(attr->readwritemode)>0)
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, attr->readwritemode);

				if(strlen(attr->unit)>0)
				{
					if(MatchFilterString(TAG_UNIT, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_UNIT, attr->unit);
				}
			}
			break;
		case attr_type_boolean:
			{
				if(MatchFilterString(TAG_BOOLEAN, filter, length))
				{
					if(attr->bNull)
						cJSON_AddNullToObject(pAttr, TAG_BOOLEAN);
					else
					{
						if(bsimulate)
						{
							int val =  (rand() % 2);
							attr->bv = val>0?true:false;
						}

						if(attr->bv)
							cJSON_AddTrueToObject(pAttr, TAG_BOOLEAN);
						else
							cJSON_AddFalseToObject(pAttr, TAG_BOOLEAN);
					}
				}
				if(strlen(attr->readwritemode)>0)
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, attr->readwritemode);
			}
			break;
		case attr_type_string:
			{
				if(attr->bNull)
				{
					if(MatchFilterString(TAG_VALUE, filter, length))
						cJSON_AddNullToObject(pAttr, TAG_STRING);
				}
				else if(attr->strlen>0)
				{
					if(MatchFilterString(TAG_STRING, filter, length))
					{
						cJSON_AddStringToObject(pAttr, TAG_STRING, attr->sv);
					}
				}
				if(strlen(attr->readwritemode)>0)
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, attr->readwritemode);
			}
			break;
		case  attr_type_date:
			{
				if(attr->bNull)
				{
					if(MatchFilterString(TAG_VALUE, filter, length))
						cJSON_AddNullToObject(pAttr, TAG_VALUE);
				}
				else if(attr->strlen>0)
				{
					if(MatchFilterString(TAG_VALUE, filter, length))
					{
						cJSON* pDateRoot = cJSON_CreateObject();
						cJSON_AddItemToObject(pAttr, TAG_VALUE, pDateRoot);
						cJSON_AddStringToObject(pDateRoot, TAG_DATE, attr->sv);
					}
				}
				if(strlen(attr->readwritemode)>0)
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, attr->readwritemode);
			}
			break;
		case  attr_type_timestamp:
			{
				if(MatchFilterString(TAG_VALUE, filter, length))
				{
					if(attr->bNull)
						cJSON_AddNullToObject(pAttr, TAG_VALUE);
					else
					{
						cJSON* pDateRoot = cJSON_CreateObject();
						cJSON_AddItemToObject(pAttr, TAG_VALUE, pDateRoot);
						cJSON_AddNumberToObject(pDateRoot, TAG_TIMESTAMP, attr->v);
					}
				}
				if(strlen(attr->readwritemode)>0)
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, attr->readwritemode);
			}
			break;
		case attr_type_numeric_array:
			{
				if(MatchFilterString(TAG_VALUE_ARRAY, filter, length))
				{
					if(attr->bNull)
						cJSON_AddNullToObject(pAttr, TAG_VALUE_ARRAY);
					else {
						cJSON *value_array, *time_array;
						value_array = cJSON_CreateDoubleArray(attr->av, attr->value_array_len);
						cJSON_AddItemToObject(pAttr, TAG_VALUE_ARRAY, value_array);
						// time array
						time_array = cJSON_CreateIntArray(attr->at, attr->value_array_len);
						cJSON_AddItemToObject(pAttr, TAG_TIME_ARRAY, time_array);
					}
				} else { // add v
					if (attr->value_array_len > 0) {
						cJSON_AddNumberToObject(pAttr, TAG_VALUE, attr->av[0]); // add first
					} else {
						cJSON_AddNullToObject(pAttr, TAG_VALUE_ARRAY);
					}
				}
				if(attr->bRange)
				{
					if(MatchFilterString(TAG_MAX, filter, length))
						cJSON_AddNumberToObject(pAttr, TAG_MAX, attr->max);
					if(MatchFilterString(TAG_MIN, filter, length))
						cJSON_AddNumberToObject(pAttr, TAG_MIN, attr->min);
				}
				if(strlen(attr->readwritemode)>0)
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, attr->readwritemode);

				if(strlen(attr->unit)>0)
				{
					if(MatchFilterString(TAG_UNIT, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_UNIT, attr->unit);
				}
			}
			break;
		case attr_type_boolean_array:
			{
				if(MatchFilterString(TAG_BOOLEAN_ARRAY, filter, length))
				{
					if(attr->bNull)
						cJSON_AddNullToObject(pAttr, TAG_BOOLEAN_ARRAY);
					else {
						cJSON *value_array, *time_array, *value;
						value_array = cJSON_CreateArray();
						if (!value_array) {
							break;
						}
						for (i = 0; i < attr->value_array_len; i++) {
							value = cJSON_CreateBool(attr->abv[i]);
							cJSON_AddItemToArray(value_array, value);
						}
						cJSON_AddItemToObject(pAttr, TAG_BOOLEAN_ARRAY, value_array);
						// time array
						time_array = cJSON_CreateIntArray(attr->at, attr->value_array_len);
						cJSON_AddItemToObject(pAttr, TAG_TIME_ARRAY, time_array);
					}
				} else { // add bv
					if (attr->value_array_len > 0) {
						if(attr->abv[0]) // add first
							cJSON_AddTrueToObject(pAttr, TAG_BOOLEAN);
						else
							cJSON_AddFalseToObject(pAttr, TAG_BOOLEAN);
					} else {
						cJSON_AddNullToObject(pAttr, TAG_VALUE_ARRAY);
					}
				}
				if(strlen(attr->readwritemode)>0)
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, attr->readwritemode);
			}
			break;
		case attr_type_string_array:
			{
				if(MatchFilterString(TAG_STRING_ARRAY, filter, length))
				{
					if(attr->bNull)
						cJSON_AddNullToObject(pAttr, TAG_STRING_ARRAY);
					else {
						cJSON *value_array, *time_array;
						value_array = cJSON_CreateStringArray((const char**)attr->asv, attr->value_array_len);
						cJSON_AddItemToObject(pAttr, TAG_STRING_ARRAY, value_array);
						// time array
						time_array = cJSON_CreateIntArray(attr->at, attr->value_array_len);
						cJSON_AddItemToObject(pAttr, TAG_TIME_ARRAY, time_array);
					}
				} else { // add empty sv for capability case
					cJSON_AddStringToObject(pAttr, TAG_STRING, "");
				}
				if(strlen(attr->readwritemode)>0)
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, attr->readwritemode);
			}
			break;
		default:
			{
			}
			break;
		}

		{
			EXT_ATTRIBUTE_T* extattr = attr->extra;
			while(extattr)
			{
				if(MatchFilterString(extattr->name, filter, length))
				{
					switch (attr->type)
					{
					case attr_type_numeric:
						if(bsimulate)
						{
							if(attr->bRange)
							{
								extattr->v = (rand() % 100);
							}
						}
						cJSON_AddNumberToObject(pAttr, extattr->name, extattr->v);
						break;
					case attr_type_boolean:
						if(bsimulate)
						{
							int val =  (rand() % 2);
							extattr->bv = val>0?true:false;
						}
						if(extattr->bv)
							cJSON_AddTrueToObject(pAttr, extattr->name);
						else
							cJSON_AddFalseToObject(pAttr, extattr->name);
						break;
					case attr_type_string:
						cJSON_AddStringToObject(pAttr, extattr->name, attr->sv);
						break;
					case  attr_type_date:
						{
							cJSON* pDateRoot = cJSON_CreateObject();
							cJSON_AddItemToObject(pAttr, extattr->name, pDateRoot);
							cJSON_AddStringToObject(pDateRoot, TAG_DATE, attr->sv);
						}
						break;
					case  attr_type_timestamp:
						{
							cJSON* pDateRoot = cJSON_CreateObject();
							cJSON_AddItemToObject(pAttr, extattr->name, pDateRoot);
							cJSON_AddNumberToObject(pDateRoot, TAG_TIMESTAMP, attr->v);
						}
						break;
					default:
						{
						}
						break;
					}
				}
				extattr = extattr->next;
			}
		}
	}
	else
	{
		switch (attr->type)
		{
		case attr_type_numeric:
			{
				if(MatchFilterString(attr->name, filter, length) || bflat)
				{
					if(attr->bNull)
						cJSON_AddNullToObject(pClass, attr->name);
					else
					{
						if(bsimulate)
						{
							if(attr->bRange)
							{
								attr->v = (rand() % (int)(attr->max-attr->min))+attr->min;
							}
							else
							{
								attr->v = (rand() % 100);
							}
						}
						cJSON_AddNumberToObject(pClass, attr->name, attr->v);
					}
				}
			}
			break;
		case attr_type_boolean:
			{
				if(MatchFilterString(attr->name, filter, length) || bflat)
				{
					if(attr->bNull)
						cJSON_AddNullToObject(pClass, attr->name);
					else
					{
						if(bsimulate)
						{
							int val =  (rand() % 2);
							attr->bv = val>0?true:false;
						}
						if(attr->bv)
							cJSON_AddTrueToObject(pClass, attr->name);
						else
							cJSON_AddFalseToObject(pClass, attr->name);
					}
				}
			}
			break;
		case attr_type_string:
			{
				if(attr->bNull)
				{
					if(MatchFilterString(TAG_VALUE, filter, length) || bflat)
						cJSON_AddNullToObject(pClass, attr->name);
				}
				else if(attr->strlen>0)
				{
					if(MatchFilterString(attr->name, filter, length) || bflat)
					{
						cJSON_AddStringToObject(pClass, attr->name, attr->sv);
					}
				}
			}
			break;
		case  attr_type_date:
			{
				if(attr->bNull)
				{
					if(MatchFilterString(attr->name, filter, length) || bflat)
						cJSON_AddNullToObject(pClass, attr->name);
				}
				else if(attr->strlen>0)
				{
					if(MatchFilterString(attr->name, filter, length) || bflat)
					{
						cJSON* pDateRoot = cJSON_CreateObject();
						cJSON_AddItemToObject(pClass, attr->name, pDateRoot);
						cJSON_AddStringToObject(pDateRoot, TAG_DATE, attr->sv);
					}
				}
			}
			break;
		case  attr_type_timestamp:
			{
				if(MatchFilterString(attr->name, filter, length) || bflat)
				{

					if(attr->bNull)
						cJSON_AddNullToObject(pClass, attr->name);
					else
					{
						cJSON* pDateRoot = cJSON_CreateObject();
						cJSON_AddItemToObject(pClass, attr->name, pDateRoot);
						cJSON_AddNumberToObject(pDateRoot, TAG_TIMESTAMP, attr->v);
					}
				}
			}
			break;
		default:
			{
			}
			break;
		}
	}
	return true;
}

cJSON * AddSingleJSONClassify(cJSON *pParent, MSG_CLASSIFY_T* msg, char** filter, int length, bool bflat)
{
	cJSON* pClass = NULL;

	if(!pParent || !msg)
		return pClass;

	if(msg->type == class_type_root)
		pClass = pParent;
	else
	{
		if(msg->type == class_type_array)
			pClass = cJSON_CreateArray();
		else
		{
			pClass = cJSON_CreateObject();
			if(msg->bIoTFormat && !bflat)
			{
				if(MatchFilterString(TAG_BASE_NAME, filter, length) && !bflat)
					cJSON_AddStringToObject(pClass, TAG_BASE_NAME, msg->classname);

				if(MatchFilterString(TAG_VERSION, filter, length) && !bflat)
					if(strlen(msg->version)>0)
						cJSON_AddStringToObject(pClass, TAG_VERSION, msg->version);
			}
		}

		if(pParent->type == cJSON_Array)
			cJSON_AddItemToArray(pParent, pClass);
		else
			cJSON_AddItemToObject(pParent, msg->classname, pClass);
	}
	return pClass;
}

bool AddJSONClassifyWithSelected(cJSON *pRoot, MSG_CLASSIFY_T* msg, char** filter, int length, char* reqItem, bool bflat, bool simulate)
{
	cJSON* pClass = pRoot;
	char *delim = "/";
	char *str=NULL;
	char *token=NULL;
	char temp[260] = {0};
	char name[30] = {0};
	MSG_CLASSIFY_T  *classify = msg;
	MSG_ATTRIBUTE_T* attr = NULL;

	if(!pRoot || !msg || !reqItem)
		return false;

	strncpy(temp,reqItem, sizeof(temp));
	str = strtok_r(temp,delim,&token);

	while (str != NULL)
	{
		if(classify == NULL)
			break;

		strncpy(name, str, sizeof(name));
		str = strtok_r(NULL, delim, &token);
		if(str == NULL)
		{
			attr = MSG_FindAttribute(classify, name, true);
			if(attr == NULL) {
				attr = MSG_FindAttribute(classify, name, false);
			}
			if(attr == NULL)
			{
				cJSON* pChild = NULL;
				classify = MSG_FindClassify(classify,name);
				if(classify == NULL)
					return false;
				pChild = cJSON_GetObjectItem(pClass, name);
				if(pChild == NULL)
				{
					AddJSONClassify(pClass, classify, filter, length, bflat, simulate);
				}
			}
			else
			{
				cJSON* pChild = cJSON_GetObjectItem(pClass, name);
				if(pChild == NULL)
				{
					AddSingleJSONAttribute(pClass, attr, filter, length, bflat, simulate);
				}
			}
		}
		else
		{
			cJSON* pChild = NULL;
			classify = MSG_FindClassify(classify,name);
			if(classify == NULL)
				return false;
			if(pClass->type == cJSON_Array)
			{
				int size = cJSON_GetArraySize(pClass);
				int i=0;

				for(i=0; i<size; i++)
				{
					cJSON* bn = NULL;
					cJSON* node = cJSON_GetArrayItem(pClass, i);
					if(node == NULL)
						continue;
					bn = cJSON_GetObjectItem(node, TAG_BASE_NAME);
					if(bn == NULL)
						continue;
					if(strcmp(bn->valuestring, name)==0)
					{
						pChild = node;
						break;
					}
				}
			}
			else
				pChild = cJSON_GetObjectItem(pClass, name);

			if(pChild == NULL)
			{
				pClass = AddSingleJSONClassify(pClass, classify, filter, length, bflat);
			}
			else
				pClass = pChild;
		}
	}
	return true;
}

char *MSG_JSONPrintUnformatted(MSG_CLASSIFY_T* msg, bool bflat, bool bsimulate)
{
	char* buffer = NULL;
	cJSON *pRoot = NULL;

	pRoot = cJSON_CreateObject();

	AddJSONClassify(pRoot, msg, NULL, 0, bflat, bsimulate);

	buffer = cJSON_PrintUnformatted(pRoot);
	cJSON_Delete(pRoot);
	pRoot = NULL;
	return buffer;
}

char *MSG_JSONPrintWithFiltered(MSG_CLASSIFY_T* msg, char** filter, int length, bool bflat, bool bsimulate)
{
	char* buffer = NULL;
	cJSON *pRoot = NULL;

	pRoot = cJSON_CreateObject();
	AddJSONClassify(pRoot, msg, filter, length, bflat, bsimulate);
	buffer = cJSON_PrintUnformatted(pRoot);
	cJSON_Delete(pRoot);
	pRoot = NULL;
	return buffer;
}

char *MSG_JSONPrintSelectedWithFiltered(MSG_CLASSIFY_T* msg, char** filter, int length, char* reqItems, bool bflat, bool bsimulate)
{
	char* buffer = NULL;
	cJSON *pRoot = NULL;
	cJSON *pReqItemList = NULL;
	cJSON *pReqItemRoot = NULL;

	if(reqItems == NULL)
		return buffer;

	pReqItemRoot = cJSON_Parse(reqItems);
	if(pReqItemRoot==NULL)
		return buffer;

	pRoot = cJSON_CreateObject();


	pReqItemList = cJSON_GetObjectItem(pReqItemRoot, "e");

	if(pReqItemList)
	{
		int size = cJSON_GetArraySize(pReqItemList);
		int i=0;
		for(i=0;i<size;i++)
		{
			cJSON* nNode = NULL;
			cJSON* item = cJSON_GetArrayItem(pReqItemList, i);
			if(item == NULL)
				continue;
			nNode = cJSON_GetObjectItem(item, "n");
			if(nNode == NULL)
				continue;
			AddJSONClassifyWithSelected(pRoot, msg, filter, length, nNode->valuestring, bflat, bsimulate);
		}
	}
	if(pReqItemList)
		buffer = cJSON_PrintUnformatted(pRoot);
	cJSON_Delete(pRoot);
	cJSON_Delete(pReqItemRoot);
	pRoot = NULL;
	return buffer;
}

MSG_ATTRIBUTE_T* MSG_FindAttributeWithPath(MSG_CLASSIFY_T *msg,char *path, bool isSensorData)
{
	MSG_CLASSIFY_T  *classify=msg;
	MSG_ATTRIBUTE_T* attr = NULL;

	char *delim = "/";
	char *str=NULL;
	char *token=NULL;
	char temp[512] = {0};
	char name[512] = { 0 };

	strncpy(temp,path, sizeof(temp));
	str = strtok_r(temp,delim,&token);

	while (str != NULL)
	{
		if(classify == NULL)
			break;

		strncpy(name, str, sizeof(name));

		str = strtok_r(NULL, delim, &token);
		if(str == NULL)
			attr = MSG_FindAttribute(classify, name, isSensorData);
		else
			classify = MSG_FindClassify(classify,name);

	}

	return attr;
}

bool MSG_IsAttributeExist(MSG_CLASSIFY_T *msg,char *path, bool isSensorData)
{
	MSG_ATTRIBUTE_T* attr = MSG_FindAttributeWithPath(msg, path, isSensorData);

	if(attr)
		return true;
	else
		return false;
}

void MSG_SetDataChangeCallback(MSG_CLASSIFY_T* msg, AttributeChangedCbf on_datachanged, void* pRev1)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;

	MSG_CLASSIFY_T* curClass = NULL;

	if(!msg)
		return;

	msg->on_datachanged = on_datachanged;
	msg->pRev1 = pRev1;

	curAttr = msg->attr_list;
	while (curAttr)
	{
		curAttr->on_datachanged = on_datachanged;
		curAttr->pRev1 = pRev1;
		curAttr = curAttr->next;
	}

	curClass = msg->sub_list;
	while (curClass)
	{
		MSG_SetDataChangeCallback(curClass, on_datachanged, pRev1);
		curClass = curClass->next;
	}

}

bool MSG_AppendIoTSensorAttributeDouble(MSG_ATTRIBUTE_T* attr, const char* attrname, double value)
{
	EXT_ATTRIBUTE_T *extattr = NULL,*extpre = NULL, *target = NULL;
	char* newname = NULL;
	if(!attr)
		return false;
	if(!attrname)
		return false;

	newname = StringReplace(attrname, "/", "@2F");
	if(newname == NULL) {
		return false;
	}

	extattr = attr->extra;

	while(extattr)
	{
		extpre = extattr;
		if(strcmp(extattr->name, newname)==0)
		{
			target = extattr;
			break;
		}
		extattr = extattr->next;
	}

	if(target == NULL)
	{
		target = calloc(1, sizeof(EXT_ATTRIBUTE_T));
		strncpy(target->name, newname, sizeof(target->name));

		if(extpre == NULL)
			attr->extra = target;
		else
			extpre->next = target;
	}

	if(target->type == attr_type_date || target->type == attr_type_string)
	{
		if(target->sv && (target->strlen >= sizeof(target->strvalue)))
			free(target->sv);
		target->sv = NULL;

		memset(target->strvalue, 0, sizeof(target->strvalue));
		target->strlen = 0;
	}
	target->v = value;
	target->type = attr_type_numeric;
	StringFree(newname);
	return true;
}

bool MSG_AppendIoTSensorAttributeBool(MSG_ATTRIBUTE_T* attr, const char* attrname, bool bvalue)
{
	EXT_ATTRIBUTE_T *extattr = NULL,*extpre = NULL, *target = NULL;
	char* newname = NULL;
	if(!attr)
		return false;
	if(!attrname)
		return false;

	newname = StringReplace(attrname, "/", "@2F");
	if(newname == NULL) {
		return false;
	}
	extattr = attr->extra;

	while(extattr)
	{
		extpre = extattr;
		if(strcmp(extattr->name, newname)==0)
		{
			target = extattr;
			break;
		}
		extattr = extattr->next;
	}

	if(target == NULL)
	{
		target = calloc(1, sizeof(EXT_ATTRIBUTE_T));
		strncpy(target->name, newname, sizeof(target->name));

		if(extpre == NULL)
			attr->extra = target;
		else
			extpre->next = target;
	}

	if(target->type == attr_type_date || target->type == attr_type_string)
	{
		if(target->sv && (target->strlen >= sizeof(target->strvalue)))
			free(target->sv);
		target->sv = NULL;

		memset(target->strvalue, 0, sizeof(target->strvalue));
		target->strlen = 0;
	}
	target->bv = bvalue;
	target->type = attr_type_boolean;
	StringFree(newname);
	return true;
}

bool MSG_AppendIoTSensorAttributeString(MSG_ATTRIBUTE_T* attr, const char* attrname, char *svalue)
{
	EXT_ATTRIBUTE_T *extattr = NULL,*extpre = NULL, *target = NULL;
	char* newname = NULL;
	if(!attr)
		return false;
	if(!attrname)
		return false;

	newname = StringReplace(attrname, "/", "@2F");
	if(newname == NULL) {
		return false;
	}
	extattr = attr->extra;

	while(extattr)
	{
		extpre = extattr;
		if(strcmp(extattr->name, newname)==0)
		{
			target = extattr;
			break;
		}
		extattr = extattr->next;
	}

	if(target == NULL)
	{
		target = calloc(1, sizeof(EXT_ATTRIBUTE_T));
		strncpy(target->name, newname, sizeof(target->name));

		if(extpre == NULL)
			attr->extra = target;
		else
			extpre->next = target;
	}

	if(target->type == attr_type_date || target->type == attr_type_string)
	{
		if(target->sv && (target->strlen >= sizeof(target->strvalue)))
			free(target->sv);
		target->sv = NULL;

		memset(target->strvalue, 0, sizeof(target->strvalue));
		target->strlen = 0;
	}

	if(svalue)
	{
		target->strlen = strlen(svalue);
		if (target->strlen >= sizeof(target->strvalue))
			target->sv = strdup(svalue);
		else
		{
			strncpy(target->strvalue, svalue, sizeof(target->strvalue));
			target->sv = target->strvalue;
		}
		target->type = attr_type_string;
	}
	StringFree(newname);
	return true;
}

bool MSG_AppendIoTSensorAttributeTimestamp(MSG_ATTRIBUTE_T* attr, const char* attrname, unsigned int value)
{
	EXT_ATTRIBUTE_T *extattr = NULL,*extpre = NULL, *target = NULL;
	char* newname = NULL;
	if(!attr)
		return false;
	if(!attrname)
		return false;

	newname = StringReplace(attrname, "/", "@2F");
	if(newname == NULL) {
		return false;
	}
	extattr = attr->extra;

	while(extattr)
	{
		extpre = extattr;
		if(strcmp(extattr->name, newname)==0)
		{
			target = extattr;
			break;
		}
		extattr = extattr->next;
	}

	if(target == NULL)
	{
		target = calloc(1, sizeof(EXT_ATTRIBUTE_T));
		strncpy(target->name, newname, sizeof(target->name));

		if(extpre == NULL)
			attr->extra = target;
		else
			extpre->next = target;
	}

	if(target->type == attr_type_date || target->type == attr_type_string)
	{
		if(target->sv && (target->strlen >= sizeof(target->strvalue)))
			free(target->sv);
		target->sv = NULL;
	}
	target->v = value;
	target->type = attr_type_timestamp;
	StringFree(newname);
	return true;
}

bool MSG_AppendIoTSensorAttributeDate(MSG_ATTRIBUTE_T* attr, const char* attrname, char *svalue)
{
	EXT_ATTRIBUTE_T *extattr = NULL,*extpre = NULL, *target = NULL;
	char* newname = NULL;
	if(!attr)
		return false;
	if(!attrname)
		return false;

	newname = StringReplace(attrname, "/", "@2F");
	if(newname == NULL) {
		return false;
	}
	extattr = attr->extra;

	while(extattr)
	{
		extpre = extattr;
		if(strcmp(extattr->name, newname)==0)
		{
			target = extattr;
			break;
		}
		extattr = extattr->next;
	}

	if(target == NULL)
	{
		target = calloc(1, sizeof(EXT_ATTRIBUTE_T));
		strncpy(target->name, newname, sizeof(target->name));

		if(extpre == NULL)
			attr->extra = target;
		else
			extpre->next = target;
	}

	if(target->type == attr_type_date || target->type == attr_type_string)
	{
		if(target->sv && (target->strlen >= sizeof(target->strvalue)))
			free(target->sv);
		target->sv = NULL;

		memset(target->strvalue, 0, sizeof(target->strvalue));
		target->strlen = 0;
	}

	if(svalue)
	{
		target->strlen = strlen(svalue);
		if(target->strlen >= sizeof(target->strvalue))
			target->sv = strdup(svalue);
		else
		{
			strncpy(target->strvalue, svalue, sizeof(target->strvalue));
			target->sv = target->strvalue;
		}
		target->type = attr_type_date;
	}
	StringFree(newname);
	return true;
}

#pragma endregion Generate_JSON


