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
	
	// �u�� container �~�i��P�ɳQ���P�� thread �s��
	// �ҥH�u�����ݭn mutex �O�@
	// ( buffer �]���L�����ΩҥH���ݭn mutex )
    HANDLE containerMutexHandle;
	HANDLE parsingThreadHandle;
    volatile bool threadingStopFlag;
	volatile unsigned int comPortReadCount;
};

int lockMutex(HANDLE containerMutexHandle); 

int releaseMutex(HANDLE containerMutexHandle);

#endif