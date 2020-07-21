#ifndef _COM_PORT_HPP_
#define _COM_PORT_HPP_

// �i�H�����b VS �M�׸̭��]�w�sĶ���Ҫ��j�M���|
#include <detail/GpsBuffer.hpp>

int retrieveComPort(HANDLE hPort, GpsBuffer& buffer,
    unsigned int readingCount);

int InitComPort(HANDLE port, 
    unsigned int nPortNum, volatile unsigned int nBaudRate);

/****************************************************
// TODO: refine the interface of this function
int getGPSComPortNum();
****************************************************/

#endif

