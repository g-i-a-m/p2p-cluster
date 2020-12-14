/*
 * @Author: your name
 * @Date: 2020-11-25 15:46:46
 * @LastEditTime: 2020-11-25 16:15:08
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/data_packet.h
 */
#include <vector>
#include <string.h>
#include <memory>

class data_packet {
public:
  data_packet() = default;

  ~data_packet();

  uint8_t data[1500];//limit by mtu
  size_t length;
  uint64_t received_time_ms;
};

std::shared_ptr<data_packet> InitDataPacket(uint8_t *data_, size_t length_, uint64_t received_time_ms_);
std::shared_ptr<data_packet> InitDataPacket(uint8_t *data_, size_t length_);
std::shared_ptr<data_packet> InitDataPacket(std::shared_ptr<data_packet> packet);
std::shared_ptr<data_packet> InitDataPacket();
