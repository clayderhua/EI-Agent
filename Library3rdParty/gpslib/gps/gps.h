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


// ��l�ƾ�Uapi�ҥαo�쪺handle, Baud Rate, �ұo�쪺�귽�C
GPS_API int EWM_InitializeGps(GPS_HANDLE* pHandle);

// ���� handle �H�Ψ䤺���ҥ]�t���Ҧ��귽
GPS_API int EWM_UnInitializeGps(GPS_HANDLE handle);

// �]�wPort Number
GPS_API int EWM_SetComPortNum(GPS_HANDLE handle, 
    unsigned int nPortNum);

// �]�wBaud Rate
GPS_API int EWM_SetComPortBaudRate(GPS_HANDLE handle, 
    unsigned int nBaudRate);


// �ھڥثe�]�w�� portNum �M baudRate�A���ͤ@��COM handle
GPS_API int EWM_InitializeComPort(GPS_HANDLE handle);

// ���� COM handle
GPS_API int EWM_UnInitializeComPort(GPS_HANDLE handle);

// �Ұ�Parser���禡
GPS_API int EWM_StartGpsParser(GPS_HANDLE handle);

// ����åB�����W�z��ƩұҰʪ� thread
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


