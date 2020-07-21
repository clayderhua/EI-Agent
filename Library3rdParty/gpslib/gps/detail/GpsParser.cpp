#include <stdafx.h>

#include <detail/GpsParser.hpp>
#include <detail/CustomizedStrToken.h>
#include <detail/GpsPacketRetriever.hpp>
#include <detail/GpsMutexControl.hpp>
#include <detail/ComPort.hpp>
#include "debug.h"


#include <cstdlib> // strcmp

#define maxComPortBufferSize 10000


// 把 buffer 內部所含的所有封包資料
// 依照不同的格式加以分析判斷
// 並且把分析結果擷取出來放到 container 裡面 
// 
// 另外要注意的是
// 這個函數假設每個傳入的 buffer 都只是一個以 '\n' 結尾的
// 單一完整封包資料 
// 不需要考慮多個 (或多種) 封包混雜在一起的狀況 

int _parseSinglePacket(GpsBuffer& buffer, GpsPacketContainer& container,
    HANDLE containerMutexHandle)
{	
	char tokenTemp[maxComPortBufferSize];
   
    buffer.copy(tokenTemp, maxComPortBufferSize, 0);
    // 先捨棄最前面的所有連續性的前置 ',' 符號再進行後續處理 
    char* token = strtok(tokenTemp, ",");

	odprintf("_parseSinglePacket");
    // 如果發現是 '$GPGGA' 開頭的字串 
	if(strcmp(token, "$GPGGA") == 0)
	{
        // 準備一個用來存放分析結果的變數 
        GGAData tempGGAData;
        
        // 呼叫 _parseGGA 分析接下來的字串, 並且把分析結果放在 tempGGAData 
        // 如果分析出來的封包是合法的話 ( 根據 bool 回傳值判斷 ) 
        // 就把它放到 container 裡面對應的 queue 的最後面 
        // ( 最新的資料放在 queue 的最後面 )
		if(GGAPacketRetriever(tokenTemp + strlen(token) + 1, &tempGGAData))
		{
		    if(lockMutex(containerMutexHandle) == 1)
			{
     		    container.GGADataQueue.push_back(tempGGAData);
				if (container.GGADataQueue.size() > 
                    MAX_QUEUE_SIZE_INSIDE_CONTAINER) {
					container.GGADataQueue.pop_front();
                }				
				releaseMutex(containerMutexHandle);
			}
			else {
			    return false;
			}
		}
	}
    // 跟上一個 if 的判斷式雷同 
	else if(strcmp(token, "$GPGLL") == 0)
	{
        GLLData tempGLLData;

		if(GLLPacketRetriever(tokenTemp + strlen(token) + 1, &tempGLLData))
		{
		    if(lockMutex(containerMutexHandle) == 1)
			{
     		    container.GLLDataQueue.push_back(tempGLLData);
				if (container.GLLDataQueue.size() > 
                    MAX_QUEUE_SIZE_INSIDE_CONTAINER) 
				{
					container.GLLDataQueue.pop_front();
                }								
				releaseMutex(containerMutexHandle);
			}
			else {
			    return false;
			}		    	     
		}
	}
    // 跟上一個 if 的判斷式雷同
	else if(strcmp(token, "$GPGSA") == 0)
	{
		GSAData tempGSAData;

		if (GSAPacketRetriever(tokenTemp + strlen(token) + 1, &tempGSAData))
		{
		    if(lockMutex(containerMutexHandle) == 1)
			{
     		    container.GSADataQueue.push_back(tempGSAData);
                if (container.GSADataQueue.size() > 
                    MAX_QUEUE_SIZE_INSIDE_CONTAINER) 
				{
					container.GSADataQueue.pop_front();
                }				
				releaseMutex(containerMutexHandle);
			}
			else {
			    return false;
			}		    	     
		}
	}
    // 跟上一個 if 的判斷式雷同
	else if(strcmp(token, "$GPGSV") == 0)
	{
		GSVData tempGSVData;
		if(GSVPacketRetriever(tokenTemp + strlen(token) + 1, &tempGSVData))
		{
		    if(lockMutex(containerMutexHandle) == 1)
			{
     		    container.GSVDataQueue.push_back(tempGSVData);
				if (container.GSVDataQueue.size() > 
                    MAX_QUEUE_SIZE_INSIDE_CONTAINER)
				{
					container.GSVDataQueue.pop_front();
                }				
				releaseMutex(containerMutexHandle);
			}
			else {
			    return false;
			}		    	     		            
		}
	}
    // 跟上一個 if 的判斷式雷同
	else if(strcmp(token, "$GPRMC") == 0)
    {
		RMCData tempRMCData;
		if (RMCPacketRetriever(tokenTemp + strlen(token) + 1, &tempRMCData))
		{
		    if(lockMutex(containerMutexHandle) == 1)
			{
     		    container.RMCDataQueue.push_back(tempRMCData);
				if (container.RMCDataQueue.size() > 
                    MAX_QUEUE_SIZE_INSIDE_CONTAINER) 
				{
					container.RMCDataQueue.pop_front();
                }				
				releaseMutex(containerMutexHandle);
			}
			else {
			    return false;
			}		    	     
		}
	}
    // 
	else if(strcmp(token, "$GPVTG") == 0)
	{
		VTGData tempVTGData;
		if (VTGPacketRetriever(tokenTemp + strlen(token) + 1, &tempVTGData))
		{
		    if(lockMutex(containerMutexHandle) == 1)
			{
     		    container.VTGDataQueue.push_back(tempVTGData);
				if (container.VTGDataQueue.size() > 
                    MAX_QUEUE_SIZE_INSIDE_CONTAINER)
				{
					container.VTGDataQueue.pop_front();
                }				
				releaseMutex(containerMutexHandle);
			}
			else {
			    return false;
			}		    	     		
		}
	}
	else {
	    // 其實如果執行到這裡,
        // 表示嘗試分析一個不完全或無法辨識的封包資料
		// 應該有其對應的回傳值  不過也許不是 GPS_FAIL
	    return false;		
	};
	return 1;
}


// 分析所有在 Buffer 裡面的所有完整的封包型態資料 
// 並且把結果放在 ReceivedMessages 裡面 
int _parseNMEA(GpsBuffer& buffer, GpsPacketContainer& container,
    HANDLE containerMutexHandle) 
{
    // 每次要分析的完整封包資料的結尾位置 
	int sentenceLastCharPosition;
	int nCount = 0;
	// 存放單一個整封包資料的暫存變數 
	GpsBuffer singlePacketBuffer;

    // 如果還找的到 '\n' 就表示還有完整的封包資料可以分析  ...
    // 一直分析到完全處理完為止
    // TODO: 是因為 \r 會在 \n 之前出現，所以只找\n
	while ((sentenceLastCharPosition = buffer.find("\n")) != std::string::npos)
	{
	    unsigned int subStrLength = sentenceLastCharPosition + 1;
		++nCount;

		// 把目前要處理的完整封包 copy 出來
		singlePacketBuffer = buffer.substr(0, subStrLength);
		//printf("\nfind GPS sentence ... %s\n", StrBuffer.c_str());
    
        // 呼叫分析函數自動化地處理這個完整封包資料
        // 並且把結果放在 receivedMessages 
		if (_parseSinglePacket(singlePacketBuffer, container, containerMutexHandle) 
		    == false) 
		{			
		    return false;
		}

        // 從所有處理得資料堆當中除去
		buffer = buffer.erase(0, subStrLength);
	}
	return 1;
}

DWORD WINAPI parsingThreadFunction(LPVOID arg)
{	
    ParsingParameter* param = (ParsingParameter*)arg;

	// 錯誤檢查
	if (param->hPort == NULL)
	{
	    return false;
	}

	// 先取得相對比較短的變數名稱
    HANDLE& containerMutexHandle = param->containerMutexHandle;	
    HANDLE& hPort = *(param->hPort);
	GpsBuffer& buffer = *(param->buffer);
    GpsPacketContainer& container = *(param->container);
	volatile bool* threadingStopFlag = param->threadingStopFlag;
	volatile unsigned int* comPortReadCount
	    = param->comPortReadCount;
		
	// 除非外部通借由 threadingStopFlag 通知此 thread
	// 該結束目前的工作, 否則他將持續不斷的工作

	while (!(*threadingStopFlag))
	{
        // 從 comPort 取得資料放入 buffer
		
        retrieveComPort(hPort, buffer, *comPortReadCount);
		odprintf("%s",buffer.c_str());
		// 把 buffer 的資料取出, 加以分析之後放入 container
        _parseNMEA(buffer, container, containerMutexHandle);
		
	}
	
	delete param;
	
	return 1;
}
