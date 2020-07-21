#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util_path.h"
#include "SUECDBAccess.h"
#include "sqlite3.h"

#ifdef linux
#define strtok_s(s1,s2,s3) strtok_r((char *)(s1),(char *)(s2),(char **)(s3))
#define strcpy_s(s1,s2,s3) strncpy((char *)(s1),(char *)(s3),(int)(s2))
#else
#include "wrapper.h"
#define snprintf(dst,size,format,...) _snprintf(dst,size,format,##__VA_ARGS__)
#endif

static sqlite3 * SUECDBHandle = NULL;
static bool IsDBInited = false;

static int CheckColumnExist(char * tableName, char * columnName);
static void InsertColumnToTable(char * tableName, char * columnName, char * type);
static void FillValToPkgContext(PPkgProcContext pCurPkgProcContext, char**columns, char * val, int col);
static void GetPkgContextFromTable(PLHList pLHList, char ** retTable, int nRow, int nColumn);
static void LHFreeSWInfoCB(void * pUserData);
extern PPkgProcContext CreatePkgProcContext();

static int do_sqlite(sqlite3 *db,
					 const char *sql,
					 int (*callback)(void*, int, char**, char**),
					 void *param,
					 char* ok_msg,
					 int ignoreError)
{
	int rc;
	char *zErrMsg = NULL;
	
#ifdef DEBUG_TEST
	LOGD("do_sqlite(%s)", sql);
#endif

	rc = sqlite3_exec(db, sql, callback, param, &zErrMsg);
	if(rc != SQLITE_OK){
		if (!ignoreError) {
			LOGE("SQL error: command=[%s]", sql);
			LOGE("zErrMsg=[%s]", zErrMsg);
		}
		sqlite3_free(zErrMsg);
	} else if (ok_msg) {
		LOGD("SQL success: msg=[%.164s]", ok_msg);
	}

	return rc;
}

bool SUECInitDB(char * dpDir)
{
    bool bRet = false;
    int iRet = 0, len = 0;
    char modulePath[MAX_PATH] = { 0 };
    char * dbPath = NULL, *dbErrMsg = NULL;
    char * tmpDBDir = NULL;
    if (dpDir == NULL)
    {      
        util_module_path_get(modulePath);
        tmpDBDir = modulePath;
    }
    else tmpDBDir = dpDir;
    len = strlen(tmpDBDir) + strlen(DEF_SUEC_DB_NAME) + 2;
    dbPath = (char *)malloc(len);
    memset(dbPath, 0, len);
    if(tmpDBDir[strlen(tmpDBDir)-1] == FILE_SEPARATOR)
        sprintf(dbPath, "%s%s", tmpDBDir, DEF_SUEC_DB_NAME);
    else
        sprintf(dbPath, "%s%c%s", tmpDBDir, FILE_SEPARATOR, DEF_SUEC_DB_NAME);
    
    iRet = sqlite3_open(dbPath, &SUECDBHandle);
    if (iRet != SQLITE_OK) goto done1;
#ifndef WIN32
	// Enable WAL mode to improve disk I/O performance
	sqlite3_exec(SUECDBHandle, "PRAGMA synchronous = NORMAL", NULL, NULL, NULL);
	sqlite3_exec(SUECDBHandle, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
#endif
    iRet = sqlite3_exec(SUECDBHandle, DEF_CREATE_PKGSCENE_TABLE_SQL_FORMAT, NULL, NULL, &dbErrMsg);
    if (iRet != SQLITE_OK)
    {
        sqlite3_free(dbErrMsg);
        goto done2;
    }
    iRet = sqlite3_exec(SUECDBHandle, DEF_CREATE_PKGINFO_TABLE_SQL_FORMAT, NULL, NULL, &dbErrMsg);
    if (iRet != SQLITE_OK)
    {
        sqlite3_free(dbErrMsg);
        goto done2;
    }
    iRet = sqlite3_exec(SUECDBHandle, DEF_CREATE_SWINFO_TABLE_SQL_FORMAT, NULL, NULL, &dbErrMsg);
    if (iRet != SQLITE_OK)
    {
        sqlite3_free(dbErrMsg);
        goto done2;
    }
    if (!CheckColumnExist(PKG_SCENE_TABLE_NAME, PKG_RETCH_SCRIPT_NAME_ITEM))
    {
        InsertColumnToTable(PKG_SCENE_TABLE_NAME, PKG_RETCH_SCRIPT_NAME_ITEM, "NVARCHAR(100)");
        InsertColumnToTable(PKG_SCENE_TABLE_NAME, PKG_REBOOT_SYS_FLAG_ITEM, "INT");
        InsertColumnToTable(PKG_SCENE_TABLE_NAME, PKG_TASK_ACTION_ITEM, "INT");
        InsertColumnToTable(PKG_SCENE_TABLE_NAME, PKG_DL_CUR_RETRY_NUM_ITEM, "INT");
        InsertColumnToTable(PKG_SCENE_TABLE_NAME, PKG_DP_CUR_RETRY_NUM_ITEM, "INT");
        InsertColumnToTable(PKG_SCENE_TABLE_NAME, PKG_OPRT_ERR_CODE_ITEM, "INT");
        InsertColumnToTable(PKG_SCENE_TABLE_NAME, PKG_SUSPEND_SCODE_ITEM, "INT");
    }
    bRet = true;
done2:
    if (!bRet)
    {
        sqlite3_close(SUECDBHandle);
        SUECDBHandle = NULL;
    }
done1:
    free(dbPath);
    dbPath = NULL;
    IsDBInited = bRet;
    return bRet;
}

bool SUECUninitDB()
{
    bool bRet = false;
    if (IsDBInited)
    {
        sqlite3_close(SUECDBHandle);
        SUECDBHandle = NULL;
        IsDBInited = false;
    }
    bRet = true;
    return bRet;
}

// replace entry in database with the same pkgName
bool ReplacePkgScene(PPkgProcContext pPkgProcContext, int resumeType)
{
    bool bRet = false;
    /*if (IsDBInited && pPkgProcContext != NULL &&
        pPkgProcContext->pkgName != NULL && pPkgProcContext->pkgType != NULL &&
        pPkgProcContext->pkgDeployFilePath != NULL && pPkgProcContext->xmlPkgInfo.installerTool != NULL)*/
    if (IsDBInited && pPkgProcContext != NULL &&
        pPkgProcContext->pkgName != NULL && pPkgProcContext->pkgType != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[2048] = { 0 };
        int thenDP = pPkgProcContext->thenDP;

        if (resumeType == DEF_SYS_STARTUP_RT)
			thenDP = 1;
        snprintf(sqlStr, sizeof(sqlStr), DEF_PKGSCENE_REPLASE_SQL_FORMAT,
            pPkgProcContext->pkgName, pPkgProcContext->pkgType,
            pPkgProcContext->pkgURL, thenDP, pPkgProcContext->taskStatusInfo.taskType,
            pPkgProcContext->taskStatusInfo.statusCode,
            pPkgProcContext->suspendSCCode,
            pPkgProcContext->taskStatusInfo.taskAction, pPkgProcContext->pkgStep,
            pPkgProcContext->order, pPkgProcContext->xmlPkgInfo.secondZipName,
            pPkgProcContext->xmlPkgInfo.os, pPkgProcContext->xmlPkgInfo.arch,
            pPkgProcContext->xmlPkgInfo.version, pPkgProcContext->xmlPkgInfo.zipPwd,
            pPkgProcContext->xmlPkgInfo.installerTool, pPkgProcContext->xmlDeployInfo.execFileName,
            pPkgProcContext->xmlDeployInfo.retCheckScript,
            pPkgProcContext->xmlDeployInfo.rebootFlag,
            pPkgProcContext->dlRetryCnt, pPkgProcContext->dpRetryCnt, 
            pPkgProcContext->curDLRetryCnt, pPkgProcContext->curDPRetryCnt,
            pPkgProcContext->isRB,
            resumeType, pPkgProcContext->errCode, "now",
			pPkgProcContext->taskStatusInfo.pkgOwnerId);
        iRet = sqlite3_exec(SUECDBHandle, sqlStr, NULL, NULL, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            LOGE("ReplacePkgScene: dbErrMsg=[%s]", dbErrMsg);
            sqlite3_free(dbErrMsg);
        }
        else bRet = true;
    }
    return bRet;
}

bool SelectAllPkgScene(PLHList pLHList)
{
    bool bRet = false;
    if (IsDBInited && pLHList != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char ** retTable = NULL;
        int nRow = 0, nColumn = 0;
        iRet = sqlite3_get_table(SUECDBHandle, DEF_PKGSCENE_SELECT_ALL_SQL_FORMAT, &retTable,
            &nRow, &nColumn, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else
        {
            if (nRow > 0 && nColumn > 0)
            {
                GetPkgContextFromTable(pLHList, retTable, nRow, nColumn);
                bRet = true;
            }
            sqlite3_free_table(retTable);
        }
    }
    return bRet;
}

bool SelectPkgSceneWithResumeType(PLHList pLHList, int resumeType)
{
    bool bRet = false;
    if (IsDBInited && pLHList != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char ** retTable = NULL;
        int nRow = 0, nColumn = 0;
        char sqlStr[1024] = { 0 };
        sprintf(sqlStr, DEF_PKGSCENE_SELECT_WITH_RESUMETYPE_SQL_FORMAT, resumeType);
        iRet = sqlite3_get_table(SUECDBHandle, sqlStr, &retTable,
            &nRow, &nColumn, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else
        {
            if (nRow > 0 && nColumn > 0)
            {
                GetPkgContextFromTable(pLHList, retTable, nRow, nColumn);
                bRet = true;
            }
            sqlite3_free_table(retTable);
        }
    }
    return bRet;
}

bool SelectOnePkgSceneWithRTPT(int resumeType, char* pkgType, PPkgProcContext * ppPkgProcContext)
{
    bool bRet = false;
    if (IsDBInited && pkgType != NULL && ppPkgProcContext != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[512] = { 0 };
        char ** retTable = NULL;
        int nRow = 0, nColumn = 0;
        sprintf(sqlStr, DEF_PKGSCENE_SELECT_ONE_SQL_FORMAT, resumeType, pkgType);
        iRet = sqlite3_get_table(SUECDBHandle, sqlStr, &retTable,
            &nRow, &nColumn, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else
        {
            if (nRow > 0 && nColumn > 0)
            {
                int j = 0;
                char * curVal = NULL;
                PPkgProcContext pCurPkgProcContext = NULL;
                pCurPkgProcContext = CreatePkgProcContext();
                for (j = 0; j < nColumn; j++)
                {
                    curVal = retTable[1 * nColumn + j];
                    FillValToPkgContext(pCurPkgProcContext, retTable, curVal, j);
                }
                *ppPkgProcContext = pCurPkgProcContext;
                bRet = true;
            }
            sqlite3_free_table(retTable);
        }
    }
    return bRet;
}

bool DeleteOnePkgScene(char * name)
{
    bool bRet = false;
    if (IsDBInited && name != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[512] = { 0 };
        sprintf(sqlStr, DEF_PKGSCENE_DELETE_ONE_SQL_FORMAT, name);
        iRet = sqlite3_exec(SUECDBHandle, sqlStr, NULL, NULL, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else bRet = true;
    }
    return bRet;
}

bool DeleteAllPkgScene()
{
    bool bRet = false;
    if (IsDBInited)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        iRet = sqlite3_exec(SUECDBHandle, DEF_PKGSCENE_DELETE_ALL_SQL_FORMAT, NULL, NULL, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else bRet = false;
    }
    return bRet;
}

bool DeletePkgSceneWithResumeType(int resumeType)
{
    bool bRet = false;
    if (IsDBInited)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[512] = { 0 };
        sprintf(sqlStr, DEF_PKGSCENE_DELETE_WITH_RESUMETYPE_SQL_FORMAT, resumeType);
        iRet = sqlite3_exec(SUECDBHandle, sqlStr, NULL, NULL, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else bRet = false;
    }
    return bRet;
}

static int CheckColumnExist(char * tableName, char * columnName)
{
    int iRet = 0;
    if (tableName != NULL && columnName != NULL)
    {
        char * dbErrMsg = NULL;
        char ** retTable = NULL;
        int nRow = 0, nColumn = 0;
        char sqlStr[256] = { 0 };
        sprintf(sqlStr, DEF_SELECT_COLUMNS_FORMAT, tableName);
        iRet = sqlite3_get_table(SUECDBHandle, sqlStr, &retTable,
            &nRow, &nColumn, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
            iRet = 0;
        }
        else
        {
            iRet = 0;
            if (nRow >= 0 && nColumn > 0)
            {
                int j = 0;
                for (j = 0; j < nColumn; j++)
                {
                    if (!strcasecmp(retTable[j], columnName))
                    {
                        iRet = 1;
                        break;
                    }
                }
            }
            sqlite3_free_table(retTable);
        }
    }
    return iRet = 0;
}

static void InsertColumnToTable(char * tableName, char * columnName, char * type)
{
    if (tableName != NULL && columnName != NULL && type != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[256] = { 0 };
        sprintf(sqlStr, DEF_ADD_COLUMN_FORMAT, tableName, columnName, type);
        iRet = sqlite3_exec(SUECDBHandle, sqlStr, NULL, NULL, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
    }
}

static void FillValToPkgContext(PPkgProcContext pCurPkgProcContext, char**columns, char * val, int col)
{
    if (pCurPkgProcContext != NULL && columns != NULL && 
        val != NULL && strcasecmp(val, DEF_DB_NULL_STR) && col >= 0)
    {
        int len = 0;
        char * curField = columns[col];
        if (!strcasecmp(curField, PKG_NAME_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->pkgName = (char *)malloc(len);
            memset(pCurPkgProcContext->pkgName, 0, len);
            strcpy(pCurPkgProcContext->pkgName, val);
        }
        else if (!strcasecmp(curField, PKG_TYPE_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->pkgType = (char *)malloc(len);
            memset(pCurPkgProcContext->pkgType, 0, len);
            strcpy(pCurPkgProcContext->pkgType, val);
        }
        else if (!strcasecmp(curField, PKG_URL_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->pkgURL = (char *)malloc(len);
            memset(pCurPkgProcContext->pkgURL, 0, len);
            strcpy(pCurPkgProcContext->pkgURL, val);
        }
        else if (!strcasecmp(curField, PKG_THEN_DP_ITEM))
        {
            pCurPkgProcContext->thenDP = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_TASK_TYPE_ITEM))
        {
            pCurPkgProcContext->taskStatusInfo.taskType = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_TASK_STATUS_ITEM))
        {
            pCurPkgProcContext->taskStatusInfo.statusCode = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_TASK_ACTION_ITEM))
        {
            pCurPkgProcContext->taskStatusInfo.taskAction = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_TASK_STEP_ITEM))
        {
            pCurPkgProcContext->pkgStep = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_TASK_ORDER_ITEM))
        {
            pCurPkgProcContext->order = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_SECOND_ZIP_NAME_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->xmlPkgInfo.secondZipName = (char *)malloc(len);
            memset(pCurPkgProcContext->xmlPkgInfo.secondZipName, 0, len);
            strcpy(pCurPkgProcContext->xmlPkgInfo.secondZipName, val);
        }
        else if (!strcasecmp(curField, PKG_OS_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->xmlPkgInfo.os = (char *)malloc(len);
            memset(pCurPkgProcContext->xmlPkgInfo.os, 0, len);
            strcpy(pCurPkgProcContext->xmlPkgInfo.os, val);
        }
        else if (!strcasecmp(curField, PKG_OS_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->xmlPkgInfo.os = (char *)malloc(len);
            memset(pCurPkgProcContext->xmlPkgInfo.os, 0, len);
            strcpy(pCurPkgProcContext->xmlPkgInfo.os, val);
        }
        else if (!strcasecmp(curField, PKG_ARCH_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->xmlPkgInfo.arch = (char *)malloc(len);
            memset(pCurPkgProcContext->xmlPkgInfo.arch, 0, len);
            strcpy(pCurPkgProcContext->xmlPkgInfo.arch, val);
        }
        else if (!strcasecmp(curField, PKG_VERSION_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->xmlPkgInfo.version = (char *)malloc(len);
            memset(pCurPkgProcContext->xmlPkgInfo.version, 0, len);
            strcpy(pCurPkgProcContext->xmlPkgInfo.version, val);
        }
        else if (!strcasecmp(curField, PKG_ZIP_PWD_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->xmlPkgInfo.zipPwd = (char *)malloc(len);
            memset(pCurPkgProcContext->xmlPkgInfo.zipPwd, 0, len);
            strcpy(pCurPkgProcContext->xmlPkgInfo.zipPwd, val);
        }
        else if (!strcasecmp(curField, PKG_INSTALL_TOOL_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->xmlPkgInfo.installerTool = (char *)malloc(len);
            memset(pCurPkgProcContext->xmlPkgInfo.installerTool, 0, len);
            strcpy(pCurPkgProcContext->xmlPkgInfo.installerTool, val);
        }
        else if (!strcasecmp(curField, PKG_EXE_FILE_NAME_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->xmlDeployInfo.execFileName = (char *)malloc(len);
            memset(pCurPkgProcContext->xmlDeployInfo.execFileName, 0, len);
            strcpy(pCurPkgProcContext->xmlDeployInfo.execFileName, val);
        }
        else if (!strcasecmp(curField, PKG_RETCH_SCRIPT_NAME_ITEM))
        {
            len = strlen(val) + 1;
            pCurPkgProcContext->xmlDeployInfo.retCheckScript = (char *)malloc(len);
            memset(pCurPkgProcContext->xmlDeployInfo.retCheckScript, 0, len);
            strcpy(pCurPkgProcContext->xmlDeployInfo.retCheckScript, val);
        }
        else if (!strcasecmp(curField, PKG_REBOOT_SYS_FLAG_ITEM))
        {
            pCurPkgProcContext->xmlDeployInfo.rebootFlag = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_DL_RETRY_CNT_ITEM))
        {
            pCurPkgProcContext->dlRetryCnt = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_DP_RETRY_CNT_ITEM))
        {
            pCurPkgProcContext->dpRetryCnt = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_DL_CUR_RETRY_NUM_ITEM))
        {
            pCurPkgProcContext->curDLRetryCnt = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_DP_CUR_RETRY_NUM_ITEM))
        {
            pCurPkgProcContext->curDPRetryCnt = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_IS_RB_ITEM))
        {
            pCurPkgProcContext->isRB= strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_OPRT_ERR_CODE_ITEM))
        {
            pCurPkgProcContext->errCode = strtol(val, NULL, 10);
        }
        else if (!strcasecmp(curField, PKG_SUSPEND_SCODE_ITEM))
        {
            pCurPkgProcContext->suspendSCCode = strtol(val, NULL, 10);
        }
		else if (!strcasecmp(curField, PKG_OWNER_ID))
        {
            pCurPkgProcContext->taskStatusInfo.pkgOwnerId = strtol(val, NULL, 10);
        }
    }
}

static void GetPkgContextFromTable(PLHList pLHList, char ** retTable, int nRow, int nColumn)
{
    if (retTable != NULL && pLHList != NULL && nRow > 0 && nColumn > 0)
    {
        int i = 0, j = 0;
        char * curVal = NULL;
        PPkgProcContext pCurPkgProcContext = NULL;
        for (i = 0; i < nRow; i++)
        {
            pCurPkgProcContext = CreatePkgProcContext();
            for (j = 0; j < nColumn; j++)
            {
                curVal = retTable[(i + 1) * nColumn + j];
                FillValToPkgContext(pCurPkgProcContext, retTable, curVal, j);
            }
            LHAddNode(pLHList, pCurPkgProcContext);
        }
    }
}

bool ReplaceSWInfo(PSWInfo pSWInfo)
{
    bool bRet = false;
    if (IsDBInited && pSWInfo != NULL &&
        pSWInfo->name != NULL && pSWInfo->version != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[512] = { 0 };
        sprintf(sqlStr, DEF_SWINFO_REPLASE_SQL_FORMAT, pSWInfo->name, 
            pSWInfo->version, pSWInfo->usable, "now");
        iRet = sqlite3_exec(SUECDBHandle, sqlStr, NULL, NULL, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else bRet = true;
    }
    return bRet;
}

bool SelectOneSWInfo(char * name, PSWInfo * ppSWInfo)
{
    bool bRet = false;
    if (IsDBInited && name != NULL && ppSWInfo != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[512] = { 0 };
        char ** retTable = NULL;
        int nRow = 0, nColumn = 0;
        sprintf(sqlStr, DEF_SWINFO_SELECT_ONE_SQL_FORMAT, name);
        iRet = sqlite3_get_table(SUECDBHandle, sqlStr, &retTable,
            &nRow, &nColumn, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else
        {
            if (nRow > 0 && nColumn > 0)
            {
                int len = 0, j = 0;
                char * curVal = NULL;
                PSWInfo tmpSWInfo = (PSWInfo)malloc(sizeof(SWInfo));
                memset(tmpSWInfo, 0, sizeof(SWInfo));
                curVal = retTable[1 * nColumn + j++];
                len = strlen(curVal) + 1;
                tmpSWInfo->name = (char *)malloc(len);
                memset(tmpSWInfo->name, 0, len);
                strcpy(tmpSWInfo->name, curVal);
                curVal = retTable[1 * nColumn + j++];
                len = strlen(curVal) + 1;
                tmpSWInfo->version = (char *)malloc(len);
                memset(tmpSWInfo->version, 0, len);
                strcpy(tmpSWInfo->version, curVal);
                curVal = retTable[1 * nColumn + j++];
                tmpSWInfo->usable = atoi(curVal);
                *ppSWInfo = tmpSWInfo;
                bRet = true;
            }
            sqlite3_free_table(retTable);
        }
    }
    return bRet;
}

bool SelectAllSWInfo(PLHList * ppLHList)
{
    bool bRet = false;
    if (IsDBInited && ppLHList != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char ** retTable = NULL;
        int nRow = 0, nColumn = 0;
        iRet = sqlite3_get_table(SUECDBHandle, DEF_SWINFO_SELECT_ALL_SQL_FORMAT, &retTable,
            &nRow, &nColumn, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else
        {
            PLHList pTmpLHList = LHInitList(LHFreeSWInfoCB);
            int i = 0, len = 0, j = 0;
            char * curVal = NULL;
            PSWInfo tmpSWInfo;
            for (i = 0; i < nRow; i++)
            {
                j = 0;
                tmpSWInfo = (PSWInfo)malloc(sizeof(SWInfo));
                memset(tmpSWInfo, 0, sizeof(SWInfo));
                curVal = retTable[(i + 1) * nColumn + j++];
                len = strlen(curVal) + 1;
                tmpSWInfo->name = (char *)malloc(len);
                memset(tmpSWInfo->name, 0, len);
                strcpy(tmpSWInfo->name, curVal);
                curVal = retTable[(i + 1) * nColumn + j++];
                len = strlen(curVal) + 1;
                tmpSWInfo->version = (char *)malloc(len);
                memset(tmpSWInfo->version, 0, len);
                strcpy(tmpSWInfo->version, curVal);
                curVal = retTable[(i + 1) * nColumn + j++];
                tmpSWInfo->usable = atoi(curVal);
                LHAddNode(pTmpLHList, tmpSWInfo);
            }
            *ppLHList = pTmpLHList;
            sqlite3_free_table(retTable);
            bRet = true;
        }
    }
    return bRet;
}

bool DeleteOneSWInfo(char * name)
{
    bool bRet = false;
    if (IsDBInited && name != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[512] = { 0 };
        memset(sqlStr, 0, sizeof(sqlStr));
        sprintf(sqlStr, DEF_SWINFO_DELETE_ONE_SQL_FORMAT, name);
        iRet = sqlite3_exec(SUECDBHandle, sqlStr, NULL, NULL, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else bRet = true;
    }
    return bRet;
}

bool DeleteAllSWInfo()
{
    bool bRet = false;
    if (IsDBInited)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        iRet = sqlite3_exec(SUECDBHandle, DEF_SWINFO_DELETE_ALL_SQL_FORMAT, NULL, NULL, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else bRet = false;
    }
    return bRet;
}

static void LHFreeSWInfoCB(void * pUserData)
{
    if (pUserData != NULL)
    {
        PSWInfo pSWInfo = (PSWInfo)pUserData;
        if (pSWInfo->name) free(pSWInfo->name);
        pSWInfo->name = NULL;
        if (pSWInfo->version) free(pSWInfo->version);
        pSWInfo->version = NULL;
        free(pSWInfo);
    }
}

bool ReplacePkgInfo(char * pkgType, char * version)
{
    bool bRet = false;
    if (IsDBInited && pkgType != NULL &&
        version != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[1024] = { 0 };
        sprintf(sqlStr, DEF_PKGINFO_REPLASE_SQL_FORMAT,
            pkgType, version, "now");
        iRet = sqlite3_exec(SUECDBHandle, sqlStr, NULL, NULL, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else bRet = true;
    }
    return bRet;
}

bool CmpVersion(char * verF, char * verS)
{
    bool bRet = false;
	char* saveptr;

    if (verF != NULL && verS != NULL)
    {
        char * verSec[8] = { 0 };
        int verFI[4] = { 0 };
        int verSI[4] = { 0 };
        int i = 0;
        char * tmpStr = NULL;
        int len = 0, verCnt = 0;
        len = strlen(verF) + 1;
        tmpStr = (char*)malloc(len);
        memset(tmpStr, 0, len);
        strcpy(tmpStr, verF);
        verSec[verCnt] = strtok_s(tmpStr, ".", &saveptr);
        if (verSec[verCnt] != NULL)
        {
            verCnt++;
            while ( (verSec[verCnt] = strtok_s(NULL, ".", &saveptr)) != NULL )
            {
                verCnt++;
                if (verCnt >= 4) break;
            }
        }
        for (i = 0; i < verCnt; i++)
        {
            verFI[i] = atoi(verSec[i]);
        }
        free(tmpStr);

        verCnt = 0;
        len = strlen(verS) + 1;
        tmpStr = (char*)malloc(len);
        memset(tmpStr, 0, len);
        strcpy(tmpStr, verS);
        verSec[verCnt] = strtok_s(tmpStr, ".", &saveptr);
        if (verSec[verCnt] != NULL)
        {
            verCnt++;
            while ( (verSec[verCnt] = strtok_s(NULL, ".", &saveptr)) != NULL )
            {
                verCnt++;
                if (verCnt >= 4) break;
            }
        }
        for (i = 0; i < verCnt; i++)
        {
            verSI[i] = atoi(verSec[i]);
        }
        free(tmpStr);

        if (verFI[0] > verSI[0]) bRet = true;
        if (verFI[0] == verSI[0] && verFI[1] > verSI[1]) bRet = true;
        if (verFI[0] == verSI[0] && verFI[1] == verSI[1] && verFI[2] > verSI[2]) bRet = true;
        if (verFI[0] == verSI[0] && verFI[1] == verSI[1] && verFI[2] == verSI[2] &&
            verFI[3] > verSI[3]) bRet = true;
        if (verFI[0] == verSI[0] && verFI[1] == verSI[1] && verFI[2] == verSI[2] &&
            verFI[3] == verSI[3]) bRet = true;
    }
    return bRet;
}
bool SelectOnePkgInfo(char * pkgType, char ** version)
{
#define DEF_PKGINFO_SELECT_PKGVERSION_SQL_FORMAT  "SELECT "PKG_INFO_VERSION_ITEM" FROM "PKG_INFO_TABLE_NAME" WHERE "PKG_INFO_TYPE_ITEM"='%s'"
#define DEF_PKGSCENE_SELECT_PKGVERSION_SQL_FORMAT  "SELECT "PKG_INFO_VERSION_ITEM" FROM "PKG_SCENE_TABLE_NAME" WHERE "PKG_INFO_TYPE_ITEM"='%s'"
    bool bRet = false;
    if (IsDBInited && pkgType != NULL && version != NULL)
    {
        int iRet = 0;
        char * dbErrMsg = NULL;
        char sqlStr[512] = { 0 };
        char ** retTable = NULL;
        int nRow = 0, nColumn = 0;
        char maxVersion[32] = {0};
        sprintf(sqlStr, DEF_PKGINFO_SELECT_PKGVERSION_SQL_FORMAT, pkgType);
        iRet = sqlite3_get_table(SUECDBHandle, sqlStr, &retTable,
            &nRow, &nColumn, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else
        {
            if (nRow > 0 && nColumn > 0)
            {
                strcpy(maxVersion, retTable[1 * nColumn]);
            }
            sqlite3_free_table(retTable);
        }

        retTable = NULL;
        nRow = 0;
        nColumn = 0;
        memset(sqlStr, 0, sizeof(sqlStr));
        sprintf(sqlStr, DEF_PKGSCENE_SELECT_PKGVERSION_SQL_FORMAT, pkgType);
        iRet = sqlite3_get_table(SUECDBHandle, sqlStr, &retTable,
            &nRow, &nColumn, &dbErrMsg);
        if (iRet != SQLITE_OK)
        {
            sqlite3_free(dbErrMsg);
        }
        else
        {
            if (nRow > 0 && nColumn > 0)
            {
                int i = 0;
                char curVersion[32] = { 0 };
                for (i = 0; i < nRow; i++)
                {
                    memset(curVersion, 0, sizeof(curVersion));
                    if (strlen(maxVersion) > 0) strcpy(curVersion, retTable[(i+1) * nColumn]);
                    else strcpy(maxVersion, retTable[(i + 1) * nColumn]);
                    if (strlen(maxVersion) > 0 && strlen(curVersion) > 0)
                    {
                        if (!CmpVersion(maxVersion, curVersion))
                        {
                            memset(maxVersion, 0, sizeof(maxVersion));
                            strcpy(maxVersion, curVersion);
                        }
                    }
                }
            }
            sqlite3_free_table(retTable);
        }
        if (strlen(maxVersion) > 0)
        {
            int len = strlen(maxVersion) + 1;
            *version = (char*)malloc(len);
            memset(*version, 0, len);
            strcpy(*version, maxVersion);
            bRet = true;
        }
    }
    return bRet;
}

// add column if not exist in table "PkgProcScene"
bool AddColumn2PkgProcScene(char* columnName, char* type)
{
	char sqlstr[256];

	snprintf(sqlstr, sizeof(sqlstr)-1, "ALTER TABLE " PKG_SCENE_TABLE_NAME " ADD COLUMN %s %s;", columnName, type);
	sqlstr[sizeof(sqlstr)-1] = '\0';
	do_sqlite(SUECDBHandle, sqlstr, NULL, NULL, NULL, 1);

	return true;
}