// gps.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "gps.h"
#include "debug.h"
#include <detail/GpsResource.hpp>
#include <detail/GpsParser.hpp>
#include <detail/GpsMutexControl.hpp>

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <iostream>

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

HANDLE hPort;
 
GPS_API int EWM_InitializeGps(GPS_HANDLE* pHandle)
{
	GpsResource* pGpsResource = new GpsResource();

	if (pGpsResource == NULL) 
	{
	    return false;
    }

	// COM Port的初始化
    pGpsResource->portNum = 8;
    pGpsResource->baudRate = 9600;
	pGpsResource->hPort = NULL;

	// Mutex 和 Thread的Handle初始化
    pGpsResource->containerMutexHandle = CreateMutex(NULL, FALSE, NULL);
    pGpsResource->parsingThreadHandle = NULL;

	// 這個 flag 設定為 true 表示 threading 停止運作
    pGpsResource->threadingStopFlag = true;

	// 抓取幾次資料後才去parsing
    pGpsResource->comPortReadCount = 5;

    *pHandle = (GPS_HANDLE)(pGpsResource);

	return 1;
}

GPS_API int EWM_UnInitializeGps(GPS_HANDLE handle)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);    
    // TODO: refine the following code 
	//       use mutex ?

	if (pGpsResource->hPort != NULL) 
	{
        EWM_UnInitializeComPort(handle);
		// not check the reture code 
	}

	if (pGpsResource->parsingThreadHandle != NULL) 
	{
        EWM_StopGpsParser(handle);
		// not check the reture code 		
	}
	

	if (pGpsResource->containerMutexHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(pGpsResource->containerMutexHandle);
	}

	if (pGpsResource != NULL)
	{
		odprintf("pGpsResource");
		delete pGpsResource;
	}

	return 1;
}

GPS_API int EWM_SetComPortNum(GPS_HANDLE handle, 
    unsigned int nPortNum)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	pGpsResource->portNum = nPortNum;

	return 1;
}

GPS_API int EWM_SetComPortBaudRate(GPS_HANDLE handle, 
    unsigned int nBaudRate)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	pGpsResource->baudRate = nBaudRate;	

    return 1;
}


GPS_API int EWM_InitializeComPort(GPS_HANDLE handle)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);  
	TCHAR sPort[100];

	int nPortNum = pGpsResource->portNum;
	if (nPortNum < 0)
	{
		return false;
	}

	wsprintf(sPort, TEXT("\\\\.\\COM%d"), nPortNum);
	odprintf("Initialize COM port :COM%d",nPortNum);
	hPort = CreateFile (
			sPort,                            // Pointer to the name of the port
			GENERIC_READ | GENERIC_WRITE,     // Access (read-write) mode                              
			0,                                // Share mode
			NULL,                             // Pointer to the security attribute
			OPEN_EXISTING,                    // How to open the serial port
			0,             // Port attributes
			NULL);                            // Handle to port with attribute to copy
    pGpsResource->hPort = hPort;

	if (hPort == INVALID_HANDLE_VALUE)
	{
		odprintf("hPort fail");
		return false;
	}

	DCB dcb;

	if (!GetCommState(hPort, &dcb))
	{
		odprintf("GetCommState fail");
		return false;
	}

	// Set baud rate
	int nBaudRate = pGpsResource->baudRate;
	dcb.BaudRate = nBaudRate;

	// Set data byte
	dcb.ByteSize = 8;

	// Set parity bit
	dcb.Parity = NOPARITY;

	// Set stop bit
	dcb.StopBits = ONESTOPBIT;

	// No flow control
	dcb.fDsrSensitivity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

	if(!SetCommState(hPort, &dcb))
	{
		odprintf("SetCommState fail");
		return false;
	}

    COMMTIMEOUTS Timeouts;
	GetCommTimeouts(hPort, &Timeouts);

    Timeouts.ReadIntervalTimeout = 100;
    Timeouts.ReadTotalTimeoutMultiplier = 1;
    Timeouts.ReadTotalTimeoutConstant = 0;
    Timeouts.WriteTotalTimeoutMultiplier = 10;
    Timeouts.WriteTotalTimeoutConstant = 1000;

    SetCommTimeouts(hPort, &Timeouts);
	return true;
}


GPS_API int EWM_UnInitializeComPort(GPS_HANDLE handle)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);

	if(!CloseHandle(pGpsResource->hPort))
	{
		return false;
	}

	return true;
}

GPS_API int EWM_StartGpsParser(GPS_HANDLE handle)
{
    GpsResource* pGpsResource = (GpsResource*)(handle);  

	// 檢查是否有parsingThread在執行
	if (pGpsResource->parsingThreadHandle != NULL)
	{
	    return false;
    }

    // 設定stop flag去允許Parser
    pGpsResource->threadingStopFlag = false;	

    DWORD threadIdTemp;
   
	//
    ParsingParameter* pParsingParameter = new ParsingParameter();
	// this instance will be deleted before the thread ends

	pParsingParameter->containerMutexHandle = pGpsResource->containerMutexHandle;
	pParsingParameter->hPort = &(pGpsResource->hPort);
	pParsingParameter->buffer = &(pGpsResource->buffer);
	pParsingParameter->container = &(pGpsResource->container);
	pParsingParameter->threadingStopFlag = &(pGpsResource->threadingStopFlag);
	pParsingParameter->comPortReadCount = &(pGpsResource->comPortReadCount); 

    pGpsResource->parsingThreadHandle = CreateThread(NULL, 0, 
	    (LPTHREAD_START_ROUTINE)parsingThreadFunction,
		pParsingParameter, 0, &threadIdTemp);

	if (pGpsResource->parsingThreadHandle == NULL)
	{
		return false;
	}
	
    return true;
}


GPS_API int EWM_StopGpsParser(GPS_HANDLE handle)
{
	 GpsResource* pGpsResource = (GpsResource*)(handle);

	// 檢查是否有parsingThread在執行 
	if (pGpsResource->parsingThreadHandle == NULL) {
	    return false;
    }
 
    pGpsResource->threadingStopFlag = true; 

	// 回收 threading handle
    if (WaitForSingleObject(pGpsResource->parsingThreadHandle , INFINITE) ==
        WAIT_FAILED) 
	{
	    return false;
	}

	CloseHandle(pGpsResource->parsingThreadHandle);
    pGpsResource->parsingThreadHandle = NULL;

	return true;
}


GPS_API int EWM_GetGpsUtcTime(GPS_HANDLE handle, double* dbUTC)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	HANDLE hMutex = pGpsResource->containerMutexHandle;
	//dbUTC;

	lockMutex(hMutex);
	std::deque<GGAData>& queue = pGpsResource->container.GGADataQueue;
	if (queue.size() > 0)
	{
        *dbUTC = queue[queue.size() - 1].dbGGAUTC;
	}
	releaseMutex(hMutex);

	return 1;
}


GPS_API int EWM_GetGpsDate(GPS_HANDLE handle, int* dbDate)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	HANDLE hMutex = pGpsResource->containerMutexHandle;

	lockMutex(hMutex);

	std::deque<RMCData>& queue = pGpsResource->container.RMCDataQueue;
	if (queue.size() > 0)
	{
        *dbDate = queue[queue.size() - 1].nRMCDate;
	}

	releaseMutex(hMutex);

	return 1;
}


GPS_API int EWM_GetGpsLatitude(GPS_HANDLE handle, double* dbLati)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	HANDLE hMutex = pGpsResource->containerMutexHandle;
	//dbUTC;

	lockMutex(hMutex);
	std::deque<GGAData>& queue = pGpsResource->container.GGADataQueue;
	if (queue.size() > 0)
	{
		if (queue[queue.size() - 1].cGGALatitudeUnit == 'N')
		{
			double Latitude = (queue[queue.size() - 1].dbGGALatitude)/100;
			int x = (int)(Latitude);
			Latitude = (double)(x) + ((Latitude - (double)(x))/60*100);
			*dbLati = Latitude;
		} 
		else 
		{
			double Latitude = (queue[queue.size() - 1].dbGGALatitude)/100;
			int x = (int)(Latitude);
			Latitude = (double)(x) + ((Latitude - (double)(x))/60*100);
			*dbLati = -Latitude;
		}
	}
	releaseMutex(hMutex);

	return 1;
}

GPS_API int EWM_GetGpsLongitude(GPS_HANDLE handle, double* dbLongi)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	HANDLE hMutex = pGpsResource->containerMutexHandle;
	//dbUTC;

	lockMutex(hMutex);
	std::deque<GGAData>& queue = pGpsResource->container.GGADataQueue;

	if (queue.size() > 0)
	{
		if(queue[queue.size() - 1].cGGALongitudeUnit == 'E')
		{
			double Longitude = (queue[queue.size() - 1].dbGGALongitude)/100;
			int x = (int)(Longitude);
			Longitude = (double)(x) + ((Longitude - (double)(x))/60*100);
			*dbLongi = Longitude;
		} else 
		{
			double Longitude = (queue[queue.size() - 1].dbGGALongitude)/100;
			int x = (int)(Longitude);
			Longitude = (double)(x) + ((Longitude - (double)(x))/60*100);
			*dbLongi = -Longitude;
		}
	}
	releaseMutex(hMutex);

	return 1;
}

GPS_API int EWM_GetGpsFixQuailty(GPS_HANDLE handle, int* nFixQuality)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	HANDLE hMutex = pGpsResource->containerMutexHandle;
	//dbUTC;

	lockMutex(hMutex);
	std::deque<GGAData>& queue = pGpsResource->container.GGADataQueue;
	if (queue.size() > 0)
	{
        *nFixQuality = queue[queue.size() - 1].nGGAQuality;
	}

	releaseMutex(hMutex);
	return 1;
}

GPS_API int EWM_GetGpsNumOfSatellite(GPS_HANDLE handle, int* nSate)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	HANDLE hMutex = pGpsResource->containerMutexHandle;
	//dbUTC;

	lockMutex(hMutex);
	std::deque<GGAData>& queue = pGpsResource->container.GGADataQueue;
	if (queue.size() > 0)
	{
        *nSate = queue[queue.size() - 1].nGGANumOfSatsInUse;
	}

	releaseMutex(hMutex);

	return 1;
}

GPS_API int EWM_GetGpsHDOP(GPS_HANDLE handle, double* dbHDOP)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	HANDLE hMutex = pGpsResource->containerMutexHandle;

	lockMutex(hMutex);

	std::deque<GGAData>& queue = pGpsResource->container.GGADataQueue;
	if (queue.size() > 0)
	{
        *dbHDOP = queue[queue.size() - 1].dbGGAHDOP;
	}

	releaseMutex(hMutex);

	return 1;
}

GPS_API int EWM_GetGpsAltitude(GPS_HANDLE handle, double* dbAlti)
{
	GpsResource* pGpsResource = (GpsResource*)(handle);
	HANDLE hMutex = pGpsResource->containerMutexHandle;

	lockMutex(hMutex);

	std::deque<GGAData>& queue = pGpsResource->container.GGADataQueue;
	if (queue.size() > 0)
	{
        *dbAlti = queue[queue.size() - 1].dbGGAAltitude;
	}

	releaseMutex(hMutex);

	return 1;
}


GPS_API int EWM_GetGpsSDKVersion(GPS_HANDLE handle, DWORD* major, DWORD* minor)
{
	*major = GPS_LIB_VER_MJ;
	*minor = GPS_LIB_VER_MR;

	return 1;
}

