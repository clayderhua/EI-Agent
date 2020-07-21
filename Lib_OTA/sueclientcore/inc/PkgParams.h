#ifndef _PKG_PARAMS_H_
#define _PKG_PARAMS_H_

#include "InternalData.h"

#define DEF_INNO_SETUP_STR             "Inno Setup"
#define DEF_ADVANCED_INSTALLER_STR     "Advanced Installer"
#define DEF_UNKNOW_INSTALLER_STR       "Unknow"

#define DEF_PKG_INFO_XML_FILE_NAME        "PackageInfo.xml"
#define DEF_DEPLOY_XML_FILE_NAME          "Deploy.xml"

#define DEF_PKG_OS_XML_PATH               "/PackageDescription/BaseSettings/OS"
#define DEF_PKG_ARCH_XML_PATH             "/PackageDescription/BaseSettings/Arch"
#define DEF_PKG_TAGS_XML_PATH             "/PackageDescription/BaseSettings/Tags"
#define DEF_PKG_NAME_XML_PATH             "/PackageDescription/BaseSettings/PackageName"
#define DEF_PKG_TYPE_XML_PATH             "/PackageDescription/BaseSettings/PackageType"
#define DEF_PKG_VERSION_XML_PATH          "/PackageDescription/BaseSettings/Version"
#define DEF_PKG_ZIP_PWD_XML_PATH          "/PackageDescription/Password"
#define DEF_PKG_INSTALLER_TOOL_PATH       "/PackageDescription/InstallerTool"

#define DEF_EXEC_FILE_NAME_XML_PATH       "/DeployDescription/DeploySetting/DeployFileName"
#define DEF_RET_CHECK_SCRIPT_XML_PATH     "/DeployDescription/DeploySetting/ResultCheckScript"
#define DEF_REBOOT_FLAG_XML_PATH          "/DeployDescription/DeploySetting/RebootFlag"

int GetPkgInfoFromXml(char * xmlFilePath, PXMLPkgInfo pXMLPkgInfo, char * pkgPath);

int GetDeployInfoFromXml(char * xmlFilePath, PXMLDeployInfo pXMLDeployInfo);

#endif
