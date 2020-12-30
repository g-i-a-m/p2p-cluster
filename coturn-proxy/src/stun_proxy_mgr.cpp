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
    if (srcport==local_proxy_port) {
        HandlePacketFromOtherProxy(srcip, srcport, pPacket, n_len);
    }
    else if (srcport == g_coturn_port) {
        HandlePacketFromCoturn(srcip, srcport, pPacket, n_len);
    }
    else {
        uint8_t *p_buffer=(uint8_t *)pPacket->GetData();
        stun_custom_header* cus_header = (stun_custom_header*)p_buffer;
        if (cus_header->IsStunExtensionRelayHeader()) {
            //indication/channel data from coturn
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
    spAmqpHandler->InitAmqp(this,"admin:admin@192.168.10.109","奔波儿灞","灞波儿奔_");
    
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
    if (method.compare(ACT_TYPE_CHANNELINFO)==0) {
        std::string mapped_ip, proxy_ip, peer_ip;
        uint16_t channel_num, mapped_port, proxy_port, peer_port;
        json.GetInt16Value(JSON_CHANNELNUM,channel_num);
        json.GetStringValue(JSON_PEERIP,peer_ip);
        json.GetInt16Value(JSON_PEERPORT,peer_port);
        json.GetStringValue(JSON_MAPPEDIP,mapped_ip);
        json.GetInt16Value(JSON_MAPPEDPORT,mapped_port);
        json.GetStringValue(JSON_PROXYIP,proxy_ip);
        json.GetInt16Value(JSON_PROXYPORT,proxy_port);
        std::string s_key = mapped_ip+std::to_string(mapped_port);

        proxyinfo info;
        info.channel_num = channel_num;
        info.srcip = stun_custom_header::ip2uint32(mapped_ip.c_str());
        info.srcport = mapped_port;
        info.proxyip = stun_custom_header::ip2uint32(proxy_ip.c_str());
        info.proxyport = proxy_port;
        mapProxyInfo[s_key] = info;//只带数据包源地址信息的包转到哪
    }
    else if (method.compare(ACT_TYPE_RELAYINFO)==0) {
        std::string mapped_ip, proxy_ip, relayip;
        uint16_t mapped_port, proxy_port, relay_port;
        json.GetStringValue(JSON_RELAYIP,relayip);
        json.GetInt16Value(JSON_RELAYPORT,relay_port);
        json.GetStringValue(JSON_MAPPEDIP,mapped_ip);
        json.GetInt16Value(JSON_MAPPEDPORT,mapped_port);
        json.GetStringValue(JSON_PROXYIP,proxy_ip);
        json.GetInt16Value(JSON_PROXYPORT,proxy_port);
        std::string s_key = relayip+std::to_string(relay_port);
        
        proxyinfo info;
        info.channel_num = 0;
        info.srcip = stun_custom_header::ip2uint32(mapped_ip.c_str());
        info.srcport = mapped_port;
        info.proxyip = stun_custom_header::ip2uint32(proxy_ip.c_str());
        info.proxyport = proxy_port;
        mapProxyInfo[s_key] = info;//携带relay-addr的包转到哪个代理上去
    }
    else {
        std::cout << "unsupport msg: " << msg << std::endl;
    }
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
    }
    else {
        std::cout << "custom buffer is too small" << std::endl;
        return;
    }

    uint16_t method = stun_get_method_str(p_buffer, n_len);
    if (is_channel_msg_str(p_buffer,n_len)) {
        // send to other proxy or coturn
        std::string key = stun_custom_header::ip2string(srcip);
        key.append(std::to_string(srcport));
        auto it = mapProxyInfo.find(key);
        if (it != mapProxyInfo.end()) {
            if (it->second.proxyip!=local_proxy_ip) {
                spProxyServer->SendToByAddr(it->second.proxyip,it->second.proxyport,(const char*)custom_packet_buffer,custom_packet_len);
            }
            else {
                spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)custom_packet_buffer,custom_packet_len);
            }
        }
        else {
            std::cout << "not found channel data router ip" << std::endl;
        }
    }
    else if (stun_is_indication_str(p_buffer,n_len)) {
        if (method == STUN_METHOD_SEND) {
            ioa_addr peer_addr;
            GetStunAttrAddress(p_buffer,n_len,STUN_ATTRIBUTE_XOR_PEER_ADDRESS,&peer_addr);
            std::string key = stun_custom_header::ip2string(peer_addr.s4.sin_addr.s_addr);
            key.append(std::to_string(peer_addr.s4.sin_port));
            auto it = mapProxyInfo.find(key);
            if (it != mapProxyInfo.end()) {
                spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)custom_packet_buffer,custom_packet_len);
            }
            else {
                spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)custom_packet_buffer,custom_packet_len);
            }
        }
        else if (method == STUN_METHOD_DATA) {
            std::cout << "recv data from client, data indication" << std::endl;
        }
        else {
            std::cout << "unsuport indication packet, method:" << method << std::endl;
        }
    }
    else if (stun_is_request_str(p_buffer,n_len)) {
        switch (method) {
        case STUN_METHOD_BINDING:
        case STUN_METHOD_ALLOCATE:
        case STUN_METHOD_REFRESH:
            spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)custom_packet_buffer,custom_packet_len);
            break;
        case STUN_METHOD_CREATE_PERMISSION: {
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

            // key = stun_custom_header::ip2string(peer_addr.s4.sin_addr.s_addr);
            // key.append(std::to_string(peer_addr.s4.sin_port));
            // auto it = mapProxyInfo.find(key);
            // if (it != mapProxyInfo.end()) {
            //     if (it->second.proxyip!=local_proxy_ip) {
            //         spProxyServer->SendToByAddr(it->second.proxyip,it->second.proxyport,(const char*)custom_packet_buffer,custom_packet_len);
            //     }
            //     else {
            //         spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)custom_packet_buffer,custom_packet_len);
            //     }
            // }
            // else {
            //     std::cout << "not found router ip" << method << std::endl;
            // }
            spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)custom_packet_buffer,custom_packet_len);
            break;}
        case STUN_METHOD_CHANNEL_BIND: {
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

            // key = stun_custom_header::ip2string(peer_addr.s4.sin_addr.s_addr);
            // key.append(std::to_string(peer_addr.s4.sin_port));
            // auto it = mapProxyInfo.find(key);
            // if (it != mapProxyInfo.end()) {
            //     if (it->second.proxyip!=local_proxy_ip) {
            //         spProxyServer->SendToByAddr(it->second.proxyip,it->second.proxyport,(const char*)custom_packet_buffer,custom_packet_len);
            //     }
            //     else {
            //         spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)custom_packet_buffer,custom_packet_len);
            //     }
            // }
            // else {
            //     std::cout << "not found router ip" << method << std::endl;
            // }
            spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)custom_packet_buffer,custom_packet_len);
            break;}
        default:
            std::cout << "unsuport request packet, method:" << method << std::endl;
            break;
        }
    }
    else {
        uint16_t method = stun_get_method_str(p_buffer,n_len);
        std::cout << "unsuport packet, method:" << method << std::endl;
    }
}

void StunProxyMgr::HandlePacketFromCoturn(uint32_t srcip, uint16_t srcport, std::shared_ptr<PacketBuffer> pPacket, size_t n_len) {
    // parse custom header
    uint8_t *p_buffer=(uint8_t *)pPacket->GetData();
    stun_custom_header* cus_header = (stun_custom_header*)p_buffer;
    if (cus_header->IsStunExtensionHeader()) {
        uint8_t* p_payload = p_buffer+cus_header->GetLength();
        size_t n_payload_len = n_len-cus_header->GetLength();
        uint16_t method = stun_get_method_str(p_payload,n_payload_len);
        if (is_channel_msg_str(p_payload,n_payload_len)) {
            // send to other proxy or client
            std::string key = cus_header->GetDstIp();
            key.append(std::to_string(cus_header->GetDstPort()));
            auto it = mapPeerInfo.find(key);
            if (it != mapPeerInfo.end()) {
                spProxyServer->SendToByAddr(it->second.srcip,it->second.srcport,(const char*)p_payload,n_payload_len);
            }
        }
        else if (stun_is_indication_str(p_payload,n_payload_len)) {
            if (method == STUN_METHOD_DATA) {
                ioa_addr peer_addr;
                GetStunAttrAddress(p_payload,n_payload_len,STUN_ATTRIBUTE_XOR_PEER_ADDRESS,&peer_addr);
                std::string key = stun_custom_header::ip2string(peer_addr.s4.sin_addr.s_addr);
                key.append(std::to_string(peer_addr.s4.sin_port));
                auto it = mapPeerInfo.find(key);
                if (it != mapPeerInfo.end()) {
                    spProxyServer->SendToByAddr(it->second.srcip,it->second.srcport,(const char*)p_payload,n_payload_len);
                }
                else {
                    //TODO send to client by mapped address
                }
            }
            else if (method == STUN_METHOD_SEND) {
                std::cout << "recv data from coturn, send indication" << std::endl;
            }
            else {
                std::cout << "unsuport indication packet, method:" << method << std::endl;
            }
        }
        else if (stun_is_success_response_str(p_payload,n_payload_len)) {
            HandleSuccessResponse(method,p_buffer,n_len);
        }
        else if (stun_is_error_response_str(p_payload,n_payload_len,nullptr,nullptr,0U)) {
            HandleErrorResponse(method,p_buffer,n_len);
        }
        else {
            
        }
    }
    else if (cus_header->IsStunExtensionRelayHeader()) {
        uint8_t* p_payload = p_buffer+cus_header->GetLength();
        size_t n_payload_len = n_len-cus_header->GetLength();
        uint16_t method = stun_get_method_str(p_payload,n_payload_len);
        if (is_channel_msg_str(p_payload,n_payload_len)) {
            //spProxyServer->SendToByAddr(htonl(cus_header->GetDstIntIp()),cus_header->GetDstPort(),(const char*)p_payload+STUN_CHANNEL_HEADER_LENGTH,n_payload_len-STUN_CHANNEL_HEADER_LENGTH);
            spProxyServer->SendToByAddr(htonl(cus_header->GetDstIntIp()),cus_header->GetDstPort(),(const char*)p_buffer,n_len);
        }
        else if (stun_is_indication_str(p_payload,n_payload_len)) {
            if (method == STUN_METHOD_SEND) {
                // uint8_t* buffer = GetStunAttrBufferData(p_payload,n_payload_len,STUN_ATTRIBUTE_DATA);
                // size_t len = GetStunAttrBufferSize(buffer-4);
                // spProxyServer->SendToByAddr(htonl(cus_header->GetDstIntIp()),cus_header->GetDstPort(),(const char*)buffer,len);
                spProxyServer->SendToByAddr(htonl(cus_header->GetDstIntIp()),cus_header->GetDstPort(),(const char*)p_buffer,n_len);
            }
            else if (method == STUN_METHOD_DATA) {
                std::cout << "recv data from coturn, data indication" << std::endl;
            }
            else {
                std::cout << "unsuport indication packet, method:" << method << std::endl;
            }
        }
    }
    else {
        std::cout << "recv illegal data from coturn, len:" << n_len << std::endl;
    }
}

void StunProxyMgr::HandlePacketFromOtherProxy(uint32_t srcip, uint16_t srcport, std::shared_ptr<PacketBuffer> pPacket, size_t n_len) {
    // parse custom header
    uint8_t *p_buffer=(uint8_t *)pPacket->GetData();
    stun_custom_header* cus_header = (stun_custom_header*)p_buffer;
    if (cus_header->IsStunExtensionHeader()) {
        std::cout << "recv data from other-proxy, len:" << n_len << std::endl;
        uint8_t* p_payload = p_buffer+cus_header->GetLength();
        size_t n_payload_len = n_len-cus_header->GetLength();
        uint16_t method = stun_get_method_str(p_payload,n_payload_len);
        if (is_channel_msg_str(p_payload,n_payload_len)) {
            // send to coturn
            spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)p_buffer,n_len);
        }
        else if (stun_is_indication_str(p_payload,n_payload_len)) {
            if (method == STUN_METHOD_DATA) {
                std::cout << "recv data from other-proxy, data indication" << std::endl;
                ioa_addr peer_addr;
                GetStunAttrAddress(p_buffer,n_len,STUN_ATTRIBUTE_XOR_PEER_ADDRESS,&peer_addr);
                std::string key = stun_custom_header::ip2string(peer_addr.s4.sin_addr.s_addr);
                key.append(std::to_string(peer_addr.s4.sin_port));
                auto it = mapProxyInfo.find(key);
                if (it != mapProxyInfo.end()) {
                    spProxyServer->SendToByAddr(it->second.proxyip,it->second.proxyport,pPacket->GetData(),pPacket->mn_Len);
                }
                else {
                    //TODO send to client by mapped address
                }
            }
            else if (method == STUN_METHOD_SEND) {
                std::cout << "recv data from other-proxy, send indication" << std::endl;
            }
            else {
                std::cout << "unsuport indication packet, method:" << method << std::endl;
            }
        }
        else if (stun_is_request_str(p_payload,n_payload_len)) {
            std::cout << "recv data from other-proxy, request" << std::endl;
            spProxyServer->SendToByAddr(g_coturn_ip,g_coturn_port,(const char*)p_buffer,pPacket->mn_Len);
        }
        else if (stun_is_success_response_str(p_payload,n_payload_len)) {
            // the response from proxy, so it must be send to client
            spProxyServer->SendToByAddr(htonl(cus_header->GetSrcIntIp()),htons(cus_header->GetSrcPort()),(const char*)p_payload,n_payload_len);
        }
        else if (stun_is_error_response_str(p_payload,n_payload_len,nullptr,nullptr,0U)) {
            // the response from proxy, so it must be send to client
            spProxyServer->SendToByAddr(htonl(cus_header->GetSrcIntIp()),htons(cus_header->GetSrcPort()),(const char*)p_payload,n_payload_len);
        }
        else {
            std::cout << "" << std::endl;
        }
    }
    else {
        std::cout << "recv illegal data from proxy, len:" << n_len << std::endl;
    }
}

void StunProxyMgr::HandleBindRequest() {

}

void StunProxyMgr::HandleAllocateRequest() {

}

void StunProxyMgr::HandleSuccessResponse(uint16_t method, uint8_t *buffer, size_t len) {
    stun_custom_header* cus_header = (stun_custom_header*)buffer;
    uint8_t* p_payload = buffer+cus_header->GetLength();
    size_t n_payload_len = len-cus_header->GetLength();
    switch (method) {
    case STUN_METHOD_BINDING:
    case STUN_METHOD_REFRESH:{
        break;
    }
    case STUN_METHOD_ALLOCATE:{
        //allocate success respone to broadcast relay-address
        ioa_addr relay_addr;
	    GetStunAttrAddress(p_payload,n_payload_len,STUN_ATTRIBUTE_XOR_RELAYED_ADDRESS,&relay_addr);
        ioa_addr mapped_addr;
	    GetStunAttrAddress(p_payload,n_payload_len,STUN_ATTRIBUTE_XOR_MAPPED_ADDRESS,&mapped_addr);
        boost::property_tree::ptree root;
        root.put(JSON_ACTION, ACT_TYPE_RELAYINFO);
        root.put(JSON_FID, Configure::GetInstance()->GetServerGuid());
        root.put(JSON_RELAYIP, stun_custom_header::ip2string(relay_addr.s4.sin_addr.s_addr));
        root.put(JSON_RELAYPORT, relay_addr.s4.sin_port);
        root.put(JSON_MAPPEDIP, stun_custom_header::ip2string(mapped_addr.s4.sin_addr.s_addr));
        root.put(JSON_MAPPEDPORT, mapped_addr.s4.sin_port);
        root.put(JSON_PROXYIP, str_local_ip);
        root.put(JSON_PROXYPORT, local_proxy_port);
        
        std::stringstream str_msg;
        boost::property_tree::write_json(str_msg, root, false);
        spAmqpHandler->publishMessage(str_msg.str());
        break;
    }
    case STUN_METHOD_CREATE_PERMISSION:{
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
        break;
    }
    case STUN_METHOD_CHANNEL_BIND:{
        //channel-bind success respone to broadcast channel number
        stun_tid tid;
        stun_tid_from_message_str(p_payload,n_payload_len,&tid);
        std::string key((char*)tid.tsx_id,STUN_TID_SIZE);
        auto it = mapRequests.find(key);
        if (it != mapRequests.end()) {
            //broadcast msg by amqp
            boost::property_tree::ptree root;
            root.put(JSON_ACTION, ACT_TYPE_CHANNELINFO);
            root.put(JSON_FID, Configure::GetInstance()->GetServerGuid());
            root.put(JSON_CHANNELNUM, it->second.channel_num);
            root.put(JSON_MAPPEDIP, stun_custom_header::ip2string(it->second.srcip));
            root.put(JSON_MAPPEDPORT, it->second.srcport);
            root.put(JSON_PEERIP, stun_custom_header::ip2string(it->second.peerip));
            root.put(JSON_PEERPORT, it->second.peerport);
            root.put(JSON_PROXYIP, str_local_ip);
            root.put(JSON_PROXYPORT, local_proxy_port);
            std::stringstream str_msg;
            boost::property_tree::write_json(str_msg, root, false);
            spAmqpHandler->publishMessage(str_msg.str());
            mapRequests.erase(it);
        }
        break;
    }
    default:
        break;
    }
    if (cus_header->GetDstIntIp()!=local_proxy_ip) {
        spProxyServer->SendToByAddr(cus_header->GetDstIntIp(),cus_header->GetDstPort(),(const char*)buffer,len);
    }
    else {
        spProxyServer->SendToByAddr(htonl(cus_header->GetSrcIntIp()),htons(cus_header->GetSrcPort()),(const char*)p_payload,n_payload_len);
    }
}

void StunProxyMgr::HandleErrorResponse(uint16_t method, uint8_t *buffer, size_t len) {
    stun_custom_header* cus_header = (stun_custom_header*)buffer;
    uint8_t* p_payload = buffer+cus_header->GetLength();
    size_t n_payload_len = len-cus_header->GetLength();
    switch (method) {
    case STUN_METHOD_BINDING:
    case STUN_METHOD_ALLOCATE: {
        stun_tid tid;
        stun_tid_from_message_str(p_payload,n_payload_len,&tid);
        std::string key((char*)tid.tsx_id,STUN_TID_SIZE);
        auto it = mapRequests.find(key);
        if (it != mapRequests.end()) {
            mapRequests.erase(it);
        }
        break;
    }
    case STUN_METHOD_REFRESH:
        break;
    default:
        break;
    }

    if (cus_header->GetDstIntIp()!=local_proxy_ip) {
        spProxyServer->SendToByAddr(cus_header->GetDstIntIp(),cus_header->GetDstPort(),(const char*)buffer,len);
    }
    else {
        spProxyServer->SendToByAddr(htonl(cus_header->GetSrcIntIp()),htons(cus_header->GetSrcPort()),(const char*)p_payload,n_payload_len);
    }
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
            }
            else{
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
        }
        else{
            sar = stun_attr_get_next_str(data, len, sar);
            continue;
        }
    }
	return value;
}
size_t StunProxyMgr::GetStunAttrBufferSize(uint8_t* data) {
    return ntohs(((uint16_t*)data)[1]);
}
