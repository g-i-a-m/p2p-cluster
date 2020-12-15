/*
 * @Author: ctf
 * @Date: 2020-11-06 10:43:41
 * @LastEditTime: 2020-11-26 16:38:50
 * @LastEditors: Please set LastEditors
 * @Description: parse stun protocol
 * @FilePath: /coturn-proxy/src/stun_msg_parser.h
 */
#ifndef STUN_MSG_PARSER_H_
#define STUN_MSG_PARSER_H_
#include <string>
#include <boost/shared_ptr.hpp>
#include "stun_proxy.h"
#include "amqp_asio.h"
#include "systhread/Scheduler.h"
#include "systhread/Worker.h"
#include "systhread/ThreadPool.h"
#include "turn/ns_turn_ioaddr.h"

using namespace std;

struct requestinfo {
    uint16_t channel_num;
    uint32_t srcip;
    uint16_t srcport;
    uint32_t peerip;
    uint16_t peerport;
};

struct proxyinfo {
    uint16_t channel_num;
    uint32_t srcip;
    uint16_t srcport;
    uint32_t dstip;
    uint16_t dstport;
    uint32_t proxyip;
    uint16_t proxyport;
};

class amqp_asio;

class StunProxyMgr : public CStunProxy::cListener, 
                     public cAmqpListener,
                     public std::enable_shared_from_this<StunProxyMgr> {
public:
    StunProxyMgr();
    ~StunProxyMgr();
    void PacketCallback(uint32_t srcip, uint16_t srcport, std::shared_ptr<PacketBuffer> pPacket, size_t n_len) override;
    void PacketCallbackAsync(uint32_t srcip, uint16_t srcport, std::shared_ptr<PacketBuffer> pPacket, size_t n_len);
    void Startup();
    void Shutdown();
    
    virtual void onBroadcastMessageArrived(const std::string &msg) override;

protected:

private:
    void HandlePacketFromClient(uint32_t srcip, uint16_t srcport, std::shared_ptr<PacketBuffer> pPacket, size_t n_len);
    void HandlePacketFromCoturn(uint32_t srcip, uint16_t srcport, std::shared_ptr<PacketBuffer> pPacket, size_t n_len);
    void HandlePacketFromOtherProxy(uint32_t srcip, uint16_t srcport, std::shared_ptr<PacketBuffer> pPacket, size_t n_len);

    void HandleBindRequest();
    void HandleAllocateRequest();

    void HandleSuccessResponse(uint16_t method, uint8_t *buffer, size_t len);
    void HandleErrorResponse(uint16_t method, uint8_t *buffer, size_t len);

    void asyncTask(std::function<void(std::shared_ptr<StunProxyMgr>)> f);
    bool GetStunAttrAddress(uint8_t* data, size_t len, int attr_type, ioa_addr* addr);
    std::shared_ptr<CStunProxy> spProxyServer;
    std::shared_ptr<amqp_asio> spAmqpHandler;
    std::shared_ptr<shark::ThreadPool> spThreadPool;
    std::shared_ptr<shark::Worker> simulated_worker;
    std::string local_ip;
    uint32_t i_local_ip;
    uint16_t local_port;
    std::unordered_map<std::string,proxyinfo> mapProxyInfo;
    std::unordered_map<std::string,proxyinfo> mapPeerInfo;
    std::map<std::string,requestinfo> mapRequests;
};

#endif //STUN_MSG_PARSER_H_
