#ifndef _COM_PORT_HPP_
#define _COM_PORT_HPP_

// 可以直接在 VS 專案裡面設定編譯環境的搜尋路徑
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

