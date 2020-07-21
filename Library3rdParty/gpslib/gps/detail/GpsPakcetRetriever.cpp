//#include <forelib.h>
#include <stdafx.h>
#include <detail/GpsPacketRetriever.hpp>
#include <detail/CustomizedStrToken.h> // customizedStrtok
#include "debug.h"

//#include <iostream> // for debug only
#include <vector>
#include <cstdio> // sscanf

// 眖硂 token ﹃讽い耝 double 篈跑计戈 
// 狦 token 琌碞р hardcoded 箇砞肚倒ㄏノ 
void _getDouble(char* token, double* data, double hardcoded = -1.0)
{
	if (token != NULL)
	{
        sscanf(token, "%lf", data);
	}
	else 
	{
        *data = hardcoded;
	}
}

// 籔ㄧ计筽 
void _getInt(char* token, int* data, int hardcoded = -1)
{
	if (token != NULL) 
	{
        sscanf(token, "%d", data);
	}
	else 
	{
        *data = hardcoded;
	}
}

// 籔ㄧ计筽 
void _getChar(char* token, char* data, char hardcoded = '@')
{
	if (token != NULL)
	{
        sscanf(token, "%c", data);
	}
	else
	{
        *data = hardcoded;
	}
}

bool GGAPacketRetriever(char *GGAStr, PGGAData GGAReal)
{
    // 硂ノㄓ┮Τ token ぇ癬﹍竚
    // 琌ノㄓ夹 vector 
	std::vector<char*> tokens;
    char* startParsingPos = GGAStr;
    // 硂琌 debug ノ
    // 種琌俱璶砆だ猂﹃ 
	//std::cout << startParsingPos << std::endl;
    char* tokenStartPos;

    // ち┮Τ token  vector 讽い 
    do
	{
        // 硂ㄧ计笲叫把σ StrToken.h & StrToken.cpp 
        customizedStrtok(&startParsingPos, &tokenStartPos, ",*\r\n");
        tokens.push_back(tokenStartPos);
    } while(startParsingPos != NULL);

    // 狦硂ぇ token 计ヘ钵盽杠 ... 
	if (tokens.size() < 10)
	{
        return false;
	}

	// std::cout << "sizeof = " << tokens.size() << "  ...... 10" << std::endl;

    // рΑい–逆戈常耝ㄓ 
	_getDouble(tokens[0], &(GGAReal->dbGGAUTC));
	_getDouble(tokens[1], &(GGAReal->dbGGALatitude));
	_getChar(tokens[2], &(GGAReal->cGGALatitudeUnit));
	_getDouble(tokens[3], &(GGAReal->dbGGALongitude));
	_getChar(tokens[4], &(GGAReal->cGGALongitudeUnit));
	_getInt(tokens[5], &(GGAReal->nGGAQuality));
	_getInt(tokens[6], &(GGAReal->nGGANumOfSatsInUse));
	_getDouble(tokens[7], &(GGAReal->dbGGAHDOP));
	_getDouble(tokens[8], &(GGAReal->dbGGAAltitude));
	_getChar(tokens[9], &(GGAReal->cGGAAltUnit));

    // ボ token 计ヘタ盽
    // 琌 available  
	return true;
}

// 籔ㄧ计筽 
bool GSVPacketRetriever(char *GSVStr, PGSVData GSVReal)
{ 
	std::vector<char*> tokens;
    char* startParsingPos = GSVStr;
	//std::cout << startParsingPos << std::endl;
    char* tokenStartPos;

    do
	{
        customizedStrtok(&startParsingPos, &tokenStartPos, ",*\r\n");
        tokens.push_back(tokenStartPos);
    } while(startParsingPos != NULL);
 	
	if (tokens.size() < 7)
	{
        return false;
	}

	// std::cout << "sizeof = " << tokens.size() << "  ...... 19" << std::endl;
	_getInt(tokens[0], &(GSVReal->nGSVNumOfMessage)); 
	_getInt(tokens[1], &(GSVReal->nGSVMessageNum)); 
	_getInt(tokens[2], &(GSVReal->nGSVNumOfSatInView)); 

	for (int i  = 0;  i < (tokens.size() - 6) / 4; ++i) 
	{
        GSVSatData GSVSatInfo;

        _getInt(tokens[3 + i * 4], &(GSVSatInfo.nSatPRN));
        _getInt(tokens[3 + i * 4 + 1], &(GSVSatInfo.nElevation));
        _getInt(tokens[3 + i * 4 + 2], &(GSVSatInfo.nAzimuth));
        _getInt(tokens[3 + i * 4 + 3], &(GSVSatInfo.nSNR));

        GSVReal->GSVSatInfo.push_back(GSVSatInfo);
	}

	return true;
}

// 籔ㄧ计筽
bool GLLPacketRetriever(char *GLLStr, PGLLData GLLReal)
{
	std::vector<char*> tokens;
    char* startParsingPos = GLLStr;
	//std::cout << startParsingPos << std::endl;
    char* tokenStartPos;

    do{
        customizedStrtok(&startParsingPos, &tokenStartPos, ",*\r\n");
		tokens.push_back(tokenStartPos);
    } while(startParsingPos != NULL);
	
	if (tokens.size() < 6)
	{
        return false;
	}

	//std::cout << "sizeof = " << tokens.size() << "  ...... 6" << std::endl;
	_getDouble(tokens[0], &(GLLReal->dbGLLLatitude));
	_getChar(tokens[1], &(GLLReal->cGLLLatitudeUnit));
	_getDouble(tokens[2], &(GLLReal->dbGLLLongitude));
	_getChar(tokens[3], &(GLLReal->cGLLLongitudeUnit));
	_getDouble(tokens[4], &(GLLReal->dbGLLUTC));
	_getChar(tokens[5], &(GLLReal->cGLLValidStatus));

	return true;
}

// 籔ㄧ计筽
bool VTGPacketRetriever(char *VTGStr, PVTGData VTGReal)
{
	std::vector<char*> tokens;
    char* startParsingPos = VTGStr;
	//std::cout << startParsingPos << std::endl;
    char* tokenStartPos;

    do
	{
        customizedStrtok(&startParsingPos, &tokenStartPos, ",*\r\n");
        tokens.push_back(tokenStartPos);
    } while(startParsingPos != NULL);
  
	if (tokens.size() < 8)
	{
        return false;
	}

	//std::cout << "sizeof = " << tokens.size() << "  ...... 8" << std::endl;
	_getDouble(tokens[0], &(VTGReal->dbTrackModeGood));
	_getChar(tokens[1], &(VTGReal->cDegreeTure));
	_getDouble(tokens[2], &(VTGReal->dbMagneticModeGood));
	_getChar(tokens[3], &(VTGReal->cDegreeMagnetic));
	_getDouble(tokens[4], &(VTGReal->dbGroundSpeedKnot));
	_getChar(tokens[5], &(VTGReal->cGSKnot));
	_getDouble(tokens[6], &(VTGReal->dbGroundSpeedKilo));
	_getChar(tokens[7], &(VTGReal->cGSKilo));

	return true;
}

// 籔ㄧ计筽
bool GSAPacketRetriever(char *GSAStr, PGSAData GSAReal)
{
	std::vector<char*> tokens;
    char* startParsingPos = GSAStr;
	//std::cout << startParsingPos << std::endl;
    char* tokenStartPos;

    do
	{
        customizedStrtok(&startParsingPos, &tokenStartPos, ",*\r\n");
        tokens.push_back(tokenStartPos);
    } while(startParsingPos != NULL);
  
	if (tokens.size() < 9)
	{
        return false;
	}

	//std::cout << "sizeof = " << tokens.size() << "  ...... 17" << std::endl; 
	_getChar(tokens[0], &(GSAReal->cGSAAutoControl));
	_getInt(tokens[1], &(GSAReal->nGSAUsedMode));
    

	int i = 0;
	for (; i < tokens.size() - 8; ++i) 
	{
		int temp;

	    _getInt(tokens[2 + i], &temp);
		GSAReal->nGSASateInUsed.push_back(temp);
	}

    _getDouble(tokens[2 + i], &(GSAReal->dbGSAPDOP));
    _getDouble(tokens[2 + i + 1], &(GSAReal->dbGSAHDOP));
    _getDouble(tokens[2 + i + 2], &(GSAReal->dbGSAVDOP));
    
	return true;
}

// 籔ㄧ计筽
bool RMCPacketRetriever(char *RMCStr, PRMCData RMCReal)
{
	std::vector<char*> tokens;
    char* startParsingPos = RMCStr;
	//std::cout << startParsingPos << std::endl;
    char* tokenStartPos;

    do
	{
        customizedStrtok(&startParsingPos, &tokenStartPos, ",*\r\n");
        tokens.push_back(tokenStartPos);
    } while(startParsingPos != NULL);
 
	//std::cout << "sizeof = " << tokens.size() << "  ...... 11" << std::endl;
	if (tokens.size() < 11)
	{
        return false;
	}

	_getDouble(tokens[0], &(RMCReal->dbRMCUTC));
	_getChar(tokens[1], &(RMCReal->cRMCSatus));
	_getDouble(tokens[2], &(RMCReal->dbRMCLatitude));
	_getChar(tokens[3], &(RMCReal->cRMCLatitudeUnit));
	_getDouble(tokens[4], &(RMCReal->dbRMCLongitude));
	_getChar(tokens[5],  &(RMCReal->cRMCLongitudeUnit));
	_getDouble(tokens[6], &(RMCReal->dbRMCSpeedOverGround));
	_getDouble(tokens[7], &(RMCReal->dbRMCCourseOverGround));
	_getInt(tokens[8], &(RMCReal->nRMCDate));
	_getDouble(tokens[9], &(RMCReal->dbRMCMSLAltitude));
	_getChar(tokens[10], &(RMCReal->cRMCMagneticVarUnit));

	return true;
}
