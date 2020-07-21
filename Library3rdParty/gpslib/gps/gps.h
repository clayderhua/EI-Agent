// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GPS_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GPS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GPS_EXPORTS
#define GPS_API extern "C" __declspec(dllexport)
#else
#define GPS_API __declspec(dllimport)
#endif

typedef void* GPS_HANDLE;

#define GPS_LIB_VER_MJ 1
#define GPS_LIB_VER_MR 101119


// 初始化整各api所用得到的handle, Baud Rate, 所得到的資源。
GPS_API int EWM_InitializeGps(GPS_HANDLE* pHandle);

// 釋放 handle 以及其內部所包含的所有資源
GPS_API int EWM_UnInitializeGps(GPS_HANDLE handle);

// 設定Port Number
GPS_API int EWM_SetComPortNum(GPS_HANDLE handle, 
    unsigned int nPortNum);

// 設定Baud Rate
GPS_API int EWM_SetComPortBaudRate(GPS_HANDLE handle, 
    unsigned int nBaudRate);


// 根據目前設定的 portNum 和 baudRate，產生一個COM handle
GPS_API int EWM_InitializeComPort(GPS_HANDLE handle);

// 釋放 COM handle
GPS_API int EWM_UnInitializeComPort(GPS_HANDLE handle);

// 啟動Parser的函式
GPS_API int EWM_StartGpsParser(GPS_HANDLE handle);

// 停止並且結束上述函數所啟動的 thread
GPS_API int EWM_StopGpsParser(GPS_HANDLE handle);

GPS_API int EWM_GetGpsUtcTime(GPS_HANDLE handle, double* dbUTC);

GPS_API int EWM_GetGpsDate(GPS_HANDLE handle, int* dbDate);

GPS_API int EWM_GetGpsLatitude(GPS_HANDLE handle, double* dbLati);

GPS_API int EWM_GetGpsLongitude(GPS_HANDLE handle, double* dbLong);

GPS_API int EWM_GetGpsFixQuailty(GPS_HANDLE handle, int* nFixQuality);

GPS_API int EWM_GetGpsNumOfSatellite(GPS_HANDLE handle, int* nSate);

GPS_API int EWM_GetGpsHDOP(GPS_HANDLE handle, double* dbHDOP);

GPS_API int EWM_GetGpsAltitude(GPS_HANDLE handle, double* dbAlti);

GPS_API int EWM_GetGpsSDKVersion(GPS_HANDLE handle, DWORD* major, DWORD* minor);


