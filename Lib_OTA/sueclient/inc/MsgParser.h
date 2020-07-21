#ifndef _MSG_PARSER_H_
#define _MSG_PARSER_H_

#include "SUEClientCoreData.h"
#include "ScheduleData.h"
#include "ListHelper.h"

//---------------sue cmd id 101-200 S---------------
#define  CMD_INVALID            0
#define  CMD_DOWNLOAD_REQ       101
#define  CMD_DEPLOY_REQ         103
#define  CMD_AUTO_STATUS_REP    104
#define  CMD_REQ_DOWNLOAD_REQ   105
#define  CMD_REQ_DOWNLOAD_REP   106
#define  CMD_STATUS_REQ         107
#define  CMD_REQP_STATUS_REP    108

#define  CMD_SET_SCHE_REQ       109
#define  CMD_SET_SCHE_REP       110
#define  CMD_DEL_SCHE_REQ       111
#define  CMD_DEL_SCHE_REP       112
#define  CMD_SCHE_INFO_REQ      113
#define  CMD_SCHE_INFO_REP      114
#define  CMD_CHECK_PKGINFO_REQ  115
#define  CMD_CHECK_PKGINFO_REP  116
#define  CMD_SCHE_STATUS_REP    118
#define  CMD_SWINFO_REQ         119
#define  CMD_SWINFO_REP         120
#define  CMD_DEL_PKG_REQ        121
#define  CMD_DEL_PKG_REP        122
//---------------sue cmd id 100-199 E---------------

//---------------sue cmd key define S---------------
#define DEF_SUECMDDATA_KEY                  "Data"
#define DEF_CMDID_KEY                       "CmdID"
#define DEF_DEVICEID_KEY                    "DevID"
#define DEF_SENDTS_KEY                      "SendTS"
#define DEF_E_KEY                           "e"

#define DEF_TASKINFO_KEY                    "TaskInfo"
#define DEF_PKGTYPE_KEY                     "PkgType"
#define DEF_URL_KEY                         "URL"
#define DEF_PKGNAME_KEY                     "PkgName"
#define DEF_PROTOCAL_KEY                    "Protocal"
#define DEF_SERCURITY_KEY                   "Sercurity"
#define DEF_ISDEPLOY_KEY                    "IsDp"
#define DEF_TASKSTATUS_KEY                  "TaskStatus"
#define DEF_TYPE_KEY                        "Type"
#define DEF_STATUS_KEY                      "Status"
#define DEF_PERCENT_KEY                     "Percent"
#define DEF_MSG_KEY                         "Msg"
#define DEF_ERRCODE_KEY                     "ErrCode"
#define DEF_TASKREQ_KEY                     "TaskReq"
#define DEF_ACTION_KEY                      "Action"
#define DEF_RETRYCNT_KEY                    "RetryCnt"
#define DEF_DLRETRY_KEY                     "DlRetry"
#define DEF_DPRETRY_KEY                     "DpRetry"
#define DEF_ISROLLBACK_KEY                  "IsRB"
#define DEF_SUBDEVICES_KEY					"SubDevices"
#define DEF_PKGOWNERID_KEY					"PkgOwnerId"

#define DEF_SCHEINFO_KEY                    "ScheInfo"
#define DEF_ACTTYPE_KEY                     "ActType"
#define DEF_REQ_PKGTYPE_KEY                 "ReqPkgType"
#define DEF_REQ_ACTTYPE_KEY                 "ReqActType"
#define DEF_SCHETYPE_KEY                    "ScheType"
#define DEF_STARTTIME_KEY                   "StartTime"
#define DEF_ENDTIME_KEY                     "EndTime"
#define DEF_SCHESTATUS_KEY                  "ScheStatus"
#define DEF_VERSION_KEY                     "Version"
#define DEF_OS_KEY                          "OS"
#define DEF_ARCH_KEY                        "Arch"
#define DEF_UPGMODE_KEY                     "UpgMode"

#define DEF_NAMES_KEY                       "Names"
#define DEF_NAME_KEY                        "Name" 
#define DEF_USABLE_KEY                      "Usable"

#define DEF_PKGNAMES_KEY                    "PkgNames"
//---------------sue cmd key define E---------------

int ParseCmdID(char * jsonStr);

int ParseDLTaskInfo(char * jsonStr, PDLTaskInfo pDLTaskInfo);

int ParseDPTaskInfo(char * jsonStr, PDPTaskInfo pDPTaskInfo);

int ParseReqDLRetInfo(char * jsonStr, PDLTaskInfo pDLTaskInfo);

int ParseReqTaskStatusInfo(char * jsonStr, char ** pkgName);

int ParseSetScheInfo(char * jsonStr, PScheduleInfo pScheInfo);

int ParseDelScheInfo(char * jsonStr,  char** pkgType, int* actType, int* pkgOwnerId);

int ParseGetSheInfo(char * jsonStr, char ** pkgType, int * actType);

int ParseCheckPkgRetInfo(char * jsonStr, PDLTaskInfo pDLTaskInfo);

int ParseReqSWInfo(char * jsonStr, char *** swNames, int *cnt);

int ParseReqDelPkgInfo(char * jsonStr, char *** pkgNames, int *cnt, long* pkgOwnerId);

int PackAutoTaskStatusInfo(PTaskStatusInfo pTSInfo, char * devID, char ** jsonStr);

int PackReqpTaskStatusInfo(PTaskStatusInfo pTSInfo, char * devID, char ** jsonStr);

int PackReqDLTaskInfo(char * pkgName, char * devID, char ** jsonStr);

int PackSetScheRepInfo(char * reqStr, int errCode, char * devID, char ** jsonStr);

int PackDelScheRepInfo(char * reqStr, int errCode, char * devID, char ** jsonStr);

int PackScheStatusInfo(char * pkgType, int actType, long pkgOwnerId, int status, int errCode, char * devID, char ** jsonStr);

int PackScheInfo(char * pkgType, int actType, LHList * pDLScheInfoList, LHList * pDPScheInfoList, char * devID, char ** jsonStr);

int PackCheckPkgInfo(char * pkgType,
					 int upgMode,
					 char * version,
					 char * os,
					 char * arch,
					 char * devID,
					 char ** subDevices,
					 int subDeviceSize,
					 long pkgOwnerId,
					 char ** jsonStr);

int PackScheInfoList(LHList * pDLScheInfoList, LHList * pDPScheInfoList, char ** jsonStr);

int ParseScheInfoList(char * jsonStr, LHList * pDLScheInfoList, LHList * pDPScheInfoList);

int PackSWInfoList(LHList * pSWInfoList, char * devID, char ** jsonStr);

int PackDelPkgRetInfoList(LHList * pDelPkgRetInfoList, char * devID, char ** jsonStr);

#endif
