#ifndef _GPS_PACKET_RETRIEVER_HPP_
#define _GPS_PACKET_RETRIEVER_HPP_

#include <detail/GpsPacket.hpp>

bool GGAPacketRetriever(char *GGAStr, PGGAData GGAReal);

bool GSVPacketRetriever(char *GSVStr, PGSVData GSVReal);

bool GLLPacketRetriever(char *GLLStr, PGLLData GLLReal);

bool VTGPacketRetriever(char *VTGStr, PVTGData VTGReal);

bool GSAPacketRetriever(char *GSAStr, PGSAData GSAReal);

bool RMCPacketRetriever(char *RMCStr, PRMCData RMCReal);

#endif