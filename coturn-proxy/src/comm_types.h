/*
 * @Author: your name
 * @Date: 2020-11-26 11:59:34
 * @LastEditTime: 2020-11-26 14:48:12
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/comm_types.h
 */
#pragma once
#include <string>

const int CONST_PROXY_PACKET_HEADER=65536001;

//for action type
const std::string ACT_TYPE_CHANNELINFO                  = "turnproxy-channelinfo";
const std::string ACT_TYPE_RELAYINFO                    = "turnproxy-relayinfo";
const std::string ACT_TYPE_MAPPEDINFO                   = "turnproxy-mappedinfo";

// for json attr
const std::string JSON_ACTION                           = "action";
const std::string JSON_FID                              = "fid";
const std::string JSON_TID                              = "tid";
const std::string JSON_REQUESTSERIAL                    = "serial";
const std::string JSON_PODID                            = "podid";
const std::string JSON_EDGEID                           = "edgeid";
const std::string JSON_CHANNELNUM                       = "channelnum";
const std::string JSON_MAPPEDIP                         = "mappedip";
const std::string JSON_MAPPEDPORT                       = "mappedport";
const std::string JSON_RELAYIP                          = "relayip";
const std::string JSON_RELAYPORT                        = "relayport";
const std::string JSON_PEERIP                           = "peerip";
const std::string JSON_PEERPORT                         = "peerport";
const std::string JSON_PROXYIP                          = "proxyip";
const std::string JSON_PROXYPORT                        = "proxyport";

const std::string JSON_SUCCEED_CODE                     = "0";

const char SZ_KEEPALIVE_TOPIC_PREFIX[]                  = "avkp_";

const char SZ_ENABLE[]                                  = "enable";
const char SZ_DISABLE[]                                 = "disable";
