#ifndef stun_proxy_hpp
#define stun_proxy_hpp

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <ifaddrs.h>
#include <netinet/in.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/format.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/spawn.hpp>
#include "configure.h"

/*
    1>.bind request:取得自己的外网地址(MAPPED ADDRESS)
    2>.allocate request:请求分配自己的中继地址(RELAY ADDRESS)
    ...此时会通过业务服务器交换relay address(包含在candidate中)
    3>.create permission:应该是验证收到的candidate中RELAY ADDRESS,探测对端中继地址PEER ADDRESS(remote RELAY ADDRESS)有效性
    4>.channel bind request:本地使用一个通道号,与对端的一个RELAY ADDRESS进行绑定.
*/

using boost::asio::ip::udp;

struct PacketFack
{
    unsigned int local_ip;
    unsigned short local_port;
    unsigned int remote_ip;
    unsigned short remote_port;
    uint64_t timestamp;
};

#define PACKET_MTU 1500
struct PacketBuffer {
	PacketBuffer() {
        mpData=new char[PACKET_MTU];
        memset(mpData,0x0,PACKET_MTU);
        mn_Len=0;
    }
	~PacketBuffer() {
        delete mpData;
    }
	char* GetData() {
        return mpData;
    }
	void SetLength(unsigned int  nLen) {
        mn_Len=nLen;
    }

	char *mpData;
	unsigned int mn_Len;
	boost::asio::ip::udp::endpoint m_GetRemoteEndpoint;
};

std::string GetHostInfo();

class CStunProxy : public std::enable_shared_from_this<CStunProxy>
{
public:
    class cListener
    {
        public:
            virtual void PacketCallback(uint32_t ip,uint16_t port,std::shared_ptr<PacketBuffer> pPacket, size_t n_len)=0;
    };
    
    CStunProxy(uint16_t port, size_t buf_size);
    void Start();

public:
    void handle_signal(boost::system::error_code code, int signal);
    void SetListen(cListener *p_listen){ mp_Listen=p_listen;};
    int SendToByAddr(unsigned int  iSrcIp,unsigned short iSrcPort,const char* pBuffer,int nLen,bool async=true);
    void handle_send(const boost::system::error_code& error);

private:
    boost::asio::ip::udp::socket* socket_;
    std::vector<unsigned char> buffer_;
    //std::unordered_map<std::string,REQ_INFO> m_hmap_req_;
    //std::unordered_map<std::string,PROXY_INFO> m_hmap_proxy_;
    udp::endpoint sender_;
    
    typedef std::map<std::string,PacketFack> UdpProxyContainer;
    UdpProxyContainer m_proxy_info;
    boost::asio::io_context io_context;
    boost::asio::io_context::strand *strand_;
	boost::asio::io_context::strand *send_strand_;
    boost::asio::signal_set *signal_;

private:
    boost::shared_mutex m_addr_mutex;
    cListener *mp_Listen;
};

#endif /* transport_udp_hpp */
