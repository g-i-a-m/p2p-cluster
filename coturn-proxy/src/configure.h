/*
 * @Author: your name
 * @Date: 2020-11-06 10:43:41
 * @LastEditTime: 2020-11-23 15:30:36
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/configure.h
 */
#pragma once

#include <string>
#define  MIN_PORT 40000
#define  MAX_PORT 50000

class Configure
{
	public:
		static Configure *GetInstance();
	public:
		Configure();
		~Configure();
	public:
		bool Init();
		inline std::string GetPublic_ip(){return ms_public_ip;};
		inline std::string GetServerGuid(){return ms_uuid;};
		unsigned int GetCoturnPort(){ return mn_coturn_port;};
		unsigned int GetProxyPort(){ return mn_proxy_port;};
		inline void  SetLocalUdpIp(unsigned int ip){ mn_local_udp_ip=ip;};
		unsigned int GetLocalUdpIp(){ return mn_local_udp_ip;};
		inline void SetLocalUdpPort(unsigned short port){ mn_local_udp_port=port;};
		unsigned short GetLocalUdpPort(){ return mn_local_udp_port;};

		inline std::string GetRabbitmq_url() {return ms_rabbitmq_url;};
		inline std::string GetRabbitmq_port() {return ms_rabbitmq_port;};
	private:
		static Configure *mp_conf;
		std::string ms_public_ip;
		std::string ms_uuid;
		uint16_t mn_coturn_port;
		uint16_t mn_proxy_port;
		unsigned int mn_local_udp_ip;
		unsigned short mn_local_udp_port;

		std::string ms_rabbitmq_url;
		std::string ms_rabbitmq_port;
};
