#include <stdafx.h>

#include <detail/GpsParser.hpp>
#include <detail/CustomizedStrToken.h>
#include <detail/GpsPacketRetriever.hpp>
#include <detail/GpsMutexControl.hpp>
#include <detail/ComPort.hpp>
#include "debug.h"


#include <cstdlib> // strcmp

#define maxComPortBufferSize 10000


// �� buffer �����ҧt���Ҧ��ʥ]���
// �̷Ӥ��P���榡�[�H���R�P�_
// �åB����R���G�^���X�ө�� container �̭� 
// 
// �t�~�n�`�N���O
// �o�Ө�ư��]�C�ӶǤJ�� buffer ���u�O�@�ӥH '\n' ������
// ��@����ʥ]��� 
// ���ݭn�Ҽ{�h�� (�Φh��) �ʥ]�V���b�@�_�����p 

int _parseSinglePacket(GpsBuffer& buffer, GpsPacketContainer& container,
    HANDLE containerMutexHandle)
{	
	char tokenTemp[maxComPortBufferSize];
   
    buffer.copy(tokenTemp, maxComPortBufferSize, 0);
    // ���˱�̫e�����Ҧ��s��ʪ��e�m ',' �Ÿ��A�i�����B�z 
    char* token = strtok(tokenTemp, ",");

	odprintf("_parseSinglePacket");
    // �p�G�o�{�O '$GPGGA' �}�Y���r�� 
	if(strcmp(token, "$GPGGA") == 0)
	{
        // �ǳƤ@�ӥΨӦs����R���G���ܼ� 
        GGAData tempGGAData;
        
        // �I�s _parseGGA ���R���U�Ӫ��r��, �åB����R���G��b tempGGAData 
        // �p�G���R�X�Ӫ��ʥ]�O�X�k���� ( �ھ� bool �^�ǭȧP�_ ) 
        // �N�⥦��� container �̭������� queue ���̫᭱ 
        // ( �̷s����Ʃ�b queue ���̫᭱ )
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
    // ��W�@�� if ���P�_���p�P 
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
    // ��W�@�� if ���P�_���p�P
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
    // ��W�@�� if ���P�_���p�P
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
    // ��W�@�� if ���P�_���p�P
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
	    // ���p�G�����o��,
        // ��ܹ��դ��R�@�Ӥ������εL�k���Ѫ��ʥ]���
		// ���Ӧ���������^�ǭ�  ���L�]�\���O GPS_FAIL
	    return false;		
	};
	return 1;
}


// ���R�Ҧ��b Buffer �̭����Ҧ����㪺�ʥ]���A��� 
// �åB�⵲�G��b ReceivedMessages �̭� 
int _parseNMEA(GpsBuffer& buffer, GpsPacketContainer& container,
    HANDLE containerMutexHandle) 
{
    // �C���n���R������ʥ]��ƪ�������m 
	int sentenceLastCharPosition;
	int nCount = 0;
	// �s���@�Ӿ�ʥ]��ƪ��Ȧs�ܼ� 
	GpsBuffer singlePacketBuffer;

    // �p�G�٧䪺�� '\n' �N����٦����㪺�ʥ]��ƥi�H���R  ...
    // �@�����R�짹���B�z������
    // TODO: �O�]�� \r �|�b \n ���e�X�{�A�ҥH�u��\n
	while ((sentenceLastCharPosition = buffer.find("\n")) != std::string::npos)
	{
	    unsigned int subStrLength = sentenceLastCharPosition + 1;
		++nCount;

		// ��ثe�n�B�z������ʥ] copy �X��
		singlePacketBuffer = buffer.substr(0, subStrLength);
		//printf("\nfind GPS sentence ... %s\n", StrBuffer.c_str());
    
        // �I�s���R��Ʀ۰ʤƦa�B�z�o�ӧ���ʥ]���
        // �åB�⵲�G��b receivedMessages 
		if (_parseSinglePacket(singlePacketBuffer, container, containerMutexHandle) 
		    == false) 
		{			
		    return false;
		}

        // �q�Ҧ��B�z�o��ư�����h
		buffer = buffer.erase(0, subStrLength);
	}
	return 1;
}

DWORD WINAPI parsingThreadFunction(LPVOID arg)
{	
    ParsingParameter* param = (ParsingParameter*)arg;

	// ���~�ˬd
	if (param->hPort == NULL)
	{
	    return false;
	}

	// �����o�۹����u���ܼƦW��
    HANDLE& containerMutexHandle = param->containerMutexHandle;	
    HANDLE& hPort = *(param->hPort);
	GpsBuffer& buffer = *(param->buffer);
    GpsPacketContainer& container = *(param->container);
	volatile bool* threadingStopFlag = param->threadingStopFlag;
	volatile unsigned int* comPortReadCount
	    = param->comPortReadCount;
		
	// ���D�~���q�ɥ� threadingStopFlag �q���� thread
	// �ӵ����ثe���u�@, �_�h�L�N�����_���u�@

	while (!(*threadingStopFlag))
	{
        // �q comPort ���o��Ʃ�J buffer
		
        retrieveComPort(hPort, buffer, *comPortReadCount);
		odprintf("%s",buffer.c_str());
		// �� buffer ����ƨ��X, �[�H���R�����J container
        _parseNMEA(buffer, container, containerMutexHandle);
		
	}
	
	delete param;
	
	return 1;
}
