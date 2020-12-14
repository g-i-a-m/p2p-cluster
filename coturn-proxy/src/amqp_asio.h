/*
 * @Author: your name
 * @Date: 2020-11-23 18:55:41
 * @LastEditTime: 2020-11-26 11:40:14
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/amqp_asio.h
 */

#include "amqpcpp.h"
#include "amqpcpp/libboostasio.h"
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/thread.hpp>
#include <atomic>

class cAmqpListener
{
    public:
        virtual void onBroadcastMessageArrived(const std::string &msg) = 0;
};

class amqp_asio_connection_handler : public AMQP::LibBoostAsioHandler
{
    public:
        explicit amqp_asio_connection_handler(boost::asio::io_service &io_service) :
            AMQP::LibBoostAsioHandler(io_service) { }
        virtual uint16_t onNegotiate(AMQP::TcpConnection *connection, uint16_t interval);
        virtual void onReady(AMQP::TcpConnection *connection);
        virtual void onLost(AMQP::TcpConnection *connection);
        virtual void onError(AMQP::TcpConnection *connection, const char *message);
};

class amqp_asio
{
    public:
        friend class amqp_asio_connection_handler;
        amqp_asio();
        ~amqp_asio();
        static amqp_asio *getInStance();

        bool InitAmqp(cAmqpListener* listener,std::string s_center_server,std::string s_exchange_fanout,std::string s_fanout_queue);
        bool InitCenterAmqp(std::string s_center_server,std::string s_exchange_fanout,std::string s_fanout_queue);
        
        void CheckHeartbeat(const boost::system::error_code& error);
        void StartReConnectTimer();
        void ReConnectAmqp(const boost::system::error_code& error);

        void recvBroadcastMessage(std::string s_message);
        void publishMessage(std::string s_message);
        void GetPublishMessage(std::string s_message);

        void Run();
        
    private:
        std::shared_ptr<boost::asio::io_service > mp_ioservice;
        boost::shared_ptr<boost::thread> message_thread_;
        std::shared_ptr<AMQP::LibBoostAsioHandler > mp_ampq_handle;
        std::shared_ptr<AMQP::TcpConnection > mp_ampq_center_connection;

        std::shared_ptr< AMQP::TcpChannel> mp_ampq_channel;
        cAmqpListener *mp_amqplistener;
        std::string ms_broadcast_exchange;
        std::string ms_container_exchange;

        std::string ms_center_server_addr;
        std::string ms_fanout_queue;
    private:
        //timer check pod;
        boost::asio::deadline_timer *mp_timer; //check queue state
        boost::asio::io_context::strand* mp_timer_stand;

        //timer reconnect
        bool mb_reconn_timer_active;
        std::shared_ptr<boost::asio::deadline_timer> mp_reconn_timer;
    private:
        static amqp_asio* mp_self;
        std::atomic<bool> mb_center_amqp_connected;
        std::atomic<int> m_interval;
};
