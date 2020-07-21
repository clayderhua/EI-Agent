#ifndef _GPS_PACKET_CONTAINER_HPP_
#define _GPS_PACKET_CONTAINER_HPP_

// 定義給
#define MAX_QUEUE_SIZE_INSIDE_CONTAINER 5

#include <detail/GpsPacket.hpp>

#include <deque>

struct GpsPacketContainer {
    // std::deque<T> 是 C++ 標準樣板函式庫當中的 Queue 容器 
	std::deque<GGAData> GGADataQueue;
	std::deque<GSVData> GSVDataQueue;
	std::deque<GLLData> GLLDataQueue;
	std::deque<VTGData> VTGDataQueue;
	std::deque<GSAData> GSADataQueue;
	std::deque<RMCData> RMCDataQueue;
};


#endif
