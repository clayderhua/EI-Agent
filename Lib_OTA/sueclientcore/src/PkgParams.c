#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PkgParams.h"
#include "XmlHelperLib.h"
#include "MiniUnzipLib.h"

int GetPkgInfoFromXml(char * xmlFilePath, PXMLPkgInfo pXMLPkgInfo, char* pkgPath)
{
	int iRet = 0, tagRet = 0;
	PXmlDoc doc = NULL;
	char zipComment[500];
	char * pkgType = NULL;

	if(pkgPath == NULL || pXMLPkgInfo == NULL) {
		fprintf(stderr, "[OTA] GetPkgInfoFromXml: pkgPath(%p) or pXMLPkgInfo(%p) is NULL\n", pkgPath, pXMLPkgInfo);
		return -1;
	}

	memset(zipComment, 0, sizeof(zipComment));
	iRet = CheckZipComment(pkgPath, zipComment, sizeof(zipComment));
	if (iRet == 1) { // has comment, new OTA style
		doc = CreateXmlDocFromComment(zipComment, sizeof(zipComment));
	} else if (xmlFilePath) { // no comment, old OTA style
		doc = CreateXmlDocFromFile(xmlFilePath);
	} else { // error
		fprintf(stderr, "[OTA] GetPkgInfoFromXml: no zip comment and xmlFilePath is NULL\n");
		return -1;
	}

	if(doc == NULL) {
		fprintf(stderr, "[OTA] GetPkgInfoFromXml: xml doc create fail\n");
		return -1;
	}

	do {
		tagRet = GetItemValueFormDocEx(doc, DEF_PKG_TAGS_XML_PATH, &pXMLPkgInfo->tags);
		iRet = GetItemValueFormDocEx(doc, DEF_PKG_OS_XML_PATH, &pXMLPkgInfo->os);
		iRet |= GetItemValueFormDocEx(doc, DEF_PKG_ARCH_XML_PATH, &pXMLPkgInfo->arch);
		if (iRet && tagRet) { // if neither os/arch nor tags
			iRet = -1;
			break;
		}

		iRet = GetItemValueFormDocEx(doc, DEF_PKG_VERSION_XML_PATH, &pXMLPkgInfo->version);
		if (iRet) {
			break;
		}
		iRet = GetItemValueFormDocEx(doc, DEF_PKG_ZIP_PWD_XML_PATH, &pXMLPkgInfo->zipPwd);
		if (iRet) {
			break;
		}

		iRet = GetItemValueFormDocEx(doc, DEF_PKG_TYPE_XML_PATH, &pkgType);
		if (iRet == 0 && pkgType != NULL) {
			int len = strlen(pkgType) + strlen(".zip") + 1;
			pXMLPkgInfo->secondZipName = (char *) malloc(len);
			sprintf(pXMLPkgInfo->secondZipName, "%s.zip", pkgType);
		}
		if (pkgType)
			free(pkgType);
		//iRet = GetItemValueFormDocEx(doc, DEF_PKG_NAME_XML_PATH, &pXMLPkgInfo->secondZipName);
	} while (0);

	GetItemValueFormDocEx(doc, DEF_PKG_INSTALLER_TOOL_PATH, &pXMLPkgInfo->installerTool);
	if (pXMLPkgInfo->installerTool == NULL || strlen(pXMLPkgInfo->installerTool) == 0)
	{
		int len = strlen(DEF_UNKNOW_INSTALLER_STR) + 1;
		if (pXMLPkgInfo->installerTool) free(pXMLPkgInfo->installerTool);
		pXMLPkgInfo->installerTool = (char *)malloc(len);
		strcpy(pXMLPkgInfo->installerTool, DEF_UNKNOW_INSTALLER_STR);
	}
	DestoryXmlDoc(doc);

	if(iRet != 0) // if error
	{
		if(pXMLPkgInfo->os)
		{
			free(pXMLPkgInfo->os);
			pXMLPkgInfo->os = NULL;
		}
		if(pXMLPkgInfo->arch)
		{
			free(pXMLPkgInfo->arch);
			pXMLPkgInfo->arch = NULL;
		}
		if(pXMLPkgInfo->tags)
		{
			free(pXMLPkgInfo->tags);
			pXMLPkgInfo->tags = NULL;
		}
		if (pXMLPkgInfo->version)
		{
			free(pXMLPkgInfo->version);
			pXMLPkgInfo->version = NULL;
		}
	}
	return iRet;
}

int GetDeployInfoFromXml(char * xmlFilePath, PXMLDeployInfo pXMLDeployInfo)
{
	int iRet = 0;
	if(NULL != xmlFilePath && NULL != pXMLDeployInfo)
	{
		PXmlDoc doc = NULL;
		doc = CreateXmlDocFromFile(xmlFilePath);
		if(doc)
		{
			iRet = GetItemValueFormDocEx(doc, DEF_EXEC_FILE_NAME_XML_PATH, &pXMLDeployInfo->execFileName);
            if (iRet == 0)
            {
                char * itemVal = NULL;
                if (GetItemValueFormDocEx(doc, DEF_REBOOT_FLAG_XML_PATH, &itemVal) == 0)
                {
                    pXMLDeployInfo->rebootFlag = atoi(itemVal);
                }
                GetItemValueFormDocEx(doc, DEF_RET_CHECK_SCRIPT_XML_PATH, &pXMLDeployInfo->retCheckScript);
                if (pXMLDeployInfo->rebootFlag && pXMLDeployInfo->retCheckScript == NULL)
                {
                    iRet = -1;
                    if (pXMLDeployInfo->execFileName)
                    {
                        free(pXMLDeployInfo->execFileName);
                        pXMLDeployInfo->execFileName = NULL;
                    }
                    if (pXMLDeployInfo->retCheckScript)
                    {
                        free(pXMLDeployInfo->retCheckScript);
                        pXMLDeployInfo->retCheckScript = NULL;
                    }
                    pXMLDeployInfo->rebootFlag = 0;
                }
                if (itemVal)free(itemVal);
            }
            DestoryXmlDoc(doc);
		}
		else iRet = 1;
	}
	else iRet = -1;
	return iRet;
}

