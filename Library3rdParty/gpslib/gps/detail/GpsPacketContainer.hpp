#ifndef _GPS_PACKET_CONTAINER_HPP_
#define _GPS_PACKET_CONTAINER_HPP_

// �w�q��
#define MAX_QUEUE_SIZE_INSIDE_CONTAINER 5

#include <detail/GpsPacket.hpp>

#include <deque>

struct GpsPacketContainer {
    // std::deque<T> �O C++ �зǼ˪O�禡�w���� Queue �e�� 
	std::deque<GGAData> GGADataQueue;
	std::deque<GSVData> GSVDataQueue;
	std::deque<GLLData> GLLDataQueue;
	std::deque<VTGData> VTGDataQueue;
	std::deque<GSAData> GSADataQueue;
	std::deque<RMCData> RMCDataQueue;
};


#endif
