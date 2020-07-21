#ifndef _MCAFEELIB_H_
#define _MCAFEELIB_H_

#ifdef MCAFEELIB_EXPORTS
#define MCAFEELIB_API __declspec(dllexport)
#else
#define MCAFEELIB_API __declspec(dllimport)
#endif

/*-----------------------------------------------------------------------------
//
//	Definition
//
//-----------------------------------------------------------------------------*/
typedef int Status_t;

#define MAXLENGTH         20
#define UPDATEMAXNUM      36

enum STATEMACHINE
{
	STATE_STOPPED,
	STATE_STATRED,
};

struct ScanResult
{
	int total_count;
	int infected_count;
};

struct ScanLog
{
	void* filename;
	char* scan_status;
	char* repair_status;
};

struct UpdateInfo
{
	int canUpdate;
	int numUpdateFile;
	char updateFileList[UPDATEMAXNUM][MAXLENGTH];
};

/*-----------------------------------------------------------------------------
//
//	Status codes
//
//-----------------------------------------------------------------------------*/
#define ADV_STATUS_SUCCESS					0
#define ADV_STATUS_ENGINE_INITIALIZED		1
#define ADV_STATUS_ENGINE_NOT_INITIALIZED	2
#define ADV_STATUS_INVALID_PARAMETER		3
#define ADV_STATUS_ERROR					4

/*-----------------------------------------------------------------------------
//
//	APIs
//
//-----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

MCAFEELIB_API Status_t WINAPI InitializeMcAfeeEngine(char* filepath);
MCAFEELIB_API Status_t WINAPI UnitializeMcAfeeEngine(bool bForce);
MCAFEELIB_API Status_t WINAPI ScanDrives(char* driverletters, int amount, ScanResult &result);
MCAFEELIB_API Status_t WINAPI GetCurrentVirusDefVersion(int &version);
MCAFEELIB_API Status_t WINAPI UpdateAutoInfo(char* updateDirectory, int currentVersion, UpdateInfo &info);
MCAFEELIB_API Status_t WINAPI UpdateAutoVirusDef(char* updateDirectory, int currentVersion);

typedef void (WINAPI * MessageCallback)(ScanLog log);
MCAFEELIB_API Status_t WINAPI SetCallback(MessageCallback messageCallback);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _MCAFEELIB_H_ */