#ifndef _GPS_MUTEX_CONTROL_HPP_
#define _GPS_MUTEX_CONTROL_HPP_

#include <windows.h> // HANDLE

int lockMutex(HANDLE handle); 

int releaseMutex(HANDLE handle);

#endif