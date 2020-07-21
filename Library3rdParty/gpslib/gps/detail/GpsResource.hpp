#ifndef _GPS_RESOURCE_HPP_
#define _GPS_RESOURCE_HPP_

#include <detail/GpsBuffer.hpp>
#include <detail/GpsPacketContainer.hpp> // GpsPacketContainer

struct GpsResource {
    unsigned int portNum;
    unsigned int baudRate;
	HANDLE hPort;
    GpsBuffer buffer;
	GpsPacketContainer container;
	
	// 只有 container 才可能同時被不同的 thread 存取
	// 所以只有它需要 mutex 保護
	// ( buffer 因為無此情形所以不需要 mutex )
    HANDLE containerMutexHandle;
	HANDLE parsingThreadHandle;
    volatile bool threadingStopFlag;
	volatile unsigned int comPortReadCount;
};

int lockMutex(HANDLE containerMutexHandle); 

int releaseMutex(HANDLE containerMutexHandle);

#endif