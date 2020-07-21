#ifndef _SUEC_DB_ACCESS_H_
#define _SUEC_DB_ACCESS_H_

#include "InternalData.h"
#include <stdbool.h>

#define DEF_SUEC_DB_NAME               "./OTAClient.db"

//---------------------Package process scene table S--------------------------
#define PKG_SCENE_TABLE_NAME        "PkgProcScene" 
#define PKG_NAME_ITEM               "PkgName"
#define PKG_TYPE_ITEM               "PkgType"
#define PKG_URL_ITEM                "PkgURL"
#define PKG_THEN_DP_ITEM            "ThenDP"
#define PKG_TASK_TYPE_ITEM          "TaskType"
#define PKG_TASK_STATUS_ITEM        "TaskStatus"
#define PKG_SUSPEND_SCODE_ITEM     "SuspendSCCode"
#define PKG_TASK_ACTION_ITEM        "TaskAction"
#define PKG_TASK_STEP_ITEM          "TaskStep"
#define PKG_TASK_ORDER_ITEM         "TaskOrder"
#define PKG_SECOND_ZIP_NAME_ITEM    "SecondZipNname"
#define PKG_OS_ITEM                 "OS"
#define PKG_ARCH_ITEM               "Arch"
#define PKG_VERSION_ITEM            "Version"
#define PKG_ZIP_PWD_ITEM            "ZipPwd"
#define PKG_INSTALL_TOOL_ITEM       "InstallTool"
#define PKG_EXE_FILE_NAME_ITEM      "ExeFileName"
#define PKG_RETCH_SCRIPT_NAME_ITEM  "RetChScriptName"
#define PKG_REBOOT_SYS_FLAG_ITEM    "RebootFlag"
#define PKG_DL_RETRY_CNT_ITEM       "DlRetryCnt"
#define PKG_DP_RETRY_CNT_ITEM       "DpRetryCnt"
#define PKG_DL_CUR_RETRY_NUM_ITEM   "DlCurRetryNum"
#define PKG_DP_CUR_RETRY_NUM_ITEM   "DpCurRetryNum"
#define PKG_IS_RB_ITEM              "IsRB"
#define RESUME_TYPE_ITEM            "ResumeType"
#define PKG_OPRT_ERR_CODE_ITEM      "ErrCode"
#define DATETIME_ITEM               "DateTime"
#define PKG_OWNER_ID                "PkgOwnerId"

#define DEF_CREATE_PKGSCENE_TABLE_SQL_FORMAT "CREATE TABLE IF NOT EXISTS "PKG_SCENE_TABLE_NAME" (" \
	PKG_NAME_ITEM" NVARCHAR(100) UNIQUE, " \
	PKG_TYPE_ITEM" NVARCHAR(100)," \
	PKG_URL_ITEM" NVARCHAR(100)," \
	PKG_THEN_DP_ITEM" INT," \
	PKG_TASK_TYPE_ITEM" INT," \
	PKG_TASK_STATUS_ITEM" INT," \
	PKG_SUSPEND_SCODE_ITEM" INT," \
	PKG_TASK_ACTION_ITEM" INT," \
	PKG_TASK_STEP_ITEM" INT," \
	PKG_TASK_ORDER_ITEM" UNSIGNED BIG INT," \
	PKG_SECOND_ZIP_NAME_ITEM" NVARCHAR(100)," \
	PKG_OS_ITEM" NVARCHAR(100)," \
	PKG_ARCH_ITEM" NVARCHAR(100)," \
	PKG_VERSION_ITEM" NVARCHAR(100)," \
	PKG_ZIP_PWD_ITEM" NVARCHAR(100)," \
	PKG_INSTALL_TOOL_ITEM" NVARCHAR(100)," \
	PKG_EXE_FILE_NAME_ITEM" NVARCHAR(100)," \
	PKG_RETCH_SCRIPT_NAME_ITEM" NVARCHAR(100)," \
	PKG_REBOOT_SYS_FLAG_ITEM" INT," \
	PKG_DL_RETRY_CNT_ITEM" INT," \
	PKG_DP_RETRY_CNT_ITEM" INT," \
	PKG_DL_CUR_RETRY_NUM_ITEM" INT," \
	PKG_DP_CUR_RETRY_NUM_ITEM" INT," \
	PKG_IS_RB_ITEM" INT," \
	RESUME_TYPE_ITEM" INT," \
	PKG_OPRT_ERR_CODE_ITEM" INT," \
	DATETIME_ITEM" DATETIME," \
	PKG_OWNER_ID" INTEGER" \
	")"

#define DEF_PKGSCENE_REPLASE_SQL_FORMAT "REPLACE INTO "PKG_SCENE_TABLE_NAME" (" \
	PKG_NAME_ITEM", " \
	PKG_TYPE_ITEM", " \
	PKG_URL_ITEM", " \
	PKG_THEN_DP_ITEM", " \
	PKG_TASK_TYPE_ITEM", " \
	PKG_TASK_STATUS_ITEM", " \
	PKG_SUSPEND_SCODE_ITEM", " \
	PKG_TASK_ACTION_ITEM", " \
	PKG_TASK_STEP_ITEM", " \
	PKG_TASK_ORDER_ITEM", "\
	PKG_SECOND_ZIP_NAME_ITEM", " \
	PKG_OS_ITEM", " \
	PKG_ARCH_ITEM", " \
	PKG_VERSION_ITEM", " \
	PKG_ZIP_PWD_ITEM", " \
	PKG_INSTALL_TOOL_ITEM", " \
	PKG_EXE_FILE_NAME_ITEM", " \
	PKG_RETCH_SCRIPT_NAME_ITEM", " \
	PKG_REBOOT_SYS_FLAG_ITEM", " \
	PKG_DL_RETRY_CNT_ITEM", " \
	PKG_DP_RETRY_CNT_ITEM", " \
	PKG_DL_CUR_RETRY_NUM_ITEM", " \
	PKG_DP_CUR_RETRY_NUM_ITEM", " \
	PKG_IS_RB_ITEM", " \
	RESUME_TYPE_ITEM", " \
	PKG_OPRT_ERR_CODE_ITEM", " \
	DATETIME_ITEM", " \
	PKG_OWNER_ID \
	") " \
	"values('%s', '%s', '%s', '%d', '%d', '%d', '%d', '%d', '%d','%lld', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s',\
	'%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', datetime('%s', 'localtime'), '%ld')"

#define DEF_PKGSCENE_SELECT_ALL_SQL_FORMAT "SELECT * FROM "PKG_SCENE_TABLE_NAME" ORDER BY "PKG_TASK_ORDER_ITEM" ASC"
#define DEF_PKGSCENE_SELECT_WITH_RESUMETYPE_SQL_FORMAT "SELECT * FROM "PKG_SCENE_TABLE_NAME" WHERE "RESUME_TYPE_ITEM"='%d' ORDER BY "PKG_TASK_ORDER_ITEM" ASC"
#define DEF_PKGSCENE_SELECT_ONE_SQL_FORMAT "SELECT * FROM "PKG_SCENE_TABLE_NAME" WHERE "RESUME_TYPE_ITEM"='%d' AND "PKG_TYPE_ITEM"='%s'"
#define DEF_PKGSCENE_DELETE_ALL_SQL_FORMAT "DELETE FROM "PKG_SCENE_TABLE_NAME
#define DEF_PKGSCENE_DELETE_ONE_SQL_FORMAT "DELETE FROM "PKG_SCENE_TABLE_NAME" WHERE "PKG_NAME_ITEM"='%s'"
#define DEF_PKGSCENE_DELETE_WITH_RESUMETYPE_SQL_FORMAT "DELETE FROM "PKG_SCENE_TABLE_NAME" WHERE "RESUME_TYPE_ITEM"='%d'"

#define DEF_SELECT_COLUMNS_FORMAT "SELECT * FROM %s"
#define DEF_ADD_COLUMN_FORMAT  "ALTER TABLE %s ADD COLUMN %s %s"

#define DEF_UNKNOW_RT             0
#define DEF_COMMON_RT             1
#define DEF_SYS_STARTUP_RT        2
#define DEF_RB_RT                 4
#define DEF_ALL_RT                (DEF_COMMON_RT|DEF_SYS_STARTUP_RT|DEF_RB_RT)
#define DEF_DB_NULL_STR           "(null)"
//---------------------Package process scene table E--------------------------

//----------------------------Package info table S----------------------------
#define PKG_INFO_TABLE_NAME        "PkgInfo" 

#define PKG_INFO_TYPE_ITEM          "PkgType"
#define PKG_INFO_VERSION_ITEM       "Version"
#define PKG_INFO_DATETIME_ITEM      "DateTime"

#define DEF_CREATE_PKGINFO_TABLE_SQL_FORMAT "CREATE TABLE IF NOT EXISTS "PKG_INFO_TABLE_NAME" \
    ("PKG_INFO_TYPE_ITEM" NVARCHAR(100) UNIQUE, "PKG_INFO_VERSION_ITEM" NVARCHAR(100)," \
     PKG_INFO_DATETIME_ITEM" DATETIME)"

#define DEF_PKGINFO_REPLASE_SQL_FORMAT "REPLACE INTO "PKG_INFO_TABLE_NAME" \
    ("PKG_INFO_TYPE_ITEM", "PKG_INFO_VERSION_ITEM", "PKG_INFO_DATETIME_ITEM") \
     values('%s', '%s', datetime('%s', 'localtime'))"

#define DEF_PKGINFO_SELECT_ONE_SQL_FORMAT "SELECT * FROM "PKG_INFO_TABLE_NAME" WHERE "PKG_INFO_TYPE_ITEM"='%s'"
//----------------------------Package info table E----------------------------

//----------------------------Software info table S---------------------------
#define SW_INFO_TABLE_NAME        "SWInfo" 
#define SW_INFO_NAME_ITEM         "Name"
#define SW_INFO_VERSION_ITEM      "Version"
#define SW_INFO_USABLE_ITEM       "Usable"
#define SW_INFO_DATETIME_ITEM     "DateTime"
#define DEF_CREATE_SWINFO_TABLE_SQL_FORMAT "CREATE TABLE IF NOT EXISTS "SW_INFO_TABLE_NAME" \
    ("SW_INFO_NAME_ITEM" NVARCHAR(100) UNIQUE, "SW_INFO_VERSION_ITEM" NVARCHAR(100), " \
     SW_INFO_USABLE_ITEM" INT, "SW_INFO_DATETIME_ITEM" DATETIME)"
#define DEF_SWINFO_REPLASE_SQL_FORMAT "REPLACE INTO "SW_INFO_TABLE_NAME" ("SW_INFO_NAME_ITEM", " \
    SW_INFO_VERSION_ITEM", "SW_INFO_USABLE_ITEM", "SW_INFO_DATETIME_ITEM") \
    values('%s', '%s', '%d', datetime('%s', 'localtime'))"
#define DEF_SWINFO_SELECT_ALL_SQL_FORMAT "SELECT * FROM "SW_INFO_TABLE_NAME" ORDER BY "SW_INFO_DATETIME_ITEM" ASC"
#define DEF_SWINFO_SELECT_ONE_SQL_FORMAT "SELECT * FROM "SW_INFO_TABLE_NAME" WHERE "SW_INFO_NAME_ITEM"='%s'"
#define DEF_SWINFO_DELETE_ALL_SQL_FORMAT "DELETE FROM "SW_INFO_TABLE_NAME
#define DEF_SWINFO_DELETE_ONE_SQL_FORMAT "DELETE FROM "SW_INFO_TABLE_NAME" WHERE "SW_INFO_NAME_ITEM"='%s'"
//---------------------------Software info table E----------------------------
bool SUECInitDB(char * dpDir);
bool SUECUninitDB();

bool ReplacePkgScene(PPkgProcContext pPkgProcContext, int resumeType);
bool SelectAllPkgScene(PLHList pLHList);
bool SelectPkgSceneWithResumeType(PLHList pLHList, int resumeType);
bool SelectOnePkgSceneWithRTPT(int resumeType, char* pkgType, PPkgProcContext * ppPkgProcContext);
bool DeleteOnePkgScene(char * name);
bool DeleteAllPkgScene();
bool DeletePkgSceneWithResumeType(int resumeType);

bool ReplaceSWInfo(PSWInfo pSWInfo);
bool SelectOneSWInfo(char * name, PSWInfo * ppSWInfo);
bool SelectAllSWInfo(PLHList * ppLHList);
bool DeleteOneSWInfo(char * name);
bool DeleteAllSWInfo();

bool ReplacePkgInfo(char * pkgType, char * version);
bool SelectOnePkgInfo(char * pkgType, char ** version);
bool AddColumn2PkgProcScene(char* columnName, char* type);

#endif
