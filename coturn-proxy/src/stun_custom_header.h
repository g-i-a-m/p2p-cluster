/*
 * @Author: your name
 * @Date: 2020-11-20 16:28:45
 * @LastEditTime: 2020-11-26 17:09:12
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/stun_custom_header.h
 */

#ifndef STUN_CUSTOM_HEADER_H_
#define STUN_CUSTOM_HEADER_H_
#include <string>
#include <arpa/inet.h>
#include <boost/algorithm/string.hpp>
#include "stun_proxy.h"

using namespace std;

#define STUN_CUSTOM_IDENTIFIER htons(0x8000)
#define STUN_CUSTOM_DATA_IDENTIFIER htons(0x8001)
#define STUN_CUSTOM_RELEASE_IDENTIFIER htons(0x8002)
#define DEFAULT_VALUE 0U
#define STUN_COSTOM_HEADER_LEN  14U

#pragma pack(8)

union stun_custom_header {
    stun_custom_header() : header(STUN_CUSTOM_IDENTIFIER, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE) {}
    void SetIdentifier(uint16_t identifier){ header.identifier=htons(identifier);}
    void SetSrcAddr(const char* ip, uint16_t port) { header.srcIp = htonl(ip2uint32(ip)); header.srcIp = htons(port); }
    void SetDstAddr(const char* ip, uint16_t port) { header.dstIp = htonl(ip2uint32(ip)); header.dstIp = htons(port); }
    void SetSrcAddr(const std::string& ip, uint16_t port) { header.srcIp = htonl(ip2uint32(ip.c_str())); header.srcIp = htons(port); }
    void SetDstAddr(const std::string& ip, uint16_t port) { header.dstIp = htonl(ip2uint32(ip.c_str())); header.dstIp = htons(port); }
    void SetSrcAddr(uint32_t ip, uint16_t port) { header.srcIp = htonl(ip); header.srcIp = htons(port); }
    void SetDstAddr(uint32_t ip, uint16_t port) { header.dstIp = htonl(ip); header.dstIp = htons(port); }

    static uint32_t ip2uint32(const char* ip) {
        struct in_addr s;
        inet_pton(AF_INET, ip, (void *)&s);
        return s.s_addr;
    }

    static std::string ip2string(uint32_t ip) {
        string szip("");
        in_addr inaddr;
        inaddr.s_addr = ip;
        szip = inet_ntoa(inaddr);
        return szip;
    }

    bool IsStunExtensionHeader() {
        if (header.identifier==STUN_CUSTOM_IDENTIFIER)
            return true;
        else
            return false;
    }

    bool IsStunExtensionRelayHeader() {
        if (header.identifier==STUN_CUSTOM_DATA_IDENTIFIER)
            return true;
        else
            return false;
    }
    bool IsStunExtensionReleaseHeader() {
        if (header.identifier==STUN_CUSTOM_RELEASE_IDENTIFIER)
            return true;
        else
            return false;
    }
    std::string GetSrcIp() {
        string szip("");
        if (header.srcIp!=0) {
            in_addr inaddr;
            inaddr.s_addr = ntohl(header.srcIp);
            szip = inet_ntoa(inaddr);
        }
        return szip;
    }

    uint32_t GetSrcIntIp() {
        return ntohl(header.srcIp);
    }

    uint16_t GetSrcPort() {
        return ntohs(header.srcPort);
    }

    std::string GetDstIp() {
        string szip("");
        if (header.dstIp!=0) {
            in_addr inaddr;
            inaddr.s_addr = ntohl(header.dstIp);
            szip = inet_ntoa(inaddr);
        }
        return szip;
    }

    uint32_t GetDstIntIp() {
        return ntohl(header.dstIp);
    }

    uint16_t GetDstPort() {
        return ntohs(header.dstPort);
    }

    uint8_t* GetData(){ return buffer_; }
    size_t GetLength() { return sizeof(buffer_); }

//private:
    struct custom_header{
        custom_header(uint16_t idf,uint32_t sip,uint16_t sport,uint32_t dip,uint16_t dport): 
            identifier(idf),srcIp(sip),srcPort(sport),dstIp(dip),dstPort(dport){}
        uint16_t identifier : 16; //0x8000~0xffff为stun协议保留标识符
        uint32_t srcIp : 32;
        uint16_t srcPort : 16;
        uint32_t dstIp : 32;
        uint64_t dstPort : 16;
    } header;

    uint8_t buffer_[14];
};

class stun_custom_packet {
public:
    static stun_custom_packet* CreateCustomPacket() {
        return nullptr;
    }
    
protected:

private:
    uint8_t buffer_[1500];
};

#endif // STUN_CUSTOM_HEADER_H_
