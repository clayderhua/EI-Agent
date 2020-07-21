#include "XmlHelperLib.h"

#include <string.h>
#include "libxml/xpath.h"
#include "libxml/parser.h"

#include "util_path.h"

#define DEF_ENCODING_TYPE     "UTF-8"

static xmlXPathObjectPtr GetNodeSet(xmlDocPtr doc, const xmlChar *pXpath);
static xmlNodePtr AddNodeToDoc(xmlDocPtr doc, const char * itemPath);

PXmlDoc CreateXmlDoc()
{
	return  xmlNewDoc(BAD_CAST("1.0"));
}

PXmlDoc CreateXmlDocFromComment(const char * comment, int size)
{
	xmlDocPtr doc = NULL;
	if (comment != NULL)
	{
		doc = xmlParseMemory(comment, size);
	}
	return doc;
}

PXmlDoc CreateXmlDocFromFile(char * cfgPath)
{
	xmlDocPtr doc = NULL;
	if(cfgPath != NULL && util_is_file_exist(cfgPath))
	{
		//doc = xmlReadFile(cfgPath, "UTF-8", XML_PARSE_RECOVER);
		doc = xmlParseFile(cfgPath);
	}
	return doc;
}

int GetItemValueFormDoc(PXmlDoc doc, char * itemPath, char * itemValue, unsigned int valueLen)
{
	int iRet = -1;
	if(NULL == itemPath || NULL == itemValue || NULL == doc) return iRet;
	{
		xmlXPathObjectPtr xpRet = NULL;
		xmlChar * pXPath = NULL;
		pXPath = BAD_CAST(itemPath);
		xpRet = GetNodeSet(doc, pXPath);
		if(xpRet) 
		{
			int i = 0;
			xmlChar *nodeValue = NULL;
			xmlNodePtr curNode = NULL;
			xmlNodeSetPtr nodeset = xpRet->nodesetval;
			for (i = 0; i < nodeset->nodeNr; i++) 
			{
				curNode = nodeset->nodeTab[i];    
				if(curNode != NULL) 
				{
					nodeValue = xmlNodeGetContent(curNode);
					if (nodeValue != NULL) 
					{
						if(strlen((char*) nodeValue) < valueLen)
						{
							strcpy(itemValue, (char*) nodeValue);
							iRet = 0;
						}
						else
						{
							iRet = strlen(itemValue) + 1;
						}
						xmlFree(nodeValue);
						break;
					}
				}
			}
			xmlXPathFreeObject(xpRet);
		}
	}
	return iRet;
}

int GetItemValueFormDocEx(PXmlDoc doc, char * itemPath, char ** itemValue)
{
	int iRet = -1;
	if(NULL == itemPath || NULL == itemValue || NULL == doc) return iRet;
	{
		xmlXPathObjectPtr xpRet = NULL;
		xmlChar * pXPath = NULL;
		pXPath = BAD_CAST(itemPath);
		xpRet = GetNodeSet(doc, pXPath);
		if(xpRet) 
		{
			int i = 0;
			xmlChar *nodeValue = NULL;
			xmlNodePtr curNode = NULL;
			xmlNodeSetPtr nodeset = xpRet->nodesetval;
			for (i = 0; i < nodeset->nodeNr; i++) 
			{
				curNode = nodeset->nodeTab[i];    
				if(curNode != NULL) 
				{
					nodeValue = xmlNodeGetContent(curNode);
					if (nodeValue != NULL) 
					{
						int len = strlen((char*) nodeValue)+1;
						*itemValue = (char *)malloc(len);
						memset(*itemValue, 0, len);
						strcpy(*itemValue, (char*) nodeValue);
						iRet = 0;
						xmlFree(nodeValue);
						break;
					}
				}
			}
			xmlXPathFreeObject(xpRet);
		}
	}
	return iRet;
}

int SetItemValueToDoc(PXmlDoc doc, const char * itemPath, const char * itemValue)
{
	int iRet = -1;
	if(NULL != doc && itemPath != NULL && itemValue != NULL)
	{
		xmlXPathObjectPtr xpRet = NULL;
		xmlChar * pXPath = NULL;
		xmlNodePtr curNode = NULL;
		pXPath = BAD_CAST(itemPath);
		xpRet = GetNodeSet(doc, pXPath);
		if(xpRet) 
		{
			int i = 0;
			xmlNodeSetPtr nodeset = xpRet->nodesetval;
			for (i = 0; i < nodeset->nodeNr; i++) 
			{
				curNode = nodeset->nodeTab[i];    
				if(curNode) 
				{
					xmlNodeSetContent(curNode, (unsigned char*) itemValue);
					iRet = 0;
					break;
				}
			}
			xmlXPathFreeObject(xpRet);
		}
		else
		{
			curNode = AddNodeToDoc(doc, itemPath);
			if(curNode)
			{
				xmlNodeSetContent(curNode, (unsigned char*) itemValue);
				iRet = 0;
			}
		}
	}
	return iRet;
}

int SaveXmlDocToFile(PXmlDoc doc, char * cfgPath)
{
	int iRet = -1;
	if(NULL != doc || NULL != cfgPath)
	{
		iRet = xmlSaveFormatFileEnc(cfgPath, doc, DEF_ENCODING_TYPE, 1);
		//iRet = xmlSaveFile(cfgPath, doc);
	}
	return iRet;
}

void DestoryXmlDoc(PXmlDoc doc)
{
	xmlFreeDoc(doc);
}

static xmlXPathObjectPtr GetNodeSet(xmlDocPtr doc, const xmlChar *pXpath) 
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

static xmlNodePtr AddNodeToDoc(xmlDocPtr doc, const char * itemPath)
{
	xmlNodePtr retNode = NULL;
	if(doc != NULL && itemPath != NULL)
	{
		char *tmpPath = NULL, *curSlice = NULL;
		xmlNode  *rootNode = NULL, *parentNode = NULL, *curNode = NULL;
		int len = strlen(itemPath)+1;
		tmpPath = (char *)malloc(len);
		memset(tmpPath, 0, len);
		strcpy(tmpPath, itemPath);
		curSlice = strtok(tmpPath, "/");
		if(curSlice)
		{
			rootNode = xmlDocGetRootElement(doc);
			if(rootNode)  // root process
			{
				if(!xmlStrcmp(rootNode->name, BAD_CAST(curSlice)))
				{
					parentNode = rootNode;
				}
			}
			else
			{
				rootNode = xmlNewNode(NULL, BAD_CAST(curSlice));
				xmlDocSetRootElement(doc, rootNode);
				parentNode = rootNode;
				retNode = rootNode;
			}
			if(parentNode) //root exist, other node process
			{
				while( (curSlice = strtok(NULL, "/")) != NULL )
				{
					curNode = parentNode->children;
					while(curNode)
					{
						if(!xmlStrcasecmp(curNode->name, BAD_CAST(curSlice)))
						{
							parentNode = curNode;
							break;
						}
						curNode = curNode->next;
					}
					if(!curNode)
					{
						curNode = xmlNewNode(NULL, BAD_CAST(curSlice));
						xmlAddChild(parentNode, curNode);
						parentNode = curNode;
						retNode = curNode;
					}
				}
			}
		}
		free(tmpPath);
	}
	return retNode;
}
