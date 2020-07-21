#include <libxml/xpath.h>
#include <libxml/parser.h>
#include <string.h>
#include <unistd.h>
#include "moduleconfig.h"
#include "util_string.h"

#define XML_CONFIG_ROOT "XMLConfigSettings"
#define XML_CONFIG_BASIC "BaseSettings"

char * module_ansitoutf8(char* wText)
{
	char * utf8RetStr = NULL;
	int tmpLen = 0;
	if(!wText)
		return utf8RetStr;
	if(!IsUTF8(wText))
	{
		utf8RetStr = ANSIToUTF8(wText);
		tmpLen = !utf8RetStr ? 0 : strlen(utf8RetStr);
		if(tmpLen == 1)
		{
			if(utf8RetStr) free(utf8RetStr);
			utf8RetStr = UnicodeToUTF8((wchar_t *)wText);
		}
	}
	else
	{
		tmpLen = strlen(wText)+1;
		utf8RetStr = (char *)malloc(tmpLen);
		memcpy(utf8RetStr, wText, tmpLen);
	}
	return utf8RetStr;
}

xmlXPathObjectPtr module_GetNodeSet(xmlDocPtr doc, const xmlChar *pXpath) 
{
	xmlXPathContextPtr context = NULL;	    
	xmlXPathObjectPtr xpRet = NULL;		
	if(doc == NULL || pXpath == NULL) return xpRet;
	{
		context = xmlXPathNewContext(doc);		
		if (context != NULL) 
		{	
			xpRet = xmlXPathEvalExpression(pXpath, context); 
			xmlXPathFreeContext(context);				
			if (xpRet != NULL) 
			{
				if (xmlXPathNodeSetIsEmpty(xpRet->nodesetval))   
				{
					xmlXPathFreeObject(xpRet);
					xpRet = NULL;
				}
			}
		}
	}

	return xpRet;	
}


xmlDocPtr module_Loadfile(char const * configFile)
{
	xmlDocPtr doc = NULL;

	if(configFile == NULL) 
		return doc;

	if(access(configFile, R_OK))
		return doc;

	xmlInitParser();
	doc = xmlReadFile(configFile, "UTF-8", 0);
	if(doc == NULL)
		return doc;

	return doc;
}

bool module_SaveFile(char const * configFile, xmlDocPtr doc)
{
	bool bRet = false;
	if(configFile == NULL) 
		return bRet;

	if(doc == NULL)
		return bRet;

	if(!access(configFile, F_OK))
	{
		int count = 10;
		while(access(configFile, W_OK))
		{
			if(count <= 0)
				return bRet;
			count--;
		}
	}
	if(xmlSaveFile(configFile, doc)>0)
		bRet = true;

	return bRet;
}
bool module_GetItemValue(xmlDocPtr doc, char const * const itemName, char * itemValue, int valueLen)
{
	bool bRet = false;
	if(NULL == doc || NULL == itemName || NULL == itemValue) return bRet;
	{
		xmlChar * pXPath = NULL;
		xmlXPathObjectPtr xpRet = NULL;
		xmlChar *nodeValue = NULL;
		xmlNodePtr curNode = NULL;
		char xPathStr[128] = {0};

		sprintf(xPathStr, "/%s/%s/%s",XML_CONFIG_ROOT, XML_CONFIG_BASIC, itemName);
		pXPath = BAD_CAST(xPathStr);

		xpRet = module_GetNodeSet(doc, pXPath);
		if(xpRet) 
		{
			int i = 0;
			xmlNodeSetPtr nodeset = xpRet->nodesetval;
			for (i = 0; i < nodeset->nodeNr; i++) 
			{
				curNode = nodeset->nodeTab[i];    
				if(curNode != NULL) 
				{
					nodeValue = xmlNodeGetContent(curNode);
					if (nodeValue != NULL) 
					{
						if(xmlStrlen(nodeValue) < valueLen-1)
						{
							strcpy(itemValue, (char*)nodeValue);
							bRet = true;
						}
						xmlFree(nodeValue);
						break;
					}
				}
			}
			xmlXPathFreeObject(xpRet);
		}
	}
	return bRet;
}

void module_FreeDoc(xmlDocPtr doc)
{
	if(doc == NULL) 
		return;

	xmlFreeDoc(doc);
	xmlCleanupParser();
}

bool module_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen)
{
	bool bRet = false;
	if(NULL == configFile || NULL == itemName || NULL == itemValue) return bRet;
	{
		xmlDocPtr doc = NULL;
		doc = module_Loadfile(configFile);
		if(doc)
		{
			bRet = module_GetItemValue(doc, itemName, itemValue, valueLen);
			module_FreeDoc(doc);
		}
	}
	return bRet;
}
bool module_set(char const * const configFile, char const * const itemName, char const * const itemValue)
{
	bool bRet = false;

	if(NULL == configFile || NULL == itemName || NULL == itemValue) return bRet;
	{
		xmlNodePtr curNode = NULL;
		char* utf8value = module_ansitoutf8(itemValue);
		xmlDocPtr doc = module_Loadfile(configFile);

		if(doc == NULL)
			return false;

		//if(strlen(itemValue))
		{
			xmlChar * pXPath = NULL;
			char xPathStr[128] = {0};
			xmlXPathObjectPtr xpRet = NULL;
			sprintf(xPathStr, "/%s/%s/%s",XML_CONFIG_ROOT, XML_CONFIG_BASIC, itemName);
			pXPath = BAD_CAST(xPathStr);
			xpRet = module_GetNodeSet(doc, pXPath);
			if(xpRet) 
			{
				int i = 0;
				xmlNodeSetPtr nodeset = xpRet->nodesetval;
				for (i = 0; i < nodeset->nodeNr; i++) 
				{
					curNode = nodeset->nodeTab[i];    
					if(curNode != NULL) 
					{
						xmlNodeSetContent(curNode, BAD_CAST(utf8value));
						bRet = true;
						break;
					}
				}
				xmlXPathFreeObject (xpRet);
			}
			else
			{
				memset(xPathStr, 0, sizeof(xPathStr));
				sprintf(xPathStr, "/%s/%s",XML_CONFIG_ROOT, XML_CONFIG_BASIC);
				pXPath = BAD_CAST(xPathStr);
				xpRet = module_GetNodeSet(doc, pXPath);
				if(xpRet)
				{
					int i = 0;
					xmlNodeSetPtr nodeset = xpRet->nodesetval;
					for (i = 0; i < nodeset->nodeNr; i++) 
					{
						curNode = nodeset->nodeTab[i];    
						if(curNode != NULL) 
						{
							xmlNewTextChild(curNode, NULL, BAD_CAST(itemName), BAD_CAST(utf8value));
							bRet = true;
							break;
						}
					}
					xmlXPathFreeObject (xpRet);
				}
				else
				{
					memset(xPathStr, 0, sizeof(xPathStr));
					sprintf(xPathStr, "/%s",XML_CONFIG_ROOT);
					pXPath = BAD_CAST(xPathStr);
					xpRet = module_GetNodeSet(doc, pXPath);
					if(xpRet)
					{
						int i = 0;
						xmlNodeSetPtr nodeset = xpRet->nodesetval;
						for (i = 0; i < nodeset->nodeNr; i++) 
						{
							curNode = nodeset->nodeTab[i];    
							if(curNode != NULL) 
							{
								xmlNodePtr tNode = xmlNewChild(curNode, NULL, BAD_CAST(XML_CONFIG_BASIC), NULL);
								xmlNewTextChild(tNode, NULL, BAD_CAST(itemName), BAD_CAST(utf8value));
								bRet = true;
								break;
							}
						}
						xmlXPathFreeObject (xpRet);
					}
				}
			}
		}
		free(utf8value);
		module_SaveFile(configFile, doc);
		module_FreeDoc(doc);
	}
	return bRet;
}