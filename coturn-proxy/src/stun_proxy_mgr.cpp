/*
 * @Author: your name
 * @Date: 2020-11-06 10:43:41
 * @LastEditTime: 2020-11-26 17:47:42
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/stun_msg_parser.cpp
 */

#include <string>
#include "stun_proxy_mgr.h"
#include "stun_custom_header.h"
#include "third/coturn/include/turn/client/TurnMsgLib.h"
#include "data_packet.h"
#include "json_parse.h"
#include "comm_types.h"

const uint16_t g_coturn_port = Configure::GetInstance()->GetCoturnPort();
const uint32_t g_coturn_ip = stun_custom_header::ip2uint32(GetHostInfo().c_str());

StunProxyMgr::StunProxyMgr() {
    spThreadPool=std::make_shared<shark::ThreadPool>(4);
    simulated_worker=spThreadPool->getLessUsedWorker();
    spThreadPool->start();
}

StunProxyMgr::~StunProxyMgr() {

}

// TODO multi-thread as srcaddr
void StunProxyMgr::PacketCallback(unsigned int srcip,unsigned short srcport,std::shared_ptr<PacketBuffer> pPacket, size_t n_len) {
    asyncTask([srcip,srcport,pPacket,n_len] (std::shared_ptr<StunProxyMgr> parser) {
        parser->PacketCallbackAsync(srcip,srcport,pPacket,n_len);
    });
}

void StunProxyMgr::PacketCallbackAsync(unsigned int srcip,unsigned short srcport, \
                                        std::shared_ptr<PacketBuffer> pPacket, size_t n_len) {
    std::string s_ip=std::to_string(srcip)+std::to_string(srcport);
    if (srcport == g_coturn_port) {
        HandlePacketFromCoturn(srcip, srcport, pPacket, n_len);
    } else {
        uint8_t *p_buffer=(uint8_t *)pPacket->GetData();
        stun_custom_header* cus_header = (stun_custom_header*)p_buffer;
        if (cus_header->IsStunExtensionRelayHeader()) {
            //indication/channel data from other-coturn
            HandlePacketFromCoturn(srcip, srcport, pPacket, n_len);
        }
        else {
            HandlePacketFromClient(srcip, srcport, pPacket, n_len);
        }
    }
}

void StunProxyMgr::Startup() {
    str_local_ip = GetHostInfo();
    local_proxy_ip = g_coturn_ip;
    printf("Proxy ip address %s\n", str_local_ip.c_str());
    local_proxy_port = Configure::GetInstance() ->GetProxyPort();

    spAmqpHandler= std::make_shared<amqp_asio>();
    spAmqpHandler->InitAmqp(this,Configure::GetInstance()->GetRabbitmq_url(),"奔波儿灞","灞波儿奔_");
    
    spProxyServer=std::make_shared<CStunProxy>(local_proxy_port, 1500);
    spProxyServer->SetListen(this);
    spProxyServer->Start();
}

void StunProxyMgr::Shutdown() {
    // to be continue...
}

void StunProxyMgr::onBroadcastMessageArrived(const std::string &msg) {
    std::cout << "rabbitmq recv msg: %s" << msg.c_str() << std::endl;
    JsonParse json(msg);
    std::string method;
    json.GetStringValue(JSON_ACTION,method);
}

void StunProxyMgr::HandlePacketFromClient(uint32_t srcip, uint16_t srcport, std::shared_ptr<PacketBuffer> pPacket, size_t n_len) {
    // parse message type and other useful infomation to add custom header
    uint8_t *p_buffer=(uint8_t *)pPacket->GetData();

    stun_custom_header header;
    header.header.identifier = STUN_CUSTOM_IDENTIFIER;
    header.header.srcIp = srcip;
    header.header.srcPort = srcport;
    header.header.dstIp = htonl(local_proxy_ip);
    header.header.dstPort = htons(local_proxy_port);
    size_t header_len = header.GetLength();
    uint8_t custom_packet_buffer[1500]={0};
    size_t custom_packet_len(n_len+header_len);
    if (custom_packet_len < 1500) {
        memcpy(custom_packet_buffer,header.GetData(),header_len);
        memcpy(custom_packet_buffer+header_len,p_buffer,n_len);
        std::shared_ptr<data_packet> custom_packet = InitDataPacket(custom_packet_buffer, custom_packet_len);
    } else {
        std::cout << "custom buffer is too small" << std::endl;
        return;
    }

    uint16_t method = stun_get_method_str(p_buffer, n_len);
    if (stun_is_request_str(p_buffer,n_len) && STUN_METHOD_CREATE_PERMISSION==method) {
        stun_tid tid;
        stun_tid_from_message_str(p_buffer,n_len,&tid);
        std::string key((char*)tid.tsx_id,STUN_TID_SIZE);
        ioa_addr peer_addr;
        GetStunAttrAddress(p_buffer,n_len,STUN_ATTRIBUTE_XOR_PEER_ADDRESS,&peer_addr);
        requestinfo info;
        info.srcip = srcip;
        info.srcport = srcport;
        info.peerip = peer_addr.s4.sin_addr.s_addr;
        info.peerport = peer_addr.s4.sin_port;
        mapRequests[key] = info;
    }
    spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)custom_packet_buffer,custom_packet_len);
}

void StunProxyMgr::HandlePacketFromCoturn(uint32_t srcip, uint16_t srcport, std::shared_ptr<PacketBuffer> pPacket, size_t n_len) {
    // parse custom header
    uint8_t *p_buffer=(uint8_t *)pPacket->GetData();
    stun_custom_header* cus_header = (stun_custom_header*)p_buffer;
    uint8_t* p_payload = p_buffer+cus_header->GetLength();
    size_t n_payload_len = n_len-cus_header->GetLength();
    uint16_t method = stun_get_method_str(p_payload,n_payload_len);
    if (cus_header->IsStunExtensionHeader()) {
        // send to client
        if (is_channel_msg_str(p_payload,n_payload_len)) {
            std::string key = cus_header->GetDstIp();
            key.append(std::to_string(cus_header->GetDstPort()));
            auto it = mapPeerInfo.find(key);
            if (it != mapPeerInfo.end()) {
                spProxyServer->SendToByAddr(it->second.srcip,it->second.srcport,(const char*)p_payload,n_payload_len);
            }
        } else if (stun_is_indication_str(p_payload,n_payload_len)) {
            if (method == STUN_METHOD_DATA) {
                ioa_addr peer_addr;
                GetStunAttrAddress(p_payload,n_payload_len,STUN_ATTRIBUTE_XOR_PEER_ADDRESS,&peer_addr);
                std::string key = stun_custom_header::ip2string(peer_addr.s4.sin_addr.s_addr);
                key.append(std::to_string(peer_addr.s4.sin_port));
                auto it = mapPeerInfo.find(key);
                if (it != mapPeerInfo.end()) {
                    spProxyServer->SendToByAddr(it->second.srcip,it->second.srcport,(const char*)p_payload,n_payload_len);
                }
            } else {
                std::cout << "unsuport indication packet, method:" << method << std::endl;
            }
        } else if (stun_is_success_response_str(p_payload,n_payload_len)) {
            HandleSuccessResponse(method,p_buffer,n_len);
        } else if (stun_is_error_response_str(p_payload,n_payload_len,nullptr,nullptr,0U)) {
            HandleErrorResponse(method,p_buffer,n_len);
        } else {
            
        }
    } else if (cus_header->IsStunExtensionRelayHeader()) {
        if (is_channel_msg_str(p_payload,n_payload_len)) {
            spProxyServer->SendToByAddr(htonl(cus_header->GetDstIntIp()),cus_header->GetDstPort(),(const char*)p_buffer,n_len);
        } else if (stun_is_indication_str(p_payload,n_payload_len)) {
            if (method == STUN_METHOD_SEND) {
                spProxyServer->SendToByAddr(htonl(cus_header->GetDstIntIp()),cus_header->GetDstPort(),(const char*)p_buffer,n_len);
            } else {
                std::cout << "unsuport indication packet, method:" << method << std::endl;
            }
        }
    } else {
        std::cout << "recv illegal data from coturn, len:" << n_len << std::endl;
    }
}

void StunProxyMgr::HandleSuccessResponse(uint16_t method, uint8_t *buffer, size_t len) {
    stun_custom_header* cus_header = (stun_custom_header*)buffer;
    uint8_t* p_payload = buffer+cus_header->GetLength();
    size_t n_payload_len = len-cus_header->GetLength();
    if (STUN_METHOD_CREATE_PERMISSION == method) {
        stun_tid tid;
        stun_tid_from_message_str(p_payload,n_payload_len,&tid);
        std::string key((char*)tid.tsx_id,STUN_TID_SIZE);
        auto it = mapRequests.find(key);
        if (it != mapRequests.end()) {
            std::string s_key = stun_custom_header::ip2string(it->second.peerip).c_str() + std::to_string(it->second.peerport);
            proxyinfo info;
            info.channel_num = 0;
            info.srcip = it->second.srcip;
            info.srcport = it->second.srcport;
            mapPeerInfo[s_key] = info;
            mapRequests.erase(it);
        }
    }
    spProxyServer->SendToByAddr(htonl(cus_header->GetSrcIntIp()),htons(cus_header->GetSrcPort()),(const char*)p_payload,n_payload_len);
}

void StunProxyMgr::HandleErrorResponse(uint16_t method, uint8_t *buffer, size_t len) {
    stun_custom_header* cus_header = (stun_custom_header*)buffer;
    uint8_t* p_payload = buffer+cus_header->GetLength();
    size_t n_payload_len = len-cus_header->GetLength();
    if (STUN_METHOD_CREATE_PERMISSION == method) {
        stun_tid tid;
        stun_tid_from_message_str(p_payload,n_payload_len,&tid);
        std::string key((char*)tid.tsx_id,STUN_TID_SIZE);
        auto it = mapRequests.find(key);
        if (it != mapRequests.end()) {
            mapRequests.erase(it);
        }
    }
    spProxyServer->SendToByAddr(htonl(cus_header->GetSrcIntIp()),htons(cus_header->GetSrcPort()),(const char*)p_payload,n_payload_len);
}

void StunProxyMgr::asyncTask(std::function<void(std::shared_ptr<StunProxyMgr>)> f) {
    std::weak_ptr<StunProxyMgr> weak_this = shared_from_this();
    simulated_worker->task([weak_this, f] {
        if (auto this_ptr = weak_this.lock()) {
        f(this_ptr);
        }
    });
}

bool StunProxyMgr::GetStunAttrAddress(uint8_t* data, size_t len, int attr_type,ioa_addr* addr) {
    bool bRet(false);
    if (addr!=nullptr) {
        addr_set_any(addr);
        stun_attr_ref sar = stun_attr_get_first_str(data, len);
        while (sar) {//TODO MAX TIMES and ipv4 or ipv6 should be carefull
            if (stun_attr_get_type(sar)!=attr_type) { // example STUN_ATTRIBUTE_XOR_PEER_ADDRESS
                sar = stun_attr_get_next_str(data, len, sar);
            } else {
                stun_attr_get_addr_str(data,len,sar,addr,NULL);
                bRet = true;
                break;
            }
        }
    }
	return bRet;
}

uint8_t* StunProxyMgr::GetStunAttrBufferData(uint8_t* data, size_t len, int attr_type) {
    uint8_t* value = nullptr;
    stun_attr_ref sar = stun_attr_get_first_str(data, len);
    while (sar) {//TODO MAX TIMES and ipv4 or ipv6 should be carefull
        if (stun_attr_get_type(sar)==attr_type) {
			value = const_cast<uint8_t*>(stun_attr_get_value(sar));
            break;
        } else {
            sar = stun_attr_get_next_str(data, len, sar);
            continue;
        }
    }
	return value;
}
size_t StunProxyMgr::GetStunAttrBufferSize(uint8_t* data) {
    return ntohs(((uint16_t*)data)[1]);
}
