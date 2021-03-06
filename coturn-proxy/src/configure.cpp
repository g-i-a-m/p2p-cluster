/*
 * @Author: your name
 * @Date: 2020-11-06 10:43:41
 * @LastEditTime: 2020-11-23 15:30:43
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/configure.cpp
 */
#include "configure.h"
#include <fstream>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "boost/algorithm/string.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/format.hpp"
#include <boost/foreach.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>     /* getenv */

Configure * Configure::mp_conf=nullptr;
Configure * Configure::GetInstance() {
    if(mp_conf==nullptr) {
        mp_conf=new Configure();
        mp_conf->Init();
    }
    return mp_conf;
}
Configure::Configure() {

}
Configure::~Configure() {

}

bool Configure::Init() {
    //read stream conf
    try {
        //read public ip
        char* pIp;
        pIp = getenv ("SLB_IP");
        if (pIp!=NULL) {
            ms_public_ip=pIp;
        }
        else {
            std::cout<<"can't get slb ip" << std::endl;
        }

        std::string s_conf("/etc/coturn-proxy.conf");

        std::ifstream t(s_conf);
        std::stringstream buffer;
        buffer << t.rdbuf();
        boost::property_tree::ptree ptree;
        boost::property_tree::read_json(buffer, ptree);

        std::string s_rid=ptree.get<std::string>("version");
        mn_coturn_port = ptree.get<uint16_t>("coturn_port");
        mn_proxy_port = ptree.get<uint16_t>("proxy_port");
        ms_rabbitmq_url=ptree.get<std::string>("rabbitmq_url");
        ms_rabbitmq_port=ptree.get<std::string>("rabbitmq_port");
    }
    catch(...) {
        std::cout<<"parse configure file get \n" << std::endl;
    }

    if(ms_uuid.empty()) {
        boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	    ms_uuid = boost::uuids::to_string(a_uuid);
    }

    return true;
}
