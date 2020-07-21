#include <stdafx.h>
#include "ComPort.hpp"

int retrieveComPort(HANDLE hPort, GpsBuffer& buffer,
    unsigned int readingCount)
{
	buffer.clear();

	DWORD dwCount = 500;  // TODO: define 500 as a macro ?
	DWORD dwComMask;
	DWORD dwBytesRead = 0;
	char newData[1000];   // TODO: define 1000 as a macro ?

	_COMMTIMEOUTS timeout = {MAXDWORD ,0,0,1,1};
    SetCommTimeouts(hPort, &timeout);

	while (readingCount > 0)
	{
		//SetCommMask(hPort, EV_RXCHAR | EV_BREAK);
		//WaitCommEvent(hPort, &dwComMask, NULL);
		Sleep(1);
	    ReadFile(
				hPort,			    // handle of file to read 
				newData,			// address of buffer that receives data  
				dwCount,			// number of bytes to read 
				&dwBytesRead,		// address of number of bytes read 
				NULL				// address of structure for data 
				); 

			// 在本次收到的資料末端加上 '\0' 以便和
			// 先前收到過封包之殘餘資料區隔開來以免受其影響 
			newData[dwBytesRead] = '\0';

			// 把收到的 char* 資料貼到 buffer 的尾端 
			buffer.insert(buffer.size(), newData);	

			// 紀錄已經又執行了一次迴圈 
		
			--readingCount;
	}
	
	return 1;
}


// 就是這個函數的 return value 用來判斷是否正常結束
// 然後真正的 Num 放在 parameter list 當中傳送 ( 以 ptr 的方式 )
