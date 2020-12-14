/*
 * @Author: your name
 * @Date: 2020-11-25 15:46:36
 * @LastEditTime: 2020-11-25 16:34:49
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/data_packet.cpp
 */
#include "data_packet.h"
#include <boost/pool/singleton_pool.hpp>

typedef struct singleton_pool_tag{}singleton_pool_tag;
   typedef boost::singleton_pool<singleton_pool_tag,sizeof(data_packet),boost::default_user_allocator_new_delete,boost::details::pool::default_mutex,1000>  global;


void DataPacketFree(data_packet* pa)
{
    global::free(pa);
}

data_packet::~data_packet()
{
}

std::shared_ptr<data_packet> InitDataPacket(uint8_t *data_, size_t length_, uint64_t received_time_ms_)
{
    data_packet *pPacket=(data_packet*)global::malloc();
    memcpy(pPacket->data, data_, length_);
    pPacket->length = length_;
    pPacket->received_time_ms = received_time_ms_;
    return std::shared_ptr<data_packet>(pPacket,DataPacketFree);
}

std::shared_ptr<data_packet> InitDataPacket(uint8_t *data_, size_t length_)
{
    data_packet *pPacket=(data_packet*)global::malloc();
    memcpy(pPacket->data, data_, length_);
    pPacket->length = length_;
    pPacket->received_time_ms = -1;
    return std::shared_ptr<data_packet>(pPacket,DataPacketFree);
}

std::shared_ptr<data_packet> InitDataPacket(std::shared_ptr<data_packet> packet)
{
    data_packet *pPacket=(data_packet*)global::malloc();
    memcpy(pPacket->data, packet->data, pPacket->length);
    pPacket->length = packet->length;
    pPacket->received_time_ms = packet->received_time_ms;
    return  std::shared_ptr<data_packet>(pPacket,DataPacketFree);
}

std::shared_ptr<data_packet> InitDataPacket()
{
    data_packet *pPacket=(data_packet*)global::malloc();
    return  std::shared_ptr<data_packet>(pPacket,DataPacketFree);
}

