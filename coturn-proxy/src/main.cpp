/*
 * @Author: your name
 * @Date: 2020-11-23 15:53:58
 * @LastEditTime: 2020-11-25 19:38:24
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/main.cpp
 */
#include "stun_proxy_mgr.h"

int main() {
    std::shared_ptr<StunProxyMgr> mgr = std::make_shared<StunProxyMgr>();
    mgr->Startup();
    // std::shared_ptr<CStunProxy> proxy = std::make_shared<CStunProxy>(9900, 1500);
    // proxy->SetListen(mgr.get());
    // proxy->Start();
}
