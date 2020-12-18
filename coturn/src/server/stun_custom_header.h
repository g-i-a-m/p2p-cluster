#ifndef STUN_CUSTOM_HEADER_H_
#define STUN_CUSTOM_HEADER_H_
#include <string.h>
#include <arpa/inet.h>

//const static uint16_t stun_custom_header_identifier(htonl(0x8000));
#define STUN_CUSTOM_IDENTIFIER htons(0x8000)
#define STUN_CUSTOM_DATA_IDENTIFIER htons(0x8001)
#define DEFAULT_VALUE 0U
#define STUN_CUSTOM_HEADER_LEN  14U
#pragma pack(8)
union stun_custom_header {
//private:
    struct custom_header{
        uint16_t identifier : 16; //0x8000~0xffff为stun协议保留标识符
        uint32_t srcIp : 32;
        uint16_t srcPort : 16;
        uint32_t dstIp : 32;
        uint64_t dstPort : 16;
    } header;
    uint8_t buffer_[14];
};

#endif // STUN_CUSTOM_HEADER_H_
