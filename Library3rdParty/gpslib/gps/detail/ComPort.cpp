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

			// �b�������쪺��ƥ��ݥ[�W '\0' �H�K�M
			// ���e����L�ʥ]���ݾl��ưϹj�}�ӥH�K����v�T 
			newData[dwBytesRead] = '\0';

			// �⦬�쪺 char* ��ƶK�� buffer ������ 
			buffer.insert(buffer.size(), newData);	

			// �����w�g�S����F�@���j�� 
		
			--readingCount;
	}
	
	return 1;
}


// �N�O�o�Ө�ƪ� return value �ΨӧP�_�O�_���`����
// �M��u���� Num ��b parameter list ���ǰe ( �H ptr ���覡 )
