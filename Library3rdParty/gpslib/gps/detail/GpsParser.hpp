#ifndef _GPS_PARSER_HPP_
#define _GPS_PARSER_HPP_

#include <detail/GpsBuffer.hpp>  // GpsBufferType
#include <detail/GpsPacketContainer.hpp> // GpsPacketContainer

//#include <windows.h> // DWORD, WINAPI, LPVOID, HANDLE

struct ParsingParameter {
    HANDLE containerMutexHandle;
    HANDLE* hPort;
    GpsBuffer* buffer;
	GpsPacketContainer* container;
	volatile bool* threadingStopFlag;
	volatile unsigned int* comPortReadCount;
};
        

DWORD WINAPI parsingThreadFunction(LPVOID arg);

#endif


