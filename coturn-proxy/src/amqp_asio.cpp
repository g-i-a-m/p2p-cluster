#include "amqp_asio.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/format.hpp>
#include <boost/asio/placeholders.hpp>

uint16_t amqp_asio_connection_handler::onNegotiate(AMQP::TcpConnection *connection, uint16_t interval)
{
    if (interval<=120 && interval>=30)
        amqp_asio::getInStance()->m_interval = interval;
    else
        std::cout << "rabbitmq server want heartbeats interval was 60 seconds" << std::endl;
    return amqp_asio::getInStance()->m_interval;
}

void amqp_asio_connection_handler::onReady(AMQP::TcpConnection *connection)
{
    if (connection == amqp_asio::getInStance()->mp_ampq_center_connection.get())
    {
        std::cout << "Connection of center rabbitmq was ready" << std::endl;
        amqp_asio::getInStance()->mb_center_amqp_connected = true;
    }
}

void amqp_asio_connection_handler::onLost(AMQP::TcpConnection *connection)
{
    bool startup = true;
    if (amqp_asio::getInStance()->mb_center_amqp_connected==false)
        startup = false;

    if (connection == amqp_asio::getInStance()->mp_ampq_center_connection.get())
    {
        std::cout << "Lost connection with center rabbitmq" << std::endl;
        amqp_asio::getInStance()->mb_center_amqp_connected = false;
    }
    if (startup)
        amqp_asio::getInStance()->StartReConnectTimer();
}

void amqp_asio_connection_handler::onError(AMQP::TcpConnection *connection, const char *message)
{
    amqp_asio::getInStance()->StartReConnectTimer();
    std::cout << "rabbitmq connect error" << std::endl;
}

amqp_asio *amqp_asio::mp_self=NULL;

amqp_asio *amqp_asio::getInStance()
{
    if(mp_self==nullptr)
    {
        mp_self=new amqp_asio();
    }
    return mp_self;
}

amqp_asio::amqp_asio():
    mp_ioservice(new boost::asio::io_service()),
    mp_amqplistener(nullptr),
    mb_reconn_timer_active(false),
    mb_center_amqp_connected(false),
    m_interval(30)
{
    mp_self = this;
    mp_ampq_handle=std::make_shared<amqp_asio_connection_handler>(*mp_ioservice);
}

amqp_asio::~amqp_asio()
{
}

bool amqp_asio::InitAmqp(cAmqpListener* listener,std::string s_center_server,std::string s_exchange_fanout,std::string s_fanout_queue)
{
    mp_amqplistener=listener;
    ms_center_server_addr = s_center_server;
    ms_broadcast_exchange=s_exchange_fanout;
    ms_fanout_queue = s_fanout_queue;

    InitCenterAmqp(ms_center_server_addr, ms_broadcast_exchange, ms_fanout_queue);

    message_thread_.reset(new boost::thread(boost::bind(&amqp_asio::Run, this)));
    mp_timer_stand=new boost::asio::io_context::strand(*mp_ioservice);
    mp_timer= new boost::asio::deadline_timer(*mp_ioservice,boost::posix_time::seconds(5));
    mp_timer->async_wait(mp_timer_stand->wrap(boost::bind(&amqp_asio::CheckHeartbeat,this,boost::asio::placeholders::error)));

    return true;
}

bool amqp_asio::InitCenterAmqp(std::string s_center_server,std::string s_exchange_fanout,std::string s_fanout_queue)
{
    boost::format center_url_fmt = boost::format("amqp://%s/")% s_center_server;
    std::string str = center_url_fmt.str();
    mp_ampq_center_connection.reset( new AMQP::TcpConnection(mp_ampq_handle.get(), AMQP::Address(center_url_fmt.str().c_str())) );
    mp_ampq_channel.reset( new AMQP::TcpChannel(mp_ampq_center_connection.get()) );

    mp_ampq_channel->declareExchange(s_exchange_fanout.c_str(), AMQP::fanout,AMQP::autodelete)
            .onSuccess([s_exchange_fanout](){
        std::cout << "amqp declare exchange " << std::endl;
    })
            .onError([s_exchange_fanout](const char* msg){
        std::cout << s_exchange_fanout.c_str() << " declare failed, desc: " << msg << std::endl;
    });
    std::string s_broadcast_edge_queue=s_fanout_queue;
    mp_ampq_channel->declareQueue(s_broadcast_edge_queue.c_str(),AMQP::autodelete)
            .onSuccess([](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
        std::cout << "amqp declared queue " << std::endl;
    })
            .onError([s_broadcast_edge_queue](const char* msg) {
        std::cout << s_broadcast_edge_queue.c_str() << " declare failed, desc: " << msg << std::endl;
    });

    mp_ampq_channel->bindQueue(s_exchange_fanout.c_str(), s_broadcast_edge_queue.c_str(),s_broadcast_edge_queue.c_str() )
            .onSuccess([](){
        std::cout << "amqp binded to queue " << std::endl;
    })
            .onError([s_exchange_fanout](const char* msg){
        std::cout << "amqp bind failed, desc: %s" << msg << std::endl;
    });

    ///consume boradcast message
    mp_ampq_channel->consume(s_broadcast_edge_queue.c_str(), AMQP::noack).onReceived(
                [this,s_broadcast_edge_queue](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::string myString(message.body(), message.body()+message.bodySize());
        this->recvBroadcastMessage(myString);
    })
            .onError([s_broadcast_edge_queue](const char* msg){
        std::cout << "%s consume failed, desc: %s" << s_broadcast_edge_queue.c_str() << msg << std::endl;
    });

    return true;
}

void amqp_asio::CheckHeartbeat(const boost::system::error_code& error)
{
    if (!error) {
        if (mb_center_amqp_connected)
            mp_ampq_center_connection->heartbeat();
        mp_timer->expires_at(mp_timer->expires_at()+boost::posix_time::seconds((int)m_interval));
        mp_timer->async_wait(mp_timer_stand->wrap(boost::bind(&amqp_asio::CheckHeartbeat,this,boost::asio::placeholders::error)));
    }
}

//multi-thread unsafe
void amqp_asio::StartReConnectTimer()
{
    if (mb_reconn_timer_active == false)
    {
        mb_reconn_timer_active = true;
        mp_reconn_timer.reset(new boost::asio::deadline_timer(*mp_ioservice,boost::posix_time::seconds(20)));
        mp_reconn_timer->async_wait(boost::bind(&amqp_asio::ReConnectAmqp,this,boost::asio::placeholders::error));
    }
}

void amqp_asio::ReConnectAmqp(const boost::system::error_code& error)
{
    if (!error)
    {
        mb_reconn_timer_active = false;
        if (!mb_center_amqp_connected)
        {
            mp_ampq_channel.reset();
            mp_ampq_center_connection->close();
            mp_ampq_center_connection.reset();
            InitCenterAmqp(ms_center_server_addr, ms_broadcast_exchange, ms_fanout_queue);
        }

        if (!mb_center_amqp_connected)
        {
            std::cout << "set next reconnection timer, edge rabbitmq:%s, center rabbitmq:%s" << (mb_center_amqp_connected?"conn":"disconn") << std::endl;
            StartReConnectTimer();
        }
    }
}

void amqp_asio::recvBroadcastMessage(std::string s_message)
{
    mp_amqplistener->onBroadcastMessageArrived(s_message);
}

void amqp_asio::publishMessage(std::string s_message)
{
    std::cout << "send to rabbit:" << s_message.c_str() << std::endl;
    mp_ioservice->post(boost::bind(&amqp_asio::GetPublishMessage, this,s_message));
}

void amqp_asio::GetPublishMessage(std::string s_message)
{
    mp_ampq_channel->publish("",ms_fanout_queue,s_message);
}

void amqp_asio::Run()
{
    mp_ioservice->run();
}
