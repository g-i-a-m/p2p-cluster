#include "stun_proxy.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <chrono>

std::string GetHostInfo() {
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    std::string Ip("");
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) {
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            std::string s_ip=addressBuffer;
            if (strcmp(addressBuffer,"127.0.0.1")!=0) {
                Ip = addressBuffer;
                break;
            }
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return Ip;
}

CStunProxy::CStunProxy(uint16_t port, size_t buf_size) :
    buffer_(buf_size),
    io_context(4)
{
    Configure::GetInstance()->SetLocalUdpPort(port);
    socket_=new boost::asio::ip::udp::socket(io_context,udp::endpoint(udp::v4(),(Configure::GetInstance()->GetLocalUdpPort())));
    //boost::asio::ip::address::from_string("192.168.10.151")
    // set socket receive send buff
    if (socket_!=nullptr) {
        boost::asio::socket_base::receive_buffer_size r_option(32*1024);
        socket_->set_option(r_option);
        boost::asio::socket_base::send_buffer_size w_option(64*1024);
        socket_->set_option(w_option);
    }

    strand_=new boost::asio::io_context::strand(io_context);
    send_strand_=new boost::asio::io_context::strand(io_context);

    signal_=new(std::nothrow) boost::asio::signal_set(io_context);
    if (signal_!=nullptr) {
        signal_->add(SIGINT);
        signal_->add(SIGTERM);
        signal_->async_wait(boost::bind(&CStunProxy::handle_signal, this, boost::placeholders::_1, boost::placeholders::_2));
    }
}

void CStunProxy::Start()
{
    auto self = shared_from_this();
    boost::asio::spawn(*strand_,
                       [this, self](boost::asio::yield_context yield)
    {
        std::cout << "start transport udp " << Configure::GetInstance()->GetLocalUdpPort() << std::endl;
        while (true)
        {
            boost::system::error_code ec;
            std::size_t n=socket_->async_receive_from(
                        boost::asio::buffer(buffer_),
                        sender_, yield[ec]);
            if(ec) {
                break;
            }
            uint16_t uSrcPort = sender_.port();
            uint32_t uSrcIP = ntohl(sender_.address().to_v4().to_ulong());
            std::string health_packet;
            if (n == 10) {
                health_packet = std::string((char*)&buffer_[0],n);
            }

            if(!health_packet.empty() && (health_packet=="offcncloud")) {
                SendToByAddr(uSrcIP,uSrcPort,health_packet.c_str(),health_packet.length());
            }
            else
            {
                std::shared_ptr<PacketBuffer> pPacket = std::make_shared<PacketBuffer>();
                pPacket->SetLength(n);
                memcpy(pPacket->GetData(),(char*)&buffer_[0],n);
                mp_Listen->PacketCallback(uSrcIP,uSrcPort,pPacket,n);
            }
        }
    });
    io_context.run();
}

void CStunProxy::handle_signal(boost::system::error_code code, int signal)
{
    if (signal == SIGINT) {
        printf("process catch a SIGINT signal...");
    }
    else if (signal == SIGTERM) {
        printf("process catch a SIGTERM signal...");
    }
    else {
        printf("process catch a %d signal...",signal);
    }
    return;
}

int CStunProxy::SendToByAddr(unsigned int  iSrcIp,unsigned short iSrcPort,const char* pBuffer,int nLen,bool async)
{
    in_addr addr_tmp;
    addr_tmp.s_addr = (iSrcIp);
    std::string strHopeIp = inet_ntoa( addr_tmp );

    if(iSrcIp==Configure::GetInstance()->GetLocalUdpIp())
        strHopeIp="127.0.0.1";

    unsigned short nPort = (iSrcPort);
    boost::asio::ip::address addr = boost::asio::ip::address::from_string(strHopeIp);
    boost::asio::ip::udp::endpoint RemoteEndpoint;
    RemoteEndpoint.address(addr);
    RemoteEndpoint.port(nPort);
    if(async) {
        socket_->async_send_to(boost::asio::buffer(pBuffer,nLen),RemoteEndpoint,
        boost::bind(&CStunProxy::handle_send, this,
        boost::asio::placeholders::error));
    }
    else {
        socket_->send_to(boost::asio::buffer(pBuffer,nLen),RemoteEndpoint);
    }

    return 1;
}

void CStunProxy::handle_send(const boost::system::error_code& error)
{

}
